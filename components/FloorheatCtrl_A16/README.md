# FloorHeat Controller A16

Lokal gulvvarmeregulering til KinCony KC868-A16 med BLE BTHome v2 temperatursensor og ESPHome External Component `heatmesh`.

## Aktuel status

- **Reference:** én lokal zone i [a16_v01_singlezone.yaml](a16_v01_singlezone.yaml).
- **Software:** PI-regulering, timeout/failsafe OFF, tidsproportional udgang og native climate-/diagnostik-entiteter.
- **Build:** ESPHome 2026.10.2 ESP32-build og host-simulering er bestået.
- **Hardware:** den nyeste v0.1.2 climate-/diagnostikversion mangler fysisk A16-test. Tidligere firmwarelogs bekræfter BLE-målinger og setpointstyret output, ikke den nye firmware.
- **Ingen commits foretages af Copilot.** Brugeren committer og pusher selv.

## Fortsæt arbejdet

1. Læs [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) for aktuel status og prioriterede næste trin.
2. Brug [QUICKSTART.md](QUICKSTART.md) til at bygge, flashe og teste sikkert.
3. Registrér resultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
4. Se [../heatmesh/README.md](../heatmesh/README.md) for zone-schema og kontroladfærd.
5. Se [PROJECT_SPEC.md](PROJECT_SPEC.md) for krav og fremtidig roadmap; fremdrift står i implementeringsplanen.

## Home Assistant-entiteter

Climate HEAT/OFF og diagnostik (`heat_demand`, `sensor_age`, `sensor_valid`, `valve_state`, `valve_switch_count`) konfigureres direkte under zonen. De er valgfrie og kræver ingen YAML-lambdas. Climate gemmer mode og setpoint; YAML `target_temperature` bruges ved første start, når der ikke findes gemt state.

## Vigtige driftsforhold

- Sensor-timeout og ugyldige målinger tvinger udgangen OFF.
- Failsafe er fast OFF; der er ikke en konfigurerbar HEAT-failsafe.
- Y01-mapping og telestatens elektriske/mekaniske specifikationer skal verificeres før rigtig last tilsluttes.
- YAML henter komponenten fra GitHub `main`; commit/push nødvendige ændringer før deployment fra en anden maskine.
