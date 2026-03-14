#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <mach/mach_time.h>

OrcaProcessor::OrcaProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    engine.reset(25, 25);

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
        // PPQ-based sync: 1 Orca frame = 1 sixteenth note = 0.25 PPQ
        // Derive frame directly from DAW position — no drift possible
        double samplesPerStep = currentSampleRate * 60.0 / (bpm * 4.0);
        int numSamples = buffer.getNumSamples();
        double ppqPerSample = bpm / (60.0 * currentSampleRate);

        for (int s = 0; s < numSamples; s++) {
            double ppq = ppqPosition + s * ppqPerSample;
            int ppqFrame = static_cast<int>(std::floor(ppq * 4.0)); // 4 sixteenths per beat
            if (ppqFrame != lastPpqFrame) {
                lastPpqFrame = ppqFrame;
                engine.grid.f = ppqFrame; // sync frame counter to DAW
                dispatchEvents(s);
            }
        }
    } else {
        // Fallback: accumulator-based timing (no PPQ available)
        double samplesPerStep = currentSampleRate * 60.0 / (bpm * 4.0);
        int numSamples = buffer.getNumSamples();

        int samplePos = 0;
        while (samplePos < numSamples) {
            double samplesToNext = samplesPerStep - frameAccumulator;
            int samplesToNextInt = static_cast<int>(std::ceil(samplesToNext));

            if (samplePos + samplesToNextInt <= numSamples) {
                samplePos += samplesToNextInt;
                frameAccumulator = 0.0;
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
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OrcaProcessor();
}
