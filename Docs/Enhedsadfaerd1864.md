# Enhedernes opførsel i slaget: hvornår reagerer de, og hvordan bruger AI'en dem

Status: **kode- og teststatus, 2026-10-09.** Dokumentet beskriver reglerne i `Source/Strategy1864`; [findes] betyder implementeret, ikke nødvendigvis bygget eller afprøvet. Se teststatus nederst.
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
| **Marchkolonne** | Resterende rute over 120 m og ingen nær fjende | Marcherer i kolonne, 4 i bredden | Fjende inden for egen lange ildrækkevidde + 35 m, eller under 25 m fra slutmålet |
| **Formering** | Se ovenfor | Går til kampformation (linje), mændene løber på plads; tilstand "Reforming" | Når mændene står; derefter "Holder formationen" |
| **Fremrykning mod fjenden** | Fremryknings-/angrebsordre | Rykker til valgt skudafstand med fronten mod fjenden | Skudafstand nået |
| **Hold og skyd** | På skudhold under fremrykning/angreb, eller spillerordre afsluttet | Annullerer bevægelsesordren og alle vejpunkter; står og skyder efter fire-politik og valgt rækkevidde | Når fjenden forlader skudhold, holder enheden stadig stedet indtil en ny spillerordre |
| **Vend front** ("svinger fronten") | Fjenden kommer fra siden (over ca. 60°) | Drejer langsomt (36°/s) mod nærmeste fjende, **det nærmeste kompagni** først | Fronten vender mod fjenden |
| **Sidetrin** | Kort flytning (under 90 m) med fjenden tæt foran | Går sidelæns, fronten forbliver mod fjenden | Målet nået |
| **Bajonetangreb** | Fjenden vakler inden for 60–110 m, mændene står fast, officeren vil | Sætter bajonetter på, løber frem | 30 sek., eller kontakt |
| **Karré** | Rytteri på vej mod enheden (se 4) | Danner karré, sætter bajonetter på | Truslen er væk i 8 sek. |
| **Tilbagetrækning** | Tab over grænsen eller moral under 16–30 (efter officerens ro) | Trækker sig ud af ilden for at samle sig | Samlet, eller ny ordre |
| **Reserve** | Chef med 4+ kompagnier, forsigtig og besindig | Holder et kompagni 90 m bag ildbasen | Basen er såret, moral under 55, eller 240 sek. efter [mangler: tidsudløsning rettet i kode, afprøves] |

**Stop og skyd annullerer marchen [findes i kode; ikke afprøvet]:** Når en fremryknings- eller angrebsordre standses for at skyde på en fjende inden for den valgte fire-politik og rækkevidde, erstattes ordren af hold på stedet. Alle vejpunkter samt destinationsboks og rutelinje fjernes straks, også vejpunktsudvidelser på vej med ordonnans. Enheden skyder, mens et gyldigt mål er på skudhold, og genoptager ikke marchen, når målet forsvinder. En ren **FLYT**-ordre bevarer det præcise mål, også på en fjendtlig position, men annulleres nu ved indgående salve (også uden tab) eller registrerede tab. Ordren erstattes af HOLD; alle vejpunkter, ventende udvidelser med ordonnans, destinationsboks og rutelinje fjernes. Enheden vender fronten og besvarer ilden efter den valgte fire-politik; HOLD ILD tillader stadig ikke skydning. **TILBAGE** og **AFBRYD** fortsætter under ild uden den almindelige ildpause.

**Angrebsmål på skudafstand [findes i kode; ikke afprøvet, 2026-10-09]:** ANGRIB HER og RYK FREM mod en fjende eller et punkt inden for 60 m af en kampdygtig fjendtlig enhed får målpunktet på linjen fra enheden til fjenden: aktiv skudafstand minus 3 m. Samme mål og frontretning bruges af bevægelsen, destinationsboksen og officerens afslutningskontrol. Målet beregnes før ordonnanslevering og ved modtagelse; enheden bakker ud med fronten mod fjenden, hvis den allerede står for tæt. FLYT og CHARGE bevarer deres mål. HOLD ILD har ingen aktiv skudafstand og forkorter derfor ikke målet.

**AFBRYD — kæmpende tilbagetrækning [findes i kode; ikke afprøvet, 2026-10-09]:** Knappen i ORDRER-panelet giver kompagnier en eksplicit ordre. Enheden vender fronten, afgiver ild efter fire-politik og salvemetode, går 9 m tilbage med fronten mod fjenden, standser og skyder igen. Et nyt skridt kræver en faktisk ny salve og et mål på skudhold; genladning og formering respekteres. Figuren følger den eksisterende glidende formationssti og beholder fronten under baglæns gang, også i crowd-visning. Når ildudvekslingen er ophørt (uden for valgt eller fjendtlig aktiv rækkevidde, eller uden ammunition), går enheden videre baglæns ud af egen lange rækkevidde uden flere salvecykler. Når ingen kampdygtig fjende er inden for LONG, erstattes ordren af HOLD. En ny leveret ordre afløser den; flugt/ødelæggelse afslutter den. Status er **AFBRYDER** i tag og enhedspanel. Virker også med officer-AI slået fra. 9 m og 0,5 sekunders minimumspause er justerbare kodeestimater, ikke historiske målinger.

**Destinationsboksen** viser kampformationen ved målet, også under march i kolonne: linjens bredde beregnes af antal mand, normal afstand og rækker; rytteri viser kavalerilinje. Karré vises kun, når enheden allerede står i og beholder karré. Boksen følger ordrens frontretning og har frontpil og enhedens navn; den stiplede rute går gennem vejpunkterne til boksen.

### 2.2 Marchen i detaljer [delvist]

1. **Ren FLYT:** ingen ild under marchordren; indgående ild/tab afløser den med HOLD og tillader derefter svarild efter fire-politikken. Resterende rute over 120 m går i kolonne, 4 i bredden, hvis fjenden ikke allerede er nær. Til og med 120 m bevares kampformationen; vejpunkter tæller med i rutelængden.
2. **Fjende i syne inden for rækkevidde + 35 m:** kolonnen opløses, kompagniet går i linje og går **det sidste stykke i formation**.
3. **Under 25 m fra målet:** samme, så det ankommer i orden.
4. **Defilé** (bro, smal vej): 2 i bredden; enheden forlader selv defilé, når den er igennem.
5. **Drejning:** fodfolkets simulerede front drejer op til 36°/s (rytteri 80°/s). Historikken beskriver **30°-reglen**: små sving som fløjsving, større sving ved gang til nye pladser. Den aktuelle fælles `StrategyVisualFormationPath.h` har dog ingen 30°-grænse: den bruger pivot ved ikke-kolonnedrejning og begrænser visuel fart efter formationens radius. Overensstemmelsen med reglen og grafikspring skal kontrolleres; den er ikke verificeret som aktuel kodeadfærd.

### 2.3 Under ild uden fjende i nærheden [findes i kode; ikke afprøvet]

Når kompagniet **står stille**, det modtager fjernild fra artilleri, mortérer eller infanteri over 150 m (også uden tab), og **ingen fjende er inden for 150 m:**

- En **god officer** (initiativ og ro over ca. 60) beordrer **spredt orden**: mændene breder sig (større afstand) og **lægger sig ned**. Det gør dem sværere at ramme (mindre træf pr. skud), men de skyder langsommere og kan ikke gå af sted med det samme.
- En **middel officer** gør det efter nogle sekunders ild.
- En **dårlig officer** gør det ikke; mændene står og tager tab.
- **Afslutning:** ilden holder op i 10 sek., eller en fjende kommer inden for 150 m, eller der kommer en ordre om at rykke. Så rejser de sig og går tilbage til den tidligere formation.
- **Grænser:** kan ikke kombineres med karré eller bajonetangreb. Kavaleriet lægger sig ikke. Artilleriet kan ikke.

Implementeret 2026-10-08 i `StrategyFieldOfficerComponent::UpdateAutomaticLooseOrderUnderFire`: initiativ og ro over 60 giver 3 sekunders reaktion; 40–60 giver 8 sekunder; under 40 reagerer ikke (laveste egenskab, justeret for svækkelse). Formationsafstande fordobles; liggende/spredt orden giver −40 % træfchance og dobbelt genladningstid. Rejsning forsinker march 3 sekunder. Stance og formationsafstande genbruges, mens udskilte skytter bevares. Tiderne er justerbare balanceestimater. Merget og bygget ifølge historikken; reaktionen er ikke afprøvet i et rigtigt slag.

---

## 3. Rytteri

| Tilstand | Udløser | Handling |
|---|---|---|
| **Marchkolonne** | Lang flytning | 4 i bredden (defilé: 2) |
| **Linje** | Nær fjenden eller på ordre | Knæ ved knæ, 1–2 rækker |
| **Skærm** (`CavalryScreenAI`) | Spejdeordre | Holder afstand, ser, melder; angriber ikke |
| **Angreb** (`CavalryCharge`) | Angrebsordre, eller officeren vælger åbent mål under eksisterende fremryknings-/angrebsordre (400–700 m efter aggression) | Galop frem, kontakt |
| **Afstigning** (dragoner) | Ordre eller knap | Stiger af, skyder som fodfolk; kegle vises først da |
| **Tilbagetrækning** | Tab og moral som ved fodfolk | Samler sig bag egne linjer |

**Angrebets udfald** afhænger af:

- **Rytteriets styrke** og fart.
- **Retning:** flanke og ryg er langt værre for målet end front.
- **Målets formation:** mod **linje og kolonne** rammer angrebet fuldt og er dødbringende; mod en **fast karré** (god moral, samhørighed og ammunition) afviser karréen angrebet: hestene vægrer sig 30–50 m fra, rytteriet tager tab fra ilden og mister fart og moral; en **rystet karré** (lav moral eller samhørighed, tom for ammunition) kan brydes.
- Efter angrebet: hold, hvis det blev afvist; ellers fortsætter officeren efter egne regler. [kampregelpakken, 2026-10-08]

---

## 4. Karré og trusler fra rytteri

### Udløser [implementeret; kamptest mangler]

- Implementeret: synligt, kampdygtigt rytteri skal nærme sig med mindst 7 m/s lukningsfart. Kun nærmeste kampdygtige kompagni på angrebslinjen vælges (retningsprikprodukt mindst 0,95; korridor med 25 m halv bredde).
- Varslingsafstand: **min(350 m, lukningsfart × 25 sek.)**, fx 300 m ved 12 m/s. Eksplicit formationsordre fra spilleren reducerer nødreaktionens afstand til 65 %. Stillestående eller bortvendt rytteri udløser ikke karré. Rigtig kamptest mangler.

### Reaktionen

1. **Kun det kompagni, rytteriet er på vej mod**, reagerer (det nærmeste på angrebslinjen). Naboerne danner kun karré, hvis rytteriet vender mod dem.
2. **Kolonne går direkte til karré** uden mellemtrin.
3. **Bajonetterne sættes på** samtidig. [findes ved karré fra kampregelpakken; prøves]
4. Dannelsen tager som udgangspunkt **25 sek.**, påvirket af forskning; enheden er sårbar og skyder ikke under omformering.
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

- **HQ følger sine kompagnier**, også gennem underlagte HQ'er: **60 m bataljon / 120 m regiment / 250 m brigade / 400 m division** bag formationen. Følgning bruger den tegnede bagkant og går uden om formationsaftryk; direkte spillerordre om HQ-flytning har forrang. Seneste afstande er merget, men ikke bygget eller afprøvet.
- **Ordreforløb:** spillerordre → ordonnans fra staben (synlig rytter, afstandsafhængig levering) → officerens læseforsinkelse → udførelse. Direkte spillerflytning bevarer det præcise mål; øvrig officerfortolkning afhænger af ordrevejen. Vejpunkter følger levering og læsetid; annullering ved stop-og-skyd rydder også ventende udvidelser af den gamle rute.
- **Chefens valg**: hvor mange kompagnier der angriber, hvem der er ildbase, hvem der flankerer, og om der holdes reserve (se 7).

---

## 7. Hvordan AI'en bruger sine tropper

### 7.1 Angreb med flere kompagnier [findes]

1. Chefen deler gruppen: **ét kompagni er ildbase** (midten), **to flankerer** (hver sin side).
2. Ildbasen rykker til skudafstand og holder og skyder. Stop for ild efter fire-politik og rækkevidde annullerer fremryknings-/angrebsordren og alle vejpunkter; destinationsboks og rute forsvinder. Enheden bliver stående, også når fjenden forlader skudhold, indtil spilleren giver en ny ordre. Ren FLYT standser ikke alene ved synet af en fjende, men annulleres ved indgående salve eller tab; TILBAGE og AFBRYD fortsætter.
3. Flankerne **går udenom ildlinjen** (ikke gennem egen ild) og **ind fra siden**, med vinkel 45° og 90° efter chefens taktik.
4. Gode chefer sender flankerne **videre ud**; dårlige holder dem tæt på og rykker frem på linje.
5. **Sidetrin** bruges, når en flytning er under 90 m med fjenden tæt foran.
6. **Reserve:** forsigtige chefer med 4+ kompagnier holder det bageste kompagni 90 m bag ildbasen og sætter det ind, når basen er såret (under 75 %), moralen er under 55, eller tiden er gået.

### 7.2 Forsvar [delvist]

- **Hold stillingen**, vend fronten mod fjenden, skyd efter fire-politik.
- **Bajonetangreb** mod en vaklende fjende nær skudhold.
- **Tilbagetrækning** i orden, når tab eller moral siger det; reserven dækker.
- [findes i kode; ikke afprøvet] **Spredt orden og ned** under artilleriild (2.3).

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
| Rytteritrussel (karré) | min(350 m, lukningsfart × 25 sek.) | `ThreatReaction` |
| Karré slippes efter | 8 sek. | `ThreatReaction.SquareReleaseDelaySeconds` |
| Drejehastighed fodfolk / rytteri | 36° / 80° pr. sek. | `MovementExecutor.TurnSpeedDegreesPerSecond` |
| Kolonnebredde fodfolk / rytteri / defilé | 4 / 4 / 2 | `FormationComponent` |
| Skudandel pr. karréside | 25 % (30 % med forskning) | `squareFaceFireShare` |
| Reserve efter | 240 sek. | `FieldOfficer` |

---

## 9. Åbne punkter til afklaring

1. **Spredt orden og ned** (2.3): implementeret med initiativ/ro, −40 % træf og dobbelt genladningstid; afprøv reaktionstid, rejsning og karréprioritet i spillet.
2. **Karréens udløser** (4): afprøv den implementerede fartafhængige reaktion og rytteri mod fast/svækket karré i et rigtigt slag.
3. **March og ild:** afprøv ren FLYT forbi fjenden og fremrykning/angreb gennem flere salver. Stop-og-skyd annullerer nu ruten; enheden holder også efter målet forsvinder. HoldFire tillader ikke ildstoppet.
4. **Mændenes gang ved drejning:** afprøv sving over/under 30°, crowd-skift og figurskala; kontrollér hop i grafikken samt flag og HQ under march, stop og sving.
5. **HQ og floder:** HQ'et går i dag direkte og passerer floder og hegn (C-20 fra gennemgangen).

## 10. Betjening og dokumenteret teststatus

- **ALT+højreklik** udvider march-/fremryknings-/angrebsruten; almindeligt højreklik erstatter den. Træk angiver slutfronten. Gruppens forskydning bevares; STOP/HOLD rydder køen. HUD viser nummererede punkter, stiplet rute og kampformationens aftryk ved målet.
- **Grafik:** LAV/MIDDEL/HØJ vælger kvalitet og 70/85/100 % renderopløsning. **MIDDEL** er standard. Figurskala er selvstændig: 1/2/5/10 mænd pr. figur, **1:1** som standard; gemte valg kan ændre begge. Færre figurer giver kompakte tegnede formationer og kegler, mens simulationens mandtal og ildfelt bevares.
- **Tid:** PAUSE/FORTSÆT, x½, x1, x2, x3, x5, x10. Kameraet kan flyttes under pause og følger realtid. Slaguret har egen omregning; historikken målte 3/6/30 slagsekunder pr. realtidssekund ved x½/x1/x5.
- **HUD:** fem bundpaneler: ENHED, LEDELSE & ILD, ORDRER, FORMATION, UNDERLAGTE; underlagte kan vælges og rulles. Keglen skjules i kolonne/omformering, mens enhedstag viser march/formering/ild/hold.

Kilde: `IMPLEMENTATION-AND-FIX-HISTORY.md`, sammenholdt med koden. Ingen nye spiltests ved denne dokumentopdatering.

| Status | Dokumenteret kontrol |
|---|---|
| Visuelt afprøvet i Duel-testslag | Kolonne til 133 m, linje, stop ved 65 m, salve/røg, knæ/ladning, rejsning/sigte, faldne og faner. Det beviser den tidligere adfærd, ikke de nyeste ændringer. |
| Skærmbilledkontrol | Tidligere slagvisning/tidsknapper, startmenuens levende Danmarkskort, kamporden og forskning. Intet dokumenteret fuldt slag. |
| Observeret i log | Husarernes charge/“Chok!” omkring 450 m og slagurets x½/x1/x5. Karréafvisning er ikke dermed afprøvet. |
| Merget og bygget; kamptest mangler | Skov/LOS, events, enhedstilpasning og spredt orden/nedlægning. Karré-/kavalerireglernes fulde forløb er ikke dokumenteret testet. |
| Merget; seneste version ikke bygget eller testet | Stop-og-skyd med annullering, kampformationsboks, fempanel-HUD og stabsafstandene 60/120/250/400 m. |
| Merget; runtime-kontrol udestår | ALT-vejpunkter, kompakte figurer, 30°-sving, naturligere gang, flag/HQ-følgning og x10. Merge alene er ikke bevis for build eller bestået test. |
