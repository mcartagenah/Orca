#pragma once

#include "Grid.h"
#include "Operator.h"
#include "EngineIO.h"
#include "LifeGrid.h"
#include <cstdlib>
#include <ctime>

namespace orca {

class Engine {
public:
    Grid grid;
    EngineIO io;
    LifeGrid lifeGrid;
    bool lifeMode = false;
    uint8_t paintChannel = 0;
    uint8_t paintOctave = 3;

    // Shadow buffer for thread-safe UI reads
    char shadowCells[kMaxGridSize];
    // Port types for UI: 0=none, 1=haste(input left/up), 2=input, 3=output, 5=locked, 8=bang output
    uint8_t shadowPorts[kMaxGridSize];
    // Port owner glyph and port index (for UI port name display)
    char shadowPortOwner[kMaxGridSize];
    uint8_t shadowPortIdx[kMaxGridSize];
    bool shadowLocks[kMaxGridSize];
    LifeCell shadowLife[kMaxGridSize];
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
        if (lifeMode) {
            int count = lifeGrid.step(outEvents, maxEvents);
            updateShadow();
            return count;
        }
        io.clear();
        grid.run();
        int count = io.run(outEvents, maxEvents);
        updateShadow();
        return count;
    }

    void enterLifeMode() {
        lifeMode = true;
        lifeGrid.resize(grid.w, grid.h);
        // Convert existing letter cells to alive LifeCells
        for (int y = 0; y < grid.h; y++) {
            for (int x = 0; x < grid.w; x++) {
                int idx = x + grid.w * y;
                char g = grid.cells[idx];
                if (isLetter(g)) {
                    lifeGrid.cells[idx].note = g;
                    lifeGrid.cells[idx].channel = paintChannel;
                    lifeGrid.cells[idx].octave = paintOctave;
                    lifeGrid.cells[idx].alive = true;
                }
            }
        }
    }

    void exitLifeMode(MidiEvent* outEvents, int maxEvents, int& eventCount) {
        // Silence all Life notes
        eventCount += lifeGrid.silence(outEvents + eventCount, maxEvents - eventCount);
        // Copy Life state back to grid
        for (int y = 0; y < grid.h; y++) {
            for (int x = 0; x < grid.w; x++) {
                int idx = x + grid.w * y;
                grid.cells[idx] = lifeGrid.cells[idx].alive ? lifeGrid.cells[idx].note : '.';
            }
        }
        lifeMode = false;
    }

    void updateShadow(bool skipParse = false) {
        if (lifeMode) {
            // Use wrap dimensions for display (visible area)
            int dispW = lifeGrid.wrapW;
            int dispH = lifeGrid.wrapH;
            int dispSize = dispW * dispH;
            // Copy only the visible portion (storage may be larger)
            for (int y = 0; y < dispH; y++) {
                for (int x = 0; x < dispW; x++) {
                    int si = x + dispW * y;  // shadow index
                    int li = x + lifeGrid.w * y;  // storage index
                    shadowLife[si] = lifeGrid.cells[li];
                    shadowCells[si] = lifeGrid.cells[li].alive ? lifeGrid.cells[li].note : '.';
                }
            }
            memset(shadowLocks, 0, dispSize * sizeof(bool));
            memset(shadowPorts, 0, dispSize);
            memset(shadowPortOwner, 0, dispSize);
            memset(shadowPortIdx, 0, dispSize);
            shadowW = dispW;
            shadowH = dispH;
            shadowF = lifeGrid.generation;
            return;
        }

        // Re-parse operators so port info is always up-to-date (even when stopped).
        // Skip during playback — step() already runs parse via grid.run(),
        // and calling parse() from the UI thread would race with the audio thread.
        if (!skipParse)
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
