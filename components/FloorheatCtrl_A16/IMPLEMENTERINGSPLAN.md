# HeatMesh — implementeringsplan og fremdrift

**Opdateret:** 2026-11-15
**Nuværende mål:** Færdiggør v0.2 med flere autonome gulvvarmezoner på KC868-A16.
**Status:** Tre zoner er afprøvet på A16. Brugeren har bekræftet isoleret zonefejl og hardwareaccept; genstart/watchdog og HA-frakobling afventer.

## Hvad der findes nu

- ESPHome External Component i `components/heatmesh/`.
- Tre zonekonfigurationer med separate sensorer og udgange i reference-YAML'en.
- PI-regulator med anti-windup, tidsproportional udgang, min. ON-tid med carry-over, valgfri min. OFF-tid og temperatur-freshness timeout.
- Fast failsafe: ugyldig eller udløbet måling tvinger udgangen OFF. Failsafe-mode kan ikke konfigureres.
- Valgfri native Home Assistant climate- og diagnostik-entiteter per zone. YAML'en kræver ingen lambdas.
- Climate understøtter HEAT/OFF. Setpoint og mode gendannes efter genstart, når climate-entiteten er konfigureret.
- YAML-reference til KC868-A16, tre BTHome v2-sensorer og Y01–Y03.

## Konfiguration i reference-YAML'en

| Parameter | Aktuel værdi | Bemærkning |
|---|---:|---|
| Sensor timeout | 15 min | Udgang OFF ved stale/ugyldig måling |
| Kp | 100 %/K | 1 K under setpoint = 100 % demand; startværdi, ikke indreguleret |
| Ki | 0.028 %/(K·s) | `Kp/Ki` ≈ 60 min integraltid |
| PI-opdatering | 30 s | |
| PWM-periode | 15 min | |
| Min. ventil-ON-puls | 5 min | Kortere beregnet tid akkumuleres til senere PWM-cyklusser |
| Min. ventil-OFF-tid | 0 s | Deaktiveret; boost og genstart overstyrer en konfigureret OFF-tid |
| Boost | 30 min | Udgang ON straks ved hævet setpoint over rumtemperatur eller OFF → HEAT under setpoint |
| Setpoint ved første boot | 21.0 °C | Climate gemmer efterfølgende sit setpoint lokalt |

Værdierne er en testkonfiguration, ikke dokumentation for endelig tuning. OFF-mode og sensorfejl stopper udgangen straks og nulstiller PI-integrator samt PWM carry-over. Hævet setpoint over rumtemperaturen starter boost; sænket setpoint til ≤ rumtemperaturen nulstiller integrator og carry-over.

## Gennemført og verificeret

- [x] V0.1.2-zonekode, climate-platform og native diagnostik-entiteter implementeret.
- [x] YAML ryddet for alle template-lambdas; ingen separat setpoint-number eller `on_boot`-workaround.
- [x] ESPHome 2026.10.2 accepterer YAML og compilerer ESP32-firmware med den lokale component checkout.
- [x] Host-build compilerer samme komponentkode.
- [x] Host-simulering verificerede HEAT/OFF, setpoint- og mode-restore, output ON/OFF, sensortimeout og recovery.
- [x] Tre zoner, uafhængig zonefejl og hardware er bekræftet af brugeren.
- [ ] Fysisk test af den nye minimum OFF-tid.

## Næste trin — prioriteret

1. **Minimum OFF-tid:** Test en ikke-nul værdi på A16, herunder boost-bypass og straks-OFF ved fejl.
2. **Sensorer:** Afklar ønsket konfigurerbar plausibilitetskontrol. Lokale sensorer håndteres direkte af zonerne; registry og deduplikering af lokale/fjerne målinger er først nødvendigt med v0.3-transporten.
3. **Udskudte tests:** Verificér opstart, gendannelse af mode/setpoint, watchdog/reboot, sensortab/recovery og drift uden Home Assistant, når det er muligt.
4. **Regulering:** Vurder PI/PWM og aktiveringer over tid; finjustering er planlagt til v0.4.

## Åbne punkter / ikke del af denne milepæl

- Dokumenter målte hardwaredata, herunder telestatstrøm og ventiltider.
- BLE/Wi-Fi coexistence og HA-frakobling på den aktuelle firmware.
- Langtidsdrift og komfortmæssig PI-tuning.
- Migrering af setpoint fra den gamle template-number-entitet: der er ingen migrering. Første climate-start bruger YAML-værdien 21 °C, hvis der ikke findes gemt climate-state.
- Plausibilitetskontrol, delt sensor registry/dublethåndtering, RS485, pumpelogik og HMI.

`PROJECT_SPEC.md` er fortsat kravgrundlaget. Dets roadmap-placering af climate under v0.2 beskriver den oprindelige plan; climate er implementeret tidligere, men den samlede v0.1 hardware-accept er stadig åben.
