# v0.1 Verification Checklist

Use this checklist to systematically validate that the implementation is ready for hardware testing.

---

## Code Quality & Compilation

- [ ] **Component files exist and are complete:**
  - [ ] `components/heatmesh/manifest.json`
  - [ ] `components/heatmesh/__init__.py`
  - [ ] `components/heatmesh/zone_controller.h`
  - [ ] `components/heatmesh/zone_controller.cpp`
  - [ ] `components/heatmesh/sensor_manager.h`
  - [ ] `components/heatmesh/sensor_manager.cpp`
  - [ ] `components/heatmesh/heatmesh.h`
  - [ ] `components/heatmesh/heatmesh.cpp`

- [ ] **Python linting:**
  - [ ] `__init__.py` has no syntax errors
  - [ ] Config schema validates (run ESPHome validation)
  - [ ] Dependencies correctly declared

- [ ] **C++ code review:**
  - [ ] No obvious memory leaks (smart pointers used)
  - [ ] PI algorithm correct (P + I terms with anti-windup)
  - [ ] Time-proportional output logic sound
  - [ ] Sensor timeout checked at entry to failsafe
  - [ ] Singleton pattern implemented correctly

- [ ] **Header guards & includes:**
  - [ ] All `.h` files have `#pragma once`
  - [ ] No circular includes
  - [ ] ESPHome includes correct

---

## Configuration & YAML

- [ ] **Test YAML valid:**
  - [ ] `a16_v01_singlezone.yaml` is present
  - [ ] Section order: esphome → esp32 → logger → api → ota → wifi → ble → i2c → pcf8574 → switch → heatmesh → sensor → binary_sensor
  - [ ] Indentation is consistent (2 spaces)
  - [ ] No undefined references to sensors/switches

- [ ] **Secrets configured:**
  - [ ] `secrets.yaml` has `wifi_ssid` and `wifi_password`
  - [ ] Encryption key in API section is valid

- [ ] **Sensor MAC placeholder:**
  - [ ] YAML shows placeholder: `4C:65:A8:D0:A1:B2`
  - [ ] Comment indicates "REPLACE WITH YOUR SENSOR MAC"
  - [ ] Format is 6 colon-separated hex pairs

- [ ] **Zone configuration:**
  - [ ] Zone ID is unique string (e.g., "test_living_room")
  - [ ] temperature_sensor references a valid sensor ID
  - [ ] valve references a valid switch ID
  - [ ] All parameters within valid ranges:
    - [ ] target_temperature: 5–30°C
    - [ ] kp: 0.1–10.0
    - [ ] ki: 0–1.0
    - [ ] period_ms: 1000–120000 (1–120 sec)
    - [ ] pwm_period_min: 5–60 minutes

---

## Documentation Completeness

- [ ] **README.md for component:**
  - [ ] Overview and feature list clear
  - [ ] Hardware requirements listed
  - [ ] Installation instructions step-by-step
  - [ ] Configuration reference table present
  - [ ] Algorithm details explained (PI, PWM, failsafe)
  - [ ] Diagnostics entities listed
  - [ ] Troubleshooting guide provided
  - [ ] References to external docs correct

- [ ] **QUICKSTART.md:**
  - [ ] Prerequisites checklist
  - [ ] Step-by-step deployment (steps 1–9)
  - [ ] Expected log output examples
  - [ ] Valve behavior expectations clear
  - [ ] Failsafe test procedure documented
  - [ ] HA-offline test procedure documented
  - [ ] Troubleshooting quick reference
  - [ ] Success criteria for v0.1 listed

- [ ] **IMPLEMENTERINGSPLAN.md:**
  - [ ] Implementation tasks marked (✅ vs ⏳)
  - [ ] Hardware requirements tracked
  - [ ] Test matrix defined
  - [ ] File structure shown
  - [ ] Next phase clearly labeled

- [ ] **IMPLEMENTATION_SUMMARY.md:**
  - [ ] Executive summary clear
  - [ ] Artifacts table complete
  - [ ] Architecture diagram/description present
  - [ ] Design decisions documented
  - [ ] Testing acceptance criteria listed
  - [ ] Next steps actionable

---

## Logic Validation

### PI Controller

- [ ] **Error calculation:**
  ```cpp
  error = target_temperature - current_temperature
  ```
  - [ ] Sign correct (positive when cold)
  - [ ] Units match (°C)

- [ ] **Proportional term:**
  ```cpp
  p_term = kp * error
  ```
  - [ ] Default Kp is 1.0–1.2 (tunable)
  - [ ] Produces 0–100% output range

- [ ] **Integral term:**
  ```cpp
  integrator += ki * error * (dt_ms / 1000)
  ```
  - [ ] Time scaling correct (seconds)
  - [ ] Anti-windup limits: ±100%
  - [ ] Resets on setpoint change
  - [ ] Ki default is 0.05–0.08 (tunable)

- [ ] **Output clamping:**
  - [ ] Final demand clamped to 0–100%
  - [ ] No negative or >100% values

### Time-Proportional Output

- [ ] **Period calculation:**
  ```cpp
  pwm_period_ms = pwm_period_min * 60000
  ```
  - [ ] Default: 15 min = 900,000 ms
  - [ ] Conversion 60×1000 correct

- [ ] **Duty cycle:**
  ```cpp
  on_time = (demand% / 100) * period
  ```
  - [ ] 0% demand → 0 ms ON, full period OFF
  - [ ] 50% demand → half period ON, half OFF
  - [ ] 100% demand → full period ON, 0 OFF

- [ ] **Switching logic:**
  - [ ] Compares elapsed time against ON time
  - [ ] Valve only switches when threshold crossed
  - [ ] No rapid toggling within cycle

### Sensor Timeout & Failsafe

- [ ] **Age tracking:**
  - [ ] Sensor age increases with each control cycle
  - [ ] Age resets when new measurement received
  - [ ] Default timeout: 15 min (900 sec)

- [ ] **Failsafe activation:**
  - [ ] Checked at every control loop iteration
  - [ ] Triggered when age > timeout
  - [ ] Logged with WARNING level
  - [ ] DEFAULT failsafe mode is OFF (safe)

- [ ] **Recovery:**
  - [ ] When sensor becomes fresh again, failsafe deactivates
  - [ ] Integrator reset to prevent windup
  - [ ] Normal control resumes

---

## Singleton & Lambda Integration

- [ ] **Static instance pointer:**
  - [ ] Declared in `heatmesh.h`: `static HeatmeshComponent* instance_`
  - [ ] Initialized in `heatmesh.cpp`: `HeatmeshComponent::instance_ = nullptr`
  - [ ] Set in `setup()`: `instance_ = this`

- [ ] **Accessor method:**
  - [ ] `static HeatmeshComponent* get_instance()` defined
  - [ ] Returns `instance_` pointer

- [ ] **Lambda access in YAML:**
  - [ ] Lambdas call `HeatmeshComponent::get_instance()`
  - [ ] Null-check before dereferencing
  - [ ] Zone lookup by ID: `hm->get_zone("zone_id")`
  - [ ] Return NAN if zone not found

---

## Diagnostics Entities

### Sensor Entities (per zone)

- [ ] **template sensor → current_temp:**
  - [ ] Calls `zone->get_current_temperature()`
  - [ ] Unit: °C
  - [ ] Decimals: 2
  - [ ] Update interval: 10s

- [ ] **template sensor → setpoint:**
  - [ ] Calls `zone->get_target_temperature()`
  - [ ] Unit: °C
  - [ ] Decimals: 1
  - [ ] Update interval: 30s

- [ ] **template sensor → heat_demand:**
  - [ ] Calls `zone->get_heat_demand_percent()`
  - [ ] Unit: %
  - [ ] Decimals: 1
  - [ ] Update interval: 10s

- [ ] **template sensor → sensor_age:**
  - [ ] Calls `zone->get_sensor_age_ms()`
  - [ ] Unit: ms
  - [ ] Decimals: 0
  - [ ] Update interval: 10s

- [ ] **template sensor → valve_switches:**
  - [ ] Calls `zone->get_valve_switch_count()`
  - [ ] No unit
  - [ ] Decimals: 0
  - [ ] Update interval: 60s

### Binary Sensor Entities (per zone)

- [ ] **template binary_sensor → sensor_valid:**
  - [ ] Calls `zone->get_sensor_valid()`
  - [ ] Update interval: 10s
  - [ ] False when timeout exceeded

- [ ] **template binary_sensor → valve_state:**
  - [ ] Calls `zone->get_valve_state()`
  - [ ] Update interval: 5s
  - [ ] True = ON, False = OFF

---

## Hardware Assumptions (Pre-Test)

These must be verified **before** connecting production valves:

- [ ] **Power Supply:**
  - [ ] 12 V PSU rated for A16 controller (min 500 mA)
  - [ ] 24 V PSU rated for all outputs (measure worst-case draw)
  - [ ] Common reference (0 V) between 12 V and 24 V supplies

- [ ] **PCF8574 I2C:**
  - [ ] outputs_1_8 at address 36 (0x24)
  - [ ] outputs_9_16 at address 37 (0x25)
  - [ ] I2C bus pull-ups present (usually on A16)

- [ ] **Valve Output Mapping:**
  - [ ] Y01 = outputs_1_8, pin 0
  - [ ] Polarity: HIGH = MOSFET ON = +24 V output
  - [ ] Inversion flag: true (standard for A16)

- [ ] **Telestat Specifications:**
  - [ ] Rated for 24 V DC (NOT AC)
  - [ ] Operating current measured (typical: 50–200 mA)
  - [ ] Terminal labels verified (C/Common, NO/Normally Open)

- [ ] **BLE Sensor:**
  - [ ] Powered and advertising BTHome v2
  - [ ] MAC address known and format correct
  - [ ] Range from A16 to sensor location acceptable (~10 m typical)
  - [ ] Not blocked by large metal/water obstacles

---

## Integration Points

- [ ] **ESPHome API:**
  - [ ] Encryption key configured
  - [ ] OTA enabled
  - [ ] No conflicts with other components

- [ ] **BLE Tracker:**
  - [ ] Configured with 1100ms scan interval/window
  - [ ] No other BLE components on same ESP32

- [ ] **I2C:**
  - [ ] Configured on GPIO 4 (SDA) + GPIO 5 (SCL)
  - [ ] Ignore strapping warning enabled
  - [ ] No conflicts with GPIO usage

- [ ] **Switch Platform:**
  - [ ] GPIO switch with PCF8574 backend
  - [ ] Output inversion set to true
  - [ ] Pin numbering matches PCF8574 (0–7 for pins 1–8)

---

## Compilation Dry-Run

- [ ] **Syntax check (local):**
  - Run: `python3 -m py_compile components/heatmesh/__init__.py`
  - Should complete without errors

- [ ] **ESPHome validation:**
  - Run: `esphome config a16_v01_singlezone.yaml`
  - Should show no errors
  - Should show all entities discovered

- [ ] **Compilation test (if ESPHome available):**
  - Run: `esphome compile a16_v01_singlezone.yaml`
  - Should complete without C++ errors
  - Binary size < 2 MB typical

---

## Documentation Cross-Reference

- [ ] **Links in README.md valid:**
  - [ ] PROJECT_SPEC.md path correct
  - [ ] ESPHome component links reachable
  - [ ] KinCony manual links functional

- [ ] **Quickstart references:**
  - [ ] Parameter ranges match component code
  - [ ] Example MAC format correct
  - [ ] Expected log messages match actual code

- [ ] **Implementation Plan tracking:**
  - [ ] All implemented tasks marked ✅
  - [ ] Deferred tasks marked ⏳
  - [ ] No contradictions between files

---

## Pre-Deployment Checklist

Before flashing to A16:

- [ ] All items above checked ✅
- [ ] Secrets file exists and has wifi credentials
- [ ] Sensor MAC updated to actual device
- [ ] 24 V PSU tested (no load)
- [ ] A16 powered up (12 V only, no valve load)
- [ ] ESPHome OTA network connectivity verified
- [ ] Backup of existing working A16 YAML taken

---

## Post-Deployment Verification

After flashing to A16:

### Boot & Setup (First 2 minutes)

- [ ] **ESPHome boots successfully:**
  - [ ] Logs show "App started"
  - [ ] WiFi connects
  - [ ] MQTT/API online

- [ ] **Component initializes:**
  - [ ] Log: "[HeatMesh] HeatmeshComponent setup starting; 1 zones configured"
  - [ ] Log: "[HeatMesh] Zone 'test_living_room': target=21.0°C, ..."
  - [ ] Log: "[HeatMesh] HeatmeshComponent setup complete"

### BLE Sensor (First 30 seconds)

- [ ] **Sensor discovered:**
  - [ ] Log: "[bthome_mithermometer] Found BTHome sensor: 4C:65:A8:D0:A1:B2"
  - [ ] Log: "Temperature: XX.X°C"

- [ ] **Sensor registry:**
  - [ ] Logs: "[heatmesh.sensor_mgr] Registered sensor: test_living_room"
  - [ ] Temperature values flow into zone controller

### PI Control (First 1 minute)

- [ ] **Control loop active:**
  - [ ] Log every 30 sec: "[heatmesh.zone] [test_living_room] PI: T=XX.X°C, target=21.0°C, ..."
  - [ ] Demand % visible (0–100%)

- [ ] **Initial valve state:**
  - [ ] Log: "[heatmesh.zone] [test_living_room] Valve OFF (switch #1, ...)"
  - [ ] or "[...] Valve ON (switch #1, ...)" depending on current temp

### Failsafe Test (After 15 minutes)

- [ ] **Disable sensor (unplug or move far away)**
- [ ] **Wait 15+ minutes**
- [ ] Log: "[heatmesh.zone] [test_living_room] Sensor timeout, age=XXXXX ms"
- [ ] Log: "[heatmesh.zone] [test_living_room] FAILSAFE: sensor invalid, heat_demand=0.0%"

### Recovery Test

- [ ] **Reconnect sensor**
- [ ] Wait for fresh reading
- [ ] Log: "Temperature: XX.X°C"
- [ ] Normal PI control resumes

---

## Sign-Off

Once all checkboxes are complete:

```
v0.1 Implementation Complete ✅
Date: _______________
Validator: _______________
Notes: _______________

Hardware Validation: Scheduled for _______________
```

---

**Document Version:** 1.0  
**Last Updated:** 2026-11-14
