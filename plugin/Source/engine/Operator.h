#pragma once

#include "Types.h"

namespace orca {

class Grid;
class EngineIO;

// Operator type enum — one per library entry
enum class OpType : uint8_t {
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Bang, Comment,
    Midi, CC, PitchBend, Mono, Osc, Udp, Self,
    Probability, Scale, Buffer, Freeze, Gate, Arp, Markov, Chord,
    Humanize, Ratchet, SwingGate, Strum,
    Null, // digit placeholders
    Count
};

struct OperatorInstance {
    OpType type;
    int x, y;
    bool passive;
    bool draw;
    char glyph;

    Port ports[kMaxPorts];
    int portCount;

    // Named port indices for quick access
    int outputPortIdx;

    void init(OpType t, int px, int py, char g, bool isPassive);
    void addPort(int idx, int rx, int ry, bool output = false, bool bang = false,
                 bool reader = false, bool sensitive = false,
                 char defaultVal = 0, int clampMin = 0, int clampMax = 36);

    // Core operator methods
    char listen(Grid& grid, const Port& port, bool toValue = false) const;
    char listenAt(Grid& grid, int rx, int ry, bool toValue = false) const;
    void output(Grid& grid, char g, const Port& port) const;
    void bang(Grid& grid, bool b) const;
    void move(Grid& grid, int dx, int dy);
    bool hasNeighbor(Grid& grid, char g) const;
    void erase(Grid& grid);
    void explode(Grid& grid);
    void lockSelf(Grid& grid);
    bool shouldUpperCase(Grid& grid) const;

    // Main execution
    void run(Grid& grid, EngineIO& io, bool force = false);
    void operation(Grid& grid, EngineIO& io, bool force, char& result, bool& hasResult);
};

// Factory: create operator from glyph
struct Grid;
bool createOperator(OperatorInstance& op, char glyph, int x, int y, bool isPassive, Grid& grid);

// Check if a glyph is a known operator
bool isOperatorGlyph(char g);

} // namespace orca
