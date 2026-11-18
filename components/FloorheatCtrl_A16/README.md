# FloorHeat Controller A16

Lokal gulvvarmeregulering til KinCony KC868-A16 med BLE BTHome v2-temperatursensorer og ESPHome External Component `heatmesh`.

## Aktuel status — v0.2.0 under udvikling

- **Reference:** tre lokale zoner — Badeværelse (Y01), Køkken (Y02) og Bryggers (Y03) — i [a16_heatmesh.yaml](a16_heatmesh.yaml).
- **Software:** PI-regulering, boost med min. ON-hold, min. ON/OFF-tid, timeout/failsafe OFF, tidsproportional udgang og native climate-/diagnostik-entiteter.
- **Hardware:** firmwaren kører på A16 med tre zoner. Brugeren har bekræftet isoleret zonefejl og hardwareopsætningen.
- **Mangler til v0.2:** plausibilitetskontrol samt A16-test af genstart/watchdog, sensortab/recovery, drift uden Home Assistant, min. OFF-tid og boost-hold.
- **Ingen commits foretages af Copilot.** Brugeren committer og pusher selv.

## Fortsæt arbejdet

1. Læs [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) for status og prioriterede næste trin.
2. Brug [QUICKSTART.md](QUICKSTART.md) til at bygge, flashe og teste.
3. Registrér resultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
4. Se [../heatmesh/README.md](../heatmesh/README.md) for zone-schema og kontroladfærd.
5. Se [PROJECT_SPEC.md](PROJECT_SPEC.md) for krav og fremtidig roadmap; fremdrift står i implementeringsplanen.

## Home Assistant-entiteter

Per zone konfigureres climate HEAT/OFF og diagnostik (`heat_demand`, `sensor_age`, `sensor_valid`, `valve_state`, `valve_switch_count`) direkte under zonen. De er valgfrie og kræver ingen YAML-lambdas. Climate gemmer mode og setpoint; YAML `target_temperature` bruges ved første start, når der ikke findes gemt state.

Enheden hedder `FloorHeat A16` i Home Assistant (`friendly_name`). `esphome.name: floorheatctrl-v01` er bevidst uændret, fordi det er hostname, OTA-mål og HA-enhedens identitet.

## Vigtige driftsforhold

- Sensor-timeout og ugyldige målinger tvinger zonens udgang OFF; de øvrige zoner fortsætter.
- Failsafe er fast OFF; der er ikke en konfigurerbar HEAT-failsafe.
- Alle endelige temperaturværdier accepteres indtil videre; plausibilitetsgrænser mangler.
- Telestaternes strøm og åbne-/lukketid er ikke målt og dokumenteret; `min_on_time` og PWM er derfor startværdier.
- PWM-perioden er 180 min med min. ON 30 min. ON-tiden låses ved cyklusstart, så et stigende varmebehov kan vente op til 3 timer, og en startet puls stopper først, når demand når 0 %. Boost kan normalt tænde straks.
- Efter reboot forskydes første ON per zone mindst 5 min, og første PWM-cyklus forskydes efter zoneorden. Det begrænser ikke senere samtidige starter eller antal åbne ventiler; forsyningens start- og holdestrømme skal stadig verificeres.
- YAML henter komponenten fra GitHub `main`; commit/push nødvendige ændringer før deployment fra en anden maskine.
