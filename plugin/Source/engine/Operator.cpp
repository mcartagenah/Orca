#include "Operator.h"
#include "Grid.h"
#include "EngineIO.h"
#include <cmath>
#include <cstdlib>

namespace orca {

// ── OperatorInstance base methods ──

void OperatorInstance::init(OpType t, int px, int py, char g, bool isPassive) {
    type = t;
    x = px;
    y = py;
    glyph = g;
    passive = isPassive;
    draw = isPassive;
    portCount = 0;
    outputPortIdx = -1;
}

void OperatorInstance::addPort(int idx, int rx, int ry, bool output, bool bang,
                               bool reader, bool sensitive,
                               char defaultVal, int clampMin, int clampMax) {
    if (idx >= kMaxPorts) return;
    auto& p = ports[idx];
    p.x = rx;
    p.y = ry;
    p.output = output;
    p.bang = bang;
    p.reader = reader;
    p.sensitive = sensitive;
    p.defaultVal = defaultVal;
    p.clampMin = clampMin;
    p.clampMax = clampMax;
    p.active = true;
    if (idx >= portCount) portCount = idx + 1;
    if (output && outputPortIdx < 0) outputPortIdx = idx;
}

char OperatorInstance::listen(Grid& grid, const Port& port, bool toValue) const {
    char g = grid.glyphAt(x + port.x, y + port.y);
    char gl = (g == '.' || g == '*') && port.defaultVal ? port.defaultVal : g;
    if (toValue) {
        int val = valueOf(gl);
        return static_cast<char>(clamp(val, port.clampMin, port.clampMax));
    }
    return gl;
}

char OperatorInstance::listenAt(Grid& grid, int rx, int ry, bool toValue) const {
    Port p;
    p.x = rx; p.y = ry;
    p.defaultVal = 0; p.clampMin = 0; p.clampMax = 36;
    p.active = true;
    return listen(grid, p, toValue);
}

void OperatorInstance::output(Grid& grid, char g, const Port& port) const {
    if (g == 0) return;
    if (shouldUpperCase(grid) && port.sensitive) {
        g = toUpper(g);
    }
    grid.write(x + port.x, y + port.y, g);
}

void OperatorInstance::bang(Grid& grid, bool b) const {
    if (outputPortIdx < 0) return;
    auto& p = ports[outputPortIdx];
    grid.write(x + p.x, y + p.y, b ? '*' : '.');
    grid.lock(x + p.x, y + p.y);
}

void OperatorInstance::move(Grid& grid, int dx, int dy) {
    int nx = x + dx, ny = y + dy;
    if (!grid.inBounds(nx, ny) || grid.glyphAt(nx, ny) != '.') {
        explode(grid);
        return;
    }
    erase(grid);
    x += dx;
    y += dy;
    grid.write(x, y, glyph);
    lockSelf(grid);
}

bool OperatorInstance::hasNeighbor(Grid& grid, char g) const {
    if (grid.glyphAt(x + 1, y) == g) return true;
    if (grid.glyphAt(x - 1, y) == g) return true;
    if (grid.glyphAt(x, y + 1) == g) return true;
    if (grid.glyphAt(x, y - 1) == g) return true;
    return false;
}

void OperatorInstance::erase(Grid& grid) {
    grid.write(x, y, '.');
}

void OperatorInstance::explode(Grid& grid) {
    grid.write(x, y, '*');
}

void OperatorInstance::lockSelf(Grid& grid) {
    grid.lock(x, y);
}

bool OperatorInstance::shouldUpperCase(Grid& grid) const {
    if (outputPortIdx < 0 || !ports[outputPortIdx].sensitive) return false;
    char val = grid.glyphAt(x + 1, y);
    if (!isLetter(val)) return false;
    return isUpper(val);
}

// ── Main run ──

void OperatorInstance::run(Grid& grid, EngineIO& io, bool force) {
    char result = 0;
    bool hasResult = false;

    operation(grid, io, force, result, hasResult);

    // Lock all non-bang ports
    for (int i = 0; i < portCount; i++) {
        if (!ports[i].active || ports[i].bang) continue;
        grid.lock(x + ports[i].x, y + ports[i].y);
    }

    if (outputPortIdx >= 0 && hasResult) {
        auto& outPort = ports[outputPortIdx];
        if (outPort.bang) {
            bang(grid, result != 0);
        } else {
            output(grid, result, outPort);
        }
    }
}

// ── Port index constants ──
// We use fixed indices for named ports to avoid string lookups.
enum PortIdx {
    PA = 0,   // 'a' input
    PB = 1,   // 'b' input
    POut = 2, // output
    PRate = 0,
    PMod = 1,
    PStep = 0,
    PMax = 1,
    PX = 0,
    PY = 1,
    PLen = 2,
    PVal = 3,
    PKey = 0,
    PWrite = 0,
    PRead = 1,
    PChannel = 0,
    POctave = 1,
    PNote = 2,
    PVelocity = 3,
    PLength = 4,
    PKnob = 1,
    PValue = 2,
    PLsb = 1,
    PMsb = 2,
    PPath = 0,
};

// ── Operator-specific logic ──

void OperatorInstance::operation(Grid& grid, EngineIO& io, bool force,
                                 char& result, bool& hasResult) {
    switch (type) {

    case OpType::A: { // add
        int a = static_cast<int>(listen(grid, ports[PA], true));
        int b = static_cast<int>(listen(grid, ports[PB], true));
        result = keyOf(a + b);
        hasResult = true;
        break;
    }

    case OpType::B: { // subtract
        int a = static_cast<int>(listen(grid, ports[PA], true));
        int b = static_cast<int>(listen(grid, ports[PB], true));
        result = keyOf(std::abs(b - a));
        hasResult = true;
        break;
    }

    case OpType::C: { // clock
        int rate = static_cast<int>(listen(grid, ports[PRate], true));
        int mod = static_cast<int>(listen(grid, ports[PMod], true));
        if (rate == 0) rate = 1;
        int val = mod > 0 ? (grid.f / rate) % mod : 0;
        result = keyOf(val);
        hasResult = true;
        break;
    }

    case OpType::D: { // delay
        int rate = static_cast<int>(listen(grid, ports[PRate], true));
        int mod = static_cast<int>(listen(grid, ports[PMod], true));
        if (rate == 0) rate = 1;
        if (mod == 0) mod = 1;
        int res = grid.f % (mod * rate);
        result = (res == 0 || mod == 1) ? 1 : 0;
        hasResult = true;
        break;
    }

    case OpType::E: { // east
        move(grid, 1, 0);
        passive = false;
        break;
    }

    case OpType::F: { // if
        char a = listen(grid, ports[PA], false);
        char b = listen(grid, ports[PB], false);
        result = (a == b) ? 1 : 0;
        hasResult = true;
        break;
    }

    case OpType::G: { // generator
        int len = static_cast<int>(listen(grid, ports[PLen], true));
        int gx = static_cast<int>(listen(grid, ports[PX], true));
        int gy = static_cast<int>(listen(grid, ports[PY], true)) + 1;
        for (int offset = 0; offset < len; offset++) {
            int inIdx = 4 + offset * 2;
            int outIdx = 4 + offset * 2 + 1;
            if (inIdx >= kMaxPorts || outIdx >= kMaxPorts) break;

            addPort(inIdx, offset + 1, 0, false, false, false, false, 0, 0, 36);
            addPort(outIdx, gx + offset, gy, true, false, false, false, 0, 0, 36);

            char res = listen(grid, ports[inIdx], false);
            output(grid, res, ports[outIdx]);
        }
        break;
    }

    case OpType::H: { // halt
        grid.lock(x + ports[POut].x, y + ports[POut].y);
        result = listen(grid, ports[POut], false);
        hasResult = false; // halt reads but doesn't write
        break;
    }

    case OpType::I: { // increment
        int step = static_cast<int>(listen(grid, ports[PStep], true));
        int mod = static_cast<int>(listen(grid, ports[PMod], true));
        int val = static_cast<int>(listen(grid, ports[POut], true));
        if (mod > 0) {
            result = keyOf((val + step) % mod);
        } else {
            result = '0';
        }
        hasResult = true;
        break;
    }

    case OpType::J: { // jumper
        char val = listenAt(grid, 0, -1, false);
        if (val != 'J') {
            int i = 0;
            while (grid.inBounds(x, y + i)) {
                char next = listenAt(grid, 0, ++i, false);
                if (next != glyph) break;
            }
            // Add input and output ports dynamically
            int inIdx = 0, outIdx = 1;
            addPort(inIdx, 0, -1, false, false, false, false, 0, 0, 36);
            addPort(outIdx, 0, i, true, false, false, false, 0, 0, 36);
            outputPortIdx = outIdx;
            result = val;
            hasResult = true;
        }
        break;
    }

    case OpType::K: { // konkat
        int len = static_cast<int>(listen(grid, ports[0], true));
        for (int offset = 0; offset < len; offset++) {
            char key = grid.glyphAt(x + offset + 1, y);
            grid.lock(x + offset + 1, y);
            if (key == '.') continue;

            int inIdx = 2 + offset * 2;
            int outIdx = 2 + offset * 2 + 1;
            if (inIdx >= kMaxPorts || outIdx >= kMaxPorts) break;

            addPort(inIdx, offset + 1, 0, false, false, false, false, 0, 0, 36);
            addPort(outIdx, offset + 1, 1, true, false, false, false, 0, 0, 36);

            char res = grid.valueIn(key);
            output(grid, res, ports[outIdx]);
        }
        break;
    }

    case OpType::L: { // lesser
        int a = static_cast<int>(listen(grid, ports[PA], true));
        int b = static_cast<int>(listen(grid, ports[PB], true));
        result = keyOf(a > b ? b : a);
        hasResult = true;
        break;
    }

    case OpType::M: { // multiply
        int a = static_cast<int>(listen(grid, ports[PA], true));
        int b = static_cast<int>(listen(grid, ports[PB], true));
        result = keyOf(a * b);
        hasResult = true;
        break;
    }

    case OpType::N: { // north
        move(grid, 0, -1);
        passive = false;
        break;
    }

    case OpType::O: { // read
        int ox = static_cast<int>(listen(grid, ports[PX], true));
        int oy = static_cast<int>(listen(grid, ports[PY], true));
        int readIdx = 3;
        addPort(readIdx, ox + 1, oy, false, false, false, false, 0, 0, 36);
        result = listen(grid, ports[readIdx], false);
        hasResult = true;
        break;
    }

    case OpType::P: { // push
        int len = static_cast<int>(listen(grid, ports[1], true));
        int key = static_cast<int>(listen(grid, ports[0], true));
        for (int offset = 0; offset < len; offset++) {
            grid.lock(x + offset, y + 1);
        }
        int outIdx = 3;
        addPort(outIdx, (len > 0) ? key % len : 0, 1, true, false, false, false, 0, 0, 36);
        outputPortIdx = outIdx;
        result = listen(grid, ports[2], false);
        hasResult = true;
        break;
    }

    case OpType::Q: { // query
        int len = static_cast<int>(listen(grid, ports[PLen], true));
        int qx = static_cast<int>(listen(grid, ports[PX], true));
        int qy = static_cast<int>(listen(grid, ports[PY], true));
        for (int offset = 0; offset < len; offset++) {
            int inIdx = 4 + offset * 2;
            int outIdx = 4 + offset * 2 + 1;
            if (inIdx >= kMaxPorts || outIdx >= kMaxPorts) break;

            addPort(inIdx, qx + offset + 1, qy, false, false, false, false, 0, 0, 36);
            addPort(outIdx, offset - len + 1, 1, true, false, false, false, 0, 0, 36);

            char res = listen(grid, ports[inIdx], false);
            output(grid, res, ports[outIdx]);
        }
        break;
    }

    case OpType::R: { // random
        int a = static_cast<int>(listen(grid, ports[PA], true));
        int b = static_cast<int>(listen(grid, ports[PB], true));
        if (a == b) {
            result = keyOf(a);
        } else if (a > b) {
            result = keyOf(std::rand() % (a - b + 1) + b);
        } else {
            result = keyOf(std::rand() % (b - a + 1) + a);
        }
        hasResult = true;
        break;
    }

    case OpType::S: { // south
        move(grid, 0, 1);
        passive = false;
        break;
    }

    case OpType::T: { // track
        int len = static_cast<int>(listen(grid, ports[1], true));
        int key = static_cast<int>(listen(grid, ports[0], true));
        for (int offset = 0; offset < len; offset++) {
            grid.lock(x + offset + 1, y);
        }
        int valIdx = 3;
        addPort(valIdx, (len > 0) ? (key % len) + 1 : 1, 0, false, false, false, false, 0, 0, 36);
        result = listen(grid, ports[valIdx], false);
        hasResult = true;
        break;
    }

    case OpType::U: { // uclid
        int step = static_cast<int>(listen(grid, ports[PStep], true));
        int max = static_cast<int>(listen(grid, ports[PMax], true));
        if (max == 0) max = 1;
        int bucket = (step * (grid.f + max - 1)) % max + step;
        result = (bucket >= max) ? 1 : 0;
        hasResult = true;
        break;
    }

    case OpType::V: { // variable
        char w = listen(grid, ports[PWrite], false);
        char r = listen(grid, ports[PRead], false);
        if (w == '.' && r != '.') {
            int outIdx = 2;
            addPort(outIdx, 0, 1, true, false, false, false, 0, 0, 36);
            outputPortIdx = outIdx;
            result = grid.valueIn(r);
            hasResult = true;
        }
        if (w != '.') {
            uint8_t idx = static_cast<uint8_t>(w);
            if (idx < 128) grid.variables[idx] = r;
        }
        break;
    }

    case OpType::W: { // west
        move(grid, -1, 0);
        passive = false;
        break;
    }

    case OpType::X: { // write
        int wx = static_cast<int>(listen(grid, ports[0], true));
        int wy = static_cast<int>(listen(grid, ports[1], true)) + 1;
        result = listen(grid, ports[2], false);  // read val before creating dynamic output
        int outIdx = 3;
        addPort(outIdx, wx, wy, true, false, false, false, 0, 0, 36);
        outputPortIdx = outIdx;
        hasResult = true;
        break;
    }

    case OpType::Y: { // jymper
        char val = listenAt(grid, -1, 0, false);
        if (val != 'Y') {
            int i = 0;
            while (grid.inBounds(x + i, y)) {
                char next = listenAt(grid, ++i, 0, false);
                if (next != glyph) break;
            }
            int inIdx = 0, outIdx = 1;
            addPort(inIdx, -1, 0, false, false, false, false, 0, 0, 36);
            addPort(outIdx, i, 0, true, false, false, false, 0, 0, 36);
            outputPortIdx = outIdx;
            result = val;
            hasResult = true;
        }
        break;
    }

    case OpType::Z: { // lerp
        int rate = static_cast<int>(listen(grid, ports[PRate], true));
        int target = static_cast<int>(listen(grid, ports[PMod], true)); // target is at port index 1
        int val = static_cast<int>(listen(grid, ports[POut], true));
        int mod;
        if (val <= target - rate) mod = rate;
        else if (val >= target + rate) mod = -rate;
        else mod = target - val;
        result = keyOf(val + mod);
        hasResult = true;
        break;
    }

    case OpType::Bang: { // *
        draw = false;
        erase(grid);
        break;
    }

    case OpType::Comment: { // #
        for (int cx = x + 1; cx <= grid.w; cx++) {
            grid.lock(cx, y);
            if (grid.glyphAt(cx, y) == '#') break;
        }
        grid.lock(x, y);
        break;
    }

    case OpType::Midi: { // :
        if (!hasNeighbor(grid, '*') && !force) break;
        char chGlyph = listen(grid, ports[PChannel], false);
        char octGlyph = listen(grid, ports[POctave], false);
        char noteGlyph = listen(grid, ports[PNote], false);
        if (chGlyph == '.' || octGlyph == '.' || noteGlyph == '.') break;
        // Note must be a letter, not a digit
        if (isDigitChar(noteGlyph)) break;

        int channel = static_cast<int>(listen(grid, ports[PChannel], true));
        if (channel > 15) break;
        int octave = static_cast<int>(listen(grid, ports[POctave], true));
        char note = noteGlyph;
        int velocity = static_cast<int>(listen(grid, ports[PVelocity], true));
        int length = static_cast<int>(listen(grid, ports[PLength], true));

        io.midi.push(channel, octave, note, velocity, length);
        draw = false;
        break;
    }

    case OpType::CC: { // !
        if (!hasNeighbor(grid, '*') && !force) break;
        char chGlyph = listen(grid, ports[PChannel], false);
        char knobGlyph = listen(grid, ports[PKnob], false);
        if (chGlyph == '.' || knobGlyph == '.') break;

        int channel = static_cast<int>(listen(grid, ports[PChannel], true));
        if (channel > 15) break;
        int knob = static_cast<int>(listen(grid, ports[PKnob], true));
        int rawValue = static_cast<int>(listen(grid, ports[PValue], true));
        int value = (int)std::ceil((127.0 * rawValue) / 35.0);

        io.cc.pushCC(channel, knob, value);
        draw = false;
        break;
    }

    case OpType::PitchBend: { // ?
        if (!hasNeighbor(grid, '*') && !force) break;
        char chGlyph = listen(grid, ports[PChannel], false);
        char lsbGlyph = listen(grid, ports[PLsb], false);
        if (chGlyph == '.' || lsbGlyph == '.') break;

        int channel = static_cast<int>(listen(grid, ports[PChannel], true));
        int rawLsb = static_cast<int>(listen(grid, ports[PLsb], true));
        int lsb = (int)std::ceil((127.0 * rawLsb) / 35.0);
        int rawMsb = static_cast<int>(listen(grid, ports[PMsb], true));
        int msb = (int)std::ceil((127.0 * rawMsb) / 35.0);

        io.cc.pushPB(channel, lsb, msb);
        draw = false;
        break;
    }

    case OpType::Mono: { // %
        if (!hasNeighbor(grid, '*') && !force) break;
        char chGlyph = listen(grid, ports[PChannel], false);
        char octGlyph = listen(grid, ports[POctave], false);
        char noteGlyph = listen(grid, ports[PNote], false);
        if (chGlyph == '.' || octGlyph == '.' || noteGlyph == '.') break;
        if (isDigitChar(noteGlyph)) break;

        int channel = static_cast<int>(listen(grid, ports[PChannel], true));
        if (channel > 15) break;
        int octave = static_cast<int>(listen(grid, ports[POctave], true));
        char note = noteGlyph;
        int velocity = static_cast<int>(listen(grid, ports[PVelocity], true));
        int length = static_cast<int>(listen(grid, ports[PLength], true));

        io.mono.push(channel, octave, note, velocity, length);
        draw = false;
        break;
    }

    case OpType::Osc: // = (no-op in plugin)
    case OpType::Udp: // ; (no-op in plugin)
    {
        // Lock eastward glyphs until '.'
        for (int lx = 1; lx <= 36; lx++) {
            char g = grid.glyphAt(x + lx, y);
            grid.lock(x + lx, y);
            if (g == '.') break;
        }
        break;
    }

    case OpType::Self: { // $
        // Lock eastward glyphs until '.'
        for (int lx = 1; lx <= 36; lx++) {
            char g = grid.glyphAt(x + lx, y);
            grid.lock(x + lx, y);
            if (g == '.') break;
        }
        // No-op in plugin context (would need commander)
        break;
    }

    case OpType::Null:
        break;

    default:
        break;
    }
}

// ── Factory ──

bool isOperatorGlyph(char g) {
    char lower = toLower(g);
    if (lower >= 'a' && lower <= 'z') return true;
    if (g >= '0' && g <= '9') return true;
    switch (g) {
        case '*': case '#': case ':': case '!':
        case '?': case '%': case '=': case ';':
        case '$':
            return true;
        default:
            return false;
    }
}

bool createOperator(OperatorInstance& op, char glyph, int x, int y, bool isPassive) {
    char lower = toLower(glyph);
    op.portCount = 0;
    op.outputPortIdx = -1;

    // Map glyph to OpType and set up ports
    switch (lower) {
    case 'a':
        op.init(OpType::A, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);              // a
        op.addPort(PB, 1, 0);               // b
        op.addPort(POut, 0, 1, true, false, false, true); // output (sensitive)
        return true;

    case 'b':
        op.init(OpType::B, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);
        op.addPort(PB, 1, 0);
        op.addPort(POut, 0, 1, true, false, false, true);
        return true;

    case 'c':
        op.init(OpType::C, x, y, glyph, isPassive);
        op.addPort(PRate, -1, 0, false, false, false, false, 0, 1, 36);
        op.addPort(PMod, 1, 0);
        op.addPort(POut, 0, 1, true, false, false, true);
        return true;

    case 'd':
        op.init(OpType::D, x, y, glyph, isPassive);
        op.addPort(PRate, -1, 0, false, false, false, false, 0, 1, 36);
        op.addPort(PMod, 1, 0, false, false, false, false, 0, 1, 36);
        op.addPort(POut, 0, 1, true, true); // bang output
        return true;

    case 'e':
        op.init(OpType::E, x, y, glyph, isPassive);
        op.draw = false;
        return true;

    case 'f':
        op.init(OpType::F, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);
        op.addPort(PB, 1, 0);
        op.addPort(POut, 0, 1, true, true); // bang output
        return true;

    case 'g':
        op.init(OpType::G, x, y, glyph, isPassive);
        op.addPort(PX, -3, 0);
        op.addPort(PY, -2, 0);
        op.addPort(PLen, -1, 0, false, false, false, false, 0, 1, 36);
        return true;

    case 'h':
        op.init(OpType::H, x, y, glyph, isPassive);
        op.addPort(POut, 0, 1, true, false, true); // reader output
        return true;

    case 'i':
        op.init(OpType::I, x, y, glyph, isPassive);
        op.addPort(PStep, -1, 0);
        op.addPort(PMod, 1, 0);
        op.addPort(POut, 0, 1, true, false, true, true); // reader, sensitive
        return true;

    case 'j':
        op.init(OpType::J, x, y, glyph, isPassive);
        // Ports added dynamically in operation
        return true;

    case 'k':
        op.init(OpType::K, x, y, glyph, isPassive);
        op.addPort(0, -1, 0, false, false, false, false, 0, 1, 36); // len
        return true;

    case 'l':
        op.init(OpType::L, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);
        op.addPort(PB, 1, 0);
        op.addPort(POut, 0, 1, true, false, false, true);
        return true;

    case 'm':
        op.init(OpType::M, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);
        op.addPort(PB, 1, 0);
        op.addPort(POut, 0, 1, true, false, false, true);
        return true;

    case 'n':
        op.init(OpType::N, x, y, glyph, isPassive);
        op.draw = false;
        return true;

    case 'o':
        op.init(OpType::O, x, y, glyph, isPassive);
        op.addPort(PX, -2, 0);
        op.addPort(PY, -1, 0);
        op.addPort(POut, 0, 1, true);
        return true;

    case 'p':
        op.init(OpType::P, x, y, glyph, isPassive);
        op.addPort(0, -2, 0);                                        // key
        op.addPort(1, -1, 0, false, false, false, false, 0, 1, 36);  // len
        op.addPort(2, 1, 0);                                          // val
        // output port added dynamically in operation at index 3
        return true;

    case 'q':
        op.init(OpType::Q, x, y, glyph, isPassive);
        op.addPort(PX, -3, 0);
        op.addPort(PY, -2, 0);
        op.addPort(PLen, -1, 0, false, false, false, false, 0, 1, 36);
        return true;

    case 'r':
        op.init(OpType::R, x, y, glyph, isPassive);
        op.addPort(PA, -1, 0);
        op.addPort(PB, 1, 0);
        op.addPort(POut, 0, 1, true, false, false, true);
        return true;

    case 's':
        op.init(OpType::S, x, y, glyph, isPassive);
        op.draw = false;
        return true;

    case 't':
        op.init(OpType::T, x, y, glyph, isPassive);
        op.addPort(0, -2, 0);                                        // key
        op.addPort(1, -1, 0, false, false, false, false, 0, 1, 36);  // len
        op.addPort(2, 0, 1, true);                                    // output
        return true;

    case 'u':
        op.init(OpType::U, x, y, glyph, isPassive);
        op.addPort(PStep, -1, 0, false, false, false, false, 0, 0, 36);
        op.addPort(PMax, 1, 0, false, false, false, false, 0, 1, 36);
        op.addPort(POut, 0, 1, true, true); // bang
        return true;

    case 'v':
        op.init(OpType::V, x, y, glyph, isPassive);
        op.addPort(PWrite, -1, 0);
        op.addPort(PRead, 1, 0);
        // Output port added dynamically
        return true;

    case 'w':
        op.init(OpType::W, x, y, glyph, isPassive);
        op.draw = false;
        return true;

    case 'x':
        op.init(OpType::X, x, y, glyph, isPassive);
        op.addPort(0, -2, 0);   // x offset
        op.addPort(1, -1, 0);   // y offset
        op.addPort(2, 1, 0);    // val
        // output port added dynamically in operation at index 3
        return true;

    case 'y':
        op.init(OpType::Y, x, y, glyph, isPassive);
        // Ports added dynamically
        return true;

    case 'z':
        op.init(OpType::Z, x, y, glyph, isPassive);
        op.addPort(PRate, -1, 0);
        op.addPort(PMod, 1, 0);  // target
        op.addPort(POut, 0, 1, true, false, true, true); // reader, sensitive
        return true;
    default:
        break;
    }

    // Special characters
    switch (glyph) {
    case '*':
        op.init(OpType::Bang, x, y, '*', true);
        op.draw = false;
        return true;

    case '#':
        op.init(OpType::Comment, x, y, '#', true);
        op.draw = false;
        return true;

    case ':':
        op.init(OpType::Midi, x, y, ':', true);
        op.addPort(PChannel, 1, 0);
        op.addPort(POctave, 2, 0, false, false, false, false, 0, 0, 8);
        op.addPort(PNote, 3, 0);
        op.addPort(PVelocity, 4, 0, false, false, false, false, 'f', 0, 16);
        op.addPort(PLength, 5, 0, false, false, false, false, '1', 0, 32);
        return true;

    case '!':
        op.init(OpType::CC, x, y, '!', true);
        op.addPort(PChannel, 1, 0);
        op.addPort(PKnob, 2, 0, false, false, false, false, 0, 0, 36);
        op.addPort(PValue, 3, 0, false, false, false, false, 0, 0, 36);
        return true;

    case '?':
        op.init(OpType::PitchBend, x, y, '?', true);
        op.addPort(PChannel, 1, 0, false, false, false, false, 0, 0, 15);
        op.addPort(PLsb, 2, 0, false, false, false, false, 0, 0, 36);
        op.addPort(PMsb, 3, 0, false, false, false, false, 0, 0, 36);
        return true;

    case '%':
        op.init(OpType::Mono, x, y, '%', true);
        op.addPort(PChannel, 1, 0);
        op.addPort(POctave, 2, 0, false, false, false, false, 0, 0, 8);
        op.addPort(PNote, 3, 0);
        op.addPort(PVelocity, 4, 0, false, false, false, false, 'f', 0, 16);
        op.addPort(PLength, 5, 0, false, false, false, false, '1', 0, 32);
        return true;

    case '=':
        op.init(OpType::Osc, x, y, '=', true);
        return true;

    case ';':
        op.init(OpType::Udp, x, y, ';', true);
        return true;

    case '$':
        op.init(OpType::Self, x, y, '$', true);
        return true;
    }

    // Digit characters (0-9)
    if (glyph >= '0' && glyph <= '9') {
        op.init(OpType::Null, x, y, '.', false);
        return true;
    }

    return false;
}

} // namespace orca
