# FloorHeat / HeatMesh — dokumentationsindeks

**Status pr. 2026-11-15:** HeatMesh v0.2.0 er under udvikling. Tre lokale zoner (Y01–Y03) kører på A16 med climate- og diagnostik-entiteter. v0.2-milepælen er ikke afsluttet: plausibilitetskontrol mangler, og genstart/watchdog, sensortab og drift uden Home Assistant er ikke testet. Se [implementeringsplanen](IMPLEMENTERINGSPLAN.md) før ændringer eller test.

## Start her

1. [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) — status per v0.2-krav, beslutninger og prioriterede næste trin.
2. [FINAL_SUMMARY_DA.md](FINAL_SUMMARY_DA.md) — kort dansk overdragelsesoverblik.
3. [QUICKSTART.md](QUICKSTART.md) — klargøring, build, flash og test.
4. [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) — hvad der er verificeret i software og på A16, og hvad der mangler.
5. [../heatmesh/README.md](../heatmesh/README.md) — HeatMesh-konfiguration, entiteter og kontroladfærd.
6. [PROJECT_SPEC.md](PROJECT_SPEC.md) — kravgrundlag og langsigtet roadmap; ikke aktuel fremdriftsrapport.

## Aktuel kildekode

- [a16_heatmesh.yaml](a16_heatmesh.yaml) — referencekonfiguration for KC868-A16 med tre BTHome v2-zoner (tidligere `a16_v01_singlezone.yaml`).
- [../heatmesh/](../heatmesh/) — ESPHome External Component: PI/PWM-regulator, boost, climate-frontend og diagnostik.
- [../../tests/heatmesh/](../../tests/heatmesh/) — host-test af zone-controlleren (min. OFF-tid og boost).

Den oprindelige A16-konfiguration `current_esphome.yaml` blev fjernet i commit `193c49d`. Hent den ved behov med `git show 1784f28:components/FloorheatCtrl_A16/current_esphome.yaml`.

## Status i korte træk

| Område | Status |
|---|---|
| Lokal PI-regulering, sensortimeout, PWM og min. ON-tid med carry-over | Implementeret; kører på A16 |
| Tre zoner (Y01–Y03) med separat fejltilstand | Kører på A16; fejlisolation bekræftet |
| Climate HEAT/OFF og setpoint/mode-restore | Implementeret; restore efter genstart ikke testet på A16 |
| Native diagnostik-entiteter uden YAML-lambdas | Implementeret; kører på A16 |
| Boost med min. ON-hold | Implementeret og host-testet; A16-test mangler |
| Valgfri min. OFF-tid | Implementeret og host-testet; A16-test mangler |
| Plausibilitetskontrol af temperatur | Ikke implementeret |
| Genstart/watchdog, sensortab/recovery og drift uden HA | Afventer test |
| Telestatmålinger, langtidsdrift og PI-tuning | Afventer målinger |
| Sensor registry/dublethåndtering, to A16'er, RS485 og pumpelogik | Ikke implementeret (v0.3+) |

## Dokumenternes formål

| Dokument | Formål |
|---|---|
| [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) | Aktuel fremdrift, beslutninger og prioriterede næste trin |
| [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) | Teknisk oversigt over den nuværende implementering |
| [QUICKSTART.md](QUICKSTART.md) | Praktisk deployment- og testvejledning |
| [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) | Verifikationsstatus og testcheckliste |
| [FINAL_SUMMARY_DA.md](FINAL_SUMMARY_DA.md) | Dansk overdragelsesoversigt |

---

**Vigtigt:** ESPHome-konfigurationen henter komponenten fra GitHub `main`. Kode- og dokumentationsændringer skal være committed og pushet, før en anden maskine eller Home Assistant kan bygge samme version. Copilot committer ikke; det gør brugeren selv.
