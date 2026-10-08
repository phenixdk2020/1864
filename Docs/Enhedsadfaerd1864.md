# Enhedernes opførsel i slaget: hvornår reagerer de, og hvordan bruger AI'en dem

Status: **designdokument, 2026-10-08.** Det beskriver, hvordan hver enhedstype skal opføre sig, og hvad der allerede findes i koden (`Source/Strategy1864`).
Hvert punkt er markeret: **[findes]**, **[delvist]** eller **[mangler]**. Tallene er startværdier, der kan justeres i spillet.

Reglen for alt nedenfor: **en reaktion har en udløser, en handling og en afslutning.** En enhed gør noget, fordi noget bestemt er sket, og den holder op igen, når det ikke gælder mere. Spillerens ordre vinder over officerens egne valg, men officeren bestemmer, *hvordan* ordren udføres.

---

## 1. Hvem bestemmer hvad

| Niveau | Bestemmer | Eksempel |
|---|---|---|
| Spilleren | Mål, retning, fire-politik, formation, holdning | "Angrib her", "hold stillingen" |
| Stab/HQ (bataljon, regiment, brigade, division) | Fordeler opgaver, følger kompagnierne, sender ordrer med ordonnans | Chefen sender "angrib" til tre kompagnier og beholder ét i reserve |
| Kompagniets officer (`FieldOfficerComponent`) | Hvordan ordren udføres: vej, afstand, flanke, sidetrin, bajonet, tilbagetrækning | Går udenom egen ild, vender fronten mod fjenden |
| Enhedens egne reflekser (`ThreatReaction`, `FormationPolicy`, `Stance`) | Hurtige reaktioner uden ordre | Kolonne til linje, karré mod rytteri, ned på knæ |

**Autoritet:** `DirectPlayer` er højest. En lavere autoritet må kun afløse, når den højere ikke længere udføres. Det betyder: **en afsluttet spillerordre betyder "hold og skyd", ikke "jag videre".** [findes, rettet 2026-10-08]

**ANGRIB HER** er en ordre, officererne selv udfører (den er aldrig "fysisk i gang" i sig selv). Den er afsluttet, når enheden er nået frem til målet (inden for 60 m).

**Officerens egenskaber** (0–100) afgør, hvor godt reaktionerne udføres: *taktik* (flanke, afstande), *initiativ* (om han reagerer uden ordre), *forsigtighed* og *aggression* (reserve, bajonet), *stab* (hvor hurtigt ordrer læses), *ro* (hvornår han trækker sig tilbage).

---

## 2. Fodfolk (kompagni)

### 2.1 Tilstande og skift

| Tilstand | Udløser | Handling | Afslutning |
|---|---|---|---|
| **Marchkolonne** | Flytning på 120 m eller mere og ingen fjende inden for rækkevidde | Marcherer i kolonne, 4 i bredden | Fjende inden for rækkevidde + 35 m, eller under 25 m fra målet |
| **Formering** | Se ovenfor | Går til kampformation (linje), mændene løber på plads; tilstand "Reforming" | Når mændene står; derefter "Holder formationen" |
| **Fremrykning mod fjenden** | Ordre eller ingen fjende på skudhold | Rykker til skudafstand (typisk 70 m for linjen) med fronten mod fjenden | Skudafstand nået |
| **Hold og skyd** | På skudhold, eller spillerordre afsluttet | Står og skyder efter fire-politik | Ny ordre, fjenden væk, eller tilbagetrækning |
| **Vend front** ("svinger fronten") | Fjenden kommer fra siden (over ca. 60°) | Drejer langsomt (36°/s) mod nærmeste fjende, **det nærmeste kompagni** først | Fronten vender mod fjenden |
| **Sidetrin** | Kort flytning (under 90 m) med fjenden tæt foran | Går sidelæns, fronten forbliver mod fjenden | Målet nået |
| **Bajonetangreb** | Fjenden vakler inden for 60–110 m, mændene står fast, officeren vil | Sætter bajonetter på, løber frem | 30 sek., eller kontakt |
| **Karré** | Rytteri på vej mod enheden (se 4) | Danner karré, sætter bajonetter på | Truslen er væk i 8 sek. |
| **Tilbagetrækning** | Tab over grænsen eller moral under 16–30 (efter officerens ro) | Trækker sig ud af ilden for at samle sig | Samlet, eller ny ordre |
| **Reserve** | Chef med 4+ kompagnier, forsigtig og besindig | Holder et kompagni 90 m bag ildbasen | Basen er såret, moral under 55, eller 240 sek. efter [mangler: tidsudløsning rettet i kode, afprøves] |

### 2.2 Marchen i detaljer [delvist]

1. **Lang flytning** (120 m eller mere): kolonne, 4 i bredden.
2. **Fjende i syne inden for rækkevidde + 35 m:** kolonnen opløses, kompagniet går i linje og går **det sidste stykke i formation**.
3. **Under 25 m fra målet:** samme, så det ankommer i orden.
4. **Defilé** (bro, smal vej): 2 i bredden; enheden forlader selv defilé, når den er igennem.
5. **Drejning:** et kompagni drejer langsomt (36°/s); mændene går i bue om midten, de svinger ikke som en hel flok. [delvist: drejehastigheden er sat, mændenes gang i bue er ikke set endnu]

### 2.3 Under ild uden fjende i nærheden [mangler]

Når kompagniet **står stille**, fjendens kanoner skyder på det, og **ingen fjende er inden for skudhold:**

- En **god officer** (initiativ og ro over ca. 60) beordrer **spredt orden**: mændene breder sig (større afstand) og **lægger sig ned**. Det gør dem sværere at ramme (mindre træf pr. skud), men de skyder langsommere og kan ikke gå af sted med det samme.
- En **middel officer** gør det efter nogle sekunders ild.
- En **dårlig officer** gør det ikke; mændene står og tager tab.
- **Afslutning:** ilden holder op i 10 sek., eller en fjende kommer inden for 150 m, eller der kommer en ordre om at rykke. Så rejser de sig og går tilbage til den tidligere formation.
- **Grænser:** kan ikke kombineres med karré eller bajonetangreb. Kavaleriet lægger sig ikke. Artilleriet kan ikke.

Findes delvist: `StrategyStanceComponent` har tre holdninger (stående, knælende, liggende) som et manuelt valg, og `StrategySkirmisherComponent` kan sprede mændene. Det **automatiske** skift, udløst af artilleriild og officerens egenskaber, mangler.

---

## 3. Rytteri

| Tilstand | Udløser | Handling |
|---|---|---|
| **Marchkolonne** | Lang flytning | 4 i bredden (defilé: 2) |
| **Linje** | Nær fjenden eller på ordre | Knæ ved knæ, 1–2 rækker |
| **Skærm** (`CavalryScreenAI`) | Spejdeordre | Holder afstand, ser, melder; angriber ikke |
| **Angreb** (`CavalryCharge`) | Ordre, eller officeren ser et mål åbent for angreb inden for 400–700 m (efter aggression) | Galop frem, kontakt |
| **Afstigning** (dragoner) | Ordre eller knap | Stiger af, skyder som fodfolk; kegle vises først da |
| **Tilbagetrækning** | Tab og moral som ved fodfolk | Samler sig bag egne linjer |

**Angrebets udfald** afhænger af:

- **Rytteriets styrke** og fart.
- **Retning:** flanke og ryg er langt værre for målet end front.
- **Målets formation:** mod **linje og kolonne** rammer angrebet fuldt og er dødbringende; mod en **fast karré** (god moral, samhørighed og ammunition) afviser karréen angrebet: hestene vægrer sig 30–50 m fra, rytteriet tager tab fra ilden og mister fart og moral; en **rystet karré** (lav moral eller samhørighed, tom for ammunition) kan brydes.
- Efter angrebet: hold, hvis det blev afvist; ellers fortsætter officeren efter egne regler. [kampregelpakken, 2026-10-08]

---

## 4. Karré og trusler fra rytteri

### Udløser [delvist]

- I dag: synligt fjendtligt rytteri **inden for 120 m**. For sent: rytteriet dækker det på ca. 10 sek., karréen skal bruge 20–30.
- **Skal være:** rytteri, der er synligt og **bevæger sig mod enheden**, inden for en afstand beregnet af rytteriets fart og karréens dannelsestid (ca. **250–350 m**). Rytteri, der ikke er på vej mod enheden, udløser ingenting.

### Reaktionen

1. **Kun det kompagni, rytteriet er på vej mod**, reagerer (det nærmeste på angrebslinjen). Naboerne danner kun karré, hvis rytteriet vender mod dem.
2. **Kolonne går direkte til karré** uden mellemtrin.
3. **Bajonetterne sættes på** samtidig. [findes ved karré fra kampregelpakken; prøves]
4. Dannelsen tager **20–30 sek.**, og enheden er sårbar imens.
5. **Fire fra karréen:** fire sider à 90°, hver med sine **25 % af mændene** (30 % med karré-forskning). Kun de sider, fjenden står foran, skyder; hjørnet dækkes af to sider. [findes fra kampregelpakken; prøves]
6. **Tab mod karréen:** artilleri og salver gør **flere tab** mod en karré (tæt, ubevægeligt mål): ca. ×1,5 for artilleri, ×1,2 for infanteriild. En karré bevæger sig meget langsomt.
7. **Afslutning:** rytteriet er væk, vendt om eller ødelagt, og der er gået 8 sek. uden trussel: **bajonetterne tages af, og enheden går tilbage til sin tidligere formation.**
8. **Spillerens egen formationsordre** tilsidesættes kun, hvis truslen er reel.

---

## 5. Artilleri

| Tilstand | Udløser | Handling |
|---|---|---|
| **Forspændt (limbered)** | Marchordre | Kører med forvogn |
| **Opstilling** (`ArtilleryPositioning`) | Ordre | Finder stilling med udsyn, uden at stå foran egne |
| **Afprotsning** | Ankommet til stilling | 12 sek. |
| **Ild** (`FireMission`) | Mål i række­vidde og udsyn | Vælger mål: **manuelt** som standard, **auto** som tilvalg |
| **Kartæsk** | Fjende tæt på (kort afstand) | Skifter til kartæsk |
| **Kontrabatteri** | Fjendens kanoner skyder | Skyder mod batteriet, hvis spilleren har sat auto |
| **Beprotsning** | Skifte stilling | 15 sek. |
| **Tilbagetrækning** | Tab, truslen fra rytteri/infanteri tæt på | Kører væk, hvis den kan; ellers kæmper batteriet til sidst |

Auto-målvalg skal **nulstilles**, når officer-AI'en slås fra (kendt fejl, rettet i kampregelpakken).

---

## 6. Stab og kommandokæde

- **HQ følger sine kompagnier** (bag dem på 65–145 m efter niveau). HQ'et gør ikke selv noget i slaget, men bærer ordrer.
- **Ordrer sendes med ordonnans** (synlig rytter); tiden vokser med afstanden. Officeren **læser** ordren efter sine egenskaber: en dårlig kan tage fejl af stedet lidt.
- **Chefens valg**: hvor mange kompagnier der angriber, hvem der er ildbase, hvem der flankerer, og om der holdes reserve (se 7).

---

## 7. Hvordan AI'en bruger sine tropper

### 7.1 Angreb med flere kompagnier [findes]

1. Chefen deler gruppen: **ét kompagni er ildbase** (midten), **to flankerer** (hver sin side).
2. Ildbasen rykker til skudafstand og holder og skyder.
3. Flankerne **går udenom ildlinjen** (ikke gennem egen ild) og **ind fra siden**, med vinkel 45° og 90° efter chefens taktik.
4. Gode chefer sender flankerne **videre ud**; dårlige holder dem tæt på og rykker frem på linje.
5. **Sidetrin** bruges, når en flytning er under 90 m med fjenden tæt foran.
6. **Reserve:** forsigtige chefer med 4+ kompagnier holder det bageste kompagni 90 m bag ildbasen og sætter det ind, når basen er såret (under 75 %), moralen er under 55, eller tiden er gået.

### 7.2 Forsvar [delvist]

- **Hold stillingen**, vend fronten mod fjenden, skyd efter fire-politik.
- **Bajonetangreb** mod en vaklende fjende nær skudhold.
- **Tilbagetrækning** i orden, når tab eller moral siger det; reserven dækker.
- [mangler] **Spredt orden og ned** under artilleriild (2.3).

### 7.3 Fjendens AI

- Samme regler som spilleren, men uden ordonnanser. Fjendens ledere har egne egenskaber (0–100), som afgør flanke, reserve og bajonet.
- **Sværhedsgrad** (`AIDifficulty`): skalerer reaktionstid, kontakt­afstand og hvor villig AI'en er til at flankere.
- **Fjenden flankerer også**, efter sine lederes egenskaber.
- **Rytteri:** spejder først, angriber åbne mål (fodfolk i linje eller kolonne), undgår karréer, jager en brudt fjende.
- **Artilleri:** samler ild på det mest truende mål, holder stilling bag fodfolket.

### 7.4 Kampagnens AI [findes delvist]

- Fjendens korps vælger mål og bevæger sig mellem byer; i et slag overtager slagets AI.
- Hjælpetropper (reserver) ankommer på dag 2 og 3.

---

## 8. Parametre (tunable)

| Navn | Nu | Hvor |
|---|---|---|
| Lang flytning = kolonne | 120 m | `FormationPolicy.LongMoveColumnThresholdCm` |
| Formering før fjenden | rækkevidde + 35 m | `FormationPolicy.DeploySafetyBufferCm` |
| Formering før mål | 25 m | `FormationPolicy` |
| Rytteritrussel (karré) | 120 m, skal være 250–350 m | `ThreatReaction.CavalryThreatDistanceCm` |
| Karré slippes efter | 8 sek. | `ThreatReaction.SquareReleaseDelaySeconds` |
| Drejehastighed fodfolk / rytteri | 36° / 80° pr. sek. | `MovementExecutor.TurnSpeedDegreesPerSecond` |
| Kolonnebredde fodfolk / rytteri / defilé | 4 / 4 / 2 | `FormationComponent` |
| Skudandel pr. karréside | 25 % (30 % med forskning) | `squareFaceFireShare` |
| Reserve efter | 240 sek. | `FieldOfficer` |

---

## 9. Åbne punkter til afklaring

1. **Spredt orden og ned** (2.3): hvilke egenskaber afgør det, og hvor meget sværere bliver mændene at ramme (forslag: −40 % træf, −50 % skudfart)?
2. **Karréens udløser** (4): fastlæg afstanden ud fra rytteriets fart, så der er tid.
3. **Fodfolk i kolonne under ild:** skal de straks gå i linje, eller først når fjenden er inden for skudhold?
4. **Mændenes gang ved drejning:** de skal gå i bue, ikke dreje på stedet (visuelt).
5. **HQ og floder:** HQ'et går i dag direkte og passerer floder og hegn (C-20 fra gennemgangen).
