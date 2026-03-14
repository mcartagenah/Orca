#pragma once

#include "Types.h"

namespace orca {

struct TransposeResult {
    int id;      // MIDI note number 0-127
    int value;   // index in note scale
    char note;   // note name character
    int octave;  // octave number
    bool valid;
};

// Note name to scale index: C=0, c=1, D=2, d=3, E=4, F=5, f=6, G=7, g=8, A=9, a=10, B=11
inline int noteIndex(char n) {
    switch (n) {
        case 'C': return 0;  case 'c': return 1;
        case 'D': return 2;  case 'd': return 3;
        case 'E': return 4;  case 'F': return 5;
        case 'f': return 6;  case 'G': return 7;
        case 'g': return 8;  case 'A': return 9;
        case 'a': return 10; case 'B': return 11;
        default:  return -1;
    }
}

// Transpose table: maps input character to (noteName, octaveOffset)
// Mirrors the JS transposeTable exactly
inline bool getTransposeEntry(char c, char& noteName, int& octaveOffset) {
    switch (c) {
        // Standard chromatic mapping
        case 'A': noteName = 'A'; octaveOffset = 0; return true;
        // 'a' lowercase = sharp (a#)
        // In Orca's table: lowercase = flat/sharp variants
        case 'B': noteName = 'B'; octaveOffset = 0; return true;
        case 'C': noteName = 'C'; octaveOffset = 0; return true;
        case 'c': noteName = 'c'; octaveOffset = 0; return true;
        case 'D': noteName = 'D'; octaveOffset = 0; return true;
        case 'd': noteName = 'd'; octaveOffset = 0; return true;
        case 'E': noteName = 'E'; octaveOffset = 0; return true;
        case 'F': noteName = 'F'; octaveOffset = 0; return true;
        case 'G': noteName = 'G'; octaveOffset = 0; return true;
        case 'g': noteName = 'g'; octaveOffset = 0; return true;
        case 'H': noteName = 'A'; octaveOffset = 0; return true;
        case 'h': noteName = 'a'; octaveOffset = 0; return true;
        case 'I': noteName = 'B'; octaveOffset = 0; return true;
        case 'J': noteName = 'C'; octaveOffset = 1; return true;
        case 'j': noteName = 'c'; octaveOffset = 1; return true;
        case 'K': noteName = 'D'; octaveOffset = 1; return true;
        case 'k': noteName = 'd'; octaveOffset = 1; return true;
        case 'L': noteName = 'E'; octaveOffset = 1; return true;
        case 'M': noteName = 'F'; octaveOffset = 1; return true;
        case 'm': noteName = 'f'; octaveOffset = 1; return true;
        case 'N': noteName = 'G'; octaveOffset = 1; return true;
        case 'n': noteName = 'g'; octaveOffset = 1; return true;
        case 'O': noteName = 'A'; octaveOffset = 1; return true;
        case 'o': noteName = 'a'; octaveOffset = 1; return true;
        case 'P': noteName = 'B'; octaveOffset = 1; return true;
        case 'Q': noteName = 'C'; octaveOffset = 2; return true;
        case 'q': noteName = 'c'; octaveOffset = 2; return true;
        case 'R': noteName = 'D'; octaveOffset = 2; return true;
        case 'r': noteName = 'd'; octaveOffset = 2; return true;
        case 'S': noteName = 'E'; octaveOffset = 2; return true;
        case 'T': noteName = 'F'; octaveOffset = 2; return true;
        case 't': noteName = 'f'; octaveOffset = 2; return true;
        case 'U': noteName = 'G'; octaveOffset = 2; return true;
        case 'u': noteName = 'g'; octaveOffset = 2; return true;
        case 'V': noteName = 'A'; octaveOffset = 2; return true;
        case 'v': noteName = 'a'; octaveOffset = 2; return true;
        case 'W': noteName = 'B'; octaveOffset = 2; return true;
        case 'X': noteName = 'C'; octaveOffset = 3; return true;
        case 'x': noteName = 'c'; octaveOffset = 3; return true;
        case 'Y': noteName = 'D'; octaveOffset = 3; return true;
        case 'y': noteName = 'd'; octaveOffset = 3; return true;
        case 'Z': noteName = 'E'; octaveOffset = 3; return true;
        // Catch letters that map to enharmonic equivalents
        case 'a': noteName = 'a'; octaveOffset = 0; return true;
        case 'e': noteName = 'F'; octaveOffset = 0; return true;
        case 'f': noteName = 'f'; octaveOffset = 0; return true;
        case 'l': noteName = 'F'; octaveOffset = 1; return true;
        case 's': noteName = 'F'; octaveOffset = 2; return true;
        case 'z': noteName = 'F'; octaveOffset = 3; return true;
        case 'b': noteName = 'C'; octaveOffset = 1; return true;
        case 'i': noteName = 'C'; octaveOffset = 1; return true;
        case 'p': noteName = 'C'; octaveOffset = 2; return true;
        case 'w': noteName = 'C'; octaveOffset = 3; return true;
        default: return false;
    }
}

inline TransposeResult transpose(char n, int o = 3) {
    TransposeResult result = { 0, 0, '.', 0, false };
    char noteName;
    int octaveOffset;
    if (!getTransposeEntry(n, noteName, octaveOffset)) return result;

    int octave = clamp(o + octaveOffset, 0, 8);
    int value = noteIndex(noteName);
    if (value < 0) return result;

    result.id = clamp((octave * 12) + value + 24, 0, 127);
    result.value = value;
    result.note = noteName;
    result.octave = octave;
    result.valid = true;
    return result;
}

} // namespace orca
