# FloorHeat Mesh v0.1 — Implementeringsplan

**Dato:** 2026-11-14  
**Status:** Implementering påbegyndt  
**Mål:** En fungerende lokal reguleringszone baseret på Xiaomi/Mijia BTHome v2 sensor

---

## ✅ GENNEMFØRTE TRIN

### 1. Komponentstruktur etableret
- ✅ `components/heatmesh/manifest.json` — ESPHome component metadata
- ✅ `components/heatmesh/__init__.py` — Config schema + codegen
- ✅ `components/heatmesh/zone_controller.h/cpp` — PI regulator + timeproportional output
- ✅ `components/heatmesh/sensor_manager.h/cpp` — Sensor input + freshness tracking
- ✅ `components/heatmesh/heatmesh.h/cpp` — Main component coordinator

### 2. PI-reguleringslogik implementeret
**Zone Controller:**
- PI-algoritme med konfigurabel Kp/Ki
- Anti-windup integrator
- Timeproportional ON/OFF output (standard 15 min periode)
- Sensor timeout/failsafe med defineret mode (OFF/HEAT)
- Ventilskift-tæller til vedligeholdelsesovervågning
- Sensor-age tracking

### 3. Test-YAML skelettet
- ✅ `a16_v01_singlezone.yaml` — Én zone med eksempel-sensor-MAC

---

## 📋 NÆSTE TRIN (Prioriteret)

### KRITISK VØR IMPLEMENTERING
1. **Hårdwaremapping verificeres**
   - [ ] Bekræft sensor-MAC for Xiaomi/Mijia sensor
   - [ ] Bekræft output Y01 pin-mapping (PCF8574, outputs_1_8, pin 0)
   - [ ] Bekræft 24 V strømforsyning på Y01

2. **ESPHome-kompatibilitet valideres**
   - [ ] Kompilering uden fejl
   - [ ] GPIO/switch-API bruges korrekt fra C++
   - [ ] BTHome v2 sensor modtages stabilt

3. **Test udføres step-by-step**
   - [ ] BLE modtagelse bekræftet (ESPHome log)
   - [ ] Zonekontroller loop kører (debug logs)
   - [ ] Ventil reagerer på sensor-temperatur
   - [ ] HA offline: regulering fortsætter
   - [ ] Sensor timeout/failsafe aktiveres

### DESIGN-ÅBNE PUNKTER
- **RS485 node-deling:** Udskudt til v0.3 (ikke i v0.1)
- **Persistent setpoint:** Kan gemmes lokalt hvis ønsket
- **Varmekald/pumpe:** Ikke implementeret i v0.1
- **HA climate entity:** Kan tilføjes som extension efter v0.1

---

## 🔧 HARDWARE-KRAV (V0.1)

| Komponent | Status | Detaljer |
|-----------|--------|----------|
| KC868-A16 | ✅ Modtaget | ESP32, 16 MOSFET-udgange, høj-side |
| Xiaomi/Mijia | ✅ Bestilt (6 stk) | PVVX firmware, BTHome v2 |
| 24 V PSU | ❓ Verificeres | Output-bank forsyning |
| 12 V PSU | ❓ Verificeres | Controller elektronik |
| Telestat | ✅ Eksisterende | 24 V DC Wavin-type |
| Kabling | ❓ Verificeres | A16 output til telestat |

---

## 🧪 TEST-MATRIX (V0.1)

| Test | Status | Kriterium |
|------|--------|-----------|
| T01 | ⏳ Afventer | BTHome sensor modtages stabilt |
| T02 | ⏳ Afventer | 1 PI-zone fortsætter uden HA |
| T03 | ⏳ Afventer | Ventil ON/OFF ved temperaturændring |
| T04 | ⏳ Afventer | Sensor timeout → failsafe |
| T05 | ⏳ Afventer | Boot-tilstand sikker (ventil OFF) |
| T06 | ⏳ Afventer | Setpoint-ændring via HA/YAML |
| T07 | ⏳ Afventer | Ingen utilsigtede korte pulser |

---

## 📂 FILSTRUKTUR

```
components/
└── heatmesh/
    ├── manifest.json           ✅ Oprettet
    ├── __init__.py             ✅ Oprettet
    ├── zone_controller.h       ✅ Oprettet
    ├── zone_controller.cpp     ✅ Oprettet
    ├── sensor_manager.h        ✅ Oprettet
    ├── sensor_manager.cpp      ✅ Oprettet
    ├── heatmesh.h              ✅ Oprettet
    └── heatmesh.cpp            ✅ Oprettet

components/FloorheatCtrl_A16/
├── PROJECT_SPEC.md            📖 Kravspec
├── current_esphome.yaml       📋 Reference A16-template
├── a16_v01_singlezone.yaml    ✅ Test-YAML v0.1
└── IMPLEMENTERINGSPLAN.md     📝 Denne fil
```

---

## 🎯 "DEFINITION OF DONE" FOR V0.1

**Acceptkriterier:**
1. ✅ ESPHome kompilerer uden fejl
2. ⏳ Én BTHome v2 sensor modtages og bruges til regulering
3. ⏳ Én telestat reguleres efter PI-algoritmen
4. ⏳ Systemet fungerer uden Home Assistant
5. ⏳ Diagnostik synlige: temperatur, setpoint, varmebehov, ventilstatus, sensoralder
6. ⏳ Sensor timeout udløser failsafe
7. ⏳ Ingen utilsigtede korte ventil-pulser
8. ⏳ Benchtest protokol dokumenteret

---

## ⚠️ VIGTIGE ANTAGELSER (VERIFICERES)

- **Sensor-MAC:** Der forventes en gyldig BTHome v2-sensor. MAC-adressen fra YAML skal opdateres til faktisk sensor.
- **Ventil-output:** Y01 antages at være PCF8574 outputs_1_8, pin 0. Dette skal bekræftes.
- **Strøm:** 24 V til output-bank, 12 V til elektronik. Ingen tests af strøm/sikring endnu.
- **BLE-stabilitet:** Klassisk ESP32 BLE-modtagelse forventes stabil; Wi-Fi/BLE coexistence observeres.

---

## 🚀 NÆSTE FASE (EFTER V0.1 VERIFICERING)

- **v0.2:** Flere zoner på samme controller
- **v0.3:** RS485 multi-node målingsdeling
- **v0.4:** Hele huset, alle 13 kredse
- **v1.0:** Open source-version med generaliseret hardware-support
