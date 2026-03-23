#include "sampler.h"
#include <SD.h>

// ============================================================
// Voice management
// ============================================================

struct VoiceState {
    bool     active;
    uint32_t startTimeUs;
    uint8_t  instrumentIndex;
};

static AudioPlaySdWav*  _voices[NUM_VOICES];
static VoiceState       _voiceStates[NUM_VOICES];

void samplerInit(AudioPlaySdWav* voiceArray[NUM_VOICES]) {
    for (int i = 0; i < NUM_VOICES; i++) {
        _voices[i] = voiceArray[i];
        _voiceStates[i].active = false;
        _voiceStates[i].startTimeUs = 0;
        _voiceStates[i].instrumentIndex = 0xFF;
    }
}

// ============================================================
// Allocate a voice — prefer free voice, else steal oldest
// ============================================================
static uint8_t allocateVoice() {
    // First pass: find a free (not playing) voice
    for (int i = 0; i < NUM_VOICES; i++) {
        if (!_voices[i]->isPlaying()) {
            _voiceStates[i].active = false;
            return i;
        }
    }

    // All busy: steal the oldest
    uint32_t oldest = UINT32_MAX;
    uint8_t oldestIdx = 0;
    uint32_t now = micros();
    for (int i = 0; i < NUM_VOICES; i++) {
        uint32_t age = now - _voiceStates[i].startTimeUs;
        if (age > (now - oldest)) {  // find smallest startTimeUs
            // Simpler: just track minimum startTimeUs
        }
    }
    // Simplified: find voice with smallest startTimeUs
    oldest = _voiceStates[0].startTimeUs;
    oldestIdx = 0;
    for (int i = 1; i < NUM_VOICES; i++) {
        if (_voiceStates[i].startTimeUs < oldest) {
            oldest = _voiceStates[i].startTimeUs;
            oldestIdx = i;
        }
    }

    _voices[oldestIdx]->stop();
    _voiceStates[oldestIdx].active = false;
    return oldestIdx;
}

// ============================================================
// Trigger a single instrument sample
// ============================================================
void samplerTrigger(uint8_t instrumentIndex, uint8_t velocity, const Pattern& pat) {
    if (instrumentIndex >= pat.numInstruments) return;
    if (velocity == 0) return;

    uint8_t voiceIdx = allocateVoice();

    // Build full path: "samples/<filename>"
    char path[64];
    snprintf(path, sizeof(path), "samples/%s", pat.instruments[instrumentIndex].filename);

    _voices[voiceIdx]->play(path);
    _voiceStates[voiceIdx].active = true;
    _voiceStates[voiceIdx].startTimeUs = micros();
    _voiceStates[voiceIdx].instrumentIndex = instrumentIndex;

    // TODO: velocity -> volume scaling
    // For now, all voices play at unity gain.
    // Future: adjust mixer gain for this voice based on velocity.
}

// ============================================================
// Trigger all instruments that have hits at a given step
// ============================================================
void samplerTriggerStep(Pattern* pat, uint8_t measureStep) {
    if (measureStep >= pat->totalSteps) return;

    for (uint8_t i = 0; i < pat->numInstruments; i++) {
        uint8_t vel = pat->grid[i][measureStep];
        if (vel > 0) {
            samplerTrigger(i, vel, *pat);
        }
    }
}

// ============================================================
// Stop all voices
// ============================================================
void samplerStopAll() {
    for (int i = 0; i < NUM_VOICES; i++) {
        _voices[i]->stop();
        _voiceStates[i].active = false;
    }
}
