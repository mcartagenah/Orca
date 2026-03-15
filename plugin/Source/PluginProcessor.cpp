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
    if (midiEndpoint) MIDIEndpointDispose(midiEndpoint);
    if (midiClient)   MIDIClientDispose(midiClient);
    midiEndpoint = 0;
    midiClient = 0;
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

    // Reset frames when DAW transport stops
    if (!isPlaying) {
        if (wasPlaying) {
            engine.grid.f = 0;
            frameAccumulator = 0.0;
            lastPpqFrame = -1;
            grooveIndex = 0;
            wasPlaying = false;
        }
        return;
    }
    wasPlaying = true;

    // Helper to dispatch MIDI events from a step
    auto dispatchEvents = [&](int sampleOffset) {
        orca::MidiEvent events[orca::kMaxEvents];
        int eventCount = engine.step(events, orca::kMaxEvents);
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
    };

    if (hasPpq) {
        // PPQ-based sync with groove support
        // Precompute cumulative PPQ thresholds for one groove cycle
        constexpr double ppqPerSixteenth = 0.25;
        double grooveCumulPpq[kMaxGrooveSlots + 1];
        grooveCumulPpq[0] = 0.0;
        for (int i = 0; i < grooveLength; i++)
            grooveCumulPpq[i + 1] = grooveCumulPpq[i] + grooves[i] * ppqPerSixteenth;
        double grooveCyclePpq = grooveCumulPpq[grooveLength]; // total PPQ for one cycle

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
            for (int i = 0; i < grooveLength; i++) {
                if (ppqInCycle >= grooveCumulPpq[i])
                    frameInCycle = i;
            }
            int grooveFrame = cycleNum * grooveLength + frameInCycle;

            if (grooveFrame != lastPpqFrame) {
                lastPpqFrame = grooveFrame;
                grooveIndex = frameInCycle; // update for UI debug display
                engine.grid.f = grooveFrame; // sync frame counter to DAW
                dispatchEvents(s);
            }
        }
    } else {
        // Fallback: accumulator-based timing with groove support
        double baseSamplesPerStep = currentSampleRate * 60.0 / (bpm * 4.0);
        int numSamples = buffer.getNumSamples();

        int samplePos = 0;
        while (samplePos < numSamples) {
            double currentStep = baseSamplesPerStep * grooves[grooveIndex];
            double samplesToNext = currentStep - frameAccumulator;
            int samplesToNextInt = static_cast<int>(std::ceil(samplesToNext));

            if (samplePos + samplesToNextInt <= numSamples) {
                samplePos += samplesToNextInt;
                frameAccumulator = 0.0;
                grooveIndex = (grooveIndex + 1) % grooveLength;
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

    copyXmlToBinary(*xml, destData);
}

void OrcaProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("OrcaState")) {
        int w = xml->getIntAttribute("w", 25);
        int h = xml->getIntAttribute("h", 25);
        int f = xml->getIntAttribute("f", 0);
        auto gridStr = xml->getStringAttribute("grid");
        engine.load(w, h, gridStr.toRawUTF8(), f);
        editorWidth = xml->getIntAttribute("editorW", 800);
        editorHeight = xml->getIntAttribute("editorH", 600);

        // Restore groove
        auto grooveStr = xml->getStringAttribute("groove", "1.0");
        auto parts = juce::StringArray::fromTokens(grooveStr, ";", "");
        grooveLength = juce::jlimit(1, kMaxGrooveSlots, parts.size());
        for (int i = 0; i < grooveLength; i++)
            grooves[i] = parts[i].getDoubleValue();
        grooveIndex = 0;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OrcaProcessor();
}
