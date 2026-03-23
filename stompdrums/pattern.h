#ifndef PATTERN_H
#define PATTERN_H

#include <Arduino.h>
#include "config.h"

// ============================================================
// Pattern Data Structures
// ============================================================

struct Instrument {
    char    tag[INST_TAG_LEN];       // "KK", "SN", "HH", etc.
    char    filename[FILENAME_LEN];  // "kick_tight.wav"
    uint8_t voiceIndex;              // assigned voice (0..NUM_VOICES-1)
};

struct Pattern {
    char    name[PATTERN_NAME_LEN];
    uint8_t beatsPerMeasure;  // time sig numerator (e.g., 4)
    uint8_t beatUnit;         // time sig denominator (e.g., 4)
    uint8_t resolution;       // subdivisions per beat
    uint8_t swing;            // 50-75
    uint16_t defaultBPM;
    uint8_t numInstruments;
    uint8_t totalSteps;       // beatsPerMeasure * resolution
    Instrument instruments[MAX_INSTRUMENTS];
    uint8_t grid[MAX_INSTRUMENTS][MAX_STEPS]; // velocity 0-127, 0=silent
};

// ============================================================
// API
// ============================================================

// Load a .sdp pattern file from SD card
bool patternLoad(const char* filepath, Pattern& pat);

// Scan /patterns/ folder on SD, fill names array, return count
uint8_t patternListFromSD(char names[][PATTERN_NAME_LEN], uint8_t maxCount);

// Convert grid character to velocity (0-127)
uint8_t velocityFromChar(char c);

#endif // PATTERN_H
