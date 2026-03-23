#include "ui.h"

// ============================================================
// UI — Stub implementation (Phase 6)
//
// For Phase 1-5, the UI only outputs to Serial.
// OLED + encoder integration comes in Phase 6.
// ============================================================

// Redraw throttle: max every 100ms
#define UI_REDRAW_INTERVAL_MS 100

void uiInit(UIState* ui) {
    ui->selectedPattern = 0;
    ui->patternCount = 0;
    ui->needsRedraw = true;
    ui->lastRedrawMs = 0;

    // Encoder pins as input with pullup
    pinMode(PIN_ENC1_A, INPUT_PULLUP);
    pinMode(PIN_ENC1_B, INPUT_PULLUP);
    pinMode(PIN_ENC1_BTN, INPUT_PULLUP);
    pinMode(PIN_ENC2_A, INPUT_PULLUP);
    pinMode(PIN_ENC2_B, INPUT_PULLUP);
    pinMode(PIN_ENC2_BTN, INPUT_PULLUP);

    // TODO Phase 6: Initialize SSD1306 OLED on SPI1
    // TODO Phase 6: Initialize Encoder library objects
    // TODO Phase 6: Initialize OneButton for click/double/long
}

void uiTick(UIState* ui, TimingState* ts, Pattern* pat) {
    // TODO Phase 6: poll encoders, buttons, redraw OLED

    // For now, throttled serial output
    uint32_t now = millis();
    if (now - ui->lastRedrawMs < UI_REDRAW_INTERVAL_MS) return;
    if (!ui->needsRedraw && ts->state != TIMING_RUNNING) return;

    ui->lastRedrawMs = now;

    // Minimal serial UI: show beat indicator while running
    if (ts->state == TIMING_RUNNING) {
        static uint8_t lastBeat = 0xFF;
        if (ts->currentBeat != lastBeat) {
            lastBeat = ts->currentBeat;
            Serial.print("Beat ");
            Serial.print(ts->currentBeat + 1);
            Serial.print("/");
            Serial.print(pat->beatsPerMeasure);
            Serial.print("  BPM=");
            Serial.print(timingGetBPM(ts), 1);
            Serial.print("  Swing=");
            Serial.println(ts->swing);
        }
    }
}
