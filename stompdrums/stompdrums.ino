// ============================================================
// StompDrums — Main Sketch
// Teensy 4.1 + Audio Shield Rev D
//
// Foot-synced drum pattern player.
// The musician taps tempo with a foot sensor; the system
// plays percussion patterns in real-time sync.
// ============================================================

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "pattern.h"
#include "timing.h"
#include "sampler.h"
#include "swing.h"
#include "ui.h"

// ============================================================
// Audio Graph
// ============================================================
// 8 stereo playback voices
AudioPlaySdWav       voice0, voice1, voice2, voice3;
AudioPlaySdWav       voice4, voice5, voice6, voice7;

// Left channel mixers (4 inputs each)
AudioMixer4          mixL1;    // voices 0-3 left
AudioMixer4          mixL2;    // voices 4-7 left
AudioMixer4          mixLout;  // final left mix

// Right channel mixers
AudioMixer4          mixR1;    // voices 0-3 right
AudioMixer4          mixR2;    // voices 4-7 right
AudioMixer4          mixRout;  // final right mix

// Output
AudioOutputI2S       audioOut;
AudioControlSGTL5000 sgtl5000;

// Input for foot sensor tap detection
AudioInputI2S        audioIn;
AudioAnalyzePeak     peakDetect;

// ---- Left channel connections ----
AudioConnection  cL0(voice0, 0, mixL1, 0);
AudioConnection  cL1(voice1, 0, mixL1, 1);
AudioConnection  cL2(voice2, 0, mixL1, 2);
AudioConnection  cL3(voice3, 0, mixL1, 3);
AudioConnection  cL4(voice4, 0, mixL2, 0);
AudioConnection  cL5(voice5, 0, mixL2, 1);
AudioConnection  cL6(voice6, 0, mixL2, 2);
AudioConnection  cL7(voice7, 0, mixL2, 3);
AudioConnection  cLm1(mixL1, 0, mixLout, 0);
AudioConnection  cLm2(mixL2, 0, mixLout, 1);
AudioConnection  cLo(mixLout, 0, audioOut, 0);

// ---- Right channel connections ----
AudioConnection  cR0(voice0, 1, mixR1, 0);
AudioConnection  cR1(voice1, 1, mixR1, 1);
AudioConnection  cR2(voice2, 1, mixR1, 2);
AudioConnection  cR3(voice3, 1, mixR1, 3);
AudioConnection  cR4(voice4, 1, mixR2, 0);
AudioConnection  cR5(voice5, 1, mixR2, 1);
AudioConnection  cR6(voice6, 1, mixR2, 2);
AudioConnection  cR7(voice7, 1, mixR2, 3);
AudioConnection  cRm1(mixR1, 0, mixRout, 0);
AudioConnection  cRm2(mixR2, 0, mixRout, 1);
AudioConnection  cRo(mixRout, 0, audioOut, 1);

// ---- Input for tap detection ----
AudioConnection  cIn(audioIn, 0, peakDetect, 0);

// ============================================================
// Voice array (for sampler module access)
// ============================================================
AudioPlaySdWav* voices[NUM_VOICES] = {
    &voice0, &voice1, &voice2, &voice3,
    &voice4, &voice5, &voice6, &voice7
};

// ============================================================
// Global State
// ============================================================
Pattern       currentPattern;
TimingState   timing;
UIState       uiState;

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);

    // Audio memory
    AudioMemory(40);

    // SGTL5000 codec
    sgtl5000.enable();
    sgtl5000.volume(0.6);
    sgtl5000.inputSelect(AUDIO_INPUT_MIC);
    sgtl5000.micGain(40);  // mic preamp gain for electret

    // Mixer gains — unity for all inputs
    float gain = 0.5;  // 8 voices summed, keep headroom
    for (int i = 0; i < 4; i++) {
        mixL1.gain(i, gain);
        mixL2.gain(i, gain);
        mixR1.gain(i, gain);
        mixR2.gain(i, gain);
    }
    mixLout.gain(0, 1.0);
    mixLout.gain(1, 1.0);
    mixRout.gain(0, 1.0);
    mixRout.gain(1, 1.0);

    // SD card (Audio Shield slot)
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD init failed!");
    } else {
        Serial.println("SD OK");
    }

    // RGB LED
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    analogWrite(PIN_LED_R, 0);
    analogWrite(PIN_LED_G, 0);
    analogWrite(PIN_LED_B, 0);

    // Initialize modules
    samplerInit(voices);
    timingInit(&timing);
    uiInit(&uiState);

    // Load pattern list from SD
    uiState.patternCount = patternListFromSD(uiState.patternNames, MAX_PATTERNS);
    Serial.print("Found patterns: ");
    Serial.println(uiState.patternCount);

    // Load first pattern if available
    if (uiState.patternCount > 0) {
        char path[64];
        snprintf(path, sizeof(path), "patterns/%s", uiState.patternNames[0]);
        if (patternLoad(path, currentPattern)) {
            Serial.print("Loaded: ");
            Serial.println(currentPattern.name);
            timing.swing = currentPattern.swing;
        }
    }

    Serial.println("StompDrums ready. Tap 'T' in serial or stomp to start.");
}

// ============================================================
// Loop
// ============================================================
void loop() {
    // 1. Check foot sensor (audio peak detection)
    checkFootSensor();

    // 2. Check serial tap simulation
    checkSerialTap();

    // 3. Timing scheduler
    timingSchedulerTick(&timing, &currentPattern);

    // 4. UI update
    uiTick(&uiState, &timing, &currentPattern);
}

// ============================================================
// Foot sensor detection via AudioAnalyzePeak
// ============================================================
void checkFootSensor() {
    if (peakDetect.available()) {
        float level = peakDetect.read();
        if (level > TAP_THRESHOLD) {
            uint32_t now = micros();
            if ((now - timing.currentTapUs) > MIN_TAP_INTERVAL_US) {
                timingProcessTap(&timing, now);
                analogWrite(PIN_LED_G, 128);  // flash green on tap
            }
        }
    }
}

// ============================================================
// Serial tap simulation (send 'T' to tap)
// ============================================================
void checkSerialTap() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'T' || c == 't') {
            uint32_t now = micros();
            timingProcessTap(&timing, now);
            Serial.print("TAP bpm=");
            Serial.println(timingGetBPM(&timing));
        }
        if (c == 'S' || c == 's') {
            timingStop(&timing);
            Serial.println("STOPPED");
        }
    }
}
