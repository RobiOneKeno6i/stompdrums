#ifndef SWING_H
#define SWING_H

#include <Arduino.h>

// ============================================================
// Swing offset calculation
//
// Swing affects pairs of subdivisions: (0,1), (2,3), (4,5)...
// Even steps (0,2,4...) stay on grid (offset = 0).
// Odd steps (1,3,5...) are delayed by the swing amount.
//
// swingPercent: 50 = straight, 66 = triplet swing, 75 = max shuffle
// Returns offset in microseconds to ADD to the straight grid time.
// ============================================================

int32_t swingOffsetUs(uint8_t stepInBeat, uint8_t resolution,
                       uint8_t swingPercent, uint32_t stepDurationUs);

#endif // SWING_H
