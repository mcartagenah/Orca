#pragma once

#include "Grid.h"
#include "Operator.h"
#include "EngineIO.h"
#include <cstdlib>
#include <ctime>

namespace orca {

class Engine {
public:
    Grid grid;
    EngineIO io;

    // Shadow buffer for thread-safe UI reads
    char shadowCells[kMaxGridSize];
    // Port types for UI: 0=none, 1=haste(input left/up), 2=input, 3=output, 5=locked, 8=bang output
    uint8_t shadowPorts[kMaxGridSize];
    // Port owner glyph and port index (for UI port name display)
    char shadowPortOwner[kMaxGridSize];
    uint8_t shadowPortIdx[kMaxGridSize];
    bool shadowLocks[kMaxGridSize];
    int shadowW = 0, shadowH = 0, shadowF = 0;

    Engine() {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        grid.io = &io;
        grid.reset(25, 25);
        updateShadow();
    }

    void reset(int w = 25, int h = 25) {
        grid.reset(w, h);
        updateShadow();
    }

    void load(int w, int h, const char* s, int f = 0) {
        grid.load(w, h, s, f);
        updateShadow();
    }

    // Execute one frame. Returns MIDI events.
    int step(MidiEvent* outEvents, int maxEvents) {
        io.clear();
        grid.run();
        int count = io.run(outEvents, maxEvents);
        updateShadow();
        return count;
    }

    void updateShadow() {
        // Re-parse operators so port info is always up-to-date (even when stopped)
        grid.parse();

        int size = grid.w * grid.h;
        memcpy(shadowCells, grid.cells, size);
        memcpy(shadowLocks, grid.locks, size * sizeof(bool));
        shadowW = grid.w;
        shadowH = grid.h;
        shadowF = grid.f;

        // Build port map from runtime operators
        memset(shadowPorts, 0, size);
        memset(shadowPortOwner, 0, size);
        memset(shadowPortIdx, 0, size);
        for (int i = 0; i < grid.runtimeCount; i++) {
            auto& op = grid.runtime[i];
            if (grid.lockAt(op.x, op.y)) continue;

            // Mark operator itself (type 0) if it draws
            if (op.draw) {
                int idx = grid.indexAt(op.x, op.y);
                if (idx >= 0) shadowPorts[idx] = 10; // operator marker
            }

            // Mark ports
            for (int p = 0; p < op.portCount; p++) {
                if (!op.ports[p].active) continue;
                int px = op.x + op.ports[p].x;
                int py = op.y + op.ports[p].y;
                int idx = grid.indexAt(px, py);
                if (idx < 0) continue;

                uint8_t portType;
                if (op.ports[p].output) {
                    if (op.ports[p].bang) portType = 8;        // bang output
                    else if (op.ports[p].reader) portType = 9; // reader output
                    else portType = 3;                          // output
                } else if (op.ports[p].x < 0 || op.ports[p].y < 0) {
                    portType = 1; // haste (input west/north)
                } else {
                    portType = 2; // input
                }
                shadowPorts[idx] = portType;
                shadowPortOwner[idx] = op.glyph;
                shadowPortIdx[idx] = static_cast<uint8_t>(p);
            }
        }
    }
};

} // namespace orca
