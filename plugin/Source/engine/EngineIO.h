#pragma once

#include "Types.h"
#include "MidiStack.h"
#include "CcStack.h"
#include "MonoStack.h"

namespace orca {

class EngineIO {
public:
    MidiStack midi;
    CcStack cc;
    MonoStack mono;

    void clear() {
        midi.clear();
        cc.clear();
        mono.clear();
    }

    // Collect all MIDI events from all stacks into output buffer
    // Returns total number of events
    int run(MidiEvent* outEvents, int maxEvents) {
        int total = 0;

        // Process note stack
        int midiEvents = midi.run(outEvents + total, maxEvents - total);
        total += midiEvents;

        // Process CC stack
        int ccEvents = cc.run(outEvents + total, maxEvents - total);
        total += ccEvents;

        // Process mono stack
        int monoEvents = mono.run(outEvents + total, maxEvents - total);
        total += monoEvents;

        return total;
    }

    void silence(MidiEvent* outEvents, int maxEvents, int& eventCount) {
        midi.silence(outEvents, maxEvents, eventCount);
        mono.silence(outEvents, maxEvents, eventCount);
    }

    // Command queue for $ (self) operator
    static constexpr int kMaxCommands = 16;
    static constexpr int kMaxCommandLen = 64;
    char commands[kMaxCommands][kMaxCommandLen];
    int commandCount = 0;

    void pushCommand(const char* cmd, int len) {
        if (commandCount >= kMaxCommands) return;
        int n = (len < kMaxCommandLen - 1) ? len : kMaxCommandLen - 1;
        for (int i = 0; i < n; i++) commands[commandCount][i] = cmd[i];
        commands[commandCount][n] = '\0';
        commandCount++;
    }

    void clearCommands() { commandCount = 0; }

    // UDP message queue for ; operator
    static constexpr int kMaxUdpMessages = 16;
    static constexpr int kMaxUdpLen = 64;
    char udpMessages[kMaxUdpMessages][kMaxUdpLen];
    int udpCount = 0;

    void pushUdp(const char* msg, int len) {
        if (udpCount >= kMaxUdpMessages) return;
        int n = (len < kMaxUdpLen - 1) ? len : kMaxUdpLen - 1;
        for (int i = 0; i < n; i++) udpMessages[udpCount][i] = msg[i];
        udpMessages[udpCount][n] = '\0';
        udpCount++;
    }

    void clearUdp() { udpCount = 0; }

    // OSC message queue for = operator
    static constexpr int kMaxOscMessages = 16;
    static constexpr int kMaxOscLen = 64;
    struct OscMessage {
        char path;       // single char path (prepended with '/')
        char data[63];   // message data (values)
        int dataLen;
    };
    OscMessage oscMessages[kMaxOscMessages];
    int oscCount = 0;

    void pushOsc(char path, const char* data, int len) {
        if (oscCount >= kMaxOscMessages) return;
        auto& m = oscMessages[oscCount];
        m.path = path;
        m.dataLen = (len < 62) ? len : 62;
        for (int i = 0; i < m.dataLen; i++) m.data[i] = data[i];
        m.data[m.dataLen] = '\0';
        oscCount++;
    }

    void clearOsc() { oscCount = 0; }
};

} // namespace orca
