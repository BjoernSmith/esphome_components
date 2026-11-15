# FloorHeat / HeatMesh — dokumentationsindeks

**Status pr. 2026-11-15:** v0.1.2 software til én lokal zone er implementeret. ESPHome 2026.10.2 ESP32-build og host-simulering er bestået. Den nye climate- og diagnostikversion er endnu ikke valideret på den fysiske A16. Se [implementeringsplanen](IMPLEMENTERINGSPLAN.md) før ændringer eller test.

## Start her

1. [FINAL_SUMMARY_DA.md](FINAL_SUMMARY_DA.md) — kort dansk overblik og status.
2. [QUICKSTART.md](QUICKSTART.md) — klargøring, build, flash og sikker test.
3. [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) — præcis skelnen mellem build-, simulerings- og hardwaretest.
4. [../heatmesh/README.md](../heatmesh/README.md) — HeatMesh-konfiguration, entiteter og kontroladfærd.
5. [PROJECT_SPEC.md](PROJECT_SPEC.md) — kravgrundlag og langsigtet roadmap; ikke aktuel fremdriftsrapport.

## Aktuel kildekode

- [a16_v01_singlezone.yaml](a16_v01_singlezone.yaml) — referencekonfiguration for KC868-A16 og én BTHome v2-zone.
- [../heatmesh/](../heatmesh/) — ESPHome External Component: PI/PWM-regulator, climate-frontend og diagnostik.
- [current_esphome.yaml](current_esphome.yaml) — oprindelig A16-referencekonfiguration; brug den som hardwarereference, ikke som HeatMesh-konfiguration.

## Status i korte træk

| Område | Status |
|---|---|
| Lokal PI-regulering, sensor timeout og PWM | Implementeret |
| En zone i A16 YAML | Implementeret |
| Climate HEAT/OFF og setpoint restore | Implementeret, host-testet |
| Native diagnostik-entiteter uden YAML-lambdas | Implementeret, host-testet |
| ESPHome 2026.10.2 ESP32 compile | Bestået |
| BLE, faktisk udgang/telestat og HA-frakobling med v0.1.2 | Afventer fysisk test |
| Langtidsstabilitet og PI-tuning på gulvvarmen | Afventer målinger |
| Flere zoner, to A16'er, RS485 og pumpelogik | Ikke implementeret/verificeret |

## Dokumenternes formål

| Dokument | Formål |
|---|---|
| [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) | Aktuel fremdrift, beslutninger og prioriterede næste trin |
| [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) | Teknisk oversigt over den nuværende implementering |
| [QUICKSTART.md](QUICKSTART.md) | Praktisk deployment- og testvejledning |
| [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) | Verifikationsstatus og fysisk testcheckliste |
| [FINAL_SUMMARY_DA.md](FINAL_SUMMARY_DA.md) | Dansk overdragelsesoversigt |

---

**Vigtigt:** ESPHome-konfigurationen henter komponenten fra GitHub `main`. Den anden maskine skal have de relevante kode- og dokumentationsændringer committed og pushet, før den kan hente samme version.
