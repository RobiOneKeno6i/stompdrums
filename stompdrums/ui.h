#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include "config.h"
#include "timing.h"
#include "pattern.h"

// ============================================================
// UI State
// ============================================================
struct UIState {
    int8_t   selectedPattern;   // current selection index
    uint8_t  patternCount;      // total patterns found on SD
    bool     needsRedraw;       // dirty flag
    uint32_t lastRedrawMs;      // throttle redraws
    char     patternNames[MAX_PATTERNS][PATTERN_NAME_LEN];
};

// ============================================================
// API
// ============================================================
void uiInit(UIState* ui);
void uiTick(UIState* ui, TimingState* ts, Pattern* pat);

#endif // UI_H
