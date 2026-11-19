# HeatMesh — overdragelse og status (dansk)

**Opdateret:** 2026-11-16
**Aktuelt trin:** HeatMesh v0.2.0 under udvikling. Tre zoner kører på A16; v0.2 mangler plausibilitetskontrol og robusthedstests.

## Hvad er implementeret?

`components/heatmesh/` indeholder lokal PI-regulering per zone med anti-windup, langsom tidsproportional ventiludgang, minimum ON-tid med carry-over, valgfri minimum OFF-tid, boost og freshness-timeout. Sensorfejl/timeout og climate OFF slukker zonens udgang med det samme. Reguleringen kræver ikke Home Assistant.

Reference-YAML'en [a16_heatmesh.yaml](a16_heatmesh.yaml) har tre zoner: Badeværelse (Y01), Køkken (Y02) og Bryggers (Y03), hver med egen BTHome v2-sensor. Hver zone har en climate-entitet (HEAT/OFF, temperatur, setpoint, action) og native diagnostik. Mode og setpoint gendannes efter genstart. YAML'en har ingen lambdas.

**Boost:** Hæves setpoint over rumtemperaturen, tænder udgangen normalt straks i op til 30 min. Første ON efter reboot kan vente på 5-minutters forskydningen, og boost-tiden tælles fra faktisk ON. Når udgangen først er tændt, forbliver den ON i mindst `min_on_time` (30 min), også selvom setpoint nås tidligere; med de nuværende værdier kører en tændt boost derfor hele 30 min.

## Verificeret — og ikke verificeret

- Firmwaren er bygget med ESPHome 2026.10.2 og kører med tre zoner på A16.
- Brugeren har bekræftet, at en zonefejl er isoleret fra de øvrige zoner, og at hardwareopsætningen fungerer.
- Host-testen i `tests/heatmesh/` består, inkl. min. OFF-tid og boost min. ON-hold.
- **Ikke testet på A16:** opstartsforskydning, boost min. ON-hold, lang PWM-cyklus (180 min / min. ON 30 min), min. OFF-tid med ikke-nul værdi, restore efter genstart, watchdog/reboot, sensortab/recovery og drift uden Home Assistant.
- **Ikke implementeret:** plausibilitetskontrol af temperaturmålinger.
- Telestatens strøm og åbne-/lukketid, 24-timers stabilitet og PI-tuning mangler.

## Før du fortsætter på en anden PC

1. Brug samme repository og branch (`main`).
2. Commit og push de lokale ændringer; Copilot committer ikke.
3. YAML'en bruger GitHub `main` som external component. Komponentændringer skal være pushet, før HA eller en anden PC kan bygge dem.
4. Konfigurér de tre secrets: `wifi_ssid`, `wifi_password` og `floorheatctrl_v01__encryption_key`.
5. Følg [QUICKSTART.md](QUICKSTART.md), og registrér testresultater i [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md).

## Startværdier i YAML

`Kp=100 %/K`, `Ki=0.007 %/(K·s)`, PI-loop 30 s, PWM 180 min, min. ON 30 min, min. OFF 0 s (slået fra), boost 30 min og sensortimeout 15 min. Komponentens defaults for Ki og PWM er 0.028 og 240 min. `Kp/Ki` giver ca. 4 timer integraltid. Det er forsøgsindstillinger, ikke færdig gulvvarmetuning.

ON-tiden beregnes ved cyklusstart og kan forkortes ved faldende demand efter min. ON. Stigende demand kan fremskynde en ny cyklus; korte pulser akkumuleres. Efter reboot er første PWM-cyklus 180, 120 eller 60 min i zoneorden, hvis den ikke afbrydes tidligt. Observer adfærden på gulvet og mål telestatens strøm samt åbne-/lukketid.

Se [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) for næste trin og [PROJECT_SPEC.md](PROJECT_SPEC.md) for kravgrundlaget og den videre roadmap.
