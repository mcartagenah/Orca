#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace orca {

static constexpr int kMaxGridW = 256;
static constexpr int kMaxGridH = 256;
static constexpr int kMaxGridSize = kMaxGridW * kMaxGridH;
static constexpr int kMaxNotes = 128;
static constexpr int kMaxCCs = 64;
static constexpr int kMaxMonoChannels = 16;
static constexpr int kMaxEvents = 256;
static constexpr int kMaxOperators = 4096;
static constexpr int kMaxPorts = 40;
static constexpr int kNumKeys = 36;

static constexpr const char* kKeys = "0123456789abcdefghijklmnopqrstuvwxyz";

struct Port {
    int x = 0;
    int y = 0;
    bool output = false;
    bool bang = false;
    bool reader = false;
    bool sensitive = false;
    char defaultVal = 0;
    int clampMin = 0;
    int clampMax = 36;
    bool active = false;
};

struct MidiEvent {
    enum Type : uint8_t { NoteOn, NoteOff, CC, PitchBend, ProgramChange, BankSelect };
    Type type;
    uint8_t bytes[3];
    int numBytes;
};

struct NoteEntry {
    int channel = 0;
    int octave = 0;
    char note = '.';
    int velocity = 0;
    int length = 0;
    bool isPlayed = false;
    bool active = false;
};

struct CcEntry {
    enum CcType : uint8_t { TypeCC, TypePB, TypePG };
    CcType type;
    int channel = 0;
    int knob = 0;
    int value = 0;
    int lsb = 0;
    int msb = 0;
    int bank = -1;
    int sub = -1;
    int pgm = -1;
    bool active = false;
};

inline int clamp(int v, int min, int max) {
    return v < min ? min : v > max ? max : v;
}

inline int valueOf(char g) {
    if (g == '.' || g == '*' || g == 0) return 0;
    char lower = (g >= 'A' && g <= 'Z') ? (g - 'A' + 'a') : g;
    for (int i = 0; i < kNumKeys; i++) {
        if (kKeys[i] == lower) return i;
    }
    return 0;
}

inline char keyOf(int val) {
    int idx = ((val % kNumKeys) + kNumKeys) % kNumKeys;
    return kKeys[idx];
}

inline bool isUpper(char c) {
    return c >= 'A' && c <= 'Z';
}

inline char toLower(char c) {
    return isUpper(c) ? (c - 'A' + 'a') : c;
}

inline char toUpper(char c) {
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
}

inline bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isDigitChar(char c) {
    return c >= '0' && c <= '9';
}

inline bool isSpecial(char g) {
    // A character is "special" if it has no case distinction (not a letter)
    // and is not a digit or dot. Matches JS: g.toLowerCase() === g.toUpperCase() && isNaN(g)
    return toLower(g) == toUpper(g) && !isDigitChar(g) && g != '.';
}

} // namespace orca
