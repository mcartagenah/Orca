#pragma once

#include "Types.h"
#include "Transpose.h"

namespace orca {

struct LifeCell {
    char note = '.';       // note letter or '.' if dead
    uint8_t channel = 0;   // MIDI channel 0-15
    uint8_t octave = 3;    // octave 0..maxOctave
    bool alive = false;
    bool locked = false;   // protected from GoL death rules
    bool fired = false;    // triggered a note this step (for UI flash)
    uint16_t age = 0;      // generations survived (0 = just born)
};

// Classic GoL pattern definitions
struct LifePattern {
    const char* name;
    int w, h;
    const char* data; // '.' = dead, 'X' = alive, row-major
};

// Built-in pattern library — grouped by category (indices must be contiguous per category)
static const LifePattern builtInPatterns[] = {
    // ── Still Lifes (0-10) ──
    { "Block",      2, 2, "XX"
                          "XX" },
    { "Beehive",    4, 3, ".XX."
                          "X..X"
                          ".XX." },
    { "Loaf",       4, 4, ".XX."
                          "X..X"
                          ".X.X"
                          "..X." },
    { "Boat",       3, 3, "XX."
                          "X.X"
                          ".X." },
    { "Tub",        3, 3, ".X."
                          "X.X"
                          ".X." },
    { "Pond",       4, 4, ".XX."
                          "X..X"
                          "X..X"
                          ".XX." },
    { "Ship",       3, 3, "XX."
                          "X.X"
                          ".XX" },
    { "Long Boat",  4, 4, ".X.."
                          "X.X."
                          ".X.X"
                          "..XX" },
    { "Barge",      4, 4, ".X.."
                          "X.X."
                          ".X.X"
                          "..X." },
    { "Mango",      5, 4, ".XX.."
                          "X..X."
                          ".X..X"
                          "..XX." },
    { "Eater 1",    4, 4, "XX.."
                          "X.X."
                          "..X."
                          "..XX" },

    // ── Oscillators (11-24) ──
    { "Blinker",    3, 1, "XXX" },
    { "Toad",       4, 2, ".XXX"
                          "XXX." },
    { "Beacon",     4, 4, "XX.."
                          "XX.."
                          "..XX"
                          "..XX" },
    { "Pulsar",    13, 13,
        "..XXX...XXX.."
        "............."
        "X....X.X....X"
        "X....X.X....X"
        "X....X.X....X"
        "..XXX...XXX.."
        "............."
        "..XXX...XXX.."
        "X....X.X....X"
        "X....X.X....X"
        "X....X.X....X"
        "............."
        "..XXX...XXX.." },
    { "Pentadecathlon", 10, 3,
        "..X....X.."
        "XX.XXXX.XX"
        "..X....X.." },
    { "Clock",      4, 4, "..X."
                          "X.X."
                          ".X.X"
                          ".X.." },
    { "Octagon 2", 8, 8,
        "...XX..."
        "..X..X.."
        ".X....X."
        "X......X"
        "X......X"
        ".X....X."
        "..X..X.."
        "...XX..." },
    { "Figure 8",  6, 6,
        "XXX..."
        "XXX..."
        "XXX..."
        "...XXX"
        "...XXX"
        "...XXX" },
    { "Tumbler",   9, 5,
        ".X.....X."
        "X.X...X.X"
        "X..X.X..X"
        "..X...X.."
        "..XX.XX.." },
    { "Fumarole",  8, 7,
        "...XX..."
        ".X....X."
        ".X....X."
        ".X....X."
        "..X..X.."
        "X.X..X.X"
        "XX....XX" },

    { "Queen Bee Shuttle", 22, 7,
        ".........XX..........."
        ".........X.X.........."
        "....XX......X........."
        "XX.X..X..X..X.......XX"
        "XX..XX......X.......XX"
        ".........X.X.........."
        ".........XX..........." },
    { "Twin Bees Shuttle", 29, 11,
        ".................X..........."
        "XX...............XX........XX"
        "XX................XX.......XX"
        ".............XX..XX.........."
        "............................."
        "............................."
        "............................."
        ".............XX..XX.........."
        "XX................XX.......XX"
        "XX...............XX........XX"
        ".................X..........." },
    { "Ants",            44, 4,
        "XX...XX...XX...XX...XX...XX...XX...XX...XX.."
        "..XX...XX...XX...XX...XX...XX...XX...XX...XX"
        "..XX...XX...XX...XX...XX...XX...XX...XX...XX"
        "XX...XX...XX...XX...XX...XX...XX...XX...XX.." },
    { "Turning Toads",   37, 8,
        "...............X......X.............."
        "..............XX.....XX......XX......"
        "......XXX.X.X.XX.X.X.XX.X.X.........."
        "..XX.X......X.X....X.X....X.X..X.XX.."
        "X..X.X...X..................XXXX.X..X"
        "XX.X.X...........................X.XX"
        "...X.............................X..."
        "...XX...........................XX..." },

    // ── Spaceships (25-33) ──
    { "Glider SE",  3, 3, ".X."
                          "..X"
                          "XXX" },
    { "Glider SW",  3, 3, ".X."
                          "X.."
                          "XXX" },
    { "Glider NE",  3, 3, "XXX"
                          "..X"
                          ".X." },
    { "Glider NW",  3, 3, "XXX"
                          "X.."
                          ".X." },
    { "LWSS",       5, 4, ".X..X"
                          "X...."
                          "X...X"
                          ".XXXX" },
    { "MWSS",       6, 5, "...X.."
                          ".X...X"
                          "X....."
                          "X....X"
                          ".XXXXX" },
    { "HWSS",       7, 5, "...XX.."
                          ".X....X"
                          "X......"
                          "X.....X"
                          ".XXXXXX" },
    { "Copperhead", 8, 12,
        ".XX..XX."
        "...XX..."
        "...XX..."
        "X.X..X.X"
        "X......X"
        "........"
        "X......X"
        ".XX..XX."
        "..XXXX.."
        "........"
        "...XX..."
        "...XX..." },
    { "Loafer",    9, 9,
        ".XX..X.XX"
        "X..X..XX."
        ".X.X....."
        "..X......"
        "........X"
        "......XXX"
        ".....X..."
        "......X.."
        ".......XX" },

    // ── Methuselahs (34-42) ──
    { "R-pentomino",3, 3, ".XX"
                          "XX."
                          ".X." },
    { "Diehard",    8, 3, "......X."
                          "XX......"
                          ".X...XXX" },
    { "Acorn",      7, 3, ".X....."
                          "...X..."
                          "XX..XXX" },
    { "Pi",         3, 3, "XXX"
                          "X.X"
                          "X.X" },
    { "B-heptomino",4, 3, "X.XX"
                          "XXX."
                          ".X.." },
    { "Thunderbird",3, 5, "XXX"
                          "..."
                          ".X."
                          ".X."
                          ".X." },
    { "Herschel",   3, 4, "X.."
                          "XXX"
                          "X.X"
                          "..X" },
    { "Rabbits",    7, 3, "X...XXX"
                          "XXX..X."
                          ".X....." },
    { "Lidka",      9, 6,
        "......X.."
        "......XXX"
        "........."
        "...XX...X"
        "...X....X"
        "XXX.....X" },

    // ── Guns (43-44) ──
    { "Gosper Gun", 36, 9,
        "........................X..........."
        "......................X.X..........."
        "............XX......XX............XX"
        "...........X...X....XX............XX"
        "XX........X.....X...XX.............."
        "XX........X...X.XX....X.X..........."
        "..........X.....X.......X..........."
        "...........X...X...................."
        "............XX......................" },

    { "Simkin Gun",  33, 14,
        "XX.....XX........................"
        "XX.....XX........................"
        "................................."
        "....XX..........................."
        "....XX..........................."
        "................................."
        "................................."
        "................................."
        "................................."
        "......................XX.XX......"
        ".....................X.....X....."
        ".....................X......X..XX"
        ".....................XXX...X...XX"
        "..........................X......" },
    { "Simkin Gun 1B", 33, 21,
        "XX.....XX........................"
        "XX.....XX........................"
        "................................."
        "....XX..........................."
        "....XX..........................."
        "................................."
        "................................."
        "................................."
        "................................."
        "......................XX.XX......"
        ".....................X.....X....."
        ".....................X......X..XX"
        ".....................XXX...X...XX"
        "..........................X......"
        "................................."
        "................................."
        "................................."
        "....................XX..........."
        "....................X............"
        ".....................XXX........."
        ".......................X........." },


    // ── Puffers (45-46) ──
    { "Puffer 1", 18, 5,
        ".XXX...........XXX"
        "X..X..........X..X"
        "...X....XX.....X.."
        "...X...X..X....X.."
        "..X....X......X..." },
    { "Switchengine", 6, 4,
        ".X.X.."
        "X....."
        ".X..X."
        "...XXX" },
};

class LifeGrid {
public:
    LifeCell cells[kMaxGridSize];
    LifeCell buffer[kMaxGridSize]; // snapshot for simultaneous update
    int w = 25, h = 25;           // storage dimensions (never shrink)
    int wrapW = 25, wrapH = 25;   // visible/wrap dimensions (toroidal boundary)
    int generation = 0;

    // Active notes for MIDI note-off tracking
    struct ActiveNote {
        uint8_t channel;
        uint8_t midiNote; // MIDI pitch 0-127
        bool active;
    };
    ActiveNote activeNotes[kMaxGridSize];

    void clear() {
        for (int i = 0; i < kMaxGridSize; i++) {
            cells[i] = LifeCell();
            activeNotes[i] = { 0, 0, false };
        }
        generation = 0;
        frameCounter = 0;
        hasInitialState = false;
    }

    void resize(int newW, int newH) {
        w = newW;
        h = newH;
        wrapW = newW;
        wrapH = newH;
        clear();
    }

    // Update toroidal wrap boundary (storage stays the same, simulation wraps at view size)
    void setWrapSize(int newWrapW, int newWrapH) {
        // Grow storage if needed (never shrink)
        if (newWrapW > w || newWrapH > h) {
            int newW = newWrapW > w ? newWrapW : w;
            int newH = newWrapH > h ? newWrapH : h;
            LifeCell oldCells[kMaxGridSize];
            ActiveNote oldNotes[kMaxGridSize];
            int oldW = w, oldH = h;
            memcpy(oldCells, cells, sizeof(LifeCell) * oldW * oldH);
            memcpy(oldNotes, activeNotes, sizeof(ActiveNote) * oldW * oldH);

            w = newW; h = newH;
            for (int i = 0; i < w * h; i++) {
                cells[i] = LifeCell();
                activeNotes[i] = { 0, 0, false };
            }
            for (int y = 0; y < oldH; y++) {
                for (int x = 0; x < oldW; x++) {
                    cells[x + w * y] = oldCells[x + oldW * y];
                    activeNotes[x + w * y] = oldNotes[x + oldW * y];
                }
            }
        }
        wrapW = newWrapW;
        wrapH = newWrapH;
    }

    int indexAt(int x, int y) const {
        if (x < 0 || x >= w || y < 0 || y >= h) return -1;
        return x + w * y;
    }

    const LifeCell& cellAt(int x, int y) const {
        int idx = indexAt(x, y);
        if (idx < 0) {
            static const LifeCell dead;
            return dead;
        }
        return cells[idx];
    }

    // Count alive neighbors (8-directional, toroidal wrapping at visible boundary)
    int countNeighbors(int x, int y) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = (x + dx + wrapW) % wrapW;
                int ny = (y + dy + wrapH) % wrapH;
                if (buffer[nx + w * ny].alive) count++;
            }
        }
        return count;
    }

    // Scale system for melodic evolution
    enum ScaleType {
        Chromatic, Major, Minor, Pentatonic, Dorian,
        Phrygian, Lydian, Mixolydian, Locrian,
        HarmonicMinor, MelodicMinor,
        MinorPentatonic, Blues,
        WholeTone, Diminished,
        NumScales
    };
    ScaleType currentScale = Chromatic;
    int rootNote = 0;  // 0=C, 1=C#, 2=D, ... 11=B

    static constexpr int kMaxScaleNotes = 12;
    static constexpr int scaleNotes[NumScales][kMaxScaleNotes] = {
        {0,1,2,3,4,5,6,7,8,9,10,11},     // chromatic
        {0,2,4,5,7,9,11,-1,-1,-1,-1,-1},  // major (ionian)
        {0,2,3,5,7,8,10,-1,-1,-1,-1,-1},  // natural minor (aeolian)
        {0,2,4,7,9,-1,-1,-1,-1,-1,-1,-1}, // pentatonic (major)
        {0,2,3,5,7,9,10,-1,-1,-1,-1,-1},  // dorian
        {0,1,3,5,7,8,10,-1,-1,-1,-1,-1},  // phrygian
        {0,2,4,6,7,9,11,-1,-1,-1,-1,-1},  // lydian
        {0,2,4,5,7,9,10,-1,-1,-1,-1,-1},  // mixolydian
        {0,1,3,5,6,8,10,-1,-1,-1,-1,-1},  // locrian
        {0,2,3,5,7,8,11,-1,-1,-1,-1,-1},  // harmonic minor
        {0,2,3,5,7,9,11,-1,-1,-1,-1,-1},  // melodic minor
        {0,3,5,7,10,-1,-1,-1,-1,-1,-1,-1},// minor pentatonic
        {0,3,5,6,7,10,-1,-1,-1,-1,-1,-1}, // blues
        {0,2,4,6,8,10,-1,-1,-1,-1,-1,-1}, // whole tone
        {0,2,3,5,6,8,9,11,-1,-1,-1,-1},   // diminished (half-whole)
    };
    static constexpr int scaleLengths[NumScales] = {12, 7, 7, 5, 7, 7, 7, 7, 7, 7, 7, 5, 6, 6, 8};

    static const char* scaleName(ScaleType s) {
        switch (s) {
            case Chromatic:      return "chromatic";
            case Major:          return "major";
            case Minor:          return "minor";
            case Pentatonic:     return "pentatonic";
            case Dorian:         return "dorian";
            case Phrygian:       return "phrygian";
            case Lydian:         return "lydian";
            case Mixolydian:     return "mixolydian";
            case Locrian:        return "locrian";
            case HarmonicMinor:  return "harm.min";
            case MelodicMinor:   return "mel.min";
            case MinorPentatonic:return "min.penta";
            case Blues:          return "blues";
            case WholeTone:      return "wholetone";
            case Diminished:     return "diminished";
            default:             return "?";
        }
    }

    // Semitone ↔ note char conversion (matches noteIndex: C=0,c=1,D=2,d=3,E=4,F=5,f=6,G=7,g=8,A=9,a=10,B=11)
    static constexpr char semitoneToNote[12] = {'C','c','D','d','E','F','f','G','g','A','a','B'};

    static const char* rootNoteName(int root) {
        static const char* names[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return names[root % 12];
    }

    static int noteToSemitone(char n) { return noteIndex(n); }

    // Step a semitone value up or down within the current scale (relative to rootNote),
    // returning new absolute semitone (0-11). Also adjusts octaveDelta for wrapping.
    int stepInScale(int semitone, int direction, int& octaveDelta) const {
        int sLen = scaleLengths[currentScale];
        const int* s = scaleNotes[static_cast<int>(currentScale)];

        // Convert absolute semitone to root-relative
        int rel = ((semitone - rootNote) % 12 + 12) % 12;

        // Find closest scale degree to this relative semitone
        int bestDeg = 0;
        int bestDist = 99;
        for (int i = 0; i < sLen; i++) {
            int dist = ((rel - s[i]) % 12 + 12) % 12;
            if (dist < bestDist) { bestDist = dist; bestDeg = i; }
            int dist2 = ((s[i] - rel) % 12 + 12) % 12;
            if (dist2 < bestDist) { bestDist = dist2; bestDeg = i; }
        }

        int newDeg = bestDeg + direction;
        octaveDelta = 0;
        if (newDeg >= sLen) { newDeg = 0; octaveDelta = 1; }
        else if (newDeg < 0) { newDeg = sLen - 1; octaveDelta = -1; }

        // Convert back to absolute semitone
        return (s[newDeg] + rootNote) % 12;
    }

    // Get neighbor cells that are alive (for birth note derivation)
    struct NeighborInfo {
        int x, y;
        char note;
        uint8_t channel;
        uint8_t octave;
    };

    int getAliveNeighbors(int x, int y, NeighborInfo* out, int maxOut) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                if (count >= maxOut) return count;
                int nx = (x + dx + wrapW) % wrapW;
                int ny = (y + dy + wrapH) % wrapH;
                const auto& c = buffer[nx + w * ny];
                if (c.alive) {
                    out[count++] = { nx, ny, c.note, c.channel, c.octave };
                }
            }
        }
        return count;
    }

    // Derive birth note using directional scale-step evolution
    LifeCell deriveBirthCell(int x, int y) const {
        NeighborInfo neighbors[8];
        int nCount = getAliveNeighbors(x, y, neighbors, 8);

        LifeCell born;
        born.alive = true;

        // --- Directional note evolution ---
        // 1. Compute centroid of parent positions
        float cx = 0, cy = 0;
        for (int i = 0; i < nCount; i++) { cx += neighbors[i].x; cy += neighbors[i].y; }
        cx /= nCount; cy /= nCount;

        // 2. Direction: born cell relative to centroid
        float dx = x - cx;  // positive = born is right of parents
        float dy = y - cy;  // positive = born is below parents
        float dir = dx + dy; // combined directional signal
        int step = (dir > 0.01f) ? 1 : (dir < -0.01f) ? -1 : 1; // default up to avoid stagnation

        // 3. Get median parent note (by semitone, pick middle value)
        int semitones[8];
        int octaves[8];
        for (int i = 0; i < nCount; i++) {
            semitones[i] = noteToSemitone(neighbors[i].note);
            octaves[i] = neighbors[i].octave;
            if (semitones[i] < 0) semitones[i] = 0; // fallback for invalid notes
        }
        // Simple sort for median (nCount is at most 8)
        for (int i = 0; i < nCount - 1; i++)
            for (int j = i + 1; j < nCount; j++) {
                int pitchI = octaves[i] * 12 + semitones[i];
                int pitchJ = octaves[j] * 12 + semitones[j];
                if (pitchJ < pitchI) {
                    int ts = semitones[i]; semitones[i] = semitones[j]; semitones[j] = ts;
                    int to = octaves[i]; octaves[i] = octaves[j]; octaves[j] = to;
                }
            }
        int medIdx = nCount / 2;
        int medSemitone = semitones[medIdx];
        int medOctave = octaves[medIdx];

        // 4. Step within scale
        int octDelta = 0;
        int newSemitone = stepInScale(medSemitone, step, octDelta);
        int newOctave = lockOctave ? medOctave : clamp(medOctave + octDelta, minOctave, maxOctave);
        born.note = semitoneToNote[newSemitone];
        born.octave = static_cast<uint8_t>(newOctave);

        // --- Channel: majority vote (unchanged) ---
        int chCounts[16] = {};
        for (int i = 0; i < nCount; i++)
            chCounts[neighbors[i].channel]++;
        uint8_t bestCh = neighbors[0].channel;
        int bestChCount = 0;
        for (int i = 0; i < 16; i++) {
            if (chCounts[i] > bestChCount) {
                bestChCount = chCounts[i];
                bestCh = static_cast<uint8_t>(i);
            }
        }
        born.channel = bestCh;

        return born;
    }

    int minOctave = 0;   // lower octave limit (0-based)
    int maxOctave = 7;   // upper octave limit (0-based)
    int evolveRate = 4;  // evolve every N frames, power-of-2 (1,2,4,8,16,32)
    int frameCounter = 0;
    bool pulseMode = true;  // false=hold (note-on at birth, note-off at death), true=pulse (retrigger every step)
    bool decay = false;           // unified age system: velocity drops + probability drops with age
    uint8_t minVelocity = 40;    // floor velocity for newborn cells (1-127)
    int minProb = 10;            // floor probability % at max age (1-100)
    int maxNotes = 0;            // max notes per step per channel (0 = unlimited)
    int notesFiredPerChannel[16] = {}; // per-channel counter, reset each evolution step

    // Dedup system: merge identical {channel, pitch} notes, scale velocity/length/CC by count
    bool dedup = false;
    int dedupCC = -1;            // CC number for count modulation (-1 = disabled, 1 = mod wheel)

    static constexpr int kMaxPreEmit = 512;
    struct PreEmitNote {
        uint8_t channel;
        uint8_t midiNote;
        uint8_t velocity;
        int cellIndex;       // for setting fired flag + activeNotes
    };
    PreEmitNote preEmit[kMaxPreEmit];
    int preEmitCount = 0;

    // Collect a note into the pre-emit buffer (used when dedup is on)
    void collectNote(uint8_t ch, uint8_t note, uint8_t vel, int cellIdx) {
        if (preEmitCount < kMaxPreEmit)
            preEmit[preEmitCount++] = { ch, note, vel, cellIdx };
    }

    // Flush pre-emit buffer: merge duplicates, emit with scaled velocity + CC
    int flushDedup(MidiEvent* outEvents, int maxEvents) {
        if (preEmitCount == 0) return 0;
        int eventCount = 0;

        // Simple O(n²) merge — fine for <512 notes
        struct Merged {
            uint8_t channel, midiNote;
            int count;
            int velSum;          // sum of velocities for averaging
            int firstCellIndex;  // for fired flag
            bool emitted;
        };
        Merged merged[kMaxPreEmit];
        int mergedCount = 0;

        for (int i = 0; i < preEmitCount; i++) {
            auto& n = preEmit[i];
            // Look for existing entry
            int found = -1;
            for (int j = 0; j < mergedCount; j++) {
                if (merged[j].channel == n.channel && merged[j].midiNote == n.midiNote) {
                    found = j;
                    break;
                }
            }
            if (found >= 0) {
                merged[found].count++;
                merged[found].velSum += n.velocity;
            } else {
                merged[mergedCount++] = { n.channel, n.midiNote, 1, (int)n.velocity, n.cellIndex, false };
            }
            // Mark all contributing cells as fired
            if (n.cellIndex >= 0 && n.cellIndex < w * h)
                cells[n.cellIndex].fired = true;
        }

        // Emit deduplicated notes
        for (int i = 0; i < mergedCount; i++) {
            auto& m = merged[i];
            if (!canFireMore(m.channel)) continue;

            // Average velocity preserves age variation, count boost adds headroom
            int vel = m.velSum / m.count;
            if (m.count > 1) {
                // Gentle log scaling: each doubling adds ~15 velocity
                float boost = std::log2(static_cast<float>(m.count)) * 15.0f;
                vel = juce::jmin(127, static_cast<int>(vel + boost));
            }

            // Emit NoteOn
            if (eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOn;
                e.bytes[0] = static_cast<uint8_t>(0x90 + m.channel);
                e.bytes[1] = m.midiNote;
                e.bytes[2] = static_cast<uint8_t>(vel);
                e.numBytes = 3;
                if (m.firstCellIndex >= 0 && m.firstCellIndex < w * h)
                    activeNotes[m.firstCellIndex] = { m.channel, m.midiNote, true };
                notesFiredPerChannel[m.channel & 0x0F]++;
            }

            // Emit CC proportional to count (0-127 mapped from count 1..16+)
            if (dedupCC >= 0 && m.count > 1 && eventCount < maxEvents) {
                int ccVal = juce::jmin(127, (m.count * 127) / 8); // 8 cells = max CC
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::CC;
                e.bytes[0] = static_cast<uint8_t>(0xB0 + m.channel);
                e.bytes[1] = static_cast<uint8_t>(dedupCC);
                e.bytes[2] = static_cast<uint8_t>(ccVal);
                e.numBytes = 3;
            }
        }

        preEmitCount = 0;
        return eventCount;
    }

    // Cellular automata rule set using S/B notation (survival/birth)
    // Stored as bitmasks: bit N = neighbor count N triggers that rule
    uint16_t birthRule = (1 << 3);               // B3
    uint16_t survivalRule = (1 << 2) | (1 << 3); // S23
    char ruleString[16] = "23/3";                // S/B display string

    // Parse S/B notation string like "23/3", "23/36", "34/34"
    bool setRuleFromString(const char* str) {
        uint16_t newSurvival = 0, newBirth = 0;
        const char* p = str;
        // Parse survival digits (before '/')
        while (*p && *p != '/') {
            if (*p >= '0' && *p <= '8')
                newSurvival |= (1 << (*p - '0'));
            p++;
        }
        if (*p == '/') p++; // skip '/'
        // Parse birth digits (after '/')
        while (*p) {
            if (*p >= '0' && *p <= '8')
                newBirth |= (1 << (*p - '0'));
            p++;
        }
        if (newBirth == 0 && newSurvival == 0) return false;
        birthRule = newBirth;
        survivalRule = newSurvival;
        // Store display string
        int len = 0;
        for (int i = 0; i < 9 && len < 14; i++)
            if ((survivalRule >> i) & 1) ruleString[len++] = '0' + i;
        ruleString[len++] = '/';
        for (int i = 0; i < 9 && len < 14; i++)
            if ((birthRule >> i) & 1) ruleString[len++] = '0' + i;
        ruleString[len] = '\0';
        return true;
    }

    // Named presets
    void setRulePreset(const char* name) {
        if (strcmp(name, "life") == 0)           setRuleFromString("23/3");
        else if (strcmp(name, "highlife") == 0)  setRuleFromString("23/36");
        else if (strcmp(name, "seeds") == 0)     setRuleFromString("/2");
        else if (strcmp(name, "daynight") == 0)  setRuleFromString("34678/3678");
        else if (strcmp(name, "diamoeba") == 0)  setRuleFromString("5678/35678");
        else if (strcmp(name, "replicator") == 0) setRuleFromString("1357/1357");
        else if (strcmp(name, "2x2") == 0)       setRuleFromString("125/36");
        else if (strcmp(name, "morley") == 0)    setRuleFromString("245/368");
        else if (strcmp(name, "34life") == 0)    setRuleFromString("34/34");
    }

    bool shouldBeBorn(int neighbors) const { return (birthRule >> neighbors) & 1; }
    bool shouldSurvive(int neighbors) const { return (survivalRule >> neighbors) & 1; }
    bool firstEvolution = true;  // treat all alive cells as born on first step after start

    // Evolve rate helpers (power-of-2 only)
    void evolveRateUp()   { if (evolveRate < 32) evolveRate *= 2; }
    void evolveRateDown() { if (evolveRate > 1)  evolveRate /= 2; }

    // Decay curve: maps cell age to a 0.0–1.0 factor (1.0 = newborn, decays toward 0.0)
    // Used for both velocity and probability
    float decayFactor(uint16_t cellAge) const {
        constexpr uint16_t maxAge = 64;
        if (cellAge >= maxAge) return 0.0f;
        return 1.0f - static_cast<float>(cellAge) / static_cast<float>(maxAge);
    }

    // Velocity: when decay is on, newborn = 127, ages toward minVelocity
    uint8_t velocityFor(int cellIndex) const {
        if (!decay) return 100; // default flat velocity
        float f = decayFactor(cells[cellIndex].age);
        // Interpolate: maxVel at birth → minVelocity at max age
        return static_cast<uint8_t>(minVelocity + (int)(f * (127 - minVelocity)));
    }

    // Probability: when decay is on, newborn = 100%, ages toward minProb%
    bool shouldFire(int cellIndex) const {
        if (!decay) return true;
        float f = decayFactor(cells[cellIndex].age);
        // Interpolate: 100% at birth → minProb% at max age
        int effectiveProb = minProb + (int)(f * (100 - minProb));
        if (effectiveProb >= 100) return true;
        return (std::rand() % 100) < effectiveProb;
    }

    // Density thinning: check if we've hit the per-channel note cap
    bool canFireMore(uint8_t channel) const {
        if (maxNotes <= 0) return true;
        return notesFiredPerChannel[channel & 0x0F] < maxNotes;
    }

    // Row phase offset: stagger note emission across the evolve cycle by row
    enum SeqMode { SeqOff = 0, SeqForward, SeqReverse, SeqMirror, SeqRandom };
    SeqMode seqMode = SeqOff;

    // Mirror ping-pong state: flips direction each evolution cycle
    bool mirrorForward = true;

    // Random permutation table for SeqRandom (row → phase frame mapping)
    int randomPhase[512]; // maps row group → frame (regenerated each evolution)
    void shufflePhaseTable() {
        int groups = evolveRate;
        for (int i = 0; i < groups; i++) randomPhase[i] = i;
        // Fisher-Yates shuffle
        for (int i = groups - 1; i > 0; i--) {
            int j = std::rand() % (i + 1);
            int tmp = randomPhase[i];
            randomPhase[i] = randomPhase[j];
            randomPhase[j] = tmp;
        }
    }

    bool lockOctave = false; // born cells inherit parent octave (no octave shifts from scale stepping)
    // Chord degree filter: which scale degrees are allowed (1-indexed, stored as 0-indexed bitmask)
    // e.g. "135" = root+3rd+5th, "1357" = seventh chord, "0" or empty = off
    int chordDegrees[7] = { 0, 2, 4 }; // default: 1st, 3rd, 5th (stored as 0-indexed)
    int chordDegreeCount = 0;           // 0 = chord filter off

    // Set chord degrees from a string like "135", "1357", "125", "off"
    void setChordDegrees(const char* str) {
        if (str[0] == '0' || str[0] == '\0') { chordDegreeCount = 0; return; } // off
        chordDegreeCount = 0;
        for (const char* p = str; *p && chordDegreeCount < 7; p++) {
            if (*p >= '1' && *p <= '7')
                chordDegrees[chordDegreeCount++] = (*p - '1'); // convert 1-indexed to 0-indexed
        }
        if (chordDegreeCount == 0) return; // nothing valid parsed
    }

    // Get chord degrees as display string (1-indexed)
    juce::String chordDegreesString() const {
        if (chordDegreeCount == 0) return "off";
        juce::String s;
        for (int i = 0; i < chordDegreeCount; i++)
            s += juce::String(chordDegrees[i] + 1);
        return s;
    }

    // Snap a MIDI note to the nearest allowed chord tone
    int snapToChordTone(int midiNote) const {
        if (chordDegreeCount == 0) return midiNote;
        int sLen = scaleLengths[static_cast<int>(currentScale)];
        const int* s = scaleNotes[static_cast<int>(currentScale)];
        // If scale has fewer notes than max requested degree, skip filtering
        int maxDegree = 0;
        for (int i = 0; i < chordDegreeCount; i++)
            if (chordDegrees[i] > maxDegree) maxDegree = chordDegrees[i];
        if (sLen <= maxDegree) return midiNote;

        // Build allowed semitones from the selected degrees
        int allowedSemitones[7];
        for (int i = 0; i < chordDegreeCount; i++)
            allowedSemitones[i] = s[chordDegrees[i]];

        // Find nearest allowed tone to this MIDI note
        int bestNote = midiNote;
        int bestDist = 999;
        for (int octShift = -1; octShift <= 1; octShift++) {
            for (int i = 0; i < chordDegreeCount; i++) {
                int base = midiNote - ((midiNote - 24 - rootNote) % 12 + 12) % 12;
                int candidate = base + allowedSemitones[i] + octShift * 12;
                int dist = std::abs(candidate - midiNote);
                if (dist < bestDist && candidate >= 0 && candidate <= 127) {
                    bestDist = dist;
                    bestNote = candidate;
                }
            }
        }
        return bestNote;
    }

    int phaseForRow(int y) const {
        if (seqMode == SeqOff || evolveRate <= 1 || wrapH <= 0) return 0;
        int forwardPhase = y * evolveRate / wrapH;
        switch (seqMode) {
            case SeqForward: return forwardPhase;
            case SeqReverse: return (evolveRate - 1) - forwardPhase;
            case SeqMirror:
                // Ping-pong: alternates forward/reverse each cycle
                return mirrorForward ? forwardPhase : (evolveRate - 1) - forwardPhase;
            case SeqRandom: return randomPhase[forwardPhase % evolveRate];
            default: return 0;
        }
    }

    // Phase-scheduled notes (emitted on intermediate frames when phaseOffset is active)
    static constexpr int kMaxPhaseNotes = 1024;
    struct PhaseNote {
        uint8_t midiNote;
        uint8_t channel;
        uint8_t velocity;
        int phaseFrame;   // which frameCounter value to fire on (1..evolveRate-1)
        int cellIndex;    // for setting fired flag + activeNotes tracking
    };
    PhaseNote phaseNotes[kMaxPhaseNotes];
    int phaseNoteCount = 0;

    int processPhaseNotes(int currentFrame, MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        int phaseNotesPerCh[16] = {};
        for (int i = 0; i < phaseNoteCount; i++) {
            auto& pn = phaseNotes[i];
            if (pn.phaseFrame != currentFrame) continue;
            if (eventCount >= maxEvents) break;

            // Per-phase maxNotes check
            if (maxNotes > 0 && phaseNotesPerCh[pn.channel & 0x0F] >= maxNotes) continue;

            auto& e = outEvents[eventCount++];
            e.type = MidiEvent::NoteOn;
            e.bytes[0] = static_cast<uint8_t>(0x90 + pn.channel);
            e.bytes[1] = pn.midiNote;
            e.bytes[2] = pn.velocity;
            e.numBytes = 3;

            phaseNotesPerCh[pn.channel & 0x0F]++;
            if (pn.cellIndex >= 0 && pn.cellIndex < w * h) {
                cells[pn.cellIndex].fired = true;
                activeNotes[pn.cellIndex] = { pn.channel, pn.midiNote, true };
            }
        }
        return eventCount;
    }

    // Ratchet system (pulse mode only): clusters of same-note cells fire sequentially
    static constexpr int kMaxRatchets = 128;
    struct Ratchet {
        uint8_t midiNote;   // MIDI pitch
        uint8_t channel;
        uint8_t remaining;  // notes left to fire (including current)
        bool activeNote;    // is a note currently sounding from this ratchet?
        uint8_t velocity;   // velocity for ratchet notes
    };
    Ratchet ratchets[kMaxRatchets];
    int ratchetCount = 0;

    // Run one Game of Life step, emit MIDI events directly
    int step(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;

        // Only evolve every N frames
        frameCounter++;
        if (frameCounter < evolveRate) {
            // Process pending ratchets and phase-scheduled notes on intermediate frames
            eventCount += processRatchets(outEvents, maxEvents);
            if (seqMode != SeqOff)
                eventCount += processPhaseNotes(frameCounter, outEvents + eventCount, maxEvents - eventCount);
            generation++;
            return eventCount;
        }
        frameCounter = 0;
        memset(notesFiredPerChannel, 0, sizeof(notesFiredPerChannel));
        phaseNoteCount = 0;
        preEmitCount = 0;
        if (seqMode == SeqRandom) shufflePhaseTable();
        if (seqMode == SeqMirror) mirrorForward = !mirrorForward;

        // Clear fired flags from previous step
        for (int i = 0; i < w * h; i++)
            cells[i].fired = false;

        // Silence any pending ratchet notes before next evolution
        eventCount += silenceRatchets(outEvents + eventCount, maxEvents - eventCount);
        ratchetCount = 0;

        // Auto-save initial state before first evolution
        if (!hasInitialState && population() > 0)
            saveInitialState();

        // Snapshot current state
        memcpy(buffer, cells, sizeof(LifeCell) * w * h);

        // In pulse mode, silence all active notes before applying rules
        if (pulseMode)
            eventCount += silence(outEvents + eventCount, maxEvents - eventCount);

        // Apply GoL rules (only within visible/wrap boundary)
        for (int y = 0; y < wrapH; y++) {
            for (int x = 0; x < wrapW; x++) {
                int idx = x + w * y;
                int neighbors = countNeighbors(x, y);
                bool wasAlive = buffer[idx].alive;

                if (wasAlive) {
                    if (!shouldSurvive(neighbors) && !buffer[idx].locked) {
                        // Death (protected cells are immune)
                        cells[idx].alive = false;
                        cells[idx].note = '.';
                        cells[idx].age = 0;

                        // Emit note-off (hold mode only; pulse already silenced above)
                        if (!pulseMode && activeNotes[idx].active && eventCount < maxEvents) {
                            auto& e = outEvents[eventCount++];
                            e.type = MidiEvent::NoteOff;
                            e.bytes[0] = static_cast<uint8_t>(0x80 + activeNotes[idx].channel);
                            e.bytes[1] = activeNotes[idx].midiNote;
                            e.bytes[2] = 0;
                            e.numBytes = 3;
                            activeNotes[idx].active = false;
                        }
                    } else {
                        // Survival — increment age
                        if (decay)
                            cells[idx].age = buffer[idx].age + 1;
                    }
                } else {
                    if (shouldBeBorn(neighbors)) {
                        // Birth
                        LifeCell born = deriveBirthCell(x, y);
                        born.age = 0;
                        cells[idx] = born;
                    }
                }
            }
        }

        if (pulseMode && evolveRate > 1 && seqMode == SeqOff) {
            // Cluster detection + ratchet scheduling
            // clusterTag: 0 = unvisited, >0 = cluster ID (uses member buffer)
            int16_t* clusterTag = clusterTagBuf;
            memset(clusterTag, 0, sizeof(int16_t) * w * h);
            int nextCluster = 1;

            for (int wy = 0; wy < wrapH; wy++) {
            for (int wx = 0; wx < wrapW; wx++) {
                int i = wx + w * wy;
                if (!cells[i].alive || clusterTag[i] != 0) continue;

                // Flood fill orthogonally for same-note, same-channel cells
                char cNote = cells[i].note;
                uint8_t cCh = cells[i].channel;
                uint8_t cOct = cells[i].octave;
                int tag = nextCluster++;
                int clusterSize = 0;

                // Simple stack-based flood fill (uses member buffer)
                int* stack = floodStack;
                int stackTop = 0;
                stack[stackTop++] = i;
                clusterTag[i] = static_cast<int16_t>(tag);

                while (stackTop > 0) {
                    int ci = stack[--stackTop];
                    clusterSize++;
                    int cx = ci % w, cy = ci / w;
                    // Check 4 orthogonal neighbors (within wrap boundary)
                    const int ddx[] = {-1, 1, 0, 0};
                    const int ddy[] = {0, 0, -1, 1};
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + ddx[d], ny = cy + ddy[d];
                        if (nx < 0 || nx >= wrapW || ny < 0 || ny >= wrapH) continue;
                        int ni = nx + w * ny;
                        if (clusterTag[ni] != 0 || !cells[ni].alive) continue;
                        if (cells[ni].note != cNote || cells[ni].channel != cCh) continue;
                        clusterTag[ni] = static_cast<int16_t>(tag);
                        stack[stackTop++] = ni;
                    }
                }

                // Create ratchet or immediate note based on cluster size
                auto tr = transpose(cNote, cOct);
                if (!tr.valid || tr.id <= 0) continue;
                tr.id = snapToChordTone(tr.id);

                // Probability + density gates
                if (!canFireMore(cCh) || !shouldFire(i)) continue;

                if (dedup) {
                    // Dedup mode: collect all cluster cells individually for merging
                    uint8_t vel = velocityFor(i);
                    collectNote(cCh, static_cast<uint8_t>(tr.id), vel, i);
                    // Also collect other cluster cells (they share the same note)
                    for (int cy2 = 0; cy2 < wrapH; cy2++)
                        for (int cx2 = 0; cx2 < wrapW; cx2++) {
                            int ci2 = cx2 + w * cy2;
                            if (ci2 != i && clusterTag[ci2] == tag)
                                collectNote(cCh, static_cast<uint8_t>(tr.id), velocityFor(ci2), ci2);
                        }
                } else if (clusterSize > 1 && ratchetCount < kMaxRatchets) {
                    // Schedule ratchet: first note fires now, rest on subsequent frames
                    int ratchetNotes = clusterSize;
                    if (ratchetNotes > evolveRate - 1) ratchetNotes = evolveRate - 1; // cap to fit

                    // Fire first ratchet note immediately
                    uint8_t vel = velocityFor(i);
                    if (eventCount < maxEvents) {
                        auto& e = outEvents[eventCount++];
                        e.type = MidiEvent::NoteOn;
                        e.bytes[0] = static_cast<uint8_t>(0x90 + cCh);
                        e.bytes[1] = static_cast<uint8_t>(tr.id);
                        e.bytes[2] = vel;
                        e.numBytes = 3;
                        cells[i].fired = true;
                        notesFiredPerChannel[cCh & 0x0F]++;
                    }

                    ratchets[ratchetCount++] = {
                        static_cast<uint8_t>(tr.id), cCh,
                        static_cast<uint8_t>(ratchetNotes - 1), // remaining after first
                        true, vel
                    };

                    // Mark all cluster cells so they don't fire individually
                    // (clusterTag already marks them, we skip tagged cells below)
                } else if (clusterSize == 1) {
                    // Single cell: fire normally
                    if (eventCount < maxEvents) {
                        auto& e = outEvents[eventCount++];
                        e.type = MidiEvent::NoteOn;
                        e.bytes[0] = static_cast<uint8_t>(0x90 + cCh);
                        e.bytes[1] = static_cast<uint8_t>(tr.id);
                        e.bytes[2] = velocityFor(i);
                        e.numBytes = 3;
                        activeNotes[i] = { cCh, static_cast<uint8_t>(tr.id), true };
                        cells[i].fired = true;
                        notesFiredPerChannel[cCh & 0x0F]++;
                    }
                }
            } // wx
            } // wy
        } else {
            // Non-ratchet path: emit note-ons (with optional phase scheduling)
            for (int yy = 0; yy < wrapH; yy++) {
            int rowPhase = phaseForRow(yy);
            for (int xx = 0; xx < wrapW; xx++) {
                int i = xx + w * yy;
                if (!cells[i].alive) continue;
                bool wasBorn = !buffer[i].alive || firstEvolution;
                if (!wasBorn && !pulseMode) continue;

                // Probability gate (applies to all modes)
                if (!shouldFire(i)) continue;

                auto tr = transpose(cells[i].note, cells[i].octave);
                if (!tr.valid || tr.id <= 0) continue;
                tr.id = snapToChordTone(tr.id);

                // Phase offset: schedule for later frame (maxNotes checked in processPhaseNotes)
                if (rowPhase > 0 && phaseNoteCount < kMaxPhaseNotes) {
                    phaseNotes[phaseNoteCount++] = {
                        static_cast<uint8_t>(tr.id),
                        cells[i].channel,
                        velocityFor(i),
                        rowPhase, i
                    };
                    continue;
                }

                if (dedup) {
                    // Dedup mode: collect for merging
                    collectNote(cells[i].channel, static_cast<uint8_t>(tr.id), velocityFor(i), i);
                } else {
                    // Density gate (phase-0 notes in seq mode, or all notes in normal mode)
                    if (!canFireMore(cells[i].channel)) continue;

                    if (eventCount < maxEvents) {
                        auto& e = outEvents[eventCount++];
                        e.type = MidiEvent::NoteOn;
                        e.bytes[0] = static_cast<uint8_t>(0x90 + cells[i].channel);
                        e.bytes[1] = static_cast<uint8_t>(tr.id);
                        e.bytes[2] = velocityFor(i);
                        e.numBytes = 3;
                        activeNotes[i] = { cells[i].channel, static_cast<uint8_t>(tr.id), true };
                        cells[i].fired = true;
                        notesFiredPerChannel[cells[i].channel & 0x0F]++;
                    }
                }
            } // xx
            } // yy
        }

        // Flush dedup buffer if active
        if (dedup && preEmitCount > 0)
            eventCount += flushDedup(outEvents + eventCount, maxEvents - eventCount);

        firstEvolution = false;
        generation++;
        return eventCount;
    }

    // Process pending ratchets (called on non-evolve frames)
    int processRatchets(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        for (int r = 0; r < ratchetCount; r++) {
            auto& rch = ratchets[r];
            if (rch.remaining == 0) continue;

            // Silence previous ratchet note
            if (rch.activeNote && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                e.bytes[0] = static_cast<uint8_t>(0x80 + rch.channel);
                e.bytes[1] = rch.midiNote;
                e.bytes[2] = 0;
                e.numBytes = 3;
            }

            // Fire next ratchet note
            if (eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOn;
                e.bytes[0] = static_cast<uint8_t>(0x90 + rch.channel);
                e.bytes[1] = rch.midiNote;
                e.bytes[2] = rch.velocity;
                e.numBytes = 3;
                rch.activeNote = true;
            }
            rch.remaining--;
        }
        return eventCount;
    }

    // Silence any currently sounding ratchet notes
    int silenceRatchets(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        for (int r = 0; r < ratchetCount; r++) {
            if (ratchets[r].activeNote && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                e.bytes[0] = static_cast<uint8_t>(0x80 + ratchets[r].channel);
                e.bytes[1] = ratchets[r].midiNote;
                e.bytes[2] = 0;
                e.numBytes = 3;
                ratchets[r].activeNote = false;
            }
        }
        return eventCount;
    }

    // Silence all active notes
    int silence(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        for (int i = 0; i < w * h; i++) {
            if (activeNotes[i].active && eventCount < maxEvents) {
                auto& e = outEvents[eventCount++];
                e.type = MidiEvent::NoteOff;
                e.bytes[0] = static_cast<uint8_t>(0x80 + activeNotes[i].channel);
                e.bytes[1] = activeNotes[i].midiNote;
                e.bytes[2] = 0;
                e.numBytes = 3;
                activeNotes[i].active = false;
            }
        }
        return eventCount;
    }

    // Trigger note-ons for all alive cells (used on transport restart)
    int triggerAlive(MidiEvent* outEvents, int maxEvents) {
        int eventCount = 0;
        for (int i = 0; i < w * h; i++) {
            if (cells[i].alive && !activeNotes[i].active) {
                auto tr = transpose(cells[i].note, cells[i].octave);
                tr.id = snapToChordTone(tr.id);
                if (tr.valid && tr.id > 0 && eventCount < maxEvents) {
                    auto& e = outEvents[eventCount++];
                    e.type = MidiEvent::NoteOn;
                    e.bytes[0] = static_cast<uint8_t>(0x90 + cells[i].channel);
                    e.bytes[1] = static_cast<uint8_t>(tr.id);
                    e.bytes[2] = velocityFor(i);
                    e.numBytes = 3;
                    activeNotes[i] = { cells[i].channel, static_cast<uint8_t>(tr.id), true };
                }
            }
        }
        return eventCount;
    }

    // Initial state snapshot for reset
    LifeCell initialState[kMaxGridSize];
    bool hasInitialState = false;

    void saveInitialState() {
        memcpy(initialState, cells, sizeof(LifeCell) * w * h);
        hasInitialState = true;
    }

    int resetToInitial(MidiEvent* outEvents, int maxEvents) {
        int eventCount = silence(outEvents, maxEvents);
        if (hasInitialState) {
            memcpy(cells, initialState, sizeof(LifeCell) * w * h);
        }
        generation = 0;
        frameCounter = 0;
        return eventCount;
    }

    int population() const {
        int count = 0;
        for (int i = 0; i < w * h; i++)
            if (cells[i].alive) count++;
        return count;
    }

private:
    // Scratch buffers for cluster detection in step() — heap-allocated as class
    // members to avoid 384KB stack allocations on the audio thread.
    int16_t clusterTagBuf[kMaxGridSize];
    int floodStack[kMaxGridSize];
};

} // namespace orca
