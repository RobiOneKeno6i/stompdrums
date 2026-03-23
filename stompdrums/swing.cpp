#include "swing.h"

// ============================================================
// Swing offset for a given step within a beat
//
// Model: each pair of consecutive steps (0,1), (2,3), (4,5)...
// The first in each pair (even index) is on-grid.
// The second (odd index) is delayed by swing amount.
//
// At swing=50: offset=0 (perfectly straight)
// At swing=66: offset = 0.32 * stepDuration (triplet feel)
// At swing=75: offset = 0.50 * stepDuration (hard shuffle)
//
// Formula: offset = (swingPercent - 50) * 2 * stepDurationUs / 100
// This maps swing 50->0%, 75->50% of step duration.
// ============================================================

int32_t swingOffsetUs(uint8_t stepInBeat, uint8_t resolution,
                       uint8_t swingPercent, uint32_t stepDurationUs) {
    (void)resolution;  // available for future curved mappings

    // Only odd steps in each pair get swing
    if ((stepInBeat % 2) == 0) return 0;

    // Linear mapping
    int32_t offset = ((int32_t)(swingPercent - 50) * 2 * (int32_t)stepDurationUs) / 100;
    return offset;
}
