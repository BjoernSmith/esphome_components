# HeatMesh v0.2.0 — teknisk status

**Dato:** 2026-11-15
**Status:** v0.2 under udvikling. Tre zoner kører på A16; plausibilitetskontrol og robusthedstests (genstart, sensortab, HA-frakobling) mangler.

## Arkitektur

```text
BTHome v2 BLE (én sensor per zone)
    ↓
ESPHome bthome_mithermometer Sensor
    ↓ callback på ny måling
ZoneController (én instans per zone, ingen kopieret reguleringskode)
    ├─ freshness/timeout og failsafe OFF
    ├─ PI + anti-windup → heat demand 0–100 %
    ├─ boost: straks ON ved hævet setpoint, min. ON-hold
    ├─ 15 min tidsproportional udgang; korte pulser carry-over; valgfri min. OFF-tid
    └─ ESPHome valve switch (Y01–Y03)

Valgfri ZoneClimate + native diagnostic entities per zone
    └─ HA-UI/overvågning; ikke afhængighed for lokal regulering
```

`HeatmeshComponent` ejer zonernes livscyklus og kalder hver zones `loop()`. Reguleringen ligger i `ZoneController`; `ZoneClimate` videresender mode/setpoint og eksponerer standard climate-state. BLE-protokollen håndteres af ESPHome-sensorplatformen. Hver zone har egen sensor, udgang og fejltilstand.

## Regulering og sikker adfærd

- `error = target - measured`; `demand = clamp(Kp × error + integrator, 0, 100 %)`.
- Integratoren opdateres i sekunder og har betinget anti-windup: den vokser ikke, mens P alene giver ≥ 100 %.
- Reference-YAML og defaults: `Kp=100 %/K`, `Ki=0.028 %/(K·s)` (Ti ≈ 60 min), loop 30 s, PWM 15 min, min. ON 5 min, min. OFF 0 s, boost 30 min, timeout 15 min.
- Demand omsættes til én ON-puls per PWM-periode. Pulser kortere end `min_on_time` akkumuleres (carry-over) til senere cyklusser.
- `min_off_time` (> 0) forsinker næste normale PWM-start efter en OFF-overgang. Boost og genstart overstyrer ventetiden.
- **Boost:** Setpoint hævet over rumtemperaturen (eller OFF → HEAT under setpoint) tænder udgangen straks i op til `boost_time` med frosset I. Når setpoint nås, slutter boost først, når udgangen har været ON i mindst `min_on_time`; nås setpoint, før udgangen er tændt, annulleres boost uden puls. Derefter starter PI en ny PWM-cyklus.
- Setpoint sænket til ≤ rumtemperatur: boost afbrydes, I og carry-over nulstilles, og udgangen slukker straks. Sænket, men stadig over rumtemperatur: PI fortsætter med bevaret I.
- OFF-mode, ugyldig måling eller timeout tvinger udgangen OFF med det samme, afbryder boost og rydder integrator og carry-over.
- Sensor-age måles fra reelle state-callbacks. En præ-cached værdi ved boot regnes ikke som ny frisk måling.
- Failsafe er fast OFF. Alle endelige temperaturværdier accepteres; plausibilitetsgrænser er ikke implementeret.

## YAML-interface

Obligatorisk per zone: `zone_id`, `temperature_sensor`, `valve`. Kontrolparametre er valgfrie. `climate`, `heat_demand`, `sensor_age`, `sensor_valid`, `valve_state` og `valve_switch_count` er valgfrie entity-konfigurationer direkte under zonen. Schemaet afviser dublerede `zone_id` og samme ventil i flere zoner.

Climate-entiteten tilbyder HEAT/OFF, temperatur, setpoint (5–30 °C med 0,5 °C step) og action. Setpoint/mode gemmes af ESPHome climate restore-state. YAML `target_temperature` er fallback ved første start. Tuningparametre ændres kun via YAML og ny build.

Diagnostik:

| YAML-nøgle | Entity | Opdatering |
|---|---|---|
| `heat_demand` | Sensor i % | Hvert 10. sekund |
| `sensor_age` | Sensor i sekunder | Hvert 10. sekund; `NaN` før første gyldige måling |
| `sensor_valid` | Binary sensor | Ved ændring; OFF indtil frisk måling |
| `valve_state` | Binary sensor | Ved ændring af den elektriske switch-kommando |
| `valve_switch_count` | Sensor | Ved outputskift; tæller siden boot |

Valve state er den elektriske kommando, ikke feedback om ventilens mekaniske position. Switch-count nulstilles ved reboot.

## Verifikation

- Brugeren har bygget firmwaren med ESPHome 2026.10.2 og kører tre zoner på A16; isoleret zonefejl og hardwareopsætning er bekræftet.
- Host-testen `tests/heatmesh/test_min_off_time.cpp` kører den rigtige `zone_controller.cpp` mod stub-headers og dækker min. OFF-tid (inkl. `millis()`-rollover), boost/failsafe, boost min. ON-hold, 100 % demand og genstart.
- Boost min. ON-hold (ændret efter commit `6cfeb94`) er host-testet, men endnu ikke bygget til ESP32 eller testet på A16.

Se [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) for afgrænsning af tests og resterende arbejde.

## Uafsluttet scope

Plausibilitetskontrol, test af genstart/watchdog, sensortab og HA-frakobling, telestatkarakterisering, PI-tuning og langtidsdrift mangler. Sensor registry/dublethåndtering, RS485, pumpekald og HMI hører til senere milepæle. Projektets krav og roadmap findes i [PROJECT_SPEC.md](PROJECT_SPEC.md).
