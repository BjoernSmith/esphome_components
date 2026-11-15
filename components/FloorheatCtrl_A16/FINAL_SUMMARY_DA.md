# HeatMesh — overdragelse og status (dansk)

**Opdateret:** 2026-11-15
**Aktuelt trin:** Software v0.1.2 implementeret; næste trin er fysisk validering af den nye firmware på A16.

## Hvad er implementeret?

`components/heatmesh/` indeholder lokal PI-regulering, anti-windup, langsom tidsproportional ventiludgang, minimum ON-tid med carry-over samt freshness-timeout. Sensorfejl/timeout og climate OFF slukker udgangen. Selve reguleringen kræver ikke Home Assistant.

Den nye climate-entitet tilbyder HEAT/OFF, aktuel temperatur, setpoint og action. ESPHome gendanner mode og setpoint efter genstart. Diagnostik konfigureres direkte i zone-YAML'en; reference-YAML'en har ingen lambdas eller dobbelt setpoint-entity.

## Verificeret — og ikke verificeret

- ESPHome 2026.10.2 genererede og kompilerede ESP32-firmware uden compile-fejl/-advarsler mod lokal component checkout.
- Host-build og simulering verificerede climate restore, output ON/OFF, timeout og sensor recovery.
- Tidligere firmwarelogs fra den fysiske installation viste BLE-målinger og setpointstyret output.
- **Den nye v0.1.2 climate-/entity-firmware er endnu ikke testet på fysisk A16.**
- Telestatens elektriske/mekaniske forhold, 24-timers stabilitet, PI-tuning og HA-afbrydelse med den nye firmware mangler.

## Før du fortsætter på en anden PC

1. Brug samme repository/branch.
2. Commit og push de lokale ændringer, der skal følge med; Copilot committer ikke.
3. YAML'en bruger GitHub `main` som external component. Hvis kun YAML ændres lokalt, skal også komponentkoden være tilgængelig fra den PC/build, der kompilerer.
4. Konfigurér de tre secrets: Wi-Fi SSID, Wi-Fi password og API encryption key.
5. Følg [QUICKSTART.md](QUICKSTART.md) og registrér fysiske testresultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).

## Startværdier i YAML

`Kp=100 %/K`, `Ki=0.028 %/(K·s)`, PI-loop 30 s, PWM 15 min, minimum ON 5 min, boost 30 min og sensortimeout 15 min. 1 K under setpoint giver 100 % demand; `Kp/Ki` giver ca. 60 min integraltid. Det er startværdier, ikke afsluttet gulvvarmetuning.

Lav demand kan blive akkumuleret over flere PWM-perioder, før der sendes en ON-puls på mindst 5 minutter. Mål først telestatens reelle åbne-/lukketid, og ændr derefter minimumspuls/periode ud fra målinger.

Se [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) for næste trin og [PROJECT_SPEC.md](PROJECT_SPEC.md) for kravgrundlaget og den videre roadmap.
