# HeatMesh — implementeringsplan og fremdrift

**Opdateret:** 2026-11-15
**Version:** HeatMesh v0.2.0 (firmware-log og `manifest.json`)
**Nuværende mål:** Færdiggør v0.2 — flere autonome gulvvarmezoner på første KC868-A16.
**Status:** Tre zoner kører på A16 med separat fejltilstand. Plausibilitetskontrol og robusthedstests (genstart/watchdog, sensortab, HA-frakobling) mangler, før v0.2 kan godkendes.

## Hvad der findes nu

- ESPHome External Component i `components/heatmesh/`; YAML-interfacet er dokumenteret i [../heatmesh/README.md](../heatmesh/README.md).
- Reference-YAML [a16_heatmesh.yaml](a16_heatmesh.yaml): tre zoner med separate BTHome v2-sensorer og udgange Y01–Y03.
- PI-regulator med anti-windup, tidsproportional udgang, min. ON-tid med carry-over, valgfri min. OFF-tid, boost og temperatur-freshness timeout.
- Boost holder udgangen ON i mindst `min_on_time`, når den først er tændt; nås setpoint, før udgangen er tændt, annulleres boost uden puls.
- Fast failsafe: ugyldig eller udløbet måling tvinger zonens udgang OFF. Failsafe-mode kan ikke konfigureres.
- Valgfri native Home Assistant climate- og diagnostik-entiteter per zone. YAML'en kræver ingen lambdas.
- Climate understøtter HEAT/OFF. Setpoint og mode gendannes efter genstart.
- Host-test i `tests/heatmesh/` af den rigtige `zone_controller.cpp`.

## Konfiguration i reference-YAML'en (alle tre zoner)

| Parameter | Aktuel værdi | Bemærkning |
|---|---:|---|
| Sensor timeout | 15 min | Udgang OFF ved stale/ugyldig måling |
| Kp | 100 %/K | 1 K under setpoint = 100 % demand; startværdi, ikke indreguleret |
| Ki | 0.028 %/(K·s) | `Kp/Ki` ≈ 60 min integraltid |
| PI-opdatering | 30 s | |
| PWM-periode | 15 min | |
| Min. ventil-ON-puls | 5 min | Kortere beregnet tid akkumuleres; gælder også boost |
| Min. ventil-OFF-tid | 0 s | Deaktiveret; boost og genstart overstyrer en konfigureret OFF-tid |
| Boost | 30 min | Straks ON ved hævet setpoint over rumtemperatur eller OFF → HEAT under setpoint |
| Setpoint ved første boot | 21.0 °C | Climate gemmer derefter setpoint lokalt |

Alle værdier er komponentens defaults og kunne udelades i YAML; de står der for at være synlige under test. Værdierne er startværdier, ikke endelig tuning.

## v0.2-krav fra PROJECT_SPEC — status

| Krav (PROJECT_SPEC §8, v0.2) | Status |
|---|---|
| Fælles C++ External Component; dokumenteret YAML-interface | ✅ Færdig |
| Flere rum, setpunkter og outputtilknytning uden kopieret reguleringskode | ✅ Tre zoner kører på A16 |
| HA climate-/diagnostik-entiteter og persistent konfiguration | ✅ Mode/setpoint gemmes; tuningparametre kun via YAML (bevidst) |
| Stale timeout | ✅ Implementeret og i drift |
| Plausibilitetscheck | ❌ Ikke implementeret; grænser og fejladfærd skal besluttes |
| Sensor registry og dublethåndtering | ⏭️ Flyttet til v0.3 (se beslutninger) |
| Minimum ON/OFF og ventilswitch-tællere dokumenteret og testet | 🟡 Dokumenteret og host-testet; min. OFF og boost-hold ikke testet på A16; tæller nulstilles ved boot |
| Test af opstart, watchdog/reboot, tab af sensor og tab af HA | ❌ Ikke testet |
| **DoD:** nødvendige zoner kører samtidig uden HA med separat fejltilstand | 🟡 Separat fejltilstand bekræftet; drift uden HA ikke testet |

## Beslutninger

- **Sensor registry/dublethåndtering → v0.3.** Lokale zoner bruger hver sin ESPHome-sensor direkte, og ESPHome 2026.10.2 afviser allerede gentagne BTHome-pakke-ID'er. Valg mellem lokale og fjerne målinger opstår først med RS485-transporten i v0.3. Den ubrugte `SensorManager` er bevaret til det arbejde.
- **Boost holder min. ON-tid.** En boost-puls kortere end `min_on_time` gav kun relæslid og switch-count uden effekt på en termisk telestat; den holdes derfor. Sænket setpoint, OFF og sensorfejl stopper stadig straks.
- **Climate implementeret før v0.2.** Oprindeligt planlagt til v0.2, men implementeret under v0.1-forløbet for at fjerne YAML-lambdas.
- **Tuning kun i YAML.** Kp/Ki/PWM/min. ON/OFF/boost ændres via YAML og ny build, ikke fra HA.

## Næste trin — prioriteret

1. **Byg og flash v0.2.0** med boost min. ON-hold; bekræft boot-log og test boost-scenarierne i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
2. **Plausibilitetskontrol:** Beslut grænser (fx min./maks. temperatur, maks. spring mellem målinger) og adfærd ved afvist måling (ignorer vs. failsafe OFF); implementér og host-test.
3. **Robusthedstests:** Genstart/restore, watchdog/reboot, sensortab/recovery for én zone, og drift uden Home Assistant.
4. **Min. OFF-tid:** Test en ikke-nul værdi på A16, eller beslut at den forbliver slået fra.
5. **Hardwaredata:** Mål telestatstrøm og åbne-/lukketid; notér board revision og Y01–Y03-mapping.
6. **Regulering:** Log demand, temperatur og switch-count over tid; finjustering er planlagt til v0.4.

## Åbne punkter / ikke del af v0.2

- BLE/Wi-Fi coexistence over længere tid.
- Langtidsdrift og komfortmæssig PI-tuning (v0.4).
- Ingen migrering af setpoint fra den gamle template-number-entitet; første climate-start bruger YAML-værdien 21 °C.
- Switch-count persisteres ikke over genstart.
- v0.3+: anden A16, RS485/packet transport, delt sensor registry/dublethåndtering, pumpelogik og HMI.

[PROJECT_SPEC.md](PROJECT_SPEC.md) er fortsat kravgrundlaget. Afkrydsningerne i dets roadmap opdateres ikke; status føres i dette dokument.
