# HeatMesh v0.1.2 — deployment og sikker test

Denne guide gælder [a16_v01_singlezone.yaml](a16_v01_singlezone.yaml), én BTHome v2-sensor og én KC868-A16-zone. Den seneste climate-/entity-version er softwaretestet, men mangler fysisk A16-validering.

## Før du begynder

- ESPHome 2026.10.2 anbefales; denne version blev brugt til den dokumenterede ESP32-build.
- Konfigurér `wifi_ssid`, `wifi_password` og `floorheatctrl_v01__encryption_key` i Home Assistant `secrets.yaml`.
- Bekræft sensorens MAC og BTHome v2-udsendelse. Eksempel-YAML bruger `A4:C1:38:14:28:EA`.
- Kontrollér komponentens GitHub-version: YAML'en henter `github://BjoernSmith/esphome_components@main`. Commit og push komponentændringer, før du bygger fra HA eller en anden PC.
- Hav ufarlig testlast klar. Tilslut ikke telestaten, før output, polaritet, spænding og strøm er bekræftet.

## Byg og flash

1. Åbn YAML'en i ESPHome og kontrollér secrets, sensor-MAC, zone- og entity-navne.
2. Compile. En vellykket build viser `Successfully compiled program`.
3. Flash/OTA til A16 efter din normale ESPHome-procedure.
4. Kontrollér logs for `HeatMesh v0.1.2`, `Zone ... Initialized` og `Sensor valid; local control enabled`.

## Home Assistant

Når sensoren har leveret en frisk måling, forvent:

- Climate `Test Zone`: HEAT/OFF, aktuel temperatur, setpoint og action (heating/idle).
- `Test Zone Heat Demand` (%), `Test Zone Sensor Age` (s), `Test Zone Sensor Valid`, `Test Zone Valve State`, `Test Zone Valve Switches`.
- Zoneventilens GPIO-switch er `internal`; HeatMesh ejer udgangen. Undlad at oprette en anden kontrolvej til samme ventil.

Ved første climate-start uden gemt climate-state er startmode HEAT og setpoint `target_temperature: 21.0`. Senere gendannes climate-mode og setpoint fra ESPHome restore-state. Tidligere setpoint fra YAML `number` migreres ikke.

## Test i denne rækkefølge

1. **Sensor:** Vent på en ny BLE-måling. `Sensor Valid` skal blive ON; sensor-age skal begynde at vise sekunder.
2. **HEAT:** Med testlast og rumtemperatur under setpoint skal demand være positiv. Udgangen kan stadig være OFF, indtil minimumspuls/carry-over giver en tilstrækkelig PWM-puls.
3. **OFF:** Sæt climate til OFF. Den elektriske udgang skal slukke straks; sensorovervågningen fortsætter.
4. **Restore:** Sæt et test-setpoint og OFF, genstart, og bekræft at begge climate-værdier gendannes.
5. **Timeout:** Stop BLE-opdateringer. Efter `sensor_timeout: 15min` forventes `Sensor Valid` OFF og valve state OFF. Fejl slukker straks; den nye gyldige måling genaktiverer HEAT-regulering, hvis climate er i HEAT.
6. **HA-frakobling:** Afbryd HA/API uden at afbryde controllerens drift; bekræft lokal regulering fortsætter.
7. **Fysisk ventil:** Først når testlast, polaritet og effekt er bekræftet, tilslut telestat. Mål start-/holdestrøm og åbne-/lukketid; brug det til at vurdere `min_on_time`.

YAML antager Y01 via PCF8574 `0x24`, kanal 0, inverteret. Bekræft denne mapping på den faktiske board-revision, før spænding sættes på lasten.

## Hvis noget ikke passer

- Ingen `Sensor Valid`: kontrollér MAC, sensorens BTHome-version, BLE-rækkevidde og logs.
- Ingen output: kontrollér mode (HEAT), sensor-valid, demand, PWM-periode og carry-over. Lav demand kan vente længe på en ≥5 min puls.
- Compilefejl på `heatmesh`: kontrollér at GitHub `main` indeholder den samme version af component som YAML'en forventer.
- Gammel climate-/number-entitet vises stadig i HA: fjern den forældede entitet efter at den nye climate er valideret; slet ikke sensorens eksisterende entity uden at have overvejet historikken.

Log testresultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
