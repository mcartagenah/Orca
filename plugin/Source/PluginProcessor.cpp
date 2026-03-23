#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <mach/mach_time.h>

OrcaProcessor::OrcaProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    engine.reset(25, 25);

    // DAW-exposed shuffle parameter (0-200%)
    // 0% = max inverse swing (1;99), 100% = straight, 200% = max swing (99;1)
    auto* param = new juce::AudioParameterFloat(
        juce::ParameterID("shuffle", 1),
        "Shuffle",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%"));
    shuffleParam = param;
    addParameter(param);

    // Create virtual MIDI port via CoreMIDI C API (bypasses JUCE's broken singleton)
    OSStatus status = MIDIClientCreate(CFSTR("OrcaPlugin"), nullptr, nullptr, &midiClient);
    if (status == noErr)
        MIDISourceCreate(midiClient, CFSTR("Orca"), &midiEndpoint);
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

    copyXmlToBinary(*xml, destData);
}

void OrcaProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("OrcaState")) {
        int w = xml->getIntAttribute("w", 25);
        int h = xml->getIntAttribute("h", 25);
        int f = xml->getIntAttribute("f", 0);
        auto gridStr = xml->getStringAttribute("grid");
        {
            const juce::SpinLock::ScopedLockType lock(engineLock);
            engine.load(w, h, gridStr.toRawUTF8(), f);

            // Restore Life mode state
            engine.paintChannel = static_cast<uint8_t>(xml->getIntAttribute("paintChannel", 0));
            engine.paintOctave = static_cast<uint8_t>(xml->getIntAttribute("paintOctave", 3));
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
                            int ch = vals[3].getIntValue();
                            int oct = vals[4].getIntValue();
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
                engine.lifeGrid.currentScale = static_cast<orca::LifeGrid::ScaleType>(
                    juce::jlimit(0, (int)orca::LifeGrid::NumScales - 1,
                                 xml->getIntAttribute("lifeScale", 0)));
                engine.lifeGrid.rootNote = juce::jlimit(0, 11, xml->getIntAttribute("lifeRoot", 0));
                engine.lifeGrid.evolveRate = juce::jlimit(1, 32, xml->getIntAttribute("lifeRate", 4));
                engine.lifeGrid.pulseMode = (xml->getIntAttribute("lifePulse", 0) != 0);
            }
        } // engineLock scope

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
