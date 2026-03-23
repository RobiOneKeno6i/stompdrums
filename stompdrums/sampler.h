#ifndef SAMPLER_H
#define SAMPLER_H

#include <Audio.h>
#include "config.h"
#include "pattern.h"

// ============================================================
// Sampler — manages 8 AudioPlaySdWav voices
// ============================================================

void    samplerInit(AudioPlaySdWav* voiceArray[NUM_VOICES]);
void    samplerTrigger(uint8_t instrumentIndex, uint8_t velocity, const Pattern& pat);
void    samplerTriggerStep(Pattern* pat, uint8_t measureStep);
void    samplerStopAll();

#endif // SAMPLER_H
