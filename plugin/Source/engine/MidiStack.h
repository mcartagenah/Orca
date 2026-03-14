#pragma once

#include "Types.h"
#include "Transpose.h"

namespace orca {

class MidiStack {
public:
    NoteEntry notes[kMaxNotes];
    int noteCount = 0;

    void clear() {
        // Remove nulled entries
        int write = 0;
        for (int i = 0; i < noteCount; i++) {
            if (notes[i].active) {
                if (write != i) notes[write] = notes[i];
                write++;
            }
        }
        noteCount = write;
    }

    void push(int channel, int octave, char note, int velocity, int length) {
        NoteEntry item;
        item.channel = channel;
        item.octave = octave;
        item.note = note;
        item.velocity = velocity;
        item.length = length;
        item.isPlayed = false;
        item.active = true;

        // Retrigger: release duplicates with same channel+octave+note
        for (int i = 0; i < noteCount; i++) {
            if (!notes[i].active) continue;
            if (notes[i].channel == channel && notes[i].octave == octave && notes[i].note == note) {
                notes[i].active = false; // mark for release
            }
        }

        if (noteCount < kMaxNotes) {
            notes[noteCount++] = item;
        }
    }

    // Process one frame: returns MIDI events via callback
    // outEvents must have space for at least kMaxNotes * 2 events
    int run(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;

        for (int i = 0; i < noteCount; i++) {
            if (!notes[i].active) {
                // Generate note-off for retriggered notes
                if (eventCount < maxEvents) {
                    auto tr = transpose(notes[i].note, notes[i].octave);
                    if (tr.valid && tr.id > 0) {
                        auto& e = outEvents[eventCount++];
                        e.type = MidiEvent::NoteOff;
                        int ch = notes[i].channel;
                        e.bytes[0] = static_cast<uint8_t>(0x80 + ch);
                        e.bytes[1] = static_cast<uint8_t>(tr.id);
                        e.bytes[2] = static_cast<uint8_t>((notes[i].velocity * 127) / 16);
                        e.numBytes = 3;
                    }
                }
                continue;
            }

            if (!notes[i].isPlayed) {
                // Press: generate note-on
                auto tr = transpose(notes[i].note, notes[i].octave);
                if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                    auto& e = outEvents[eventCount++];
                    e.type = MidiEvent::NoteOn;
                    int ch = notes[i].channel;
                    e.bytes[0] = static_cast<uint8_t>(0x90 + ch);
                    e.bytes[1] = static_cast<uint8_t>(tr.id);
                    e.bytes[2] = static_cast<uint8_t>((notes[i].velocity * 127) / 16);
                    e.numBytes = 3;
                }
                notes[i].isPlayed = true;
            }

            if (notes[i].length < 1) {
                // Release
                auto tr = transpose(notes[i].note, notes[i].octave);
                if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                    auto& e = outEvents[eventCount++];
                    e.type = MidiEvent::NoteOff;
                    int ch = notes[i].channel;
                    e.bytes[0] = static_cast<uint8_t>(0x80 + ch);
                    e.bytes[1] = static_cast<uint8_t>(tr.id);
                    e.bytes[2] = static_cast<uint8_t>((notes[i].velocity * 127) / 16);
                    e.numBytes = 3;
                }
                notes[i].active = false;
            } else {
                notes[i].length--;
            }
        }

        return eventCount;
    }

    void silence(MidiEvent* outEvents, int maxEvents, int& eventCount) {
        for (int i = 0; i < noteCount; i++) {
            if (!notes[i].active) continue;
            auto tr = transpose(notes[i].note, notes[i].octave);
            if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                int ch = notes[i].channel;
                e.bytes[0] = static_cast<uint8_t>(0x80 + ch);
                e.bytes[1] = static_cast<uint8_t>(tr.id);
                e.bytes[2] = 0;
                e.numBytes = 3;
            }
            notes[i].active = false;
        }
        noteCount = 0;
    }

    int length() const {
        int count = 0;
        for (int i = 0; i < noteCount; i++)
            if (notes[i].active) count++;
        return count;
    }
};

} // namespace orca
