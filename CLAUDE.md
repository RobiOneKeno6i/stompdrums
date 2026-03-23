# StompDrums

Progetto stompbox musicale su Teensy 4.1 + Audio Shield Rev D.
Il musicista batte il tempo col piede su un sensore; il sistema esegue pattern percussivi in sync.

## Stato
Firmware skeleton completo (8 moduli). In attesa di hardware per test (Fase 1 del piano).

## Hardware
- Teensy 4.1 + Audio Shield Rev D (SGTL5000)
- SSD1306 128x64 SPI (su SPI1)
- W25Q128 16MB flash SPI (su SPI1, CS separato)
- 2 encoder rotativi con pulsante
- Sensore piede: electret mic su Audio Shield (poi piezo)
- LED RGB (PWM pin 33/36/37)

## Formato pattern
File `.sdp` testo ASCII in `stompdrums/patterns/`. 4 pattern di esempio inclusi.

## Piano implementazione
Dettagliato in `docs/piano.md`. 8 fasi, dalla Fase 1 (playback base) alla Fase 8 (flash caching).

## Git
Repo locale: `/Users/robertocarluccio/Claude_local/stompdrums/`
GitHub: `RobiOneKeno6i/stompdrums`
L'utente lavora su due computer sincronizzati via GitHub.
