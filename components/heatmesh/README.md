# HeatMesh ESPHome Component

**Version:** 0.1.0 (In Development)  
**Status:** Initial implementation of local PI-based zone control  
**Target Platform:** KinCony KC868-A16 + Xiaomi/Mijia BTHome v2 sensors

---

## Overview

HeatMesh is an ESPHome External Component that implements **local, autonomous floor heating zone control** using:

- **PI temperature regulation** with configurable parameters (Kp, Ki)
- **Time-proportional ON/OFF output** to slow thermal actuators (typical period: 15 minutes)
- **BLE sensor input** via Xiaomi/Mijia sensors with BTHome v2 advertisements
- **Sensor timeout/failsafe** to maintain system safety without Home Assistant
- **Distributed architecture** (multi-controller support planned for v0.3)

The component is designed to work **completely independently** of Home Assistant, network, or any central supervision. HA integration is optional and provides monitoring only.

---

## Key Features

✅ **Local PI control** without external dependencies  
✅ **Sensor freshness tracking** and failsafe fallback  
✅ **Multi-zone support** on a single controller  
✅ **Climate entity** per zone (HEAT/OFF, setpoint restored after reboot)  
✅ **Diagnostics** exposed as native sensor/binary_sensor entities  
✅ **Minimal YAML configuration** (no lambdas needed)  
✅ **Anti-windup integrator** to prevent actuator saturation  
✅ **Configurable control periods** and PWM timeproportional periods  
✅ **Valve switch counting** for maintenance monitoring  

⏳ **Planned (v0.2+):**  
- RS485-based multi-controller temperature sharing  

---

## Hardware Requirements (v0.1)

| Component | Model | Notes |
|-----------|-------|-------|
| **Controller** | KinCony KC868-A16 | ESP32, 16 MOSFET outputs (high-side) |
| **Temperature Sensor** | Xiaomi/Mijia with PVVX | BTHome v2, no pairing required |
| **Power Input** | 12 V DC (controller) + 24 V DC (outputs) | Separate supplies recommended |
| **Thermal Actuators** | 24 V DC Wavin-type telestats | Slow, on/off binary control |

---

## Installation

### 1. Add Component to ESPHome Config

```yaml
external_components:
  - source:
      type: local
      path: components
```

### 2. Enable BLE Tracker

```yaml
esp32_ble_tracker:
  scan_parameters:
    interval: 1100ms
    window: 1100ms
```

### 3. Add BTHome Sensor

```yaml
bthome_mithermometer:
  - mac_address: "4C:65:A8:D0:A1:B2"  # Replace with your sensor MAC
    temperature:
      name: "Room Temperature"
      id: room_temp
```

### 4. Configure HeatMesh Component

```yaml
heatmesh:
  zones:
    - zone_id: "living_room"
      temperature_sensor: room_temp
      valve: valve_output_1
      target_temperature: 21.0
      sensor_timeout: "15min"
      kp: 1.2
      ki: 0.001
      period_ms: 30000      # Control loop interval
      pwm_period_min: 15    # Time-proportional period in minutes
      min_on_time: 5min     # Shorter pulses are carried into later cycles
      climate:
        name: "Living Room"
      heat_demand:
        name: "Living Room Heat Demand"
```

---

## Configuration Reference

### Heatmesh Component

```yaml
heatmesh:
  zones:                    # List of controlled zones
    - zone_id: "ZONE_NAME"
      id: zone_ctrl_1                   # Optional C++ id of the zone (for lambdas)
      temperature_sensor: SENSOR_ID
      valve: SWITCH_ID
      target_temperature: 21.0          # Initial setpoint until the climate has stored one (5–30°C)
      sensor_timeout: "15min"           # Max age of valid sensor reading
      kp: 1.2                          # Proportional gain (0.1–10.0)
      ki: 0.001                        # Integral gain, %/(K·s); Ti = kp/ki
      period_ms: 30000                 # Control loop interval (1–120 sec)
      pwm_period_min: 15               # Time-proportional period (5–60 min)
      min_on_time: "5min"              # Shortest valve ON pulse (must be < pwm period)
      # Optional Home Assistant entities (standard ESPHome entity options apply):
      climate:            { name: "Zone" }
      heat_demand:        { name: "Zone Heat Demand" }
      sensor_age:         { name: "Zone Sensor Age" }
      sensor_valid:       { name: "Zone Sensor Valid" }
      valve_state:        { name: "Zone Valve State" }
      valve_switch_count: { name: "Zone Valve Switches" }
```

### Zone Parameters

| Parameter | Type | Default | Range | Purpose |
|-----------|------|---------|-------|---------|
| `zone_id` | string | — | — | Unique identifier for diagnostics |
| `temperature_sensor` | sensor_id | — | — | BLE/ESPHome temperature source |
| `valve` | switch_id | — | — | GPIO switch for actuator output |
| `target_temperature` | float | 21.0 | 5–30 | Initial setpoint (°C); the climate entity's stored setpoint wins after first boot |
| `sensor_timeout` | time | 15min | 30s–∞ | Max age before failsafe |
| `kp` | float | 1.0 | 0.1–10 | Proportional gain |
| `ki` | float | 0.001 | 0–1 | Integral gain, %/(K·s); Ti = kp/ki (anti-windup) |
| `period_ms` | int | 30000 | 1s–2min | Control loop execution interval |
| `pwm_period_min` | int | 15 | 5–60 | Time-proportional ON/OFF period |
| `min_on_time` | time | 5min | > 0, < pwm period | Shortest valve ON pulse; shorter demand is carried to later cycles |
| `climate` | climate | — | — | Optional thermostat entity (see below) |
| `heat_demand` | sensor | — | — | Optional PI output (%) |
| `sensor_age` | sensor | — | — | Optional seconds since last valid reading (diagnostic) |
| `sensor_valid` | binary_sensor | — | — | Optional; ON while the reading is fresh (diagnostic) |
| `valve_state` | binary_sensor | — | — | Optional electrical output command |
| `valve_switch_count` | sensor | — | — | Optional output transitions since boot (diagnostic) |

---

## Algorithm Details

### PI Controller

Each zone runs an independent PI regulator:

```
error = target_temperature - current_temperature
p_term = Kp * error
i_term = Ki * integral(error) with anti-windup
output = p_term + i_term  (clamped to 0–100%)
```

**Anti-windup:** Integrator is limited to ±100%, preventing saturation when the actuator is stuck.

**Setpoint changes** keep the integrator and the running PWM cycle. The new demand takes effect at the next PWM cycle.

### Time-Proportional Output

The continuous PI output (0–100%) is converted to discrete ON/OFF actuator commands using a **long PWM period** (default: 15 minutes):

```
ON_time = (demand_percent / 100) * period_ms
OFF_time = period_ms - ON_time

Valve is switched ON for ON_time, then OFF for OFF_time, repeatedly.
```

Pulses shorter than `min_on_time` are not sent to the valve. Their on-time is carried into later cycles, so the average still matches demand, but a slow actuator never receives a pulse too short to move it.

This approach preserves actuator lifespan (vs. fast PWM) while providing smooth control.

### Sensor Timeout & Failsafe

If sensor age exceeds `sensor_timeout`:
- Zone enters **failsafe mode**
- Depending on config, either:
  - `HEAT` mode: Request maximum heat (conservative, prevents freeze)
  - `OFF` mode: Demand zero heat (safe in warm climates)

Failsafe is **explicitly logged** and must be cleared when sensor recovers.

---

## Diagnostics & Home Assistant Integration

Entities are declared directly under each zone; no template sensors or lambdas are needed.
All entities are optional — regulation runs identically without them.

### Climate (per zone)
- Current temperature, target temperature (5–30 °C, step 0.5)
- Modes: `HEAT` (regulating) and `OFF` (output forced OFF; sensor supervision continues)
- Action: `heating` while the output is ON, `idle` otherwise
- Mode and setpoint are stored in flash and restored after reboot. A setpoint change keeps
  the integrator and takes effect from the next PWM cycle.

### Sensor Entities (per zone)
- `heat_demand` — PI output (%), published every 10 s
- `sensor_age` — Seconds since last valid temperature, published every 10 s
- `valve_switch_count` — Output transitions since boot, published on change

### Binary Sensor Entities (per zone)
- `sensor_valid` — ON if the temperature reading is fresh
- `valve_state` — ON while the output is commanded ON (not measured valve position)

### Lambda Access (optional)

```yaml
heatmesh:
  id: heatmesh_controller
# ...
lambda: |-
  auto *zone = id(heatmesh_controller).get_zone("living_room");
  return zone ? zone->get_heat_demand_percent() : NAN;
```

---

## Testing & Verification

### Minimum Tests (v0.1 Definition of Done)

| Test | Acceptance Criterion |
|------|----------------------|
| T01 | BTHome sensor received and parsed correctly |
| T02 | Zone regulator loop executes without crashes |
| T03 | Valve output toggles in response to temperature change |
| T04 | Sensor timeout triggers failsafe after configured period |
| T05 | Setpoint change via YAML/API updates goal without crash |
| T06 | No continuous rapid valve pulses (min period ~5 min in failsafe) |
| T07 | System continues regulation with Home Assistant offline |

### Recommended Benchtest Procedure

1. **Flash YAML** with one test zone, one sensor, one output
2. **Wait for sensor** to appear in ESPHome logs
3. **Manually change temperature** (e.g., place sensor near heater)
4. **Observe valve** switching in the log (ON/OFF events)
5. **Disconnect Home Assistant** → verify valve continues switching
6. **Physically disconnect BLE sensor** → observe failsafe activation after timeout
7. **Reconnect sensor** → verify recovery and normal control

---

## Known Limitations & Design Decisions (v0.1)

### By Design
- **High-side MOSFET outputs only** — Existing A16 hardware is high-side; low-side option deferred
- **No proportional valve support** — Slow ON/OFF actuators only (PWM period ≥15 min)
- **No RS485 multi-node in v0.1** — Single controller per YAML; node sharing planned for v0.3
- **Climate is a front-end only** — Regulation stays in the zone controller; HA is not required

### Intentional Scope Reductions
- **Ethernet alternative deferred** — RS485 + BLE prioritized for v0.1 MVP
- **HMI integration later** — Touchscreen support planned after basic control verified
- **Varmebehov/pump call partial** — Valve state exposed; pump logic framework deferred

### Hardware Assumptions (Must Verify)
- 24 V output supply has sufficient capacity for all simultaneous on-demand valves
- Telestats are genuine 24 V DC types with verified startup/holding currents
- BLE range from A16 reaches intended zones reliably
- PCF8574 I2C expander addresses (36, 37) match intended output banks

---

## Future Roadmap

### v0.2 — Multiple Zones, Full Diagnostics
- [ ] Expand to 10+ zones on single controller
- [x] Climate entity with HEAT/OFF modes for HA
- [x] Persistent local setpoint storage
- [ ] Enhanced HA dashboard template

### v0.3 — Multi-Controller RS485 Sharing
- [ ] RS485 sensor transport layer
- [ ] Cross-node temperature sharing
- [ ] Master/peer failover strategy
- [ ] Collision-free bus arbitration

### v0.4 — Complete System
- [ ] All 13 zones (10+3) fully mapped and tested
- [ ] Pump control based on aggregate heat demand
- [ ] Varmekilde (heat source) integration
- [ ] Long-term stability testing

### v1.0 — Open Source Release
- [ ] Generic hardware abstraction
- [ ] Support for alternative sensors (Homie, ESPHome native)
- [ ] Alternate transport options (UDP, Ethernet)
- [ ] Public GitHub repository

---

## Troubleshooting

### Sensor Not Appearing
- Verify MAC address is correct and sensor is powered
- Check BLE signal strength (bring sensor closer)
- Monitor ESPHome logs for `bthome_mithermometer` errors

### Valve Not Switching
- Confirm GPIO switch ID in YAML matches component config
- Test switch manually: `service switch.turn_on entity_id: switch.xxx`
- Verify 24 V is present on output bank
- Check for PCF8574 I2C communication errors in logs

### Valve Switching Too Frequently
- Increase `pwm_period_min` (reduce control responsiveness)
- Reduce `ki` (lower integral gain to reduce overshoot)
- Check sensor value for noise/jitter

### No Failsafe Activation After Timeout
- Verify `sensor_timeout` is set to expected value
- Disable sensor in test to confirm failsafe works
- Check logs for explicit "Sensor timeout" warning

---

## Development & Contributing

This component is developed in VS Code with ESPHome extension.

**To compile locally:**
```bash
esphome compile a16_v01_singlezone.yaml
```

**To upload:**
```bash
esphome upload a16_v01_singlezone.yaml --device IP_ADDRESS
```

**To monitor logs:**
```bash
esphome logs a16_v01_singlezone.yaml --device IP_ADDRESS
```

---

## License

This component is developed as part of a private home automation project. Future open-source licensing (MIT/Apache) is planned after v1.0 release.

**Author:** @BjoernSmith  
**Created:** 2026-11-14

---

## References

- [KinCony KC868-A16 ESPHome Device](https://devices.esphome.io/devices/kincony-kc868-a16/)
- [ESPHome BTHome MiThermometer](https://esphome.io/components/sensor/bthome_mithermometer/)
- [ESPHome External Components](https://esphome.io/components/external_components/)
- [PROJECT_SPEC.md](../FloorheatCtrl_A16/PROJECT_SPEC.md) — Full system requirements

---

**Last Updated:** 2026-11-14  
**Next Review:** After v0.1 hardware validation
