# Verifikation — HeatMesh v0.1.2

**Opdateret:** 2026-11-15
Markér kun hardwarepunkter efter fysisk test. Build- og host-testresultater er softwareverifikation, ikke dokumentation for korrekt tilsluttet A16/telestat.

## Software verificeret

- [x] Python schema/codegen og ESPHome config genererer for reference-YAML'en.
- [x] ESP32 compile bestået med ESPHome 2026.10.2 og lokal component checkout.
- [x] Host-platform build bestået.
- [x] Host-test: zone starter HEAT med YAML-setpoint, og setpoint/mode gendannes efter genstart.
- [x] Host-test: sensorværdi driver PI-output, output kan tænde/slukke, og climate action følger output.
- [x] Host-test: timeout stopper output; frisk måling gendanner kontrol.
- [x] YAML-reference har ingen lambdas eller template climate/number/diagnostic entities.

## Fysisk A16 — stadig åben

- [ ] Push den commit, der indeholder både kode og YAML, til remote `main`, før HA opdaterer external component.
- [ ] Bekræft fysisk board revision og forsynings-/I/O-mapping.
- [ ] Test Y01 med ufarlig testlast før telestat. Reference antager PCF8574-adresse `0x24`, kanal 0 og inverteret output.
- [ ] Flash v0.1.2 og bekræft ESPHome 2026.10.2 boot/setup logs.
- [ ] Bekræft BTHome-sensoren opdaterer `Current Temperature`; kontrollér `Sensor Age` og `Sensor Valid`.
- [ ] Test HEAT med testlast og temperatur under setpoint; bekræft demand/output/climate action.
- [ ] Skift til OFF; bekræft output OFF med det samme.
- [ ] Genstart; bekræft climate mode og setpoint gendannes. Hvis ingen climate-state er gemt, forvent YAML fallback på 21 °C og HEAT.
- [ ] Test timeout ved at standse BLE-sensoropdateringer; vent 15 min, bekræft sensor invalid og output OFF.
- [ ] Genoptag sensor; bekræft sensor valid og kontrol genoptages.
- [ ] Test HA/API-frakobling uden at fjerne enhedens nødvendige netværksforbindelse; lokal kontrol skal fortsætte.
- [ ] Først efter testlast: tilslut telestat og mål startstrøm, holdestrøm, åbn-/lukketid og passende minimumspuls.
- [ ] Observer stabil drift i mindst 24 timer; noter temperatur, demand, switch-count og eventuelle genstarter.

## Data der skal noteres under hardwaretest

| Felt | Måling |
|---|---|
| A16 board/PCB revision | |
| ESPHome version og firmware commit | |
| Sensor MAC / kryptering | |
| Y01 output/polaritet bekræftet | |
| Testlast / telestat type og spænding | |
| Målt åbne-/lukketid | |
| Timeout observeret efter | |
| HA-frakoblet test varighed | |
| 24-timers drift / genstarter | |
| Ændrede PI/PWM-parametre og begrundelse | |

**Sikkerhed:** YAML'en skjuler ventilswitchen i HA for at undgå konkurrerende styring. Brug ikke en Home Assistant-switchservice som manuel Y01-test; benyt en testkonfiguration og ufarlig testlast.
