# HeatMesh v0.1.2 — teknisk status

**Dato:** 2026-11-15
**Status:** Én-zones software implementeret; ESP32-build og host-simulering bestået; hardwarevalidering af v0.1.2 mangler.

## Arkitektur

```text
BTHome v2 BLE
    ↓
ESPHome bthome_mithermometer Sensor
    ↓ callback på ny måling
ZoneController
    ├─ freshness/timeout og failsafe OFF
    ├─ PI + anti-windup → heat demand 0–100 %
    ├─ 15 min tidsproportional udgang; korte pulser carry-over
    └─ ESPHome valve switch

Valgfri ZoneClimate + native diagnostic entities
    └─ HA-UI/overvågning; ikke afhængighed for lokal regulering
```

`HeatmeshComponent` ejer zone-livscyklussen. Zone-reguleringen er i `ZoneController`; `ZoneClimate` videresender mode/setpoint og eksponerer standard climate-state. BLE-protokollen håndteres fortsat af ESPHome-sensorplatformen.

## Regulering og sikker adfærd

- `error = target - measured`; `demand = clamp(Kp × error + integrator, 0, 100%)`.
- Integrator opdateres i sekunder og har anti-windup.
- Reference-YAML: `Kp=100 %/K`, `Ki=0.028 %/(K·s)` (Ti ≈ 60 min), loop 30 s, PWM 15 min, min. ON 5 min, boost 30 min, timeout 15 min.
- Setpoint hævet over rumtemperaturen (eller OFF → HEAT under setpoint): udgangen ON straks i op til `boost_time` med frosset I; derefter overtager PI med en ny PWM-cyklus. Setpoint sænket til ≤ rumtemperatur: I og carry-over nulstilles, udgang OFF. Sænket, men stadig over rumtemperatur: PI fortsætter med bevaret I.
- OFF-mode, ugyldig måling eller timeout tvinger udgangen OFF med det samme og rydder integrator og carry-over.
- Sensor-age måles fra reelle state-callbacks. En præ-cached værdi ved boot regnes ikke som ny frisk måling.
- Failsafe er fast OFF; der findes ikke længere et konfigurerbart HEAT/OFF failsafe-valg.

## YAML-interface

Obligatorisk per zone: `zone_id`, `temperature_sensor`, `valve`. Kontrolparametre er valgfrie. `climate`, `heat_demand`, `sensor_age`, `sensor_valid`, `valve_state` og `valve_switch_count` er valgfrie entity-konfigurationer direkte under zonen. Standard ESPHome entity-indstillinger som `name` og `id` bruges.

Climate-entiteten tilbyder HEAT/OFF, temperatur, setpoint (5–30 °C med 0,5 °C step) og action. Setpoint/mode gemmes af ESPHome climate restore-state. YAML `target_temperature` er fallback ved første start. Fysisk mode/setpoint-migrering fra den tidligere YAML-number findes ikke.

Diagnostik:

| YAML-nøgle | Entity | Opdatering |
|---|---|---|
| `heat_demand` | Sensor i % | Hvert 10. sekund |
| `sensor_age` | Sensor i sekunder | Hvert 10. sekund; `NaN` før første gyldige måling |
| `sensor_valid` | Binary sensor | Ved ændring; OFF indtil frisk måling |
| `valve_state` | Binary sensor | Ved elektrisk switch-kommandoens ændring |
| `valve_switch_count` | Sensor | Ved outputskift; tæller siden boot |

Valve state er den elektriske kommando, ikke feedback om den mekaniske ventils position. Switch-count nulstilles ved reboot.

## Verifikation

- ESPHome 2026.10.2 konfigurationsgenerering og ESP32 compile gennemført mod lokal checkout af komponenten; ingen compile-fejl/-advarsler.
- Host-platform-build gennemført.
- Host-simulering verificerede output ON/OFF, HEAT/OFF, timeout/recovery og gendannelse af mode/setpoint efter genstart.
- Tidligere fysisk firmware havde fungerende BLE-målinger og setpointstyret output ifølge indsendte logs. Ny climate-implementering er ikke dermed fysisk godkendt.

Se [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) for afgrænsning af tests og resterende hardwarearbejde.

## Uafsluttet scope

Fysisk validering, telestatkarakterisering, PI-tuning, langtidsdrift, HA-afbrydelse på ny firmware, flere zoner i drift, RS485, pumpekald og HMI er ikke afsluttet. Projektets krav og den større roadmap findes i [PROJECT_SPEC.md](PROJECT_SPEC.md).
