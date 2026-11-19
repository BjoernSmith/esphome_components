# HeatMesh — implementeringsplan og fremdrift

**Opdateret:** 2026-11-16
**Version:** HeatMesh v0.2.0 (firmware-log og `manifest.json`)
**Nuværende mål:** Færdiggør v0.2 — flere autonome gulvvarmezoner på første KC868-A16.
**Status:** Tre zoner kører på A16 med separat fejltilstand. Plausibilitetskontrol og robusthedstests (genstart/watchdog, sensortab, HA-frakobling) mangler, før v0.2 kan godkendes.

## Hvad der findes nu

- ESPHome External Component i `components/heatmesh/`; YAML-interfacet er dokumenteret i [../heatmesh/README.md](../heatmesh/README.md).
- Reference-YAML [a16_heatmesh.yaml](a16_heatmesh.yaml): tre zoner med separate BTHome v2-sensorer og udgange Y01–Y03.
- PI-regulator med anti-windup, tidsproportional udgang, min. ON-tid med carry-over, valgfri min. OFF-tid, boost og temperatur-freshness timeout.
- Boost holder udgangen ON i mindst `min_on_time`, når den først er tændt; nås setpoint, før udgangen er tændt, annulleres boost uden puls.
- Efter reboot forskydes første ventil-ON mindst 5 min mellem zoner; første PWM-cyklus er forskudt efter zoneorden. Ingen YAML-option; hardwaretest mangler.
- Fast failsafe: ugyldig eller udløbet måling tvinger zonens udgang OFF. Failsafe-mode kan ikke konfigureres.
- Valgfri native Home Assistant climate- og diagnostik-entiteter per zone. YAML'en kræver ingen lambdas.
- Climate understøtter HEAT/OFF. Setpoint og mode gendannes efter genstart.
- Host-test i `tests/heatmesh/` af den rigtige `zone_controller.cpp`.

## Konfiguration i reference-YAML'en (alle tre zoner)

| Parameter | Aktuel værdi | Bemærkning |
|---|---:|---|
| Sensor timeout | 15 min | Udgang OFF ved stale/ugyldig måling |
| Kp | 100 %/K | 1 K under setpoint = 100 % demand; startværdi, ikke indreguleret |
| Ki | 0.007 %/(K·s) | `Kp/Ki` ≈ 4 timer integraltid; forsøgsindstilling |
| PI-opdatering | 30 s | |
| PWM-periode | 180 min | Første cyklus kan være kortere; PWM reagerer på ændret demand |
| Min. ventil-ON-puls | 30 min | Kortere beregnet tid akkumuleres (demand < 16,7 % giver ingen puls i en normal cyklus); gælder også boost |
| Min. ventil-OFF-tid | 0 s | Deaktiveret; boost og genstart overstyrer en konfigureret OFF-tid |
| Boost | 30 min | Normalt straks ON; første ON efter reboot kan vente på 5-minutters forskydningen |
| Setpoint ved første boot | 20.0 °C | Climate gemmer derefter setpoint lokalt |

Værdierne er startværdier, ikke endelig tuning. PWM-perioden i YAML afviger fra komponentens default på 240 min.

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
- **Responsiv PWM:** ON-pulser kan forkortes ved faldende demand efter min. ON; stigende demand kan fremskynde næste cyklus. Normal periode er 180 min i reference-YAML (default 240 min).
- **Opstart efter reboot:** Første ON per zone forskydes mindst 5 min på samme controller; første PWM-cyklus forskydes efter zoneorden. Efterfølgende starter er ikke begrænset, og forsyningens kapacitet skal stadig verificeres.

## Næste trin — prioriteret

1. **Byg og flash v0.2.0** med boost min. ON-hold og 180 min PWM; bekræft boot-log og test opstart og boost i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
2. **Plausibilitetskontrol:** Beslut grænser (fx min./maks. temperatur, maks. spring mellem målinger) og adfærd ved afvist måling (ignorer vs. failsafe OFF); implementér og host-test.
3. **Robusthedstests:** Genstart/restore, watchdog/reboot, sensortab/recovery for én zone, og drift uden Home Assistant.
4. **Min. OFF-tid:** Test en ikke-nul værdi på A16, eller beslut at den forbliver slået fra.
5. **Hardwaredata:** Mål telestatstrøm og åbne-/lukketid; notér board revision og Y01–Y03-mapping.
6. **Regulering:** Log demand, temperatur, ON-tid og switch-count over flere dage. Vurder respons og overshoot med responsiv PWM og `ki: 0.007`; finjustering er planlagt til v0.4.

## Åbne punkter / ikke del af v0.2

- BLE/Wi-Fi coexistence over længere tid.
- Langtidsdrift og komfortmæssig PI-tuning (v0.4).
- Ingen migrering af setpoint fra den gamle template-number-entitet; første climate-start bruger YAML-værdien 20 °C.
- Switch-count persisteres ikke over genstart.
- v0.3+: anden A16, RS485/packet transport, delt sensor registry/dublethåndtering, pumpelogik og HMI.

## Idéer til senere udvikling — ikke planlagt

Noteret til senere; ændrer ikke næste trin, milepæle eller transportbeslutninger.

- **MAC-nøglet sensorpulje mellem noderne.** Hver node sender de BTHome-målinger, den selv hører (MAC, værdi, alder, origin-node), og zoner peger kun på en MAC. Det fjerner `provider`/`remote_id`-par per fjernsensor. Modtagne målinger videresendes aldrig, og alderen følger med, så en gammel måling ikke kan fremstå som ny. Detaljer, pseudo-YAML, framestørrelse og estimat: [PROJECT_SPEC.md](PROJECT_SPEC.md) §4, *Idé til senere udvikling: MAC-nøglet sensorpulje*.
- **Ønsket master/slave-indstilling og 30 s-cyklus.** Én node konfigureres som master, den anden som slave. Master sender sine egne lokale sensordata hver 30 s og frigiver bussen; slave svarer derefter med sine egne lokale sensordata, aldrig uopfordret. Udvekslingen fortsætter også uden nye/lokale målinger. Datasættet synkroniseres, men gamle BLE-målinger bliver ikke friske igen: alder/gyldighed følger med, og eksisterende timeout/failsafe gælder. Afgrænset, ikke-blokerende svar-timeout og protokoldetaljer skal afklares senere. Se [PROJECT_SPEC.md](PROJECT_SPEC.md) §4, *Til senere udvikling: konfigurerbar master/slave med 30 s udveksling*. Kun udviklingsnote; ingen ændring af fysisk bus, nuværende firmware eller milepæle.
- **Observation (ESPHome 2026.10.2):** `packet_transport` gensender afsenderens seneste værdi hver `update_interval` (default 15 s), også når kilde-sensoren er stale, og sender ingen alder. Relevant for v0.3-testen af officiel `packet_transport` (PROJECT_SPEC §4, verifikationspunkt 3).
- **Senere mulighed:** HeatMesh dekoder selv BTHome v2, så kun MAC konfigureres. Afviger fra PROJECT_SPEC §7's anbefaling om `bthome_mithermometer`; kun en idé.

[PROJECT_SPEC.md](PROJECT_SPEC.md) er fortsat kravgrundlaget. Afkrydsningerne i dets roadmap opdateres ikke; status føres i dette dokument.
