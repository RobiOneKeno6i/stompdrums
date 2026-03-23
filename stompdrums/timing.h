#ifndef TIMING_H
#define TIMING_H

#include <Arduino.h>
#include "config.h"

// Forward declaration
struct Pattern;

// ============================================================
// Timing State
// ============================================================
enum TimingStateEnum {
    TIMING_IDLE,       // No taps yet
    TIMING_FIRST_TAP,  // One tap, waiting for second to set tempo
    TIMING_RUNNING,    // Tempo established, scheduling steps
    TIMING_STOPPED     // Manually stopped
};

struct TimingState {
    TimingStateEnum state;
    uint32_t lastTapUs;         // previous tap timestamp (micros)
    uint32_t currentTapUs;      // latest tap timestamp (micros)
    uint32_t quarterDurationUs; // duration of one beat (quarter note)
    uint32_t stepDurationUs;    // duration of one step
    uint32_t nextStepUs;        // scheduled time for next step
    uint8_t  currentBeat;       // 0..beatsPerMeasure-1
    uint8_t  measureStep;       // 0..totalSteps-1 (global position in measure)
    uint8_t  swing;             // current swing value (50-75)
    bool     newTapFlag;        // set on tap, cleared after processing
};

// ============================================================
// API
// ============================================================
void     timingInit(TimingState* ts);
void     timingProcessTap(TimingState* ts, uint32_t tapMicros);
void     timingSchedulerTick(TimingState* ts, Pattern* pat);
void     timingStop(TimingState* ts);
void     timingReset(TimingState* ts);
float    timingGetBPM(const TimingState* ts);
uint8_t  timingGetCurrentBeat(const TimingState* ts);
uint8_t  timingGetMeasureStep(const TimingState* ts);

#endif // TIMING_H
