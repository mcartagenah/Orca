#pragma once

#include "Types.h"
#include "Transpose.h"

namespace orca {

class MonoStack {
public:
    NoteEntry channels[kMaxMonoChannels];

    // Pending note-offs from push() replacing an active note
    struct PendingRelease {
        int channel;
        char note;
        int octave;
        int velocity;
        bool active;
    };
    PendingRelease pendingReleases[kMaxMonoChannels];
    int pendingCount = 0;

    MonoStack() {
        for (int i = 0; i < kMaxMonoChannels; i++)
            channels[i].active = false;
        pendingCount = 0;
    }

    void clear() {
        // Mono stack persists across frames (no clear per frame)
    }

    void push(int channel, int octave, char note, int velocity, int length) {
        if (channel < 0 || channel >= kMaxMonoChannels) return;

        // Queue note-off for the previous note on this channel
        if (channels[channel].active && channels[channel].isPlayed && pendingCount < kMaxMonoChannels) {
            auto& pr = pendingReleases[pendingCount++];
            pr.channel = channel;
            pr.note = channels[channel].note;
            pr.octave = channels[channel].octave;
            pr.velocity = channels[channel].velocity;
            pr.active = true;
        }

        channels[channel].channel = channel;
        channels[channel].octave = octave;
        channels[channel].note = note;
        channels[channel].velocity = velocity;
        channels[channel].length = length;
        channels[channel].isPlayed = false;
        channels[channel].active = true;
    }

    int run(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;

        // Emit pending note-offs from replaced notes first
        for (int i = 0; i < pendingCount; i++) {
            if (!pendingReleases[i].active) continue;
            auto tr = transpose(pendingReleases[i].note, pendingReleases[i].octave);
            if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                e.bytes[0] = static_cast<uint8_t>(0x80 + pendingReleases[i].channel);
                e.bytes[1] = static_cast<uint8_t>(tr.id);
                e.bytes[2] = static_cast<uint8_t>((pendingReleases[i].velocity * 127) / 16);
                e.numBytes = 3;
            }
        }
        pendingCount = 0;

        for (int ch = 0; ch < kMaxMonoChannels; ch++) {
            if (!channels[ch].active) continue;

            if (channels[ch].length < 1) {
                // Release
                auto tr = transpose(channels[ch].note, channels[ch].octave);
                if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                    auto& e = outEvents[eventCount++];
                    e.type = MidiEvent::NoteOff;
                    e.bytes[0] = static_cast<uint8_t>(0x80 + ch);
                    e.bytes[1] = static_cast<uint8_t>(tr.id);
                    e.bytes[2] = static_cast<uint8_t>((channels[ch].velocity * 127) / 16);
                    e.numBytes = 3;
                }
                channels[ch].active = false;
                continue;
            }

            if (!channels[ch].isPlayed) {
                // Press
                auto tr = transpose(channels[ch].note, channels[ch].octave);
                if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                    auto& e = outEvents[eventCount++];
                    e.type = MidiEvent::NoteOn;
                    e.bytes[0] = static_cast<uint8_t>(0x90 + ch);
                    e.bytes[1] = static_cast<uint8_t>(tr.id);
                    e.bytes[2] = static_cast<uint8_t>((channels[ch].velocity * 127) / 16);
                    e.numBytes = 3;
                }
                channels[ch].isPlayed = true;
            }

            channels[ch].length--;
        }

        return eventCount;
    }

    void silence(MidiEvent* outEvents, int maxEvents, int& eventCount) {
        for (int ch = 0; ch < kMaxMonoChannels; ch++) {
            if (!channels[ch].active) {
                continue;
            }
            auto tr = transpose(channels[ch].note, channels[ch].octave);
            if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                e.bytes[0] = static_cast<uint8_t>(0x80 + ch);
                e.bytes[1] = static_cast<uint8_t>(tr.id);
                e.bytes[2] = 0;
                e.numBytes = 3;
            }
            channels[ch].active = false;
        }
    }

    int length() const {
        int count = 0;
        for (int i = 0; i < kMaxMonoChannels; i++)
            if (channels[i].active) count++;
        return count;
    }
};

} // namespace orca
