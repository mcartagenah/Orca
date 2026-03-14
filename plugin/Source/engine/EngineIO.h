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
};

} // namespace orca
