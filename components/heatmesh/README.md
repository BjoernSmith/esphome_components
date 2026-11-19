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
      pwm_period_min: 240
      min_on_time: 30min
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
| `target_temperature` | 20 °C | Fallback ved første climate-opstart; 5–30 °C |
| `sensor_timeout` | 15 min | Positiv varighed, maks. 2,147,483,647 ms |
| `kp` | 100 | 0.1–1000, procentpoint/K (100 = 1 K under setpoint giver 100 %) |
| `ki` | 0.028 | 0–1, procentpoint/(K·s) |
| `period_ms` | 30,000 | 1,000–120,000 ms |
| `pwm_period_min` | 240 | 30–300 min |
| `min_on_time` | 30 min | Positiv; skal være kortere end PWM-perioden (ved `pwm_period_min: 30` skal den sættes lavere end default) |
| `min_off_time` | 0 s | `0s` deaktiverer; højst 24 timer |
| `boost_time` | 30 min | `0s` slår boost fra; ellers mindst `min_on_time` |

Komponentens defaults er `kp: 100`, `ki: 0.028`, PWM 240 min og min. ON 30 min. Reference-YAML bruger `ki: 0.007` (integraltid ca. 4 timer), PWM 180 min og første setpoint 20 °C. Det er forsøgsindstillinger, ikke endeligt indreguleret på gulvet.

## Regulering og failsafe

- PI: `error = target - temperature`; output begrænses til 0–100 % med anti-windup. Integratoren vokser ikke, mens P alene giver ≥100 % (≥1 K under setpoint med defaults).
- Ved cyklusstart beregnes en ON-puls ud fra demand og eventuel carry. Falder demand, kan pulsen forkortes, dog tidligst efter `min_on_time` regnet fra faktisk ON. Demand 0 %, OFF og sensorfejl slukker straks. Stigende demand kan ikke forlænge pulsen ud over den oprindeligt beregnede længde.
- Efter reboot er hver zones første PWM-cyklus automatisk forskudt efter zoneordenen. Ved tre zoner med 180 min periode er de første planlagte cyklusser 180, 120 og 60 min; senere cyklusser bruger 180 min. En cyklus kan afbrydes tidligt; en planlagt første cyklus er aldrig kortere end `min_on_time`, og en for kort puls akkumuleres ud fra den faktiske første cykluslængde.
- De første faktiske ventil-ON-kommandoer efter reboot er mindst 5 min fra hinanden per controller, også ved boost. Kun zoner, der faktisk skal tænde, optager en plads. En ventende normal PWM-cyklus begynder først, når den kan tænde; OFF, ugyldig måling og timeout slukker stadig straks.
- Forsinkelsen gælder kun hver zones første ON efter reboot, ikke senere ON-skift. Den begrænser ikke antallet af åbne ventiler og koordinerer ikke flere controllere. Forsyningen skal stadig dimensioneres efter de faktiske start- og holdestrømme; 5 min er ikke en verificeret strømgrænse.
- Stiger demand, kan en ny cyklus begynde tidligt, når en puls mindst svarende til `min_on_time` kan dannes, og udgangen har været OFF i mindst `(1 − demand/100) × PWM-perioden` og `min_off_time`. Ved 100 % demand kan en ny cyklus fortsætte uden OFF-blink, hvis `min_off_time` er 0. En cyklus uden puls kan kun starte tidligt, når demand stiger; dens carry begrænses til den forløbne del.
- Beregnede pulser kortere end `min_on_time` akkumuleres (carry-over) til senere cyklusser. Med defaults giver demand under 12,5 % (30/240 min) ingen puls i den aktuelle cyklus.
- Reguleringen reagerer ikke direkte på temperaturens faldhastighed, og gulvets lagrede varme kan stadig give overshoot. Observer respons, temperatur og ventilskift på hardware.
- Efter et udgangsskift til OFF forsinker `min_off_time` næste normale PWM-start. En forsinket cyklus begynder først, når tiden er udløbet; tabt varmetid under ventetiden akkumuleres ikke. PI fortsætter med at regulere under ventetiden.
- Boost overstyrer `min_off_time` og kan normalt tænde straks, men respekterer 5-minutters forskydningen ved zonens første ON efter reboot. Tidligere OFF-tid huskes ikke ved reboot. OFF-mode og sensorfejl slukker altid straks; efterfølgende normal regulering respekterer OFF-tiden.
- **Boost:** Hæves setpoint over rumtemperaturen, eller skiftes OFF → HEAT under setpoint, tændes udgangen i op til `boost_time`. Boost-tiden tælles fra faktisk ON, også når første start må vente. I er frosset under boost; derefter starter PI en ny PWM-cyklus. Boost stopper tidligt, når setpoint nås, men først når udgangen har været ON i mindst `min_on_time`. Nås setpoint, før udgangen er tændt, annulleres boost uden puls. Med defaults er `boost_time` og `min_on_time` begge 30 min, så en tændt boost kører i praksis hele boost-tiden.
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

Host-testene kører den rigtige regulering mod stub-headers. `test_min_off_time.cpp` dækker minimum OFF-tid, boost og failsafe; `test_startup_staggering.cpp` dækker flere zoner, første cyklus, boost under ventetid og failsafe. Eksempel med GCC fra repository-roden:

```sh
g++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_min_off_time.cpp components/heatmesh/zone_controller.cpp -o test_min_off_time
./test_min_off_time
g++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_startup_staggering.cpp components/heatmesh/zone_controller.cpp components/heatmesh/heatmesh.cpp -o test_startup_staggering
./test_startup_staggering
```

På Windows uden GCC kan Zig-compileren fra pip bruges med de samme argumenter:

```powershell
python -m pip install ziglang
python -m ziglang c++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_min_off_time.cpp components/heatmesh/zone_controller.cpp -o test_min_off_time.exe
.\test_min_off_time.exe
python -m ziglang c++ -std=c++17 -Itests/heatmesh/stubs tests/heatmesh/test_startup_staggering.cpp components/heatmesh/zone_controller.cpp components/heatmesh/heatmesh.cpp -o test_startup_staggering.exe
.\test_startup_staggering.exe
```

## Begrænsninger

PI-indregulering, plausibilitetsfiltrering, RS485, pumpekald og HMI er ikke afsluttet. Lokale zoner bruger ESPHome-sensorer direkte; ESPHome 2026.10.2 afviser gentaget BTHome-pakke-ID, når pakken indeholder det. HeatMesh accepterer derimod alle endelige temperaturværdier, også urealistiske; plausibilitetsgrænser og fejladfærd mangler at blive valgt. Den ubrugte `SensorManager` håndterer hverken valg af lokale/fjerne målinger eller fuld dublethåndtering. Sådan en registry bliver relevant for de delte sensorer i v0.3. De større projektkrav findes i [PROJECT_SPEC.md](../FloorheatCtrl_A16/PROJECT_SPEC.md).
