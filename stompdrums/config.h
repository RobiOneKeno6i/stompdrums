#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// StompDrums — Configuration & Pin Definitions
// Teensy 4.1 + Audio Shield Rev D
// ============================================================

// ---- Audio Shield Rev D (SPI0) — pins occupied ----
// 6(MEMCS), 7(RX/DIN), 8(TX/DOUT), 10(SDCS),
// 11(MOSI), 12(MISO), 13(SCK),
// 18(SDA), 19(SCL), 20(LRCLK), 21(BCLK), 23(MCLK)

// ---- SPI1 bus (OLED + W25Q128 Flash) ----
#define PIN_SPI1_MOSI   26
#define PIN_SPI1_MISO    1
#define PIN_SPI1_SCK    27

// ---- OLED SSD1306 128x64 SPI ----
#define PIN_OLED_DC     30
#define PIN_OLED_CS     31
#define PIN_OLED_RST    32
#define OLED_WIDTH     128
#define OLED_HEIGHT     64

// ---- Winbond W25Q128 Flash ----
#define PIN_FLASH_CS    29

// ---- Encoder 1 (pattern selection) ----
#define PIN_ENC1_A       2
#define PIN_ENC1_B       3
#define PIN_ENC1_BTN     4

// ---- Encoder 2 (swing / params) ----
#define PIN_ENC2_A       5
#define PIN_ENC2_B       9
#define PIN_ENC2_BTN    22

// ---- RGB Status LED (PWM) ----
#define PIN_LED_R       33
#define PIN_LED_G       36
#define PIN_LED_B       37

// ---- Audio / Tap Detection ----
#define TAP_THRESHOLD       0.15f   // AudioAnalyzePeak threshold (0.0–1.0)
#define MIN_TAP_INTERVAL_US 200000  // 200ms debounce = max 300 BPM
#define MAX_TAP_GAP_FACTOR  2.5f    // gap > factor*quarterDur → reset to FIRST_TAP
#define BPM_MIN             40.0f
#define BPM_MAX            300.0f

// ---- Pattern Limits ----
#define MAX_INSTRUMENTS      8
#define MAX_STEPS           64      // max steps per measure
#define INST_TAG_LEN         4
#define FILENAME_LEN        32
#define PATTERN_NAME_LEN    32
#define MAX_PATTERNS        32

// ---- Swing ----
#define SWING_MIN           50
#define SWING_MAX           75
#define SWING_DEFAULT       50      // straight

// ---- Voice Count ----
#define NUM_VOICES           8

#endif // CONFIG_H
