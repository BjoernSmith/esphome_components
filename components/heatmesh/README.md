# HeatMesh ESPHome External Component

**Implementationsversion:** 0.2.0 (firmware-log og `manifest.json`)
**Status:** v0.2 under udvikling. Tre zoner kører på A16. Boost min. ON-hold og minimum OFF-tid er host-testet, men endnu ikke afprøvet på A16; plausibilitetskontrol samt genstarts- og HA-frakoblingstests mangler.

HeatMesh udfører lokal PI-regulering per zone. BLE-udlæsning leveres af en ESPHome `sensor` (referenceprojektet bruger `bthome_mithermometer`). Home Assistant climate- og diagnostik-entiteter er valgfrie; kontrolsløjfen afhænger ikke af HA/API.

## Minimal zonekonfiguration

```yaml
heatmesh:
  id: heatmesh_controller
  zones:
    - zone_id: living_room
      temperature_sensor: room_temperature
      valve: room_valve
      target_temperature: 21.0
      sensor_timeout: 15min
      kp: 100
      ki: 0.028
      period_ms: 30000
      pwm_period_min: 15
      min_on_time: 5min
      min_off_time: 0s
      boost_time: 30min
      climate:
        name: "Living Room"
      heat_demand:
        name: "Living Room Heat Demand"
      sensor_age:
        name: "Living Room Sensor Age"
      sensor_valid:
        name: "Living Room Sensor Valid"
      valve_state:
        name: "Living Room Valve State"
      valve_switch_count:
        name: "Living Room Valve Switches"
```

Påkrævet per zone: `zone_id`, `temperature_sensor`, `valve`. Kontrolparametre har defaults; climate- og diagnoseentities er valgfrie. Almindelige ESPHome entity-indstillinger (`name`, `id`, osv.) accepteres.

## Parametre

| Parameter | Default | Grænse / betydning |
|---|---:|---|
| `target_temperature` | 21 °C | Fallback ved første climate-opstart; 5–30 °C |
| `sensor_timeout` | 15 min | Positiv varighed, maks. 2,147,483,647 ms |
| `kp` | 100 | 0.1–1000, procentpoint/K (100 = 1 K under setpoint giver 100 %) |
| `ki` | 0.028 | 0–1, procentpoint/(K·s) |
| `period_ms` | 30,000 | 1,000–120,000 ms |
| `pwm_period_min` | 15 | 5–60 min |
| `min_on_time` | 5 min | Positiv; skal være kortere end PWM-perioden |
| `min_off_time` | 0 s | `0s` deaktiverer; højst 24 timer |
| `boost_time` | 30 min | `0s` slår boost fra; ellers mindst `min_on_time` |

Default og reference-YAML: `kp: 100`, `ki: 0.028`, dvs. fuld demand ved 1 K under setpoint og en integraltid `Kp/Ki` på ca. 60 min. Det er startværdier, ikke endeligt indreguleret på gulvet.

## Regulering og failsafe

- PI: `error = target - temperature`; output begrænses til 0–100 % med anti-windup. Integratoren vokser ikke, mens P alene giver ≥100 % (≥1 K under setpoint med defaults).
- Positive demand konverteres til én ON-puls per PWM-periode; under 1 K fejl giver P delvis demand, og I bygger op, så små fejl giver en puls inden for ca. en time.
- Beregnede pulser kortere end `min_on_time` akkumuleres (carry-over) til senere cyklusser.
- Efter et udgangsskift til OFF forsinker `min_off_time` næste normale PWM-start. En forsinket cyklus begynder først, når tiden er udløbet; tabt varmetid under ventetiden akkumuleres ikke. PI fortsætter med at regulere under ventetiden.
- Boost overstyrer `min_off_time` og kan tænde straks. Efter genstart huskes tidligere OFF-tid ikke, så normal regulering kan starte ved første friske sensormåling. OFF-mode og sensorfejl slukker altid straks; efterfølgende normal regulering respekterer OFF-tiden.
- **Boost:** Hæves setpoint over rumtemperaturen, eller skiftes OFF → HEAT under setpoint, tændes udgangen straks i op til `boost_time`. I er frosset under boost; derefter starter PI en ny PWM-cyklus. Boost stopper tidligt, når setpoint nås, men først når udgangen har været ON i mindst `min_on_time`. Nås setpoint, før udgangen er tændt, annulleres boost uden puls.
- Sænkes setpoint til ≤ rumtemperaturen, nulstilles I og carry-over, og udgangen slukker. Sænkes det, men stadig over rumtemperaturen, fortsætter PI med bevaret I.
- Climate OFF, ikke-finite måling eller sensor-timeout tvinger udgangen OFF straks, afbryder boost og nulstiller integrator/carry-over.
- Sensoren skal publicere en ny måling efter opstart; en cached state alene tæller ikke som frisk.
- Failsafe er fast output OFF, ikke en konfigurerbar HEAT/OFF-indstilling.

## Climate

HEAT og OFF er de understøttede modes. Entiteten viser aktuel temperatur, setpoint og action `heating` mens den elektriske udgang er ON, ellers `idle` (eller `off` i OFF-mode). ESPHome climate restore-state gemmer mode/setpoint lokalt. Uden gemt climate-state starter en ny climate-entitet i HEAT med `target_temperature` fra YAML. Eksisterende værdi fra en tidligere template-number migreres ikke.

## Diagnostik

| Nøgle | Type | Opdatering |
|---|---|---|
| `heat_demand` | Sensor, % | 10 s |
| `sensor_age` | Sensor, s | 10 s; `NaN` indtil første måling |
| `sensor_valid` | Binary sensor | Ved ændring; initielt OFF |
| `valve_state` | Binary sensor | Ved outputskift |
| `valve_switch_count` | Sensor | Ved outputskift; siden boot |

`valve_state` angiver switch-kommandoen, ikke målt mekanisk ventilstilling. Switch-count nulstilles ved reboot.

## Build og fysisk test

Referencekonfiguration: [a16_heatmesh.yaml](../FloorheatCtrl_A16/a16_heatmesh.yaml). Den er kompileret med ESPHome 2026.10.2 og kører med tre zoner på A16. Ny funktionalitet, herunder boost min. ON-hold og minimum OFF-tid, skal stadig testes på hardware. Se [A16-verifikationslisten](../FloorheatCtrl_A16/VERIFICATION_CHECKLIST.md).

Host-testen `tests/heatmesh/test_min_off_time.cpp` kører den rigtige `zone_controller.cpp` mod stub-headers og dækker minimum OFF-tid, boost (inkl. min. ON-hold) og failsafe. Eksempel med GCC fra repository-roden:

```sh
g++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_min_off_time.cpp components/heatmesh/zone_controller.cpp -o test_min_off_time
./test_min_off_time
```

På Windows uden GCC kan Zig-compileren fra pip bruges med de samme argumenter:

```powershell
python -m pip install ziglang
python -m ziglang c++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_min_off_time.cpp components/heatmesh/zone_controller.cpp -o test_min_off_time.exe
.\test_min_off_time.exe
```

## Begrænsninger

PI-indregulering, plausibilitetsfiltrering, RS485, pumpekald og HMI er ikke afsluttet. Lokale zoner bruger ESPHome-sensorer direkte; ESPHome 2026.10.2 afviser gentaget BTHome-pakke-ID, når pakken indeholder det. HeatMesh accepterer derimod alle endelige temperaturværdier, også urealistiske; plausibilitetsgrænser og fejladfærd mangler at blive valgt. Den ubrugte `SensorManager` håndterer hverken valg af lokale/fjerne målinger eller fuld dublethåndtering. Sådan en registry bliver relevant for de delte sensorer i v0.3. De større projektkrav findes i [PROJECT_SPEC.md](../FloorheatCtrl_A16/PROJECT_SPEC.md).
