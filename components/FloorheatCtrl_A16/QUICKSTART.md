# HeatMesh v0.2.0 — deployment og test

Denne guide gælder [a16_heatmesh.yaml](a16_heatmesh.yaml): KC868-A16 med tre BTHome v2-sensorer og tre zoner.

| Zone (`zone_id`) | Climate-navn | Udgang | PCF8574 `0x24` |
|---|---|---|---|
| `bathroom_room` | Badeværelse | Y01 | kanal 0, inverteret |
| `kitchen_room` | Køkken | Y02 | kanal 1, inverteret |
| `bryggers_room` | Bryggers | Y03 | kanal 2, inverteret |

## Før du begynder

- Firmwaren er bygget og kørt med ESPHome 2026.10.2.
- Konfigurér `wifi_ssid`, `wifi_password` og `floorheatctrl_v01__encryption_key` i ESPHome `secrets.yaml`. Secret-navnet er bevaret fra v0.1, så den eksisterende API-nøgle fortsat passer.
- Kontrollér sensor-MAC'erne i `substitutions` (`sensor_01_mac_address` … `sensor_03_mac_address`), og at sensorerne udsender BTHome v2.
- YAML'en henter komponenten fra `github://BjoernSmith/esphome_components@main` med `refresh: 0s`. Commit og push komponentændringer, før du bygger fra HA eller en anden PC.
- `esphome.name` er fortsat `floorheatctrl-v01`; det er hostname, OTA-mål og HA-enhed. Ændr det ikke uden at planlægge ny adoption i HA.
- Brug ufarlig testlast, når ny udgangsadfærd testes første gang.

## Byg og flash

1. Åbn YAML'en i ESPHome og kontrollér secrets, sensor-MAC'er samt zone- og entity-navne.
2. Compile; en vellykket build viser `Successfully compiled program`.
3. Flash/OTA til A16.
4. Kontrollér logs for `HeatMesh v0.2.0: setup starting; 3 zones configured`, `[<zone_id>] Initialized` og derefter `[<zone_id>] Sensor valid; local control enabled` for hver zone.

## Home Assistant

Per zone (Badeværelse, Køkken, Bryggers):

- Climate `<zone>`: HEAT/OFF, aktuel temperatur, setpoint og action (heating/idle/off).
- `<zone> Heat Demand` (%), `<zone> Sensor Age` (s), `<zone> Sensor Valid`, `<zone> Valve State` og `<zone> Valve Switches`.
- BLE-sensorernes egne entiteter: `<sensor> Temp`, `<sensor> Humidity` og `<sensor> Battery`.
- Ventilernes GPIO-switches er `internal`; HeatMesh ejer udgangene. Opret ikke en anden kontrolvej til samme ventil.

Ved første climate-start uden gemt state er mode HEAT og setpoint `target_temperature: 21.0`. Senere gendannes mode og setpoint per zone fra ESPHome restore-state.

`friendly_name` er ændret fra `FloorHeat_v01_SingleZone` til `FloorHeat A16`. Enhedens visningsnavn i HA ændres; eksisterende entity-id'er bevares.

## Test i denne rækkefølge

Punkt 1–4 er grundfunktioner, der allerede kører; gentag dem efter ny firmware. Punkt 5–11 er åbne v0.2-tests.

1. **Sensorer:** Vent på nye BLE-målinger. `Sensor Valid` skal blive ON for hver zone, og `Sensor Age` skal vise sekunder.
2. **Boost:** Hæv setpoint over rumtemperaturen. Udgangen tænder normalt straks (`Boost requested for up to 1800 s`); første ON efter reboot kan vente på opstartsforskydningen. Da `boost_time` og `min_on_time` begge er 30 min, kører en tændt boost i praksis hele 30 min fra faktisk ON. Derefter starter en PWM-cyklus.
3. **Sænk setpoint:** Sænk setpoint til rumtemperaturen eller derunder. Udgangen slukker straks, og loggen viser `Setpoint <= ... C: PI reset, no heating`.
4. **OFF:** Sæt climate til OFF. Udgangen slukker straks (`Mode OFF; output forced OFF`); sensorovervågningen fortsætter.
5. **Boost-hold tæt på setpoint:** Med rumtemperatur lige under det ønskede setpoint (fx 20,9 °C), hæv setpoint fra under rumtemperaturen til lige over (fx 20,5 → 21,0 °C). Boost tænder udgangen, og den må ikke slukke igen før 30 min (`min_on_time`), selv om målingen når setpoint tidligere.
6. **Restore og opstart:** Sæt forskellige setpoints og én zone i OFF, genstart, og bekræft at mode og setpoint gendannes per zone. Med ufarlige testlaster og varmebehov i flere zoner skal første ON-kommandoer ligge mindst 5 min fra hinanden.
7. **Timeout og recovery:** Stop én sensors BLE-opdateringer. Efter `sensor_timeout: 15min` forventes `Sensor Valid` OFF og udgang OFF for den zone, mens de andre zoner regulerer videre. Den næste gyldige måling genaktiverer reguleringen, hvis climate er i HEAT.
8. **HA-frakobling:** Afbryd HA/API uden at afbryde controllerens strøm; lokal regulering skal fortsætte, og enheden må ikke genstarte (`reboot_timeout: 0s`).
9. **Min. OFF-tid:** Sæt midlertidigt fx `min_off_time: 10min` i én zone. Efter en OFF-overgang skal næste PWM-start vente (`Waiting for minimum OFF time before starting PWM`); boost overstyrer min. OFF, men ikke en ventende første ON efter reboot. OFF/sensorfejl skal stadig slukke straks.
10. **Lang PWM-cyklus (180 min):** Observer mindst én normal cyklus uden at ændre setpoint. Ved cyklusstart beregnes ON-tiden som `demand × 180 min` (mindst 30 min, ellers carry-over) og holdes hele cyklussen. Første cyklus efter reboot er 180, 120 eller 60 min i zoneorden. Notér temperatur, demand, ON-tid, overshoot og reaktionstid, når rummet køler af midt i en cyklus.
11. **Telestat:** Mål start-/holdestrøm og åbne-/lukketid; brug målingerne til at vurdere `min_on_time`, `min_off_time` og PWM-perioden.

## Hvis noget ikke passer

- Ingen `Sensor Valid`: kontrollér MAC, sensorens BTHome-version, BLE-rækkevidde og logs.
- Ingen output: kontrollér mode (HEAT), `Sensor Valid`, demand, PWM-periode, carry-over og eventuel opstartsventetid. Demand under 16,7 % giver ingen puls i en normal 180 min-cyklus; tiden akkumuleres, og næste beslutning kommer først ved næste cyklusstart (op til 3 timer). En kortere første cyklus kræver højere demand for at give en puls.
- Udgangen er stadig ON, selv om setpoint er nået: forventet i op til `min_on_time`, hvis boost netop har tændt udgangen. Ellers er PWM-pulsen låst for cyklussen og stopper først, når demand når 0 % (ca. `I/Kp` over setpoint).
- Udgangen tænder ikke, selv om rummet er blevet koldere: ON-tiden blev låst ved cyklusstart. En ny beregning sker ved næste cyklusstart, når demand har været 0 %, eller når setpoint hæves (boost).
- Compilefejl på `heatmesh`: kontrollér, at GitHub `main` indeholder den komponentversion, YAML'en forventer.
- Forældede `Test Zone`-entiteter fra tidligere firmware i HA: fjern dem manuelt, når de nye zoner er valideret.

Log testresultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).
