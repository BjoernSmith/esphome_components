# HeatMesh ESPHome External Component

**Implementationsversion:** 0.1.2 (firmware/config label; manifest metadata remains 0.1.0)
**Status:** ESPHome 2026.10.2 ESP32 compile og host-simulering bestået; fysisk A16-test af climate-versionen mangler.

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
      kp: 1.2
      ki: 0.001
      period_ms: 30000
      pwm_period_min: 15
      min_on_time: 5min
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

Påkrævet per zone: `zone_id`, `temperature_sensor`, `valve`. Kontrolparametre har defaults; climate- og diagnoseentities er valgfrie. Almindelige ESPHome entity-indstillinger (`name`, `id`, osv.) accepteres. Flere zoner accepteres af schemaet, men den aktuelle hardwareverifikation dækker kun én zone.

## Parametre

| Parameter | Default | Grænse / betydning |
|---|---:|---|
| `target_temperature` | 21 °C | Fallback ved første climate-opstart; 5–30 °C |
| `sensor_timeout` | 15 min | Positiv varighed, maks. 2,147,483,647 ms |
| `kp` | 1.0 | 0.1–10.0, procentpoint/K |
| `ki` | 0.001 | 0–1, procentpoint/(K·s) |
| `period_ms` | 30,000 | 1,000–120,000 ms |
| `pwm_period_min` | 15 | 5–60 min |
| `min_on_time` | 5 min | Positiv; skal være kortere end PWM-perioden |

I reference-YAML'en er `kp: 1.2`, `ki: 0.001`, dvs. en nominelt beregnet integraltid `Kp/Ki` på ca. 20 min. Det er ikke en endeligt indreguleret værdi.

## Regulering og failsafe

- PI: `error = target - temperature`; output begrænses til 0–100 % med anti-windup.
- Positive demand konverteres til én ON-puls per PWM-periode.
- Beregnede pulser kortere end `min_on_time` akkumuleres (carry-over) til senere cyklusser.
- Setpointændring beholder integrator og aktuel PWM-cyklus.
- Climate OFF, ikke-finite måling eller sensor-timeout tvinger udgangen OFF straks og nulstiller integrator/carry-over.
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

Referencekonfiguration: `components/FloorheatCtrl_A16/a16_v01_singlezone.yaml`. Denne blev kompileret med ESPHome 2026.10.2 mod lokal component checkout. En host-platform build og simulering verificerede mode/setpoint restore, ON/OFF, timeout og recovery. Den nye version skal stadig testes på den fysiske A16 med ufarlig testlast før telestat tilsluttes. Se [A16-verifikationslisten](../FloorheatCtrl_A16/VERIFICATION_CHECKLIST.md).

## Begrænsninger

Fysisk sensor-/ventilvalidering, PI-indregulering, flere samtidige zoner i drift, sensor registry/dublethåndtering, RS485, pumpekald og HMI er ikke afsluttet. De større projektkrav findes i [PROJECT_SPEC.md](../FloorheatCtrl_A16/PROJECT_SPEC.md).
