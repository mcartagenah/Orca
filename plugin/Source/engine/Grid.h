#pragma once

#include "Types.h"
#include "Operator.h"

namespace orca {
class EngineIO;

class Grid {
public:
    int w = 1;
    int h = 1;
    int f = 0;

    char cells[kMaxGridSize];
    bool locks[kMaxGridSize];
    char variables[128]; // indexed by char value, stores glyph

    OperatorInstance runtimeStorage[kMaxOperators];
    OperatorInstance* runtime = runtimeStorage;
    int runtimeCount = 0;
    EngineIO* io = nullptr;

    Grid();

    void reset(int newW = 25, int newH = 25);
    void load(int newW, int newH, const char* s, int frame = 0);
    void run();

    // Grid access
    char glyphAt(int x, int y) const;
    int valueAt(int x, int y) const;
    bool inBounds(int x, int y) const;
    int indexAt(int x, int y) const;
    bool write(int x, int y, char g);

    // Locks
    void release();
    void lock(int x, int y);
    void unlock(int x, int y);
    bool lockAt(int x, int y) const;

    // Variables
    char valueIn(char key) const;

    // Helpers
    bool isAllowed(char g) const;
    char keyOfVal(int val, bool uc = false) const;
    int valOfGlyph(char g) const;

    // Block operations
    void getBlock(int x, int y, int bw, int bh, char* out, int outSize) const;
    void writeBlock(int x, int y, const char* block, bool overlap = false);

    // Formatting
    int format(char* out, int outSize) const;

    // Parse operators (public for shadow buffer updates)
    void parse();

private:
    void operate();
    void clean(const char* src, char* dst, int maxLen);
};

} // namespace orca
