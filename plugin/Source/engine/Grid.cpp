#include "Grid.h"
#include "Operator.h"
#include "EngineIO.h"
#include <cstring>

namespace orca {

Grid::Grid() {
    runtimeCount = 0;
    reset();
}

void Grid::reset(int newW, int newH) {
    w = newW;
    h = newH;
    f = 0;
    int size = w * h;
    for (int i = 0; i < size; i++) cells[i] = '.';
    for (int i = size; i < kMaxGridSize; i++) cells[i] = 0;
    memset(locks, 0, sizeof(locks));
    memset(variables, '.', sizeof(variables));
}

void Grid::load(int newW, int newH, const char* s, int frame) {
    w = newW;
    h = newH;
    f = frame;
    int size = w * h;
    int srcLen = s ? (int)strlen(s) : 0;

    for (int i = 0; i < size; i++) {
        if (i < srcLen) {
            char g = s[i];
            if (g == '\n' || g == '\r') {
                cells[i] = '.';
            } else if (!isAllowed(g)) {
                cells[i] = '.';
            } else {
                cells[i] = g;
            }
        } else {
            cells[i] = '.';
        }
    }
}

void Grid::run() {
    release();
    parse();
    operate();
    f++;
}

// Grid access

char Grid::glyphAt(int x, int y) const {
    int idx = indexAt(x, y);
    if (idx < 0) return '.';
    return cells[idx];
}

int Grid::valueAt(int x, int y) const {
    return valOfGlyph(glyphAt(x, y));
}

bool Grid::inBounds(int x, int y) const {
    return x >= 0 && x < w && y >= 0 && y < h;
}

int Grid::indexAt(int x, int y) const {
    if (!inBounds(x, y)) return -1;
    return x + (w * y);
}

bool Grid::write(int x, int y, char g) {
    if (g == 0) return false;
    if (!inBounds(x, y)) return false;
    if (glyphAt(x, y) == g) return false;
    int idx = indexAt(x, y);
    cells[idx] = isAllowed(g) ? g : '.';
    return true;
}

// Locks

void Grid::release() {
    int size = w * h;
    memset(locks, 0, size * sizeof(bool));
    memset(variables, '.', sizeof(variables));
}

void Grid::lock(int x, int y) {
    int idx = indexAt(x, y);
    if (idx < 0) return;
    locks[idx] = true;
}

void Grid::unlock(int x, int y) {
    int idx = indexAt(x, y);
    if (idx < 0) return;
    locks[idx] = false;
}

bool Grid::lockAt(int x, int y) const {
    int idx = indexAt(x, y);
    if (idx < 0) return false;
    return locks[idx];
}

// Variables

char Grid::valueIn(char key) const {
    uint8_t idx = static_cast<uint8_t>(key);
    if (idx >= 128) return '.';
    char v = variables[idx];
    return v ? v : '.';
}

// Helpers

bool Grid::isAllowed(char g) const {
    if (g == '.') return true;
    return isOperatorGlyph(g);
}

char Grid::keyOfVal(int val, bool uc) const {
    int idx = ((val % kNumKeys) + kNumKeys) % kNumKeys;
    char c = kKeys[idx];
    return uc ? toUpper(c) : c;
}

int Grid::valOfGlyph(char g) const {
    return valueOf(g);
}

// Block operations

void Grid::getBlock(int x, int y, int bw, int bh, char* out, int outSize) const {
    int pos = 0;
    for (int _y = y; _y < y + bh && pos < outSize - 1; _y++) {
        for (int _x = x; _x < x + bw && pos < outSize - 1; _x++) {
            out[pos++] = glyphAt(_x, _y);
        }
        if (pos < outSize - 1) out[pos++] = '\n';
    }
    out[pos] = 0;
}

void Grid::writeBlock(int x, int y, const char* block, bool overlap) {
    if (!block) return;
    int _x = x, _y = y;
    for (int i = 0; block[i] != 0; i++) {
        if (block[i] == '\n') {
            _y++;
            _x = x;
            continue;
        }
        char g = block[i];
        if (overlap && g == '.') {
            g = glyphAt(_x, _y);
        }
        write(_x, _y, g);
        _x++;
    }
}

// Formatting

int Grid::format(char* out, int outSize) const {
    int pos = 0;
    for (int y = 0; y < h && pos < outSize - 1; y++) {
        for (int x = 0; x < w && pos < outSize - 1; x++) {
            out[pos++] = glyphAt(x, y);
        }
        if (pos < outSize - 1) out[pos++] = '\n';
    }
    out[pos] = 0;
    return pos;
}

// Internal

void Grid::parse() {
    runtimeCount = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            char g = glyphAt(x, y);
            if (g == '.' || !isAllowed(g)) continue;
            if (runtimeCount >= kMaxOperators) break;

            bool isPassive = isUpper(g) || isSpecial(g);
            if (createOperator(runtime[runtimeCount], g, x, y, isPassive, *this)) {
                runtimeCount++;
            }
        }
    }
    // Remove operators whose cells were locked during parse
    // (e.g. chord ] locks its west input cells to prevent them being operators)
    int writeIdx = 0;
    for (int i = 0; i < runtimeCount; i++) {
        if (!lockAt(runtime[i].x, runtime[i].y)) {
            if (writeIdx != i) runtime[writeIdx] = runtime[i];
            writeIdx++;
        }
    }
    runtimeCount = writeIdx;
}

void Grid::operate() {
    for (int i = 0; i < runtimeCount; i++) {
        auto& op = runtime[i];
        if (lockAt(op.x, op.y)) continue;
        if (op.passive || op.hasNeighbor(*this, '*')) {
            op.run(*this, *io, false);
        }
    }
}

} // namespace orca
