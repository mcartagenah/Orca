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

    // Virtual MIDI output via CoreMIDI (bypasses JUCE's broken singleton)
    MIDIClientRef midiClient = 0;
    MIDIEndpointRef midiEndpoint = 0;
    void sendMidiToVirtualPort(const uint8_t* data, int numBytes);

    // Debug: count MIDI events for UI display
    std::atomic<int> midiNoteOnCount { 0 };
    std::atomic<int> lastNoteOnPitch { -1 };
    std::atomic<int> udpSendCount { 0 };  // total UDP messages queued from audio thread

    // Persisted editor window size
    int editorWidth = 800, editorHeight = 600;

    // Groove/shuffle
    static constexpr int kMaxGrooveSlots = 16;
    double grooves[kMaxGrooveSlots] = { 1.0 };
    int grooveLength = 1;
    int grooveIndex = 0;  // current position in groove cycle (accumulator path)
    void setGroove(const double* ratios, int count);

    // DAW-exposed shuffle parameter (0-200%, 100=straight)
    juce::AudioParameterFloat* shuffleParam = nullptr;
    float lastShuffleValue = 100.0f; // track changes (default = center/straight)

    // ── Thread-safe pending message queues ──
    // All queues protected by a single SpinLock (tiny critical sections,
    // audio-thread-safe, no allocation/syscall).
    juce::SpinLock pendingLock;

    // Protects engine state (grid/lifeGrid) from concurrent audio+UI access.
    // Audio thread locks during engine.step(), UI thread locks during mutations.
    juce::SpinLock engineLock;
    std::atomic<bool> transportRunning { false }; // set by processBlock, read by UI

    // Standalone transport: space bar toggles this when no DAW playhead is available
    std::atomic<bool> standalonePlay { false };

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

    // Initialize UDP socket (binds to any available port for sending)
    void setupUdpSocket();

private:
    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;
    double frameAccumulator  = 0.0;
    bool   wasPlaying        = false;
    int    lastPpqFrame      = -1;  // last frame derived from PPQ

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrcaProcessor)
};
