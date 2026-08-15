#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <mach/mach_time.h>

juce::AudioProcessorValueTreeState::ParameterLayout OrcaProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ── Shuffle ──
    auto shuffleGroup = std::make_unique<juce::AudioProcessorParameterGroup>("shuffle_group", "Shuffle", "|");
    shuffleGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("shuffle", 1), "Shuffle",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::move(shuffleGroup));

    // ── Life: Timing ──
    auto timingGroup = std::make_unique<juce::AudioProcessorParameterGroup>("life_timing", "Life: Timing", "|");
    timingGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_rate", 1), "Evolve Rate", 1, 32, 4));
    timingGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("life_seq", 1), "Seq Mode",
        juce::StringArray{"Off", "Forward", "Reverse", "Mirror", "Random", "Euclid"}, 0));
    timingGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_euclid", 1), "Euclid Pulses", 1, 32, 3));
    timingGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_seq_horiz", 1), "Seq Horizontal", false));
    timingGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_pulse", 1), "Pulse Mode", true));
    timingGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_conductor", 1), "Conductor Mode", false));
    layout.add(std::move(timingGroup));

    // ── Life: Pitch ──
    auto pitchGroup = std::make_unique<juce::AudioProcessorParameterGroup>("life_pitch", "Life: Pitch", "|");
    pitchGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("life_scale", 1), "Scale",
        juce::StringArray{"Chromatic", "Major", "Minor", "Pentatonic", "Dorian",
                          "Phrygian", "Lydian", "Mixolydian", "Locrian",
                          "Harmonic Minor", "Melodic Minor", "Minor Pentatonic",
                          "Blues", "Whole Tone", "Diminished"}, 0));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("life_root", 1), "Root Note",
        juce::StringArray{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}, 0));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_minoct", 1), "Min Octave", 0, 8, 0));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_maxoct", 1), "Max Octave", 0, 8, 7));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_lockoct", 1), "Lock Octave", false));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("life_chord", 1), "Chord Filter",
        juce::StringArray{"Off", "135", "1357", "125", "145", "1356", "12356", "1234567"}, 0));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_microtune", 1), "Microtuning", false));
    pitchGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_microtune_amt", 1), "Microtune Amount", 0, 100, 50));
    layout.add(std::move(pitchGroup));

    // ── Life: Dynamics ──
    auto dynGroup = std::make_unique<juce::AudioProcessorParameterGroup>("life_dynamics", "Life: Dynamics", "|");
    dynGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_decay", 1), "Decay", false));
    dynGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_minvel", 1), "Min Velocity", 1, 127, 40));
    dynGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_minprob", 1), "Min Probability", 1, 100, 10));
    dynGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_maxnotes", 1), "Max Notes", 0, 32, 0));
    layout.add(std::move(dynGroup));

    // ── Life: Processing ──
    auto procGroup = std::make_unique<juce::AudioProcessorParameterGroup>("life_processing", "Life: Processing", "|");
    procGroup->addChild(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("life_dedup", 1), "Dedup", false));
    procGroup->addChild(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("life_dedupcc", 1), "Dedup CC", -1, 127, -1));
    procGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("life_rule", 1), "CA Rule",
        juce::StringArray{"Life", "HighLife", "Seeds", "Day & Night", "Diamoeba",
                          "Replicator", "2x2", "Morley", "34 Life"}, 0));
    layout.add(std::move(procGroup));

    return layout;
}

OrcaProcessor::OrcaProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    engine.reset(25, 25);

    // Cache raw parameter pointers for fast audio-thread reads
    shuffleParam     = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter("shuffle"));
    lifeRateParam    = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_rate"));
    lifeDecayParam   = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter("life_decay"));
    lifeMinVelParam  = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_minvel"));
    lifeMinProbParam = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_minprob"));
    lifeMaxNotesParam= dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_maxnotes"));
    lifeSeqParam     = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("life_seq"));
    lifeLockOctParam = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter("life_lockoct"));
    lifeChordParam   = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("life_chord"));
    lifeDedupParam   = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter("life_dedup"));
    lifeDedupCCParam = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_dedupcc"));
    lifeMinOctParam  = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_minoct"));
    lifeMaxOctParam  = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_maxoct"));
    lifeScaleParam   = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("life_scale"));
    lifeRootParam    = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("life_root"));
    lifePulseParam   = dynamic_cast<juce::AudioParameterBool*>  (apvts.getParameter("life_pulse"));
    lifeEuclidParam  = dynamic_cast<juce::AudioParameterInt*>   (apvts.getParameter("life_euclid"));
    lifeSeqHorizParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter("life_seq_horiz"));
    lifeRuleParam    = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("life_rule"));
    lifeConductorParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("life_conductor"));
    lifeMicrotuneParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("life_microtune"));
    lifeMicrotuneAmtParam = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("life_microtune_amt"));

    // Create virtual MIDI port via CoreMIDI C API (bypasses JUCE's broken singleton)
    midiClient = 0;
    midiEndpoint = 0;
    OSStatus status = MIDIClientCreate(CFSTR("OrcaPlugin"), nullptr, nullptr, &midiClient);
    if (status == noErr)
        MIDISourceCreate(midiClient, CFSTR("Orca"), &midiEndpoint);
}

int OrcaProcessor::findChordPresetIndex(const juce::String& degrees) const {
    for (int i = 0; i < numChordPresets; i++)
        if (degrees == chordPresets[i]) return i;
    return 0; // "off"
}

void OrcaProcessor::syncParamsToLifeGrid() {
    auto& lg = engine.lifeGrid;
    int newRate     = lifeRateParam->get();
    bool rateChanged = (newRate != lg.evolveRate);
    lg.evolveRate   = newRate;
    lg.decay        = lifeDecayParam->get();
    lg.minVelocity  = static_cast<uint8_t>(lifeMinVelParam->get());
    lg.minProb      = lifeMinProbParam->get();
    lg.maxNotes     = lifeMaxNotesParam->get();
    auto newSeqMode  = static_cast<orca::LifeGrid::SeqMode>(lifeSeqParam->getIndex());
    if (newSeqMode == orca::LifeGrid::SeqRandom &&
        (lg.seqMode != orca::LifeGrid::SeqRandom || rateChanged))
        lg.shufflePhaseTable();
    int newEuclidPulses = lifeEuclidParam->get();
    if (newSeqMode == orca::LifeGrid::SeqEuclid &&
        (lg.seqMode != orca::LifeGrid::SeqEuclid || rateChanged || newEuclidPulses != lg.euclidPulses)) {
        lg.euclidPulses = newEuclidPulses;
        lg.generateEuclidean();
    }
    lg.seqMode      = newSeqMode;
    lg.seqHorizontal = lifeSeqHorizParam->get();
    lg.lockOctave   = lifeLockOctParam->get();
    lg.dedup        = lifeDedupParam->get();
    lg.dedupCC      = lifeDedupCCParam->get();
    lg.minOctave    = lifeMinOctParam->get();
    lg.maxOctave    = lifeMaxOctParam->get();
    if (lg.minOctave > lg.maxOctave) lg.minOctave = lg.maxOctave;
    lg.pulseMode    = lifePulseParam->get();
    lg.conductorMode = lifeConductorParam->get();
    lg.microtuning  = lifeMicrotuneParam->get();
    lg.microtuneAmount = lifeMicrotuneAmtParam->get();
    lg.currentScale = static_cast<orca::LifeGrid::ScaleType>(lifeScaleParam->getIndex());
    lg.rootNote     = lifeRootParam->getIndex();

    // Chord filter: map choice index to degree string
    int chordIdx = lifeChordParam->getIndex();
    if (chordIdx == 0)
        lg.chordDegreeCount = 0;
    else
        lg.setChordDegrees(chordPresets[chordIdx]);

    // CA rule: map choice index to preset
    int ruleIdx = lifeRuleParam->getIndex();
    if (ruleIdx >= 0 && ruleIdx < numRulePresets)
        lg.setRulePreset(rulePresets[ruleIdx]);
}

OrcaProcessor::~OrcaProcessor() {
    // Tear down network sockets before MIDI
    oscSender.reset();
    udpSocket.reset();

    if (midiEndpoint) MIDIEndpointDispose(midiEndpoint);
    if (midiClient)   MIDIClientDispose(midiClient);
    midiEndpoint = 0;
    midiClient = 0;
}

void OrcaProcessor::setupUdpSocket() {
    udpSocket = std::make_unique<juce::DatagramSocket>();
    if (!udpSocket->bindToPort(0))
        DBG("UDP: Failed to bind socket");
}

void OrcaProcessor::sendMidiToVirtualPort(const uint8_t* data, int numBytes) {
    if (!midiEndpoint || numBytes <= 0) return;

    MIDIPacketList packetList;
    MIDIPacket* packet = MIDIPacketListInit(&packetList);
    packet = MIDIPacketListAdd(&packetList, sizeof(packetList), packet,
                               mach_absolute_time(), numBytes, data);
    if (packet)
        MIDIReceived(midiEndpoint, &packetList);
}

void OrcaProcessor::setGroove(const double* ratios, int count) {
    grooveLength = juce::jlimit(1, kMaxGrooveSlots, count);
    for (int i = 0; i < grooveLength; i++)
        grooves[i] = ratios[i];
    grooveIndex = 0;
}

const juce::String OrcaProcessor::getName() const { return JucePlugin_Name; }
bool OrcaProcessor::acceptsMidi() const { return true; }
bool OrcaProcessor::producesMidi() const { return true; }
bool OrcaProcessor::isMidiEffect() const { return false; }
double OrcaProcessor::getTailLengthSeconds() const { return 0.0; }
int OrcaProcessor::getNumPrograms() { return 1; }
int OrcaProcessor::getCurrentProgram() { return 0; }
void OrcaProcessor::setCurrentProgram(int) {}
const juce::String OrcaProcessor::getProgramName(int) { return {}; }
void OrcaProcessor::changeProgramName(int, const juce::String&) {}

void OrcaProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    frameAccumulator = 0.0;
}

void OrcaProcessor::releaseResources() {}

bool OrcaProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void OrcaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear(); // synth outputs silence

    // Sync shuffle parameter → groove array when slider is moved by user/DAW
    // 0% = max inverse swing (1;99;50), 100% = straight, 200% = max swing (99;1;50)
    float currentShuffle = shuffleParam->get();
    if (currentShuffle != lastShuffleValue) {
        lastShuffleValue = currentShuffle;
        if (std::abs(currentShuffle - 100.0f) < 0.5f) {
            // Center (100%) = straight timing
            double r = 1.0;
            setGroove(&r, 1);
        } else {
            // Map slider 0-200 to iv0 1-99 linearly, centered at 100→50
            //   0%   → iv0=1  → groove [1;99;50] (max inverse)
            //  50%   → iv0=25 → groove [25;75;50]
            // 100%   → straight (handled above)
            // 150%   → iv0=75 → groove [75;25;50]
            // 200%   → iv0=99 → groove [99;1;50] (max swing)
            // Symmetric mapping centered at 100:
            //   50→25, 100→50(straight), 150→75, 0→1, 200→99
            double half = (currentShuffle - 100.0) / 2.0; // -50 to +50
            int iv0 = juce::jlimit(1, 99, 50 + (int)std::round(half));
            int iv1 = 100 - iv0;
            double ratios[3] = { iv0 / 50.0, iv1 / 50.0, 1.0 };
            setGroove(ratios, 3);
        }
    }

    // Sync Life mode parameters from APVTS → engine
    if (engine.lifeMode)
        syncParamsToLifeGrid();

    // Conductor mode: scan incoming MIDI for NoteOn to trigger evolution
    if (engine.lifeMode && engine.lifeGrid.conductorMode) {
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                engine.lifeGrid.conductorTrigger.store(true, std::memory_order_relaxed);
        }
    }

    // Get DAW transport state
    double bpm = 120.0;
    bool isPlaying = false;
    bool hasPpq = false;
    double ppqPosition = 0.0;

    if (auto* playHead = getPlayHead()) {
        if (auto pos = playHead->getPosition()) {
            if (auto b = pos->getBpm())
                bpm = *b;
            isPlaying = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) {
                hasPpq = true;
                ppqPosition = *ppq;
            }
        }
    }

    // Standalone mode: use internal play state with accumulator timing
    if (wrapperType == wrapperType_Standalone && standalonePlay.load(std::memory_order_relaxed)) {
        isPlaying = true;
        hasPpq = false; // force accumulator path — standalone has no meaningful PPQ
    }

    transportRunning.store(isPlaying, std::memory_order_relaxed);

    // Reset frames when DAW transport stops
    if (!isPlaying) {
        if (wasPlaying) {
            engine.grid.f = 0;
            frameAccumulator = 0.0;
            lastPpqFrame = -1;
            grooveIndex = 0;
            wasPlaying = false;
            // Silence Life mode notes on stop and reset sequencer phase
            // Auto-clean movers/bangs from Orca grid on stop
            if (!engine.lifeMode && autoClean) {
                auto& g = engine.grid;
                for (int y = 0; y < g.h; y++) {
                    bool inComment = false;
                    for (int x = 0; x < g.w; x++) {
                        char ch = g.glyphAt(x, y);
                        if (ch == '#') { inComment = true; continue; }
                        if (inComment) continue;
                        if (ch == 'N' || ch == 'n' || ch == 'S' || ch == 's' ||
                            ch == 'E' || ch == 'e' || ch == 'W' || ch == 'w' || ch == '*') {
                            if (y > 0) {
                                char above = g.glyphAt(x, y - 1);
                                if (above == 'H' || above == 'h') continue;
                            }
                            g.write(x, y, '.');
                        }
                    }
                }
            }
            if (engine.lifeMode) {
                orca::MidiEvent events[orca::kMaxEvents];
                int eventCount = 0;
                // Silence ratchets first
                eventCount += engine.lifeGrid.silenceRatchets(events + eventCount, orca::kMaxEvents - eventCount);
                engine.lifeGrid.ratchetCount = 0;
                // Silence all active notes
                eventCount += engine.lifeGrid.silence(events + eventCount, orca::kMaxEvents - eventCount);
                for (int i = 0; i < eventCount; i++) {
                    midiMessages.addEvent(events[i].bytes, events[i].numBytes, 0);
                    sendMidiToVirtualPort(events[i].bytes, events[i].numBytes);
                }
                // Reset sequencer state so next step() triggers evolution immediately
                engine.lifeGrid.frameCounter = engine.lifeGrid.evolveRate - 1;
                engine.lifeGrid.phaseNoteCount = 0;
                engine.lifeGrid.firstEvolution = true;
            }
        }
        return;
    }
    // On transport start: reset frameCounter so first frame hits cycle boundary immediately
    if (!wasPlaying && engine.lifeMode) {
        engine.lifeGrid.frameCounter = engine.lifeGrid.evolveRate - 1;
        engine.lifeGrid.firstEvolution = true;
    }
    // Retrigger alive Life notes on transport start (skip in seq mode — let phase handle it)
    if (!wasPlaying && engine.lifeMode && engine.lifeGrid.seqMode == orca::LifeGrid::SeqOff) {
        orca::MidiEvent events[orca::kMaxEvents];
        int count = engine.lifeGrid.triggerAlive(events, orca::kMaxEvents);
        for (int i = 0; i < count; i++) {
            midiMessages.addEvent(events[i].bytes, events[i].numBytes, 0);
            sendMidiToVirtualPort(events[i].bytes, events[i].numBytes);
        }
    }
    wasPlaying = true;

    // Dispatch any pending Life mode events (e.g. note-offs from mode exit)
    {
        orca::MidiEvent localEvents[kMaxPendingLifeEvents];
        int localCount = 0;
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            localCount = pendingLifeEventCount;
            for (int i = 0; i < localCount; i++)
                localEvents[i] = pendingLifeEvents[i];
            pendingLifeEventCount = 0;
        }
        for (int i = 0; i < localCount; i++) {
            midiMessages.addEvent(localEvents[i].bytes, localEvents[i].numBytes, 0);
            sendMidiToVirtualPort(localEvents[i].bytes, localEvents[i].numBytes);
        }
    }

    // Helper to dispatch MIDI events from a step
    auto dispatchEvents = [&](int sampleOffset) {
        orca::MidiEvent events[orca::kMaxEvents];
        int eventCount;
        {
            const juce::SpinLock::ScopedLockType lock(engineLock);
            eventCount = engine.step(events, orca::kMaxEvents);
        }
        for (int i = 0; i < eventCount; i++) {
            auto& e = events[i];
            midiMessages.addEvent(e.bytes, e.numBytes,
                                   juce::jmin(sampleOffset, buffer.getNumSamples() - 1));
            sendMidiToVirtualPort(e.bytes, e.numBytes);
            if (e.numBytes >= 3 && (e.bytes[0] & 0xF0) == 0x90 && e.bytes[2] > 0) {
                midiNoteOnCount.fetch_add(1, std::memory_order_relaxed);
                lastNoteOnPitch.store(e.bytes[1], std::memory_order_relaxed);
            }
        }
        // Drain $ operator commands, UDP, and OSC messages to UI thread
        if (engine.io.udpCount > 0) {
            udpSendCount.fetch_add(engine.io.udpCount, std::memory_order_relaxed);
            DBG("dispatchEvents: udp=" + juce::String(engine.io.udpCount) + " osc=" + juce::String(engine.io.oscCount));
        }
        if (engine.io.commandCount > 0 || engine.io.udpCount > 0 || engine.io.oscCount > 0) {
            const juce::SpinLock::ScopedLockType lock(pendingLock);

            // Commands
            for (int i = 0; i < engine.io.commandCount && pendingCommandCount < kMaxPendingCommands; i++) {
                int len = 0;
                while (len < kMaxPendingCommandLen - 1 && engine.io.commands[i][len] != '\0') {
                    pendingCommands[pendingCommandCount][len] = engine.io.commands[i][len];
                    len++;
                }
                pendingCommands[pendingCommandCount][len] = '\0';
                pendingCommandCount++;
            }
            engine.io.clearCommands();

            // UDP
            for (int i = 0; i < engine.io.udpCount && pendingUdpCount < kMaxPendingUdp; i++) {
                int len = 0;
                while (len < kMaxPendingUdpLen - 1 && engine.io.udpMessages[i][len] != '\0') {
                    pendingUdp[pendingUdpCount][len] = engine.io.udpMessages[i][len];
                    len++;
                }
                pendingUdp[pendingUdpCount][len] = '\0';
                pendingUdpCount++;
            }
            engine.io.clearUdp();

            // OSC
            for (int i = 0; i < engine.io.oscCount && pendingOscCount < kMaxPendingOsc; i++) {
                pendingOsc[pendingOscCount].path = engine.io.oscMessages[i].path;
                pendingOsc[pendingOscCount].dataLen = engine.io.oscMessages[i].dataLen;
                for (int j = 0; j < engine.io.oscMessages[i].dataLen; j++)
                    pendingOsc[pendingOscCount].data[j] = engine.io.oscMessages[i].data[j];
                pendingOsc[pendingOscCount].data[engine.io.oscMessages[i].dataLen] = '\0';
                pendingOscCount++;
            }
            engine.io.clearOsc();
        }
    };

    if (hasPpq) {
        // PPQ-based sync with groove support
        // Snapshot groove config under lock (UI thread may update via commander)
        double localGrooves[kMaxGrooveSlots];
        int localGrooveLength;
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            localGrooveLength = grooveLength;
            for (int i = 0; i < localGrooveLength; i++)
                localGrooves[i] = grooves[i];
        }

        // Precompute cumulative PPQ thresholds for one groove cycle
        constexpr double ppqPerSixteenth = 0.25;
        double grooveCumulPpq[kMaxGrooveSlots + 1];
        grooveCumulPpq[0] = 0.0;
        for (int i = 0; i < localGrooveLength; i++)
            grooveCumulPpq[i + 1] = grooveCumulPpq[i] + localGrooves[i] * ppqPerSixteenth;
        double grooveCyclePpq = grooveCumulPpq[localGrooveLength]; // total PPQ for one cycle

        int numSamples = buffer.getNumSamples();
        double ppqPerSample = bpm / (60.0 * currentSampleRate);

        for (int s = 0; s < numSamples; s++) {
            double ppq = ppqPosition + s * ppqPerSample;
            if (ppq < 0.0) continue;

            // Map absolute PPQ to groove-aware frame
            int cycleNum = static_cast<int>(std::floor(ppq / grooveCyclePpq));
            double ppqInCycle = ppq - cycleNum * grooveCyclePpq;

            // Find which frame within the cycle
            int frameInCycle = 0;
            for (int i = 0; i < localGrooveLength; i++) {
                if (ppqInCycle >= grooveCumulPpq[i])
                    frameInCycle = i;
            }
            int grooveFrame = cycleNum * localGrooveLength + frameInCycle;

            if (grooveFrame != lastPpqFrame) {
                lastPpqFrame = grooveFrame;
                grooveIndex = frameInCycle; // update for UI debug display
                engine.grid.f = grooveFrame; // sync frame counter to DAW
                dispatchEvents(s);
            }
        }
    } else {
        // Fallback: accumulator-based timing with groove support
        // Snapshot groove config under lock
        double localGrooves[kMaxGrooveSlots];
        int localGrooveLength;
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            localGrooveLength = grooveLength;
            for (int i = 0; i < localGrooveLength; i++)
                localGrooves[i] = grooves[i];
        }

        double baseSamplesPerStep = currentSampleRate * 60.0 / (bpm * 4.0);
        int numSamples = buffer.getNumSamples();

        int samplePos = 0;
        while (samplePos < numSamples) {
            int gi = grooveIndex % localGrooveLength;
            double currentStep = baseSamplesPerStep * localGrooves[gi];
            double samplesToNext = currentStep - frameAccumulator;
            int samplesToNextInt = static_cast<int>(std::ceil(samplesToNext));

            if (samplePos + samplesToNextInt <= numSamples) {
                samplePos += samplesToNextInt;
                frameAccumulator = 0.0;
                grooveIndex = (grooveIndex + 1) % localGrooveLength;
                dispatchEvents(samplePos);
            } else {
                frameAccumulator += static_cast<double>(numSamples - samplePos);
                break;
            }
        }
    }
}

juce::AudioProcessorEditor* OrcaProcessor::createEditor() {
    return new OrcaEditor(*this);
}

bool OrcaProcessor::hasEditor() const { return true; }

void OrcaProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto xml = std::make_unique<juce::XmlElement>("OrcaState");
    xml->setAttribute("w", engine.grid.w);
    xml->setAttribute("h", engine.grid.h);
    xml->setAttribute("f", engine.grid.f);

    // Save grid content as string
    juce::String gridStr;
    int size = engine.grid.w * engine.grid.h;
    for (int i = 0; i < size; i++)
        gridStr += juce::String::charToString(engine.grid.cells[i]);
    xml->setAttribute("grid", gridStr);
    xml->setAttribute("editorW", editorWidth);
    xml->setAttribute("editorH", editorHeight);

    // Save groove
    juce::String grooveStr;
    for (int i = 0; i < grooveLength; i++) {
        if (i > 0) grooveStr += ";";
        grooveStr += juce::String(grooves[i]);
    }
    xml->setAttribute("groove", grooveStr);

    // Save Life mode state
    xml->setAttribute("lifeMode", engine.lifeMode ? 1 : 0);
    xml->setAttribute("paintChannel", (int)engine.paintChannel);
    xml->setAttribute("paintOctave", (int)engine.paintOctave);
    if (engine.lifeMode) {
        // Save alive cells as compact format: x,y,note,channel,octave;...
        juce::String lifeStr;
        for (int y = 0; y < engine.lifeGrid.h; y++) {
            for (int x = 0; x < engine.lifeGrid.w; x++) {
                int idx = x + engine.lifeGrid.w * y;
                auto& cell = engine.lifeGrid.cells[idx];
                if (cell.alive) {
                    if (lifeStr.isNotEmpty()) lifeStr += ";";
                    lifeStr += juce::String(x) + "," + juce::String(y) + ","
                             + juce::String::charToString(cell.note) + ","
                             + juce::String((int)cell.channel) + ","
                             + juce::String((int)cell.octave) + ","
                             + juce::String(cell.locked ? 1 : 0);
                }
            }
        }
        xml->setAttribute("lifeCells", lifeStr);
        xml->setAttribute("lifeScale", (int)engine.lifeGrid.currentScale);
        xml->setAttribute("lifeRoot", engine.lifeGrid.rootNote);
        xml->setAttribute("lifeRate", engine.lifeGrid.evolveRate);
        xml->setAttribute("lifePulse", engine.lifeGrid.pulseMode ? 1 : 0);
    }

    // Save UDP/OSC config
    xml->setAttribute("udpIP", udpTargetIP);
    xml->setAttribute("udpPort", udpOutputPort);
    xml->setAttribute("oscPort", oscOutputPort);

    // Save APVTS state (all automatable parameters)
    auto apvtsState = apvts.copyState();
    xml->addChildElement(apvtsState.createXml().release());

    copyXmlToBinary(*xml, destData);
}

void OrcaProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("OrcaState")) {
        int w = juce::jlimit(1, orca::kMaxGridW,
                             xml->getIntAttribute("w", 25));
        int h = juce::jlimit(1, orca::kMaxGridH,
                             xml->getIntAttribute("h", 25));
        int f = xml->getIntAttribute("f", 0);
        auto gridStr = xml->getStringAttribute("grid");
        {
            const juce::SpinLock::ScopedLockType lock(engineLock);
            engine.load(w, h, gridStr.toRawUTF8(),
                        gridStr.getNumBytesAsUTF8(), f);

            // Restore Life mode state
            engine.paintChannel = static_cast<uint8_t>(juce::jlimit(
                0, 15, xml->getIntAttribute("paintChannel", 0)));
            engine.paintOctave = static_cast<uint8_t>(juce::jlimit(
                0, 8, xml->getIntAttribute("paintOctave", 3)));
            if (xml->getIntAttribute("lifeMode", 0) == 1) {
                engine.lifeMode = true;
                engine.lifeGrid.resize(w, h);
                auto lifeStr = xml->getStringAttribute("lifeCells", "");
                if (lifeStr.isNotEmpty()) {
                    auto cells = juce::StringArray::fromTokens(lifeStr, ";", "");
                    for (auto& cellStr : cells) {
                        auto vals = juce::StringArray::fromTokens(cellStr, ",", "");
                        if (vals.size() >= 5) {
                            int cx = vals[0].getIntValue();
                            int cy = vals[1].getIntValue();
                            char note = vals[2][0];
                            int ch = juce::jlimit(0, 15, vals[3].getIntValue());
                            int oct = juce::jlimit(0, 8, vals[4].getIntValue());
                            int idx = engine.lifeGrid.indexAt(cx, cy);
                            if (idx >= 0) {
                                engine.lifeGrid.cells[idx].note = note;
                                engine.lifeGrid.cells[idx].channel = static_cast<uint8_t>(ch);
                                engine.lifeGrid.cells[idx].octave = static_cast<uint8_t>(oct);
                                engine.lifeGrid.cells[idx].alive = true;
                                engine.lifeGrid.cells[idx].locked = (vals.size() >= 6 && vals[5].getIntValue() != 0);
                            }
                        }
                    }
                }
                // Legacy Life params (for old saves without APVTS)
                engine.lifeGrid.currentScale = static_cast<orca::LifeGrid::ScaleType>(
                    juce::jlimit(0, (int)orca::LifeGrid::NumScales - 1,
                                 xml->getIntAttribute("lifeScale", 0)));
                engine.lifeGrid.rootNote = juce::jlimit(0, 11, xml->getIntAttribute("lifeRoot", 0));
                engine.lifeGrid.evolveRate = juce::jlimit(1, 32, xml->getIntAttribute("lifeRate", 4));
                engine.lifeGrid.pulseMode = (xml->getIntAttribute("lifePulse", 0) != 0);
            }
        } // engineLock scope

        // Restore APVTS state (overrides legacy params if present)
        auto* apvtsXml = xml->getChildByName(apvts.state.getType());
        if (apvtsXml)
            apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));

        // Sync restored APVTS values to engine so Life params match saved state
        if (engine.lifeMode)
            syncParamsToLifeGrid();

        editorWidth = xml->getIntAttribute("editorW", 800);
        editorHeight = xml->getIntAttribute("editorH", 600);

        // Restore groove
        auto grooveStr = xml->getStringAttribute("groove", "1.0");
        auto parts = juce::StringArray::fromTokens(grooveStr, ";", "");
        grooveLength = juce::jlimit(1, kMaxGrooveSlots, parts.size());
        for (int i = 0; i < grooveLength; i++)
            grooves[i] = parts[i].getDoubleValue();
        grooveIndex = 0;

        // Restore UDP/OSC config (lock protects juce::String from concurrent UI reads)
        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            udpTargetIP = xml->getStringAttribute("udpIP", "127.0.0.1");
            udpOutputPort = xml->getIntAttribute("udpPort", 49161);
            oscOutputPort = xml->getIntAttribute("oscPort", 49162);
        }
        // Reset sockets so they reconnect with new config on next use
        udpSocket.reset();
        oscSender.reset();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OrcaProcessor();
}
