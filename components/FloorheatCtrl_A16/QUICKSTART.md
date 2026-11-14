# FloorHeat v0.1 — Quickstart Guide

**Goal:** Deploy a single regulated zone with one temperature sensor and one valve output.

---

## Prerequisites

- ✅ KinCony KC868-A16 flashed with ESPHome (any version)
- ✅ Xiaomi/Mijia sensor with PVVX firmware in BTHome v2 mode
- ✅ VS Code + ESPHome extension
- ✅ 24 V DC power to test output
- ⚠️ Know your sensor MAC address (e.g., `4C:65:A8:D0:A1:B2`)

---

## Step 1: Prepare Your YAML

1. Open `a16_v01_singlezone.yaml`
2. **Replace sensor MAC** (line ~56):
   ```yaml
   bthome_mithermometer:
     - mac_address: "4C:65:A8:D0:A1:B2"  # ← YOUR SENSOR MAC HERE
   ```
3. **Confirm WiFi secrets** are set in `secrets.yaml`:
   ```yaml
   wifi_ssid: "MyNetwork"
   wifi_password: "MyPassword"
   ```
4. **Optionally adjust control parameters** (lines ~90–98):
   ```yaml
   target_temperature: 21.0      # Desired room temp
   kp: 1.2                       # Proportional gain
   ki: 0.08                      # Integral gain
   period_ms: 30000              # Control loop (30 sec)
   pwm_period_min: 15            # Output period (15 min)
   ```

---

## Step 2: Verify Hardware Connections

### Sensor
- Xiaomi sensor powered and broadcasting BTHome v2 advertisements
- MAC matches your YAML config

### Valve Output
- **Pin:** Y01 (PCF8574 outputs_1_8, pin 0)
- **Wiring:** A16 Y01 (+24 V when ON) → one telestat terminal
- **Reference:** Other telestat terminal → 24 V PSU negative
- **Test:** Manually trigger in ESPHome API: `switch.test_valve_y01` → observe clicking

### Power
- 12 V DC to A16 controller input
- 24 V DC to output bank input (separate supply preferred)

---

## Step 3: Compile & Deploy

### In VS Code ESPHome Extension:

1. **Create** → Select `a16_v01_singlezone.yaml`
2. **Compile** → Should complete without errors
3. **Upload** → To your A16 (OTA or USB)
4. **Monitor** → Logs will appear

### Expected Initial Logs

```
[HeatMesh] HeatmeshComponent setup starting; 1 zones configured
[HeatMesh] Zone 'test_living_room': target=21.0°C, timeout=900000 ms, kp=1.20, ki=0.08
[HeatMesh] HeatmeshComponent setup complete
```

---

## Step 4: Verify Sensor Input

### Check ESPHome Logs

Wait ~30 seconds after boot. You should see:

```
[bthome_mithermometer] Found BTHome sensor: 4C:65:A8:D0:A1:B2
[bthome_mithermometer] Temperature: 22.3°C
[bthome_mithermometer] Humidity: 45%
```

If **no sensor appears:**
- Bring sensor physically closer to A16
- Verify MAC address is spelled correctly
- Check sensor is powered and in pairing/advertising mode

---

## Step 5: Trigger Regulation

### Method 1: Manual Temperature Adjustment (Quick Test)

1. Hold sensor near a warm spot (e.g., computer, lamp)
2. **Observe logs** for temperature increase:
   ```
   [heatmesh.zone] [test_living_room] PI: T=25.5°C, target=21.0°C, error=-4.50, p=-5.40, i=..., demand=0.0%
   ```
3. When temp exceeds setpoint → demand drops to 0%
4. When temp drops below setpoint → demand rises (expect 0–100% depending on error)

### Method 2: Set Different Temperatures (If HA Available)

In Home Assistant (optional):
- Entity → `sensor.test_zone_heat_demand` should show 0–100%
- Entity → `binary_sensor.test_zone_valve_state` should toggle ON/OFF

---

## Step 6: Observe Valve Behavior

### Expected Behavior

**When demand > 0%:**
- Logs show: `[heatmesh.zone] [test_living_room] Valve ON (switch #1, demand=50.5%)`
- Telestat audibly clicks
- Valve stays on for ~50% of a 15-minute cycle (~7.5 min)

**When demand = 0%:**
- Logs show: `[heatmesh.zone] [test_living_room] Valve OFF (switch #2, demand=0.0%)`
- Telestat stops
- Valve stays off until next cycle

**No switching every few seconds!** If you see rapid ON/OFF, something is wrong (see troubleshooting).

---

## Step 7: Test Failsafe (Critical)

### Disable Sensor & Observe Failsafe

1. **Physically move sensor far away** or power it off
2. Wait for timeout (default 15 min) → logs will show:
   ```
   [heatmesh.zone] [test_living_room] FAILSAFE: sensor invalid, heat_demand=X%
   [heatmesh.zone] [test_living_room] Sensor timeout, age=900001 ms
   ```
3. **Failsafe mode is OFF** (default) → demand should become 0%, valve OFF
4. Reconnect sensor → normal operation resumes

---

## Step 8: Test HA Offline (Critical)

1. **Stop Home Assistant** or disconnect from network
2. **Controller should continue regulating** without interruption
3. Confirm in logs that `loop()` is still called every ~1 second
4. Adjust sensor temperature → valve should still toggle

---

## Step 9: Monitor Long-Term Stability

Run for at least **1 hour** and check:

- **No crashes or reboots** → see `Uptime` in logs
- **Valve switches are spaced** → min 5 min apart, not continuous
- **Temperature setpoint stable** → no unexpected drifts
- **Sensor age grows linearly** → sensor is fresh, not stale

---

## Troubleshooting Quick Reference

| Problem | Diagnosis | Fix |
|---------|-----------|-----|
| Sensor not seen | Check MAC in logs | Verify MAC spelling; move sensor closer |
| Valve not switching | Check output bank power | Apply 24 V to Y01; test with manual switch command |
| Rapid ON/OFF pulses | Tune Kp/Ki or increase PWM period | Reduce `ki` (0.02–0.04); increase `pwm_period_min` (20–25) |
| Failsafe not activating | Check timeout in config | Verify `sensor_timeout: "15min"` is set; disable sensor manually |
| ESPHome won't compile | Check Python/C++ syntax | Review error message; check quotes and indentation |

---

## Next Steps After v0.1 Validation

Once this single zone is stable for 24 hours:

1. **Add more zones** (v0.2):
   - Duplicate zone config block in YAML
   - Add new sensors and valves
   - Verify no interference between zones

2. **Connect HA dashboard** (optional):
   - Create template sensors for each zone
   - Build automation/scripts

3. **Plan RS485 multi-node** (v0.3):
   - Install second A16 in extension
   - Test sensor sharing via RS485

---

## Performance Targets (v0.1)

| Metric | Target | Measurement |
|--------|--------|-------------|
| Sensor age at steady state | < 1 min | Check logs or HA entity |
| Valve switches per hour | 2–4 | Count "Valve ON/OFF" in logs |
| PI control loop interval | 30 sec | Configurable; 10–60 sec typical |
| Boot time to first regulation | < 2 min | From power-on to first valve action |
| HA offline run time | Unlimited | No limit; system is autonomous |

---

## Getting Help

1. **Check logs first** → ESPHome "Monitor" tab shows all activity
2. **Compare with YAML comments** → Parameter ranges and defaults documented
3. **Review PROJECT_SPEC.md** → Full system design rationale
4. **Check heatmesh/README.md** → Component-level details

---

## Success Criteria for v0.1 "Done"

✅ Sensor receives data reliably  
✅ PI controller loop executes every 30 seconds  
✅ Valve toggles ON/OFF based on temperature (not random)  
✅ Failsafe activates after sensor timeout  
✅ System continues working without Home Assistant  
✅ No crashes or watchdog resets  
✅ Valve does not pulse excessively (min 5–15 min cycle)  

---

**Estimated time to completion:** 30 minutes (including hardware setup)  
**Support:** Review logs in ESPHome and cross-reference with PROJECT_SPEC.md

Good luck! 🚀
