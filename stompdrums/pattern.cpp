#include "pattern.h"
#include <SD.h>

// ============================================================
// Velocity mapping: character -> 0-127
// '.' = 0 (silence)
// '1'-'9' = 14, 28, 42, 56, 70, 84, 98, 112, 127
// ============================================================
uint8_t velocityFromChar(char c) {
    if (c == '.' || c == ' ' || c == '0') return 0;
    if (c >= '1' && c <= '9') {
        uint8_t digit = c - '0';
        if (digit == 9) return 127;
        return digit * 14;
    }
    return 0;
}

// ============================================================
// Trim whitespace from both ends of a string (in place)
// ============================================================
static void trimString(char* s) {
    // Trim leading
    char* start = s;
    while (*start == ' ' || *start == '\t') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    // Trim trailing
    int len = strlen(s);
    while (len > 0 && (s[len - 1] == ' '  || s[len - 1] == '\t' ||
                        s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

// ============================================================
// Parse time signature string "4/4" -> beats=4, unit=4
// ============================================================
static bool parseTimeSig(const char* s, uint8_t& beats, uint8_t& unit) {
    int b, u;
    if (sscanf(s, "%d/%d", &b, &u) == 2) {
        beats = (uint8_t)b;
        unit = (uint8_t)u;
        return true;
    }
    return false;
}

// ============================================================
// Parse instrument line: "KK = kick_tight.wav"
// ============================================================
static bool parseInstrument(const char* s, Instrument& inst) {
    // Find '='
    const char* eq = strchr(s, '=');
    if (!eq) return false;

    // Tag: everything before '='
    int tagLen = eq - s;
    if (tagLen <= 0 || tagLen >= INST_TAG_LEN) return false;

    // Copy tag, trimming spaces
    char tagBuf[16];
    strncpy(tagBuf, s, tagLen);
    tagBuf[tagLen] = '\0';
    trimString(tagBuf);
    strncpy(inst.tag, tagBuf, INST_TAG_LEN - 1);
    inst.tag[INST_TAG_LEN - 1] = '\0';

    // Filename: everything after '='
    const char* fn = eq + 1;
    while (*fn == ' ' || *fn == '\t') fn++;
    strncpy(inst.filename, fn, FILENAME_LEN - 1);
    inst.filename[FILENAME_LEN - 1] = '\0';
    trimString(inst.filename);

    inst.voiceIndex = 0;
    return true;
}

// ============================================================
// Parse grid line: "KK  9...5...9...5..."
// ============================================================
static bool parseGrid(const char* s, Pattern& pat) {
    // Skip leading whitespace
    while (*s == ' ' || *s == '\t') s++;

    // Extract tag (first non-space token)
    char tag[INST_TAG_LEN];
    int i = 0;
    while (*s && *s != ' ' && *s != '\t' && i < INST_TAG_LEN - 1) {
        tag[i++] = *s++;
    }
    tag[i] = '\0';

    // Find matching instrument
    int instIdx = -1;
    for (int j = 0; j < pat.numInstruments; j++) {
        if (strcmp(pat.instruments[j].tag, tag) == 0) {
            instIdx = j;
            break;
        }
    }
    if (instIdx < 0) return false;

    // Skip whitespace to reach grid data
    while (*s == ' ' || *s == '\t') s++;

    // Parse grid characters
    uint8_t step = 0;
    while (*s && step < MAX_STEPS) {
        if (*s == ' ' || *s == '\t' || *s == '|') {
            s++;  // skip separators
            continue;
        }
        pat.grid[instIdx][step] = velocityFromChar(*s);
        step++;
        s++;
    }

    return true;
}

// ============================================================
// Load pattern from .sdp file
// ============================================================
bool patternLoad(const char* filepath, Pattern& pat) {
    File f = SD.open(filepath, FILE_READ);
    if (!f) {
        Serial.print("Cannot open pattern: ");
        Serial.println(filepath);
        return false;
    }

    // Zero out
    memset(&pat, 0, sizeof(Pattern));
    pat.beatsPerMeasure = 4;
    pat.beatUnit = 4;
    pat.resolution = 4;
    pat.swing = SWING_DEFAULT;
    pat.defaultBPM = 120;

    char line[256];
    while (f.available()) {
        // Read line
        int len = 0;
        while (f.available() && len < (int)sizeof(line) - 1) {
            char c = f.read();
            if (c == '\n') break;
            if (c != '\r') line[len++] = c;
        }
        line[len] = '\0';

        // Skip empty lines and comments
        trimString(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        // Parse key: value
        char* colon = strchr(line, ':');
        if (!colon) continue;

        // Extract key
        char key[32];
        int keyLen = colon - line;
        if (keyLen >= (int)sizeof(key)) keyLen = sizeof(key) - 1;
        strncpy(key, line, keyLen);
        key[keyLen] = '\0';
        trimString(key);

        // Extract value
        char* val = colon + 1;
        while (*val == ' ' || *val == '\t') val++;

        // Match keys
        if (strcmp(key, "name") == 0) {
            strncpy(pat.name, val, PATTERN_NAME_LEN - 1);
        }
        else if (strcmp(key, "timesig") == 0) {
            parseTimeSig(val, pat.beatsPerMeasure, pat.beatUnit);
        }
        else if (strcmp(key, "resolution") == 0) {
            pat.resolution = atoi(val);
        }
        else if (strcmp(key, "swing") == 0) {
            pat.swing = constrain(atoi(val), SWING_MIN, SWING_MAX);
        }
        else if (strcmp(key, "bpm_default") == 0) {
            pat.defaultBPM = atoi(val);
        }
        else if (strcmp(key, "instrument") == 0) {
            if (pat.numInstruments < MAX_INSTRUMENTS) {
                if (parseInstrument(val, pat.instruments[pat.numInstruments])) {
                    pat.instruments[pat.numInstruments].voiceIndex =
                        pat.numInstruments % NUM_VOICES;
                    pat.numInstruments++;
                }
            }
        }
        else if (strcmp(key, "grid") == 0) {
            parseGrid(val, pat);
        }
    }

    f.close();

    // Calculate total steps
    pat.totalSteps = pat.beatsPerMeasure * pat.resolution;
    if (pat.totalSteps > MAX_STEPS) pat.totalSteps = MAX_STEPS;

    Serial.print("Pattern loaded: ");
    Serial.print(pat.name);
    Serial.print(" [");
    Serial.print(pat.beatsPerMeasure);
    Serial.print("/");
    Serial.print(pat.beatUnit);
    Serial.print("] res=");
    Serial.print(pat.resolution);
    Serial.print(" steps=");
    Serial.print(pat.totalSteps);
    Serial.print(" instruments=");
    Serial.println(pat.numInstruments);

    return true;
}

// ============================================================
// List .sdp pattern files from /patterns/ on SD
// ============================================================
uint8_t patternListFromSD(char names[][PATTERN_NAME_LEN], uint8_t maxCount) {
    File dir = SD.open("patterns");
    if (!dir) {
        Serial.println("No /patterns/ folder on SD");
        return 0;
    }

    uint8_t count = 0;
    while (count < maxCount) {
        File entry = dir.openNextFile();
        if (!entry) break;

        const char* fname = entry.name();
        int len = strlen(fname);

        // Check .sdp extension
        if (len > 4 && strcasecmp(fname + len - 4, ".sdp") == 0) {
            strncpy(names[count], fname, PATTERN_NAME_LEN - 1);
            names[count][PATTERN_NAME_LEN - 1] = '\0';
            count++;
        }
        entry.close();
    }
    dir.close();

    return count;
}
