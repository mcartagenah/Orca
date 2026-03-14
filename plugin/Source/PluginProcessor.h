#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <CoreMIDI/CoreMIDI.h>
#include "engine/Engine.h"

class OrcaProcessor : public juce::AudioProcessor {
public:
    OrcaProcessor();
    ~OrcaProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    orca::Engine engine;

    // Virtual MIDI output via CoreMIDI (bypasses JUCE's broken singleton)
    MIDIClientRef midiClient = 0;
    MIDIEndpointRef midiEndpoint = 0;
    void sendMidiToVirtualPort(const uint8_t* data, int numBytes);

    // Debug: count MIDI events for UI display
    std::atomic<int> midiNoteOnCount { 0 };
    std::atomic<int> lastNoteOnPitch { -1 };

    // Persisted editor window size
    int editorWidth = 800, editorHeight = 600;

private:
    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;
    double frameAccumulator  = 0.0;
    bool   wasPlaying        = false;
    int    lastPpqFrame      = -1;  // last frame derived from PPQ

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrcaProcessor)
};
