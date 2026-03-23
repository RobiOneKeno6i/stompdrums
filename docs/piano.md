# StompDrums — Piano di Implementazione

## Context

Progetto greenfield: stompbox musicale su Teensy 4.1 + Audio Shield Rev D. Il musicista batte il tempo con il piede su un sensore; il sistema esegue pattern ritmici percussivi sincronizzati in tempo reale. Pattern e campioni risiedono su microSD, con caching opzionale su flash Winbond W25Q128.

---

## Hardware

| Componente | Dettaglio |
|---|---|
| MCU | Teensy 4.1 |
| Audio | Audio Shield Rev D (SGTL5000, SD slot) |
| Display | SSD1306 128x64 SPI (su SPI1) |
| Flash | W25Q128 16MB SPI (su SPI1, CS separato) |
| Input | 2 encoder rotativi con pulsante |
| Sensore | Electret mic su Audio Shield (poi piezo) |
| LED | RGB status LED (PWM) |

### Pin Assignment

**Audio Shield (SPI0 — occupati):** 6, 7, 8, 10, 11, 12, 13, 18, 19, 20, 21, 23

**SPI1 (OLED + Flash):**

| Pin | Funzione |
|-----|----------|
| 26 | SPI1 MOSI |
| 27 | SPI1 SCK |
| 1 | SPI1 MISO (solo flash) |
| 30 | OLED DC |
| 31 | OLED CS |
| 32 | OLED RST |
| 29 | FLASH CS |

**Encoder + LED:**

| Pin | Funzione |
|-----|----------|
| 2, 3 | Encoder 1 (A, B) — selezione pattern |
| 4 | Encoder 1 button |
| 5, 9 | Encoder 2 (A, B) — swing |
| 22 | Encoder 2 button |
| 33 | LED R (PWM) |
| 36 | LED G (PWM) |
| 37 | LED B (PWM) |

**Sensore piede:** input audio SGTL5000 (mic/line-in), nessun pin GPIO extra.

---

## Formato File Pattern (.sdp)

```
# StompDrums Pattern v1
name: Rock Basic 4/4
timesig: 4/4
resolution: 4
swing: 55
bpm_default: 120

instrument: KK = kick_tight.wav
instrument: SN = snare_dry.wav
instrument: HH = hihat_closed.wav
instrument: HO = hihat_open.wav

# resolution: 4 = 16esimi (4 suddivisioni per beat)
# Per 4/4 con resolution 4 → 16 step totali
# Caratteri: '.' = silenzio, '1'-'9' = velocity (1=pp, 5=mf, 9=ff)
#
# Beat:  1 . . . 2 . . . 3 . . . 4 . . .
grid: KK  9.......9.......
grid: SN  ....9.......9...
grid: HH  5.5.5.5.5.5.5.5.
grid: HO  ..............5.
```

- `resolution` = suddivisioni per beat (4=16esimi, 3=terzine, 2=ottavi)
- `totalSteps = beatsPerMeasure * resolution`
- Lunghezza grid = totalSteps
- `swing` 50=straight, 66=triplet shuffle, 75=max

---

## Audio Graph (Teensy Audio Library)

```
8 x AudioPlaySdWav (voice0..voice7)
    ↓ (L/R channels)
2 x AudioMixer4 per canale (mixL1, mixL2, mixR1, mixR2)
    ↓
1 x AudioMixer4 per canale (mixLout, mixRout)
    ↓
AudioOutputI2S → SGTL5000 → uscita cuffie/linea

AudioInputI2S → AudioAnalyzePeak (rilevamento tap piede)
```

8 voci con voice stealing (la voce piu vecchia viene riutilizzata).

---

## Algoritmo Timing Engine

### Stati
`IDLE → FIRST_TAP → RUNNING → STOPPED`

### Su ogni tap (Super Pulse):
1. `quarterDurationUs = currentTapUs - lastTapUs`
2. `stepDurationUs = quarterDurationUs / resolution`
3. Se `timeSinceLastTap > 2 * quarterDurationUs` → reset a FIRST_TAP
4. Altrimenti → aggiorna tempo, avanza beat, triggera step 0 del beat

### In loop() — schedulerTick():
1. Se `micros() >= nextStepUs` → avanza step
2. Triggera strumenti con hit in quel step
3. Calcola `nextStepUs` con eventuale offset swing
4. Swing: step pari = on-grid, step dispari = ritardati di `(swing-50)*2*stepDur/100`

### Re-sync:
- Ogni tap = prossimo beat quarter
- Se pausa troppo lunga → stato FIRST_TAP, attende nuovo tap per stabilire tempo

---

## Struttura Moduli

| File | Responsabilita |
|------|---------------|
| `stompdrums.ino` | Audio graph, setup(), loop(), stato globale |
| `config.h` | Pin defines, costanti, limiti MAX |
| `timing.h/.cpp` | Rilevamento tap, calcolo tempo, scheduler step |
| `pattern.h/.cpp` | Parsing file .sdp, strutture dati pattern |
| `sampler.h/.cpp` | Gestione voci audio, trigger, voice stealing |
| `swing.h/.cpp` | Calcolo offset swing |
| `ui.h/.cpp` | OLED display, encoder, pulsanti |
| `flash_loader.h/.cpp` | W25Q128 caching (fase 8) |

### Strutture dati chiave

```cpp
struct Instrument { char tag[4]; char filename[32]; uint8_t voiceIdx; };

struct Pattern {
    char name[32]; uint8_t beats, beatUnit, resolution, swing;
    uint16_t defaultBPM; uint8_t numInst; uint8_t totalSteps;
    Instrument inst[8]; uint8_t grid[8][64]; // grid[inst][step]=velocity 0-127
};

struct TimingState {
    enum State { IDLE, FIRST_TAP, RUNNING, STOPPED } state;
    uint32_t lastTapUs, currentTapUs, quarterDurUs, stepDurUs, nextStepUs;
    uint8_t currentBeat, measureStep;
};
```

---

## Fasi di Implementazione

### Fase 1 — Playback audio base
- `stompdrums.ino` con 1 voce AudioPlaySdWav → mixer → output
- Init SD, SGTL5000
- Tasto/serial per triggerare un WAV
- **Verifica:** suono in cuffia

### Fase 2 — Multi-voice mixing
- Espandere a 8 voci con mixer tree
- `sampler.cpp`: trigger, allocazione voci, voice stealing
- **Verifica:** trigger multipli simultanei da serial

### Fase 3 — Timing engine (serial tap)
- `timing.cpp`: processamento tap, scheduler
- Pattern hardcoded (kick 1-3, snare 2-4, hihat tutti gli ottavi)
- Tap simulato via serial ("T")
- **Verifica:** pattern eseguito in sync con i tap seriali

### Fase 4 — Parser pattern .sdp
- `pattern.cpp`: parsing formato testo
- Scansione cartella `/patterns/` su SD
- 3-4 file .sdp di esempio
- **Verifica:** caricamento e esecuzione pattern da file

### Fase 5 — Sensore piede
- AudioInputI2S + AudioAnalyzePeak nel graph
- Rilevamento tap con soglia e debounce
- Test con electret mic sulla shield
- **Verifica:** pattern segue il tempo battuto dal piede

### Fase 6 — UI (OLED + encoder)
- SSD1306 su SPI1 (Adafruit_SSD1306)
- 2 encoder con OneButton per click/double/long
- Display: nome pattern, BPM, swing, indicatore beat
- **Verifica:** navigazione pattern e cambio swing funzionanti

### Fase 7 — Swing
- `swing.cpp`: calcolo offset
- Integrazione nello scheduler
- Encoder 2 regola swing in tempo reale
- **Verifica:** differenza udibile tra swing 50 e 66

### Fase 8 — Flash caching W25Q128
- W25Q128 su SPI1 con LittleFS_SPIFlash
- Copia campioni SD → flash al caricamento pattern
- Playback da flash (classe custom o AudioPlaySerialflashRaw)
- **Verifica:** latenza ridotta, playback stabile

---

## Verifica Finale

1. Caricare 3+ pattern .sdp sulla SD con campioni WAV stereo 44.1kHz
2. Boot → OLED mostra lista pattern
3. Encoder 1 seleziona pattern, click conferma
4. Battere il piede sull'electret → pattern parte in sync
5. Cambiare tempo → pattern segue
6. Encoder 2 regola swing → timing cambia in tempo reale
7. Pausa lunga → sistema si ferma → riprendere battendo
