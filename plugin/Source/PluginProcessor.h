#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>
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

    // ── AudioProcessorValueTreeState ──
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached raw pointers for fast audio-thread reads
    juce::AudioParameterFloat*  shuffleParam     = nullptr;
    juce::AudioParameterInt*    lifeRateParam     = nullptr;
    juce::AudioParameterBool*   lifeDecayParam    = nullptr;
    juce::AudioParameterInt*    lifeMinVelParam   = nullptr;
    juce::AudioParameterInt*    lifeMinProbParam  = nullptr;
    juce::AudioParameterInt*    lifeMaxNotesParam = nullptr;
    juce::AudioParameterChoice* lifeSeqParam      = nullptr;
    juce::AudioParameterBool*   lifeLockOctParam  = nullptr;
    juce::AudioParameterChoice* lifeChordParam    = nullptr;
    juce::AudioParameterBool*   lifeDedupParam    = nullptr;
    juce::AudioParameterInt*    lifeDedupCCParam  = nullptr;
    juce::AudioParameterInt*    lifeMinOctParam   = nullptr;
    juce::AudioParameterInt*    lifeMaxOctParam   = nullptr;
    juce::AudioParameterChoice* lifeScaleParam    = nullptr;
    juce::AudioParameterChoice* lifeRootParam     = nullptr;
    juce::AudioParameterBool*   lifePulseParam    = nullptr;
    juce::AudioParameterChoice* lifeRuleParam     = nullptr;
    juce::AudioParameterBool*   lifeConductorParam = nullptr;
    juce::AudioParameterBool*   lifeMicrotuneParam = nullptr;
    juce::AudioParameterInt*    lifeMicrotuneAmtParam = nullptr;

    float lastShuffleValue = 100.0f;

    // Virtual MIDI output via CoreMIDI (bypasses JUCE's broken singleton)
    MIDIClientRef midiClient = 0;
    MIDIEndpointRef midiEndpoint = 0;
    void sendMidiToVirtualPort(const uint8_t* data, int numBytes);

    // Debug: count MIDI events for UI display
    std::atomic<int> midiNoteOnCount { 0 };
    std::atomic<int> lastNoteOnPitch { -1 };
    std::atomic<int> udpSendCount { 0 };

    // Persisted editor window size
    int editorWidth = 800, editorHeight = 600;

    // Groove/shuffle
    static constexpr int kMaxGrooveSlots = 16;
    double grooves[kMaxGrooveSlots] = { 1.0 };
    int grooveLength = 1;
    int grooveIndex = 0;
    void setGroove(const double* ratios, int count);

    // ── Thread-safe pending message queues ──
    juce::SpinLock pendingLock;
    juce::SpinLock engineLock;
    std::atomic<bool> transportRunning { false };
    std::atomic<bool> standalonePlay { false };
    bool autoClean = false; // remove movers/bangs on transport stop

    // Pending MIDI events from Life mode transitions (UI thread → audio thread)
    static constexpr int kMaxPendingLifeEvents = 64;
    orca::MidiEvent pendingLifeEvents[kMaxPendingLifeEvents];
    int pendingLifeEventCount = 0;

    void pushLifeEvent(const orca::MidiEvent& e) {
        const juce::SpinLock::ScopedLockType lock(pendingLock);
        if (pendingLifeEventCount < kMaxPendingLifeEvents)
            pendingLifeEvents[pendingLifeEventCount++] = e;
    }

    // Pending commands from $ operator (audio thread → UI thread)
    static constexpr int kMaxPendingCommands = 16;
    static constexpr int kMaxPendingCommandLen = 64;
    char pendingCommands[kMaxPendingCommands][kMaxPendingCommandLen];
    int pendingCommandCount = 0;

    // Pending UDP messages (audio thread → UI thread)
    static constexpr int kMaxPendingUdp = 16;
    static constexpr int kMaxPendingUdpLen = 64;
    char pendingUdp[kMaxPendingUdp][kMaxPendingUdpLen];
    int pendingUdpCount = 0;

    // Pending OSC messages (audio thread → UI thread)
    struct PendingOscMsg {
        char path;
        char data[63];
        int dataLen;
    };
    static constexpr int kMaxPendingOsc = 16;
    PendingOscMsg pendingOsc[kMaxPendingOsc];
    int pendingOscCount = 0;

    // UDP output
    juce::String udpTargetIP { "127.0.0.1" };
    int udpOutputPort = 49161;
    std::unique_ptr<juce::DatagramSocket> udpSocket;

    // OSC output
    int oscOutputPort = 49162;
    std::unique_ptr<juce::OSCSender> oscSender;

    void setupUdpSocket();

    // ── Helpers for Commander → APVTS sync ──
    // Chord degree presets (must match createParameterLayout order)
    static constexpr const char* chordPresets[] = {
        "off", "135", "1357", "125", "145", "1356", "12356", "1234567"
    };
    static constexpr int numChordPresets = 8;

    static constexpr const char* rulePresets[] = {
        "life", "highlife", "seeds", "daynight", "diamoeba",
        "replicator", "2x2", "morley", "34life"
    };
    static constexpr int numRulePresets = 9;

    // Find closest chord preset index for a degree string
    int findChordPresetIndex(const juce::String& degrees) const;

private:
    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;
    double frameAccumulator  = 0.0;
    bool   wasPlaying        = false;
    int    lastPpqFrame      = -1;

    // Sync APVTS params → lifeGrid fields (called from processBlock)
    void syncParamsToLifeGrid();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrcaProcessor)
};
