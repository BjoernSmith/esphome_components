# HeatMesh — implementeringsplan og fremdrift

**Opdateret:** 2026-11-15
**Nuværende mål:** Færdiggør og hardwarevalidér én autonom gulvvarmezone på KC868-A16.
**Status:** Software v0.1.2 implementeret; build og host-simulering bestået; ny firmware endnu ikke valideret på fysisk A16.

## Hvad der findes nu

- ESPHome External Component i `components/heatmesh/`.
- Én eller flere zonekonfigurationer i schemaet; kun én-zone-konfigurationen er build- og simuleringsverificeret.
- PI-regulator med anti-windup, tidsproportional udgang, min. ON-tid med carry-over og temperatur-freshness timeout.
- Fast failsafe: ugyldig eller udløbet måling tvinger udgangen OFF. Failsafe-mode kan ikke konfigureres.
- Valgfri native Home Assistant climate- og diagnostik-entiteter per zone. YAML'en kræver ingen lambdas.
- Climate understøtter HEAT/OFF. Setpoint og mode gendannes efter genstart, når climate-entiteten er konfigureret.
- YAML-reference til KC868-A16, én BTHome v2-sensor og Y01.

## Konfiguration i reference-YAML'en

| Parameter | Aktuel værdi | Bemærkning |
|---|---:|---|
| Sensor timeout | 15 min | Udgang OFF ved stale/ugyldig måling |
| Kp | 1.2 %/K | Bench-værdi; ikke indreguleret på gulvvarme |
| Ki | 0.001 %/(K·s) | Med Kp 1.2 svarer `Kp/Ki` til ca. 20 min |
| PI-opdatering | 30 s | |
| PWM-periode | 15 min | |
| Min. ventil-ON-puls | 5 min | Kortere beregnet tid akkumuleres til senere PWM-cyklusser |
| Setpoint ved første boot | 21.0 °C | Climate gemmer efterfølgende sit setpoint lokalt |

Værdierne er en testkonfiguration, ikke dokumentation for endelig tuning. OFF-mode og sensorfejl stopper udgangen straks og nulstiller PI-integrator samt PWM carry-over. Setpointændring nulstiller ikke integratoren og ændrer ikke den allerede igangværende PWM-cyklus.

## Gennemført og verificeret

- [x] V0.1.2-zonekode, climate-platform og native diagnostik-entiteter implementeret.
- [x] YAML ryddet for alle template-lambdas; ingen separat setpoint-number eller `on_boot`-workaround.
- [x] ESPHome 2026.10.2 accepterer YAML og compilerer ESP32-firmware med den lokale component checkout.
- [x] Host-build compilerer samme komponentkode.
- [x] Host-simulering verificerede HEAT/OFF, setpoint- og mode-restore, output ON/OFF, sensortimeout og recovery.
- [x] Tidligere firmware gav reelle sensor-/reguleringslogs; disse logs er ikke verifikation af den nye climate-firmware.
- [ ] Fysisk A16-test af den nye v0.1.2-firmware.

## Næste trin — prioriteret

1. **Synkronisér repository til den anden PC.** Commit og push ændringer, der skal med; YAML'en peger på GitHub `main`.
2. **Byg og flash på A16.** Brug testlast først. Bekræft, at climate, sensordata og diagnostik vises korrekt i HA.
3. **Test HEAT/OFF.** Bekræft HEAT regulerer, OFF slukker udgangen straks, og mode/setpoint overlever genstart.
4. **Test failsafe sikkert.** Fjern BLE-sensoren eller stop dens udsendelser; efter 15 min skal `Sensor Valid` være OFF og udgangen OFF. Bekræft recovery efter næste gyldige måling.
5. **Bekræft Y01 elektrisk og mekanisk.** YAML antager PCF8574 `0x24`, kanal 0, inverteret GPIO-switch. Bekræft board revision, spænding, polaritet og testlast før tilslutning af telestat.
6. **Kør uden Home Assistant.** Afbryd HA/API efter sikkerhedsforholdene er afklaret; lokal regulering skal fortsætte.
7. **Log driften over tid.** Først efter hardwaretest vurderes Kp/Ki, minimumspuls og PWM-periode; mål telestatens åbne-/lukketid.

## Åbne punkter / ikke del af denne milepæl

- Fysisk validering af hardwaremapping, telestatstrøm og ventiltider.
- BLE/Wi-Fi coexistence og HA-frakobling på den aktuelle firmware.
- Langtidsdrift og komfortmæssig PI-tuning.
- Migrering af setpoint fra den gamle template-number-entitet: der er ingen migrering. Første climate-start bruger YAML-værdien 21 °C, hvis der ikke findes gemt climate-state.
- Flere samtidige zoner på hardware, sensor registry/dublethåndtering, plausibilitetskontrol, RS485, pumpelogik og HMI.

`PROJECT_SPEC.md` er fortsat kravgrundlaget. Dets roadmap-placering af climate under v0.2 beskriver den oprindelige plan; climate er implementeret tidligere, men den samlede v0.1 hardware-accept er stadig åben.
