#pragma once

#include "Types.h"

namespace orca {

class CcStack {
public:
    CcEntry entries[kMaxCCs];
    int count = 0;
    int offset = 64;

    void clear() {
        count = 0;
    }

    void pushCC(int channel, int knob, int value) {
        if (count >= kMaxCCs) return;
        auto& e = entries[count++];
        e.type = CcEntry::TypeCC;
        e.channel = channel;
        e.knob = knob;
        e.value = value;
        e.active = true;
    }

    void pushPB(int channel, int lsb, int msb) {
        if (count >= kMaxCCs) return;
        auto& e = entries[count++];
        e.type = CcEntry::TypePB;
        e.channel = channel;
        e.lsb = lsb;
        e.msb = msb;
        e.active = true;
    }

    int run(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        for (int i = 0; i < count; i++) {
            if (!entries[i].active) continue;
            if (entries[i].type == CcEntry::TypeCC && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::CC;
                e.bytes[0] = static_cast<uint8_t>(0xB0 + entries[i].channel);
                e.bytes[1] = static_cast<uint8_t>(offset + entries[i].knob);
                e.bytes[2] = static_cast<uint8_t>(entries[i].value);
                e.numBytes = 3;
            } else if (entries[i].type == CcEntry::TypePB && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::PitchBend;
                e.bytes[0] = static_cast<uint8_t>(0xE0 + entries[i].channel);
                e.bytes[1] = static_cast<uint8_t>(entries[i].lsb);
                e.bytes[2] = static_cast<uint8_t>(entries[i].msb);
                e.numBytes = 3;
            }
        }
        return eventCount;
    }

    void setOffset(int newOffset) {
        offset = newOffset;
    }
};

} // namespace orca
