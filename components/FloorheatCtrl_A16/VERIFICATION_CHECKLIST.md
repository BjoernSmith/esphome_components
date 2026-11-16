# Verifikation — HeatMesh v0.2.0

**Opdateret:** 2026-11-16
Markér kun A16-punkter efter fysisk test. Host-testresultater er softwareverifikation, ikke dokumentation for korrekt tilsluttet A16/telestat.

## Bekræftet på A16 (oplyst af brugeren)

- [x] Firmware med tre zoner (Y01–Y03) er bygget med ESPHome 2026.10.2 og kører på A16.
- [x] BLE-målinger modtages, og climate-setpoint styrer zonernes udgange.
- [x] En zonefejl er isoleret fra de øvrige zoner.
- [x] Hardwareopsætningen er accepteret.

Måledata og testdetaljer er ikke registreret; udfyld tabellen nederst ved næste test.

## Software verificeret

- [x] Python schema/codegen og ESPHome-konfiguration accepteres for reference-YAML'en.
- [x] Schema-defaults: en zone uden `pwm_period_min`/`min_on_time` accepteres med 240 min/30 min (`esphome config`, ESPHome 2026.10.2).
- [x] Host-test `tests/heatmesh/test_min_off_time.cpp` består: min. OFF-tid (inkl. `millis()`-rollover), boost og failsafe, boost min. ON-hold, boost annulleret før ON, 100 % demand og genstart.
- [x] Tidligere host-simulering (v0.1.2): HEAT/OFF, setpoint/mode-restore, timeout og recovery.
- [x] Reference-YAML har ingen lambdas eller template climate/number/diagnostic entities.

## Åbne tests for v0.2

Push commit med kode og YAML til `main`, før HA bygger ny firmware.

- [ ] ESP32-build og A16-boot med v0.2.0 (`HeatMesh v0.2.0: setup starting; 3 zones configured`).
- [ ] Boost: hævet setpoint og OFF → HEAT tænder straks; ophør efter boost-tid.
- [ ] Boost min. ON-hold: når setpoint nås kort efter boost-start, forbliver udgangen ON i mindst `min_on_time`.
- [ ] Sænket setpoint til ≤ rumtemperatur under boost: udgangen slukker straks.
- [ ] Konfigureret `min_off_time` (> 0): PWM venter, boost overstyrer, og sensorfejl/OFF stopper straks.
- [ ] Lang PWM-cyklus (240 min, min. ON 30 min): ON-tid = `demand × 240 min` ved cyklusstart og holdes cyklussen ud; demand < 12,5 % giver carry-over. Notér reaktionstid og overshoot.
- [ ] Genstart: mode og setpoint gendannes per zone; uden gemt state forventes HEAT og 21 °C.
- [ ] Watchdog/reboot: enheden kommer op med alle udgange OFF og genoptager regulering efter friske målinger.
- [ ] Sensortab: stop én sensor; efter 15 min er kun den zones `Sensor Valid` og udgang OFF.
- [ ] Sensor-recovery: næste gyldige måling genaktiverer zonen.
- [ ] HA/API-frakobling: lokal regulering fortsætter, og enheden genstarter ikke.
- [ ] Plausibilitetskontrol (når implementeret): urealistiske værdier afvises som defineret.

## Hardwaredata og langtidsdrift

- [ ] Bekræft og notér board revision samt forsynings- og I/O-mapping for Y01–Y03.
- [ ] Mål telestaternes startstrøm, holdestrøm og åbne-/lukketid; vurder `min_on_time`, `min_off_time` og PWM-periode.
- [ ] Observer stabil drift i mindst 24 timer; notér temperatur, demand, switch-count og eventuelle genstarter.

## Data der skal noteres under hardwaretest

| Felt | Måling |
|---|---|
| A16 board/PCB revision | |
| ESPHome version og firmware commit | |
| Sensor-MAC'er / kryptering | |
| Y01–Y03 output/polaritet bekræftet | |
| Telestattype og spænding | |
| Målt start-/holdestrøm | |
| Målt åbne-/lukketid | |
| Timeout observeret efter | |
| HA-frakoblet test varighed | |
| 24-timers drift / genstarter | |
| Ændrede PI/PWM-parametre og begrundelse | |

**Sikkerhed:** YAML'en skjuler ventilswitchene i HA for at undgå konkurrerende styring. Brug ikke en Home Assistant-switchservice som manuel udgangstest; benyt en testkonfiguration og ufarlig testlast.
