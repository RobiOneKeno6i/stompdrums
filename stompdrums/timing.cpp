#include "timing.h"
#include "pattern.h"
#include "sampler.h"
#include "swing.h"

// ============================================================
// Init
// ============================================================
void timingInit(TimingState* ts) {
    ts->state = TIMING_IDLE;
    ts->lastTapUs = 0;
    ts->currentTapUs = 0;
    ts->quarterDurationUs = 0;
    ts->stepDurationUs = 0;
    ts->nextStepUs = 0;
    ts->currentBeat = 0;
    ts->measureStep = 0;
    ts->swing = SWING_DEFAULT;
    ts->newTapFlag = false;
}

// ============================================================
// Process a foot tap (Super Pulse)
// ============================================================
void timingProcessTap(TimingState* ts, uint32_t tapMicros) {
    switch (ts->state) {

    case TIMING_IDLE:
    case TIMING_STOPPED:
        // First tap — just record time, wait for second
        ts->currentTapUs = tapMicros;
        ts->state = TIMING_FIRST_TAP;
        ts->currentBeat = 0;
        ts->measureStep = 0;
        break;

    case TIMING_FIRST_TAP:
        // Second tap — establish tempo
        ts->lastTapUs = ts->currentTapUs;
        ts->currentTapUs = tapMicros;
        ts->quarterDurationUs = ts->currentTapUs - ts->lastTapUs;

        // Sanity check BPM range
        float bpm = 60000000.0f / ts->quarterDurationUs;
        if (bpm < BPM_MIN || bpm > BPM_MAX) {
            // Unreasonable tempo, stay in FIRST_TAP
            ts->currentTapUs = tapMicros;
            return;
        }

        ts->state = TIMING_RUNNING;
        ts->currentBeat = 0;
        ts->measureStep = 0;
        ts->newTapFlag = true;
        // stepDurationUs will be set by the scheduler based on pattern resolution
        break;

    case TIMING_RUNNING: {
        uint32_t elapsed = tapMicros - ts->currentTapUs;

        // Check if too long since last tap → reset
        if (elapsed > (uint32_t)(ts->quarterDurationUs * MAX_TAP_GAP_FACTOR)) {
            ts->currentTapUs = tapMicros;
            ts->state = TIMING_FIRST_TAP;
            ts->currentBeat = 0;
            ts->measureStep = 0;
            return;
        }

        // Update tempo from new interval
        ts->lastTapUs = ts->currentTapUs;
        ts->currentTapUs = tapMicros;
        ts->quarterDurationUs = ts->currentTapUs - ts->lastTapUs;

        // Advance to next beat in the measure
        ts->currentBeat++;
        // Let the pattern handle wrap-around via totalSteps
        ts->newTapFlag = true;
        break;
    }
    }
}

// ============================================================
// Scheduler Tick — called every loop() iteration
// ============================================================
void timingSchedulerTick(TimingState* ts, Pattern* pat) {
    if (ts->state != TIMING_RUNNING) return;
    if (pat->totalSteps == 0) return;

    uint8_t resolution = pat->resolution;
    if (resolution == 0) return;

    ts->stepDurationUs = ts->quarterDurationUs / resolution;
    if (ts->stepDurationUs == 0) return;

    // On new tap: snap to beat boundary and trigger step 0 of that beat
    if (ts->newTapFlag) {
        ts->newTapFlag = false;

        // Wrap beat around measure
        if (ts->currentBeat >= pat->beatsPerMeasure) {
            ts->currentBeat = 0;
        }

        // Calculate global measure step
        ts->measureStep = ts->currentBeat * resolution;

        // Trigger instruments at this step
        samplerTriggerStep(pat, ts->measureStep);

        // Schedule next step
        uint8_t stepInBeat = 0;  // we're on beat boundary = step 0
        int32_t swOff = swingOffsetUs(stepInBeat + 1, resolution,
                                       ts->swing, ts->stepDurationUs);
        ts->nextStepUs = ts->currentTapUs + ts->stepDurationUs + swOff;
        return;
    }

    // Between taps: check if it's time for the next step
    uint32_t now = micros();
    if ((int32_t)(now - ts->nextStepUs) >= 0) {
        // Advance step
        ts->measureStep++;
        if (ts->measureStep >= pat->totalSteps) {
            ts->measureStep = 0;
            ts->currentBeat = 0;
        } else {
            ts->currentBeat = ts->measureStep / resolution;
        }

        uint8_t stepInBeat = ts->measureStep % resolution;

        // Trigger instruments at this step
        samplerTriggerStep(pat, ts->measureStep);

        // Schedule next step with swing
        uint8_t nextStepInBeat = (stepInBeat + 1) % resolution;
        int32_t swOff = swingOffsetUs(nextStepInBeat, resolution,
                                       ts->swing, ts->stepDurationUs);
        // Base: one step duration from current scheduled time
        ts->nextStepUs += ts->stepDurationUs;
        // Adjust for swing on the next step
        // Swing offset is relative to straight grid, so we add/subtract
        // the difference between this step's offset and next step's offset
        int32_t curOff = swingOffsetUs(stepInBeat, resolution,
                                        ts->swing, ts->stepDurationUs);
        ts->nextStepUs += (swOff - curOff);
    }
}

// ============================================================
// Controls
// ============================================================
void timingStop(TimingState* ts) {
    ts->state = TIMING_STOPPED;
}

void timingReset(TimingState* ts) {
    timingInit(ts);
}

float timingGetBPM(const TimingState* ts) {
    if (ts->quarterDurationUs == 0) return 0.0f;
    return 60000000.0f / (float)ts->quarterDurationUs;
}

uint8_t timingGetCurrentBeat(const TimingState* ts) {
    return ts->currentBeat;
}

uint8_t timingGetMeasureStep(const TimingState* ts) {
    return ts->measureStep;
}
