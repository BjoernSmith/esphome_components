# ESPHome HeatMesh — projektgrundlag og udviklingsplan

**Projekt:** Egen gulvvarmestyring til privat hus  
**Status:** Designgrundlag; første KC868-A16 er modtaget, flashet med en fungerende ESPHome-template og I/O er testet  
**Dato:** 2026-11-14  
**Primært udviklingsmiljø:** VS Code + GitHub Copilot, ESPHome, C++  
**Dokumentets rolle:** Overdragelsesdokument og *source of truth* for den videre udvikling

> **Vigtig prioritering:** Version 1 skal bygges specifikt til **KinCony KC868-A16** og **Xiaomi/Mijia med PVVX-firmware og BTHome v2**. En hardwareuafhængig, offentligt genanvendelig komponent er et senere mål, ikke et krav for den første fungerende version.
>
> Skeln konsekvent mellem **BESLUTTET** (allerede valgt), **FORESLÅET** (udgangspunkt til implementering/test) og **SKAL VERIFICERES** (må ikke antages at virke).

> **Statusnote (2026-11-15):** Dette dokument fastholder krav og den oprindelige roadmap; det er ikke den aktuelle fremdriftsrapport, og afkrydsningerne i afsnit 8 opdateres ikke løbende. Aktuelt trin er HeatMesh v0.2.0 under udvikling med tre zoner i drift på A16. Afvigelser fra roadmap: climate-entiteten blev implementeret allerede under v0.1, og sensor registry/dublethåndtering er flyttet fra v0.2 til v0.3. Se [IMPLEMENTERINGSPLAN.md](IMPLEMENTERINGSPLAN.md) for status per v0.2-krav og næste trin samt [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) for hvad der er verificeret i software og på fysisk hardware.

---

## 1. Formål og installation

Et selvstændigt, distribueret gulvvarmesystem skal erstatte den eksisterende Wavin-styring:

| Placering | Eksisterende løsning | Ny løsning | Aktuelle varmekredse |
|---|---|---|---:|
| Oprindeligt hus | Wavin AHC9000 | KC868-A16, controller/node 1 | 10 |
| Tilbygning (modsat ende af huset) | Separat 3-udgangs Wavin-styring | KC868-A16, controller/node 2 | 3 |
| **I alt** | | **2 controllere** | **13** |

- Huset er relativt langt. BLE-dækningen fra controlleren i hovedhuset kan derfor være utilstrækkelig til enkelte af dens egne rum.
- Controlleren i tilbygningen skal kunne modtage en BTHome-sensor og **dele dens måling** med controlleren i hovedhuset — også når Home Assistant er slukket.
- Hver controller **ejer og regulerer kun sine egne ventiludgange**. En fjerncontroller deler målinger, men fjernstyrer ikke den andens telestater direkte.
- **Home Assistant og HMI er valgfrie overbygninger**; ingen af dem må være nødvendig for den normale reguleringssløjfe.
- Lokale setpunkter og øvrige driftskritiske parametre skal kunne gemmes/reetableres efter genstart.

### Designmål

1. God og stabil rumkomfort med vandbåren gulvvarme.
2. Lang levetid for de eksisterende termiske telestater: undgå unødvendigt hyppige skift.
3. Fuld lokal autonomi ved Home Assistant-/netværksudfald, så langt det er muligt med tilgængelige sensorer.
4. Fejltolerant temperaturdeling mellem to controllere via RS485.
5. Temperaturer, setpunkter, faktisk udgangstilstand, varmebehov og fejlstatus synlige i Home Assistant.
6. Implementering som genanvendelig **ESPHome External Component** med C++-reguleringslogik og simpel YAML-konfiguration.
7. Mulighed for senere at udvide projektet til open source for andre ESP32-baserede installationer.

---

## 2. Fastlagte hardwarevalg

### 2.1 Controller: KinCony KC868-A16 — BESLUTTET

- **Præcis model:** KinCony **KC868-A16** (ikke KC868-A16S og ikke A16v3).
- **Processor:** klassisk **ESP32** på det leverede A16-print. Tidligere overvejede vi ESP32-WROOM-32D og senere ESP32-S3; begge idéer er afløst af det allerede indkøbte A16-print.
- **16 digitale MOSFET-udgange** pr. print; der bruges 10 på controller 1 og 3 på controller 2 i første version.
- Udgangene er **high-side/P-MOSFET** og leverer en switchende positiv forsyning. De er **ikke** de oprindeligt ønskede low-side-pulldowns.
- Producentens oplyste grænse: **maks. 500 mA pr. udgang** ved 12/24 V DC.
- Ethernet, BLE på ESP32, RS485 og I²C er til rådighed.
- Digital I/O styres via I²C-expandere i ESPHome-templaten. **Bevar de allerede fungerende I/O-definitioner**, frem for at gætte GPIO eller ændre hardwarelaget.
- Den første A16 er modtaget; ESPHome-template er flashet, og brugeren har bekræftet, at udgangene og den øvrige grundfunktion fungerer.
- Controller 2 skal anskaffes/installeres senere.

**Forsyning — vigtigt skel:**

- Dokumentationen for denne A16 angiver **12 V DC / 2,5 A** til **controllerens egen power-indgang**.
- Output-bankene har særskilte **12/24 V DC input-terminaler** til den spænding, som MOSFET-udgangene skal switch'e.
- De eksisterende Wavin-telestater er **24 V DC** (bekræftet af brugeren). Output-bankene forsynes derfor med **24 V DC**.
- **Tilfør ikke automatisk 24 V til controllerens 12 V-indgang.** Om en bestemt PCB-revision accepterer dette, må kun afgøres ved verificeret revision/datablad/måling. Brug indtil videre 12 V til elektronik og 24 V til ventiler.
- Dimensionér 24 V-forsyningen efter målt startstrøm, varm strøm og antal samtidige telestater. Worst case skal omfatte alle udgange, der kan være aktive samtidig.

**Grundprincip for én udgang:**

```text
24 V PSU (+) --> A16 output-bankens +24 V input
A16 kanal Y1 (+24 V når ON) --> telestatens ene terminal
Telestatens anden terminal --> 24 V PSU (0 V)

12 V PSU --> A16 controllerens separate 12 V power-input
```

- **SKAL VERIFICERES:** korrekt fælles 0 V/reference mellem relevante kredse, output-bankens terminalbetegnelser, PCB-revision, polaritet og telestaternes strømforbrug.
- Telestaterne er langsomme termiske on/off-aktuatorer, ikke proportionalventiler. PWM/tidsproportionering må derfor have lange perioder.
- Kontroller tilstand ved boot/restart: der må ikke forekomme utilsigtet fuld åbning af alle ventiler.

**Relevante verificerede hardwarehenvisninger:**

- ESPHome hardwaretemplate og pinout: https://devices.esphome.io/devices/kincony-kc868-a16/
- KinCony A-serie parametre: https://www.kincony.com/download/KC868-Smart-Controller-parameters-v1.9.pdf
- A16-brugervejledning med adskilte power/output-indgange: https://manuals.plus/ae/1005003738525547

### 2.2 Temperatur-/fugtsensorer — BESLUTTET

- **6 stk. Xiaomi/Mijia** små hvide LCD-termometre med smiley er bestilt.
- De flashes med kompatibel **PVVX/MiThermometer-firmware**, indstillet til **BTHome v2 advertisements**.
- Sensorerne sender temperatur, luftfugtighed og eventuelt batteridata som BLE-broadcasts; der er ingen parring nødvendig ved ukrypterede annoncer.
- Primær ESPHome-modtagelse: `esp32_ble_tracker` + `bthome_mithermometer`.
- Sensorer identificeres efter unik MAC/sensoridentitet; mål dem og knyt dem til logiske rum-ID'er.
- Et lokalt BTHome-broadcast skal kunne indgå direkte i regulatoren — **ikke via Home Assistant**.
- Begge controllere må modtage **samme** BLE-sensor. Dubletter håndteres i sensorlaget.
- De 6 bestilte sensorer er første etape; der er 13 varmekredse, men ikke nødvendigvis 13 selvstændige rum. Antal endelige rumtemperatursensorer afgøres af zonetilknytning; der må ikke automatisk antages 1 sensor pr. varmekreds.

ESPHome-reference: https://esphome.io/components/sensor/bthome_mithermometer/

### 2.3 HMI — FINDES, INTEGRATION SENERE

- Der findes allerede et touchskærmsboard, som kan køre ESPHome og har RS485.
- Det skal på sigt vise rumtemperatur, setpunkter, ventiler og fejl samt kunne ændre setpunkter.
- HMI må ikke være master for reguleringens eksistens eller en single point of failure.
- Hvis HMI og controllerdeling skal benytte **samme RS485-bus**, skal de anvende en **fælles kompatibel protokol**. Modbus RTU og en anden vilkårlig seriel protokol kan ikke bare blandes på samme ledningspar.
- HMI-tilslutning holdes uden for første MVP. Separat RS485-interface, kompatibel busprotokol eller Ethernet kan vælges senere.

---

## 3. Systemarkitektur — BESLUTTET

```text
       BLE / BTHome v2                                  BLE / BTHome v2
    Xiaomi/Mijia sensorer                              Xiaomi/Mijia sensorer
            |                                                 |
            v                                                 v
   +----------------------+                         +----------------------+
   | A16 NODE 1           | <---- RS485 data -----> | A16 NODE 2           |
   | Oprindeligt hus      |                         | Tilbygning           |
   | - BLE modtagelse     |                         | - BLE modtagelse     |
   | - sensorregister     |                         | - sensorregister     |
   | - 10 lokale zoner    |                         | - 3 lokale zoner     |
   | - PI + ventilstyring |                         | - PI + ventilstyring |
   | - failsafe           |                         | - failsafe           |
   +----------------------+                         +----------------------+
             |                                                |
             +-------------- Ethernet/LAN/WIFI ---------------+
                                      |
                      +---------------+--------------+
                      | Home Assistant (valgfri)     |
                      | Overvågning + setpunkter     |
                      +------------------------------+

     Touch-HMI: senere, med særskilt afklaret interface/protokol.
```

### Vigtigste ansvar og grænser

**På hver A16:**

- Modtag lokale BLE-advertisements.
- Hold et sensorregister med tid, oprindelse og gyldighed.
- Modtag/dele målinger via RS485.
- Kør egne PI-zoner lokalt og styr de lokale MOSFET-udgange.
- Eksponér diagnostik og justerbare setpunkter til Home Assistant via ESPHome API.
- Fortsæt regulering uden Home Assistant, Wi-Fi, MQTT-broker eller HMI.

**Aldrig afhængig af:**

- HA-automations, HA-climate-regulatorer, MQTT-broker, Internet/cloud eller fjern-HMI til reguleringsloopet.
- Ved udfald af RS485 kan en zone med **kun fjernmåling** ikke fortsætte normal, fuldt informeret regulering; her skal en eksplicit failsafe-strategi aktiveres.

### ESPHome eller egen firmware? — BESLUTTET

**ESPHome vælges**, ikke en fuld firmware skrevet fra bunden.

- ESPHome leverer hardwaredrivere, BLE, API, OTA, Ethernet, logging og en allerede fungerende A16-template.
- **Reguleringslogikken** skrives som genanvendelig C++ i en **External Component**.
- Sensor-, output- og HA-tilknytninger defineres i YAML.
- Brug YAML `packages` til at genbruge gentagne hardware-/konfigurationselementer, hvor det giver mening.
- Implementér ikke 13 kopier af reguleringsalgoritmen i lange YAML-lambdas.
- Start praktisk med én zone som test og flyt/byg den i komponenten; undgå at udvikle hele frameworket før første hardwaretest.

ESPHome External Components: https://esphome.io/components/external_components/

---

## 4. Kommunikation og BLE-deling

### Valg af forbindelse — BESLUTTET / PRIORITERET

| Transport | Beslutning | Begrundelse |
|---|---|---|
| **RS485** | **Foretrukket til node-node målingsdeling** | Kabelforbundet, robust, HA-uafhængig; findes på A16 |
| Ethernet/UDP | Alternativ og mulig senere transport | Let fejlsøgning, indbygget Ethernet og ESPHome-support |
| 433 MHz RF | Ikke valgt til version 1 | Kræver ekstra protokol, håndtering af fejl og retransmission |

Tidligere blev Ethernet fremhævet som let førstevalg; efterfølgende blev RS485 gjort til det konkrete spor for en generisk komponent. **Version 1 skal derfor først afklare og demonstrere RS485.** Ethernet beholdes til Home Assistant og eventuelt alternativ datatransport.

**A16 UART-forbindelse ifølge ESPHome-template:**

- RS485 TX = `GPIO13`
- RS485 RX = `GPIO16`
- Fysisk bus: A til A, B til B, passende fælles reference og eventuel isolation efter hardwareforholdene.
- Terminering typisk 120 ohm i **begge fysiske busender**, hvis det ikke allerede er monteret. Undgå dubleret terminering og lange stubs.
- Prøvekonfiguration: 9.600 eller 19.200 baud. Endelig hastighed fastlægges ved test, ikke som hårdt designkrav.

### ESPHome Packet Transport — KANDIDAT, IKKE FÆRDIG VERIFICERET

ESPHome har en officiel `packet_transport`-komponent, som kan sende sensorværdier direkte mellem noder uden server/broker. Den understøtter bl.a. **UART** og **UDP**:

- https://esphome.io/components/packet_transport/
- https://esphome.io/components/packet_transport/uart/
- https://esphome.io/components/sensor/packet_transport/

**SKAL VERIFICERES før valg af endelig RS485-implementering:**

1. At den konkrete A16-transceiver og dens eventuelle TX/RX-retningsstyring fungerer med valgt UART-transport.
2. At to noder på half-duplex RS485 ikke skaber tab pga. samtidige transmissioner. Packet Transport dokumenterer ikke i sig selv en komplet kollisionsfri multimaster-busprotokol til vores hardware.
3. At vi kan overføre/rekonstruere målingens **oprindelige alder/gyldighed**, ikke kun den senest modtagne float-værdi.
4. At dubletter og loop-videresendelse undgås.
5. At timeout og reconnect virker deterministisk efter node-reset og brudt kabel.

**Arkitekturregel:** Lav et lille transport-interface, så regulatoren er uafhængig af, om vi bruger Packet Transport over UART, en tynd custom RS485-framing, eller senere UDP. Undgå at skrive en ny protokol, hvis den officielle løsning kan opfylde kravene.

Hvis den officielle transport ikke håndterer busadgang pålideligt, er næste valg en **minimal masterstyret polling/token-protokol** med CRC og sekvensnumre; ikke blindt broadcast fra begge noder. Dette er en fallback-implementering, ikke allerede besluttet kode.

### Hvilke data deles?

**Målinger og metadata** deles, ikke rå BLE-trafik. For hver måling ønskes mindst:

```text
logical_sensor_id / BLE device identity
origin_node_id
measurement (temperature; optional humidity, battery, RSSI)
measurement_age_ms eller original sample timestamp i gyldig lokal tidsbase
sequence_number eller deduplikeringsinformation (efter behov)
valid / stale information
```

Node-til-node-transport skal kunne fungere uden synkroniseret NTP. Hvis absolutte tidsstempler ikke kan stoles på, sendes alder ved afsendelse plus receiverens modtagelsestid; **en videresendt gammel måling må aldrig fremstå som ny**.

### Sensorprioritet — FORESLÅET

- Brug **nyeste gyldige faktiske måling** uanset modtagende node, med lokal måling som tie-breaker ved omtrent samme alder.
- En lokal og en fjern kopi af samme sensor er ét logisk målepunkt, ikke to uafhængige sensorer.
- Remote updates må ikke nulstille freshness baseret alene på pakketid.
- Sensorer registreres én gang logisk; en temperaturværdi skal kun påvirke zoneregulatoren for de zoner, der er tilknyttet sensoren.
- Eventuel plausibilitetsfiltrering og støjfiltrering skal være konfigurerbar og må ikke skjule stale data.

### Idé til senere udvikling: MAC-nøglet sensorpulje — IKKE BESLUTTET

> Kun data til senere udvikling. Afsnittet ændrer ikke transportvalg, RS485-busdetaljer, busadgang eller arbejdsplanen i §8.

**Problem:** Med `packet_transport` skal hver fjernsensor konfigureres på modtageren med afsendernode (`provider`) og afsenderens sensor-ID (`remote_id`). Med 13 kredse fordelt på to noder bliver det mange par, som skal holdes ens i to YAML-filer.

**Idé:** Hver node lægger de BTHome-målinger, den selv hører, i en fælles pulje nøglet på sensorens BLE-MAC. Peer-noden modtager hele puljen og bruger kun de MAC'er, dens zoner er tilknyttet. En zone peger alene på en MAC; hvilken node der hørte sensoren, er ligegyldigt.

**Principper:**

1. Hver node sender periodisk én frame med én post per sensor, den **selv** har hørt: MAC (6 byte), temperatur (evt. fugt og batteri), målingens alder ved afsendelse og origin `node_id`.
2. Modtageren gemmer posterne i en tabel nøglet på MAC: værdi, måletidspunkt i egen tidsbase (modtagetid minus alder), origin-node og lokal/fjern.
3. Zonen bruger nyeste gyldige måling for sin MAC med lokal måling som tie-breaker ved omtrent samme alder (jf. *Sensorprioritet* ovenfor).
4. Kun egne målinger sendes; modtagne fjernmålinger videresendes aldrig. Det udelukker forwarding-loops, og alderen kan ikke nulstilles undervejs.
5. Stopper en BLE-sensor med at sende, vokser dens alder på begge noder, og zonens `sensor_timeout` udløser den eksisterende failsafe (udgang OFF).

Dermed dækkes kravene om alder/gyldighed, loop-forbud og "en videresendt gammel måling må aldrig fremstå som ny" uden synkroniseret ur.

**Konfigurationskoncept — PSEUDO-YAML, ikke implementeret:**

```yaml
heatmesh:
  node_id: 1
  transport:                       # som konceptet i §7; uændret
    ...
  sensors:                         # samme liste på begge noder, fx via !include
    - mac: A4:C1:38:14:28:EA
      temperature: zone1_temp      # lokal bthome_mithermometer-entitet
    - mac: A4:C1:38:BA:4E:58
      temperature: zone2_temp
  zones:
    - id: living_room
      sensor_mac: A4:C1:38:14:28:EA
      valve: valve_1
```

- En ny sensor kræver én linje i den fælles liste og én reference i zonen; ingen afsender/modtager-par.
- Begge noder lytter efter alle MAC'er i listen (samme `bthome_mithermometer`-blokke), så hver node kan bidrage med det, den hører.
- Listen fungerer som whitelist: naboers BTHome-enheder kommer ikke i puljen, og frames forbliver små.

**Framestørrelse:** ca. 15 byte per post (MAC 6, temperatur 2, fugt 1, batteri 1, alder 3, flag/kvalitet 2) plus header med `node_id`, sekvensnummer og CRC. 13 sensorer ≈ 200 byte, dvs. ca. 0,1 s på bussen ved 19.200 baud og ca. 0,2 s ved 9.600 baud (8N1). Busadgang følger de eksisterende regler ovenfor; puljen ændrer dem ikke.

**Dubletter:** Samme advertisement kan høres af begge noder. Puljen håndterer det via alder (nyeste vinder). ESPHomes `bthome_mithermometer` (2026.10.2) bruger BTHome packet-id internt til at afvise gentagelser, men eksponerer det ikke; packet-id kan derfor kun indgå i puljen, hvis HeatMesh senere selv dekoder BTHome.

**Observationer i ESPHome 2026.10.2-kildekoden (`packet_transport`):**

- Hver fjernsensor kræver `provider` og `remote_id` på modtageren.
- Afsenderen sender ved ændring og gensender **alle** sensorers seneste værdi hver `update_interval` (default 15 s), også hvis kildesensoren er holdt op med at sende. Modtageren publicerer værdien igen, så en stale BLE-sensor på afsendernoden ser frisk ud på modtageren.
- Pakken bærer ingen alder eller origin-metadata.
- Kabelbrud eller død peer opdages derimod: der kommer ingen pakker, og zonens timeout udløses.

Uden ekstra tiltag opfylder `packet_transport` altså ikke verifikationspunkt 3 (oprindelig alder/gyldighed). Arkitekturreglen om at undgå en ny protokol gælder fortsat; puljen er kandidat, hvis den officielle løsning ikke kan opfylde kravet.

**Placering i arkitekturen:** Puljen er en konkret udgave af *Sensor input/registry* i §7 og ligger over det planlagte transport-interface. Adaptere for UART/RS485 og eventuelt UDP ligger under det og kan udskiftes uden at ændre puljen eller regulatoren.

**Estimat og risici:**

- Ca. 400–600 linjer C++: pulje/registry, frame-encode/decode med CRC og sekvensnummer samt én adapter per transport.
- Host-tests: alderberegning, dubletter fra begge noder, timeout, loop-forbud og afvisning af korrupte frames.
- Ingen kryptering; acceptabelt på et dedikeret kabel mellem to noder.
- Mulig senere udvidelse: HeatMesh dekoder selv BTHome v2 fra BLE, så kun MAC skal konfigureres. Enkelt for ukrypterede PVVX-sensorer; kryptering kræver bindkey per sensor. Det afviger fra anbefalingen i §7 om at bruge ESPHomes `bthome_mithermometer` og er derfor kun en idé.

### Til senere udvikling: konfigurerbar master/slave med 30 s udveksling

**Brugerpræference:** De to noder skal have en eksplicit rolleindstilling: én master og én slave. Eksempelvis `role: master` / `role: slave` (kun koncept, ikke eksisterende YAML-API). Rollen gælder kommunikationen, ikke ejerskab af sensorer eller regulering.

- Master starter en udveksling hver **30 s** ved at sende de seneste målinger fra sine egne lokale sensorer med identitet, alder og gyldighed. Denne transmission fungerer samtidig som forespørgsel til slave.
- Når den komplette, gyldige forespørgsel er modtaget, og master har frigivet bussen, svarer slave med sine egne lokale sensorers seneste målinger og metadata. Slave sender ikke uopfordret; fjernmålinger videresendes ikke.
- Udvekslingen gennemføres også uden nye målinger eller med en tom lokal sensorliste, så manglende lokale data ikke forhindrer forespørgsel/svar.
- Ved normal kommunikation får begge noder dermed opdateret deres fælles datasæt én gang per 30 s-cyklus, efter transmission og svar. **Det er ikke en garanti for en ny BLE-måling hver 30 s.** Gensendte værdier beholder deres oprindelige alder; pakketid må aldrig nulstille sensorens freshness eller timeout.
- Ventetid på svar skal være afgrænset og ikke blokere reguleringen. Ved manglende svar ældes fjernmålingerne fortsat frem mod zonens eksisterende timeout/failsafe; gyldige lokale målinger bruges uafhængigt af peer.

Dette er en præcisering af den ønskede senere polling-adfærd, ikke implementeret funktionalitet eller en ændring af milepæle, fysisk kabling, pins eller baudrate. Konkret framing, retningsskift/svartid, timeout og håndtering af forsinkede/dobbelte svar skal afklares og testes som del af det eksisterende transportarbejde.

---

## 5. Regulering af telestater

### Besluttet princip

- **Lokal PI-regulering pr. rum/zone** (evt. med anti-windup), i ESP32'en.
- Udgangen til et termisk on/off-telestat er **tidsproportional**: PI-varmebehov 0–100 % omsættes til langsomme ON/OFF-perioder.
- Der vælges **ikke** hurtig elektronisk PWM på telestaterne.
- Wavin AHC9000 er tidligere identificeret som PI-baseret i dokumentation; dens konkrete interne PI-parametre og taktning kendes ikke. Vi kopierer derfor princippet, ikke en påstået identisk algoritme.
- Reguleringen skal afbalancere komfort med lavt antal aktueringer, herunder eventuelle minimum ON/OFF-tider.
- Systemet skal kunne udvides til **16 udgange pr. controller**, men version 1 skal dække 10 + 3.

### Foreslåede reguleringsparametre — IKKE fastlagte værdier

| Parameter | Forslag til første benchtest | Status |
|---|---|---|
| Reguleringsberegning | Hvert 30–60 sekund eller ved ny måling | FORESLÅET |
| Tidsproportional periode | Ca. 15–20 minutter | FORESLÅET; tilpasses aktuator/gulv |
| Minimum ON/OFF | 5–10 minutter, hvor praktisk muligt | FORESLÅET; kan konflikte med små duty cycles |
| Sensorens gyldighed | Op til ca. 15 minutter | FORESLÅET; afhænger af advertisement interval |
| Advarsel/failsafe | Advarsel efter sensor-timeout, defineret fallback senest omkring 60 minutter | FORESLÅET; skal designes og testes |
| PI-parametre Kp/Ki | Ikke valgt endnu | KRÆVER tuning |
| Maksimal rum-setpunkt | Konfigurerbar, med rimelige grænser | KRÆVER valg |

**Vigtig algoritmisk detalje:** En fast PWM-periode og et krav om minimum ON/OFF kan modstride hinanden ved små duty cycles. Komponenten skal derfor have en tydelig strategi: f.eks. samle duty over længere vinduer, springe perioder over eller bruge hysterese nær setpunkt, frem for at generere korte pulser. Ingen af disse strategier er endnu endeligt valgt.

### Opstartsforskydning efter genstart — IMPLEMENTERET

- Hver zones første faktiske ventil-ON efter reboot forskydes mindst **5 minutter** fra andre zoners første ON på samme controller. Det gælder både normal PWM og boost, uden en YAML-indstilling. En zone uden gyldig måling, behov eller tilstrækkelig PWM-puls optager ingen plads.
- En ventende PWM-cyklus og dens ON-tid starter først, når udgangen må tænde. En ventende boost tæller først `boost_time` fra faktisk ON og annulleres, hvis setpoint nås før ON. OFF og sensorfejl slukker straks.
- Første PWM-cyklus forkortes efter zoneorden: med `N` zoner og nulbaseret indeks `i` bruges `max(min_on_time, periode × (N − i) / N)`. Ved tre zoner med 180 min periode er længderne 180, 120 og 60 min. ON-tid og carry beregnes ud fra den faktiske cykluslængde; senere cyklusser bruger den normale periode.
- Forskydningen gælder kun første ON pr. zone efter reboot. Den forhindrer ikke senere samtidige starter eller samtidige åbne ventiler og koordinerer ikke forskellige boards. Fem minutter er en brugerbeslutning, ikke dokumentation for strømforsyningens kapacitet; dimensionér stadig efter målte strømme og worst case.

### Planlagt senere ændring: tidlig cyklusstart — IKKE IMPLEMENTERET

> Besluttet som senere ændring; milepæl er ikke fastlagt, og arbejdsplanen i §8 er ikke ændret. Nuværende firmware låser ON-tiden per PWM-cyklus.

**Problem:** Med lang PWM-periode (reference 240 min) bruges et stigende varmebehov først ved næste cyklusstart. Efter en kort puls, eller en cyklus uden puls fordi demand var under ca. 12,5 % ved cyklusstart, kan ventilen derfor blive ved med at være OFF i op til en hel periode, selv om demand i mellemtiden er højt.

**Hvorfor ikke en simpel grænse:** En regel som "ventil OFF og demand ≥ 50 % → start nu" ødelægger normal drift. Ved konstant 60 % demand slutter pulsen på 144 min, demand er stadig over grænsen, og ventilen tændes straks igen; i praksis kører den så ca. 100 %.

**Regel:** Mens ventilen er OFF i en cyklus (pulsen er slut, eller der var ingen puls), startes en ny cyklus straks, når begge betingelser er opfyldt:

1. Den nye puls bliver reelt til: `demand × periode + carry ≥ min_on_time` (ved 240/30 min ca. demand ≥ 12,5 %).
2. Ventilen har allerede været OFF i mindst `(1 − demand) × periode` og i mindst `min_off_time`. OFF-tiden regnes fra seneste ventilskift, også hvis det lå før den aktuelle cyklus.

Ved konstant demand giver reglen præcis samme timing og antal ventilskift som i dag. Når demand stiger, bliver OFF-pausen kortere. Ventetid efter endt puls ved 240 min periode, højst:

| Demand | Maks. ventetid |
|---:|---:|
| 50 % | 120 min |
| 75 % | 60 min |
| 90 % | 24 min |
| 100 % | 0 min |

I dag ventes til den 240 min lange cyklus er slut.

**Besluttet:** Altid aktiv; ingen ny YAML-option, da reglen opfører sig som i dag ved konstant demand.

**Implementeringsdetaljer, der skal afklares og testes:**

- Reglen evalueres, før ventilen slukkes ved pulsens slutning, så en ny cyklus, der allerede er due, holder ventilen ON i stedet for et OFF-blink uden længde.
- Carry fra en afbrudt cyklus uden puls skal forholdsmæssigt reduceres til den forløbne del af cyklussen, så samme demand ikke tælles to gange.
- PI-støj kan udløse en cyklusstart lidt tidligere end ved konstant demand; integratoren korrigerer, men det skal observeres i drift.
- Uændret: demand 0 % slukker straks; boost, sænket setpoint, OFF-mode og sensorfejl virker som i dag.
- Host-tests: konstant demand giver samme timing som i dag; stigende demand forkorter OFF-pausen efter formlen; cyklus uden puls + stigende demand starter straks; `min_off_time` overholdes; ingen ekstra ventilskift ved uændret demand.
- Ved implementering opdateres dokumentationen, der i dag beskriver op til 4 timers reaktionstid (heatmesh README, QUICKSTART, IMPLEMENTERINGSPLAN).

### Regulatorens opgaver

1. Beregn fejl `e = T_setpoint - T_room`.
2. Opdater PI med en defineret tidsbase og integratorbegrænsning (anti-windup).
3. Beregn varmebehov 0–100 %.
4. Omsæt behov til faktiske telestat ON/OFF-tilstande under krav om minimumsskifttid og aktuatorens lange responstid.
5. Understøt `OFF`, `HEAT`, og en dokumenteret `FAULT/FALLBACK`-tilstand.
6. Ved setpunktændring, genstart og sensorfejl: undgå ukontrolleret integrator og udgangsaktivering.
7. Saml varmekald pr. node og på sigt på tværs af noder til cirkulationspumpe/varmekilde.
8. Tilbyd overvågning af output duty, faktisk udgang, PI-fejl, sensorstale og antal ventilskift.

**Varmekald/pumpe:** Ønskes understøttet, men fysisk pumpeudgang, forsinkelser, flowkrav og samlet systemlogik er ikke endeligt fastlagt. Må ikke bygges som en implicit antagelse i v0.1.

### Sensorfejl og autonomi

- Hvis gyldig lokal temperatur findes: fortsæt normal regulering.
- Hvis lokal måling mangler, men en **frisk** fjernmåling findes: brug denne.
- Hvis begge mangler eller er stale: indgå en **eksplicit, konfigurerbar fallback**.
- Fallback er **ikke automatisk 100 % varme** og heller ikke nødvendigvis permanent `OFF`; det afhænger af frostrisiko, bolig, vandtemperatur og ønsket sikkerhedsprincip.
- **Krav:** En fejl må aldrig føre til permanent fuld opvarmning, ubegrænset åbning eller et fejlagtigt 'sensor OK' pga. genudsendte stale målinger.
- Hvis en node genstarter, skal zoner og telestater komme tilbage i en veldefineret tilstand, inden sensor-/kommunikationsdata erklæres gyldige.

### Ventilmotion efter lang stilstand — KRAV

**Krav (brugerbeslutning):** En ventiludgang, der har været OFF uafbrudt i mere end **2 måneder (regnes som 60 dage)**, skal tændes **ON i 20 minutter** og derefter slukkes igen. Formålet er at forhindre, at telestater og ventiler sætter sig fast efter lange perioder uden varmebehov, typisk om sommeren.

- Gælder per udgang/zone og uafhængigt af varmebehov, også i zone-mode `OFF`, som netop er sommertilfældet.
- Enhver almindelig ON-periode (PWM-puls eller boost) og selve motionen nulstiller stilstandstiden.
- Genstart må hverken nulstille stilstandstiden eller i sig selv udløse en motion. Stilstandstiden skal derfor gemmes persistent med begrænset flash-slid (fx højst én skrivning i døgnet per zone) og må ikke kræve Home Assistant eller NTP.
- 32-bit `millis()` løber over efter ca. 49,7 dage, altså før 60 dage; tidsbasen skal håndtere det.
- Motionen tæller med i ventilskift-tælleren og skal kunne ses i diagnostik (fx dage siden sidste ON, og om motion er aktiv).
- Test med forkortede tider i host-test og på A16; se T13 i §9.

**Åbne detaljer — skal besluttes før implementering:**

1. Forskydning mellem zoner, så ikke alle telestater tændes samtidig (24 V-strøm). Efter første flash har alle zoner samme stilstandstid.
2. Om motionen må køre, mens sensorfejl/failsafe tvinger udgangen OFF. Motionen afhænger ikke af temperatur, men det er en undtagelse fra den nuværende faste failsafe-regel.
3. Samspil med normal regulering under motionen, fx nyt varmebehov, sænket setpoint, mode-skift og `min_off_time` bagefter.
4. Om motionen skal indgå i varmekald til pumpe/varmekilde (forslag: nej).
5. Om 60 dage og 20 min skal være faste eller konfigurerbare per zone, og om funktionen kan slås fra.
6. Tidspunkt på døgnet (fx om natten) kræver en tidskilde; uden den køres motionen, når grænsen nås.
7. Startværdi ved første boot uden gemt historik (forslag: stilstandstiden regnes fra første boot).

Milepæl er ikke fastlagt; arbejdsplanen i §8 er ikke ændret.

---

## 6. ESPHome / Home Assistant / Bluetooth-proxy

### Besluttede roller

**ESPHome:** kører alt driftskritisk lokalt på hver A16.

**Home Assistant:**

- Viser temperatur, luftfugtighed, batteri, zonestatus, setpunkt, varmebehov, ventil ON/OFF, status for RS485 og sensorfejl.
- Kan ændre setpunkter og (kontrolleret) udvalgte tuning-parametre.
- Må gerne være offline i længere tid uden at stoppe reguleringen.
- Kommunikerer helst direkte med begge controllere via deres Ethernet-forbindelse og ESPHome Native API.
- Skal **ikke** beregne den primære PI-regulering.

**Bluetooth-proxy — VALGFRI, IKKE ET KRAV:**

- Begge ESP32'er modtager selv BTHome v2 lokalt.
- Sensorernes temperatur-/fugtighedsdata kan eksponeres til Home Assistant via almindelige ESPHome-entiteter, **uden Bluetooth-proxy**.
- Bluetooth-proxy kan senere aktiveres, hvis andre BLE-enheder skal til Home Assistant. Test eventuel påvirkning af BLE-stabilitet/ressourcer, især på klassisk ESP32.
- Ethernet foretrækkes som uplink til Home Assistant, så Wi-Fi/BLE-coexistence ikke skaber unødig belastning.

### Foreslået HA-entitetsmodel

Pr. zone, hvor relevant:

```text
climate.<zone>            current/target temperature, HEAT/OFF, action
sensor.<zone>_humidity
sensor.<zone>_heat_demand_percent
binary_sensor.<zone>_valve_on
sensor.<zone>_sensor_age_seconds
text_sensor.<zone>_sensor_source    local/remote/none
binary_sensor.<zone>_sensor_fault
sensor.<zone>_valve_switch_count
```

Pr. controller:

```text
binary_sensor.heatmesh_rs485_online
sensor.heatmesh_active_zones
binary_sensor.heatmesh_heat_call
sensor.heatmesh_uptime
sensor.heatmesh_free_heap
```

Navne er **forslag**, ikke færdige implementerede entiteter. Brug entiteter, der passer til ESPHome API og ikke skaber utilsigtede dubletter i Home Assistant.

---

## 7. Softwarestruktur — BESLUTTET RETNING

Der bygges en **ESPHome External Component** med C++-runtime og Python-konfigurationsvalidering. Den første udgave er A16/BTHome-orienteret, men implementeres med rene interfaces mellem hardware/sensorinput, transport og regulering, så senere generalisering er mulig.

### Foreslået projektstruktur

```text
heatmesh/
├── README.md                        # Kort projektoversigt
├── PROJECT_SPEC.md                  # Dette dokument
├── components/
│   └── heatmesh/
│       ├── __init__.py              # ESPHome config schema + codegen
│       ├── heatmesh.h
│       ├── heatmesh.cpp
│       ├── zone.h                    # PI + output scheduling
│       ├── zone.cpp
│       ├── sensor_registry.h         # Identitet, freshness, valg af måling
│       ├── sensor_registry.cpp
│       ├── transport.h               # Abstraktion; konkret RS485 adapter senere
│       └── transport.cpp
├── esphome/
│   ├── a16_main_house.yaml           # Node 1: 10 zoner
│   ├── a16_extension.yaml            # Node 2: 3 zoner
│   ├── packages/
│   │   └── a16_hardware.yaml         # Udtræk af den allerede fungerende template
│   └── secrets.yaml.example
├── docs/
│   ├── wiring.md
│   ├── rs485-protocol.md
│   ├── control-algorithm.md
│   └── commissioning.md
└── tests/
    ├── pi_control_cases.md
    ├── sensor_timeout_cases.md
    └── rs485_fault_cases.md
```

> Dette er et foreslået mappe-layout. Ved implementering kan ESPHomes faktiske komponent-API nødvendiggøre en anden fordeling af filer. Brug eksisterende ESPHome-API frem for at opfinde ikke-understøttede integrationstyper.

### Overordnet komponentopdeling

- **Sensor input/registry:** tager lokale ESPHome BTHome-sensorværdier, gemmer gyldighed/age og kombinerer evt. remote-værdier.
- **Zone controller:** uafhængig tilstandsmaskine/PI-algoritme per zone.
- **Valve adapter:** oversætter et logisk `on/off` til den valgte A16 ESPHome `switch`-udgang.
- **Transport adapter:** overfører sensordata/metadata til peer via RS485. Senere eventuelt UDP.
- **Controller coordinator:** samlet heat call, status, fejl og HA-diagnostik.

**Ingen krav om at ekstern komponent direkte skal dekode BLE-pakker.** Det anbefales i første version at lade ESPHomes dokumenterede `bthome_mithermometer` levere lokale målinger, mens komponenten udfører regulering og node-sharing.

### YAML-grænseflade — UDELUKKENDE KONSEPT

Nedenstående er et **ønsket API**, ikke eksisterende gyldig ESPHome-konfiguration. Copilot skal først implementere passende `CONFIG_SCHEMA` og codegen, før denne syntaks kan anvendes.

```yaml
# PSEUDO-YAML — må ikke antages at compile
heatmesh:
  node_id: 1
  transport:
    type: rs485
    uart_id: rs485_bus
  zones:
    - id: living_room
      temperature_sensor: living_room_temperature
      valve: valve_1
      target_temperature: 21.0
      sensor_timeout: 15min
      control: pi
    - id: kitchen
      temperature_sensor: kitchen_temperature
      valve: valve_2
      target_temperature: 21.0
      sensor_timeout: 15min
      control: pi
```

**Eksisterende faktisk ESPHome-API til lokale eksterne komponenter:**

```yaml
external_components:
  - source:
      type: local
      path: components
```

Den eksisterende fungerende `KC868-A16.yaml` skal være den tekniske sandhed for boardets PCF8574-output-ID'er, Ethernet og øvrige pin-definitioner. **Den konkrete YAML er ikke vedlagt dette dokument**; importér den til repositoryet, før kode genereres til udgangene.

---

## 8. Arbejdsplan og milepæle

### v0.1 — Én fysisk gulvvarmezone, lokalt

- [ ] Gem fungerende A16 ESPHome-template i Git med version og PCB-revision.
- [ ] Dokumentér faktisk 12 V elektronikforsyning, 24 V output-bank og telestattilslutning.
- [ ] Mål telestatens startstrøm, stationære strøm, åbne-/lukketid og eventuel polaritet.
- [ ] Flash én Xiaomi/Mijia med PVVX, vælg BTHome v2, dokumentér MAC og eventuel encryption/bindkey.
- [ ] Modtag temperatur/fugt/batteri stabilt lokalt via ESPHome.
- [ ] Styr én fysisk MOSFET-udgang fra én regulatorzone; test med ufarlig testlast før tilslutning af telestat.
- [ ] Byg PI + langsom tidsproportionering, logging og grundlæggende timeout/failsafe.
- [ ] Test at alt fungerer med Home Assistant frakoblet.

**Definition of Done:** Én telestat regulerer efter BLE-måling uden HA, med synlig setpoint, demand og valve state, og uden korte utilsigtede pulser.

### v0.2 — Flere zoner på første A16

- [ ] Flyt/færdiggør fælles C++ External Component; dokumentér YAML-interface.
- [ ] Tilføj flere rum, setpunkter og outputtilknytning uden kopiering af reguleringskode.
- [ ] Opret HA `climate`-/diagnostik-entiteter og persistent konfiguration.
- [ ] Implementér sensor registry, dublethåndtering, plausibilitetscheck og stale timeout.
- [ ] Dokumentér og test minimum ON/OFF og ventilswitch-tællere.
- [ ] Test opstart, watchdog/reboot, tab af sensor og tab af Home Assistant.

**Definition of Done:** Mindst de nødvendige lokale zoner kan køre samtidig uden HA, med separat fejltilstand per zone.

### v0.3 — To A16-noder + RS485

- [ ] Modtag/installer anden A16 med egen ESPHome-konfiguration.
- [ ] Verificér A16 RS485-fysik, halvduplex/retningsstyring og kabling.
- [ ] Lav kontrolleret udveksling af én temperaturmåling node 2 -> node 1.
- [ ] Test officiel `packet_transport` via UART på den faktiske RS485-forbindelse.
- [ ] Løs framing/busadgang/kollisioner, hvis Packet Transport alene ikke er tilstrækkeligt.
- [ ] Overfør korrekt sensor-age/gyldighed og unik identitet; forhindr forwarding-loops.
- [ ] Modtag samme BTHome-sensor på begge noder, og verificér valg af friskeste gyldige måling.
- [ ] Test fysisk afbrudt RS485, genstart af peer, tab af BLE og genopkobling.

**Definition of Done:** En zone på node 1 kan bruge en frisk måling, der kun høres af node 2; gamle målinger bliver aldrig genbrugt som friske.

### v0.4 — Hele huset, Home Assistant og varmebehov

- [ ] Installer og map 10 + 3 varmekredse til fysiske zoner/rum.
- [ ] Etabler tilstrækkelige BLE-temperatursensorer til den valgte rumopdeling.
- [ ] Finjustér PI-parameter, filter, slow-PWM og levetidsbegrænsninger ved faktisk drift.
- [ ] Tilføj heat-call/evt. pumpeudgang efter verificeret varmehydraulik.
- [ ] Udbyg HA-dashboard for zoner, aktuatorer, fejl, sensor-alder og samlet varmebehov.
- [ ] Planlæg HMI-betjening uden at bryde RS485-controllertrafikken.

**Definition of Done:** Hele huset kan køre selvstændigt under normalt netværks-/HA-udfald, bortset fra dokumenterede afhængigheder af delte fjernsensorer ved *også* at miste RS485.

### v1.0 — Mulig open source-udgave (senere)

- [ ] Generalisér input til almindelige ESPHome-sensorer og output til switches.
- [ ] Gør transport valgfri (RS485/UART og eventuelt UDP).
- [ ] Dokumentér forbindelser, constraints, API, failsafe og tuning.
- [ ] Tilføj automatiserede tests og eksempler til andre ESP32-boards.
- [ ] Publicér som installérbar External Component via GitHub.

---

## 9. Test- og acceptkriterier

| ID | Test | Acceptkriterium |
|---|---|---|
| T01 | BTHome v2 lokal modtagelse | Temperatur/fugt fra korrekt MAC registreres stabilt på A16 |
| T02 | 1 PI-zone, HA offline | Regulator og udgang fortsætter uden HA |
| T03 | 13 zoner fordelt 10+3 | Alle konfigurerede zoner regulerer uafhængigt |
| T04 | BTHome-sensor høres kun af anden node | Remote måling kan bruges via RS485 |
| T05 | Samme sensor høres af begge | Ingen dobbeltregulering eller fejlprioritering |
| T06 | Sensor stopper med at sende | Data bliver stale efter korrekt alder; fallback aktiveres |
| T07 | RS485-kabel trækkes ud | Ingen hængning; gyldig lokal sensor prioriteres; fjern-only zone går i fallback |
| T08 | Node genstarter | Udgang i defineret boot-tilstand, setpunkter gendannes, stale data genbruges ikke |
| T09 | HA frakobles | Uændret normal regulering og relevante lokale setpunkter bevares |
| T10 | Flere zoner kalder på varme | CPU, heap, BLE-modtagelse og output-skift forbliver stabile |
| T11 | Lang drift | Ingen utilsigtede korte aktuatorpulser; mål antal switch-events |
| T12 | RS485 peer sender samtidig | Kollisions-/busadgangsadfærd er dokumenteret og robust |
| T13 | Ventil OFF i mere end 2 måneder | Udgangen er ON i 20 min og derefter OFF; genstart nulstiller ikke stilstandstiden og udløser ikke motion |
| T14 | Første ventilstarter efter reboot | Første ON pr. zone på samme controller er mindst 5 min fra hinanden; ventende boost/PWM og failsafe virker som beskrevet i §5 |

**Målemetoder:** Log ESPHome-loop-/heap-statistik, tidsstempler for advertisement/modtagelse, sensor-age, udgangsskift, PI-output og eventuelle UART-fejl. Langtidstest før overtagelse af husets primære varmeregulering.

---

## 10. Åbne beslutninger, som Copilot ikke må gætte

1. **Aktuel A16 YAML:** præcis PCF8574-switch-mapping og identificerede pin-konflikter. Brug eksisterende fungerende template.
2. **PCB-revision og strøm:** elektronik-input 12 V; output 24 V; konkret sikring, strøm og 0 V-forbindelse skal verificeres.
3. **VVS-kort:** rum/navn, BLE MAC, lokal/remote nodetilknytning og fysisk ventilkanal for alle 13 kredse.
4. **PI-parametre:** Kp/Ki, tuning, output-perioder, anti-windup og styringsstrategi nær setpunkt.
5. **Failsafe-policy:** præcis handling ved stale sensor, RS485-brud og genstart; frostsikring og overophedningsgrænser.
6. **RS485-bus:** om officiel UART Packet Transport fungerer korrekt på A16, og hvordan halvduplex-kollisioner håndteres.
7. **Persistent setpoints:** storage/restore og forventet flash-skrivefrekvens.
8. **HMI:** konkret board og valg af selvstændigt interface/protokol.
9. **Pumpe/varmekald:** udgang, forsinkelser og kobling til varmepumpe/buffer, når hydraulikken er fastlagt.
10. **Antal rum vs. kredse:** 6 bestilte sensorer er en start, ikke bekræftet komplet sensorbestykning.

### Fravalg og afklarede misforståelser

- **Ikke A16S:** Den indkøbte model er **KC868-A16 med MOSFET**, ikke en mekanisk relæversion.
- **Ikke ESP32-S3:** En sådan CPU blev anbefalet tidligt, men valgt hardware er original KC868-A16 med klassisk ESP32.
- **Ikke low-side:** MOSFET-udgangene på A16 er high-side, så den eksisterende ventilkabling kan kræve omlægning.
- **Ikke nødvendigvis 24 V til hele printet:** 24 V er bekræftet til telestater/output-bank; printets controller-input er dokumenteret som 12 V.
- **Ikke primær regulering i HA:** HA må kun være supervision/konfiguration; local autonomy er et hårdt krav.
- **Ikke obligatorisk BLE-proxy:** BTHome-modtagelse og HA-eksponering kan ske direkte via ESPHome.
- **Ikke 433 MHz i v1:** RS485 prioriteres til deling.
- **Ikke færdig RS485 mesh-kode:** Vi har valgt et design, ikke verificeret kollisionsfri transport på konkrete print.
- **Ikke en publiceret generisk komponent endnu:** Fokus er første funktionelle installation; generalisering senere.

---

## 11. Instruktion til GitHub Copilot i VS Code

> **Brug dette dokument som projektets kravspecifikation.** Arbejd inkrementelt. Start med brugerens allerede fungerende `KC868-A16.yaml`. Projektets første mål er én lokal BTHome v2-målt PI-zone på KC868-A16 med én 24 V telestat, fuldt fungerende uden Home Assistant. Implementér reguleringslogik i en lokal ESPHome External Component (`components/heatmesh/`) med dokumenteret YAML-config, frem for 10 kopierede lambdas. Verificér compile-/ESPHome-konfigurationskompatibilitet efter hver ændring. Design logikken, så sensorinput, ventiloutput og RS485 senere kan kobles til uden omskrivning af PI-regulatoren. Lav ikke en fiktiv færdig YAML-platform, før Python schema/codegen og C++-klasserne er implementeret. Undlad at antage 24 V controllerforsyning, konkrete PI-parametre eller pålidelig RS485 multimaster-kommunikation uden test. Spørg efter manglende hardwaremapping frem for at gætte. Skriv først en kort implementeringsplan og en liste med filer, der skal ændres, og implementér derefter **v0.1**.

**Første konkrete Copilot-opgave:**

1. Gennemgå `esphome/a16_main_house.yaml` (kopi af den fungerende controllerkonfiguration).
2. Find den eksisterende MOSFET-outputentitet, der kan reserveres som `valve_1`.
3. Tilføj én `bthome_mithermometer` temperaturmåling med faktisk MAC.
4. Byg et minimalt lokalt External Component-skelet med én PI-zone, setpoint, sensor-timeout og ventil-ON/OFF-styring; sørg for at konfigurationen kompilerer.
5. Tilføj diagnostik: temperatur, setpoint, PI-output, valve state, sensor age/source og fault.
6. Lever en kort benchtest-procedure, inkl. HA-offline, boot-state og mistet BLE.

---

## 12. Referencer

- KinCony KC868-A16 ESPHome enhedstemplate/pinout: https://devices.esphome.io/devices/kincony-kc868-a16/
- KinCony A-serie hardwareparametre: https://www.kincony.com/download/KC868-Smart-Controller-parameters-v1.9.pdf
- KC868-A16 user manual (12 V controller, 12/24 V output-bank): https://manuals.plus/ae/1005003738525547
- ESPHome BTHome MiThermometer: https://esphome.io/components/sensor/bthome_mithermometer/
- ESPHome ESP32 BLE tracker: https://esphome.io/components/esp32_ble_tracker/
- ESPHome Packet Transport: https://esphome.io/components/packet_transport/
- ESPHome UART Packet Transport: https://esphome.io/components/packet_transport/uart/
- ESPHome Packet Transport Sensor: https://esphome.io/components/sensor/packet_transport/
- ESPHome External Components: https://esphome.io/components/external_components/
- ESPHome PID Climate: https://esphome.io/components/climate/pid/
- ESPHome Slow PWM Output: https://esphome.io/components/output/slow_pwm/

---

## 13. Kort beslutningslog

| Emne | Endelig nuværende beslutning |
|---|---|
| Board | KC868-A16 original med klassisk ESP32 og 16 MOSFET-udgange |
| Antal controllere | 2 (10 zoner + 3 zoner) |
| Telestater | Genbrug eksisterende Wavin, 24 V DC |
| Outputtopologi | High-side P-MOSFET; ikke low-side |
| Sensorer | Xiaomi/Mijia, PVVX, BTHome v2; 6 bestilte |
| Regulering | Lokal PI med langsom tidsproportional ON/OFF og failsafe |
| Ventilmotion | 20 min ON, når en udgang har været OFF i mere end 2 måneder |
| Implementering | ESPHome + lokal C++ External Component |
| Kommunikation | RS485 prioriteres til sensor-shared data; Packet Transport vurderes/testes først |
| Ethernet | Foretrukket til Home Assistant, mulig alternativ inter-node transport |
| Bluetooth proxy | Valgfri, ikke nødvendigt for normal drift |
| Home Assistant | Kun betjening, målinger og overvågning, ikke påkrævet for varme |
| HMI | Findes, senere integration; må ikke blive kritisk afhængighed |
| Fremtidig genbrug | Komponentdesign, mulig open source efter fungerende A16-løsning |

**Næste handling:** Opret Git-repository, læg dette dokument ind som `PROJECT_SPEC.md`, tilføj den **faktisk fungerende** A16 ESPHome YAML, og start v0.1 med én sensor og én udgang.
