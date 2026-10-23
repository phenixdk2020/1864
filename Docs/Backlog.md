# Backlog: status for de 20 opgaver (campaign-kortet)

Nabokort, 2026-10-10: minimal scenarie-/kortintegration er implementeret. Se `Naboer.md` for status og resterende arbejde: build/spiltest efter anmodning, v31/v32-save-rundtur, kildeverifikation, hydrografi/vegetation, taktiske nabofæstninger og omplanlægning ved ændret passage. Generelt nationskrigssystem og fuld udenlandsk økonomi er fortsat ikke implementeret.

Status pr. 2026-10-08: de oprindelige 20 opgaver blev registreret som lavet i v00.00.50. Det betyder implementering, ikke fuld afprøvning; Sverige-Norge har fortsat kun forberedelse, og søtransport mangler. Detaljerne står i de nævnte dokumenter.

| # | Opgave | Dokument |
|---|---|---|
| 1 | Rekognoscering og tåge | Intel1851.md |
| 2 | Fjendens AI | Intel1851.md |
| 3 | Flåden | Navy1851.md |
| 4 | Vejr og føre | Weather1851.md |
| 5 | Sygdom og lazaretter | Health1851.md |
| 6 | Officerer ældes | Army1851.md |
| 7 | Ministre og politik | Politics1851.md |
| 8 | Stormagternes AI | Politics1851.md |
| 9 | Fredsforhandling med vilkår | Politics1851.md |
| 10 | Handelsvarer og markedspriser | Economy1851.md |
| 11 | Opinionen | Politics1851.md |
| 12 | Statsgæld og lån | Economy1851.md |
| 13 | De historiske fæstningsværker | Siege1851.md |
| 14 | Belejring | Siege1851.md |
| 15 | Krigsfanger og udveksling | Health1851.md |
| 16 | Avisen | Economy1851.md |
| 17 | Statistik og grafer | Economy1851.md |
| 18 | Opslagsværk | Economy1851.md |
| 19 | Sverige-Norge som spilbar nation (forberedelse) | Endgame1851.md |
| 20 | Balance og slutning | Endgame1851.md |

## Ikke testet endnu

- Spredt orden/nedlægning og rejsning under fjernild.
- Rytteri mod fast/svækket karré i et rigtigt slag; charge er set i loggen, men udfaldet er ikke bekræftet.
- Stop-og-skyd med annullering, ny fempanel-HUD og stabsafstande: seneste merge er hverken bygget eller testet.
- ALT-vejpunkter, destinationsboks, 30°-sving og figurskalaskift; kontrollér hop i grafikken samt flag og HQ under march, standsning og sving.
- En belejring er ikke fremprovokeret i en kørt test. Testflag og kørselsvejledning er tilføjet 2026-10-08: [Siege-Test.md](Siege-Test.md); afventer build og afprøvning.
- Søtransport af tropper er endnu ikke lavet.
- Testbeløbet i statskassen (5.000.000 rd.) skal ned på 150.000 før rigtigt spil.

## Senere (ikke prioriteret endnu)
- Sverige-Norge med eget kort, og andre landes kort (Preussen, Østrig).
- Søtransport af regimenter mellem havne.
- Jernbanemobilisering koblet til køreplaner på kortet.

## Bygget 2026-10 (kampagne og slag), ikke afprøvet i et rigtigt slag

Overskriften omfatter også implementerede/mergede ændringer; merge alene er ikke bevis for build eller test.

- Udført siden sidste opdatering: særskilte 1825-data; datadrevne events med spillerpåvirkelige betingelser; militære/civile forskningsfaner; navne, uniformpalette og våbenombygning (Events.md og UnitCustomisation.md). Skov i sigtelinjen, skjul og dækning er merget og bygget. Runtime-forløb og balance mangler; uniformmaterialernes asset-forberedelse og visuelle kontrol udestår.
- Implementeret: karré med fire 90°-ildzoner og bajonetter, fartafhængig rytteritrussel, kavalerichok og afvisning ved stabil karré. Afprøvning af hele regelforløbet mangler.
- Implementeret/merget: nyt HUD, stop-og-skyd annullering, kampformationsboks, ALT-vejpunkter, kompakte figurer, flag/HQ-følgning og naturligere gang. Nyeste HUD/annullering/stabsafstande er endnu ikke bygget.
- Startmenu med levende Danmarkskort og testknapper er lavet; menu og tidligere slagvisning er kontrolleret med skærmbilleder. Standard: MIDDEL grafik og selvstændig figurskala 1:1. Tid: pause, x½, x1, x2, x3, x5, x10.
- Scenarier: 1825 (standard) og 1851 (vælges i spilmenuen); gem gemmer scenariet.
- Forskning: åbner efter år i 1825, militær og civil i to spor med faner; Jernbaneanlæg kræver forskning.
- Kamporden: organisationsdiagram, kompagnier foldes ud, enhedskort pr. kompagni, styrke pr. kompagni og overførsel af mænd (også eskadroner og på tværs af enheder).
- Kort: Bornholm og grov verdensflade; naboer til Danmark skal detaljeres senere.
- Slag: sejrspoint, ordonnanser (synlige ryttere) og officerers tolkning af ordrer, slagur og tid på dagen, højst 3 dage, forstærkninger dag 2 og 3.
- Officerer: såret/fanget med løsesum, prestige og moral; fanget chef giver enheden lavere moral.
- Taktik: holdning- og afsidningsknapper, flankering for begge sider efter lederens egenskaber, sidetrin, reserve-kompagni.
- Testslag: Start-Test-1 (kompagni mod kompagni), Start-Test-2 (bataljon med reserve mod kompagni) og Start-Test-3 (rytteri); præcise filnavne og flag i TestFlags.md.
- Ikke prøvet endnu: højreklik-kommando med retningspil, skift af scenarie i menuen, dragonernes afsidning, de to testslag fra ende til anden, "LG 1"-etiketten der sidder for langt til venstre, danske reserver der ankommer.
- Udført siden denne listes første version: fjern overordnet stab i delingsbilledet samt særskilte 1825-data for hær, officerer, byer og nationer. Historisk validering/balance udestår. Åbent ønske: flere civile forskningsemner (post, dampskibe) med reel effekt.

## Testslag (midlertidigt)
- Knapperne "TEST 1 MOD 1", "TEST 4 MOD 1" og "TEST RYTTERI" i startmenuen, med det levende Danmarkskort som baggrund. De første to starter passivt; rytteri-testens husarer får en fremrykningsordre. Fjernes igen, når testene er færdige.
- Derefter større testslag: to bataljoner under regiment-HQ, brigade og division ovenpå, flere fjendtlige kompagnier (flankering, reserver, ordonnanser) og forstærkninger dag 2 og 3.

## Kamp-AI design v2.0 (2026-10-09)

Designgrundlaget for kamp-AI'en ligger i `Docs/Kamp-AI-Design-v2.0.md` (ordre med latency og betingelser, kontakter med alder og tillid, mission/reaktion med SuspendedMission, combat slots og lanes, ildfelter, artilleribeskyttelse, kavaleri, epoke- og udstyrsprofiler som data, Auto-toggle pr. kommandonode, ydelsesbudget, debug og acceptscenarier T1-T10). Rækkefølge: fase 1 kontrakter, fase 2 vertikal skive (1 mod 1, bataljon, debug, T1-T3 og T7, auto-toggle), playtest, derefter artilleri, kavaleri og morter, til sidst brigade/division og betingede ordrer. Afstemning med det byggede (Enhedsadfaerd1864.md) mangler.

## After action report (implementeret 2026-10-10; spiltest afventer)

Tabsregnskab, rapport efter 3D-slag, kampagneoverførsel, tjenestehistorik og officerskarriere er implementeret. Se `Docs/AfterAction.md` for felter, spilestimater og den korte test. Automatisk afgørelse har en rapport med tydeligt markerede, umålte felter. Officerer kan kun såres eller tages til fange i slag. Spil-/editorverifikation og et egentligt tidsforløb for moral/cohesion afventer.

## Uniformbilleder i 1825 (2026-10-10)

Indkaldelsesvinduet (Indkald en ny enhed) og enhedskortet viser nu 1851-uniformbillederne også i 1825 (de var skjult i 1825 og viste kun en streg-figur). Billederne er fra 1851 og passer ikke historisk til 1825 (glatløbet musket, andre uniformer): se på det, og lav eller skaf 1825-billeder (linje, jæger, dragon, artilleri, rytteri) pr. våben.

## After action report skriver historik (2026-10-10)

Rapporten (se ovenfor) skal også opdatere enhedens tjenestehistorik (`FCampaign1851Regiment` service record: slag, sted, dato, mand ved start, faldne, sårede, fanger, erobret og tabt udstyr, holdt feltet eller trak sig, ammunition) og totalerne (TotalKilled, TotalWounded, TotalCaptured, TotalEnemyKilled), og hver officers karriere (`Campaign1851Career.cpp`: deltaget i slaget, såret, fanget, udmærkelse). Rapporten føres ind i historikken ved AFSLUT SLAGET, ikke kun vist.

## Beslutning: officerer dør ikke i kamp (2026-10-10)

En officer kan ikke dø i slaget. Han kan blive såret (ude af tjeneste til han er rask) eller fanget (tilbage ved udveksling), men ikke falde. Derfor skal vi ikke bruge tusindvis af officerportrætter endnu; de 96 + 25 ministre er nok. Rapportens "officerer faldet" udgår (kun såret og fanget). Tages op igen, hvis vi senere vil have officersdød og et stort portrætbibliotek.

## Erfaring: hvordan den optjenes og bruges (2026-10-10, til gennemgang)

Se `Docs/Erfaring.md` (beskrivelse af nuværende regler og forslag). Kort: enhedens erfaring stiger i træning (op til 40), på march (0,02 pr. dag op til 70) og ved slag (+5, flat); officerens stiger 0,05 pr. dag på march, 0,01 ellers og +6 ved sejr/+3 ved nederlag. I 3D-slaget bruges enhedens erfaring til nøjagtighed/moral-chok (±15-20 pct), officerens til stress-reaktion og beslutningsstabilitet. Mangler: erfaring pr. våbenfærdighed ved brug, pris for nederlag, bonus for veteraner i kampagnen, officerens erfaring påvirker enhedens bonus, udmærkelser.


### Udført 2026-10-10 — Kort og indkaldelse

- [x] Permanent kortvisningsbjælke med aktiv markering, uafhængig af Bornholm-inset; kontroltags tilbage på kortet.
- [x] Skansebillboards med Star_Fort-ikon, nærzoomnavn og garnison/kanoner-tooltip på tværs af kortvisninger.
- [x] Københavns to broer samlet ved krydsningen med 200 m afstand og forskudte labels.
- [x] Klikbart indkaldelsespreview med ressourcer, officerer, færdigheder, uddannelse, budget og kasernekapacitet; klik udenfor lukker.
- [ ] Visuel kontrol af de fire ændringer i spillet ved forskellige zoomniveauer og vinduesstørrelser (ikke startet som del af opgaven).

## Naboerne inddeles i amter som Danmark (2026-10-10)

Alle nabolande skal have samme inddeling som Danmark: amter/distrikter med grænser på ejergrid, befolkning, rekrutter, skat og byer. Rækkefølge: 1) syd (Mecklenburg-Schwerin, Preussen/Pommern; Codex-opgaven "south" er i gang: Aemter, Regierungsbezirke, Kreise), 2) Hannover, Oldenburg, Hamborg, Luebeck, Bremen, 3) Sverige (laen: Malmoehus, Kristianstad, Blekinge, Halland, Goeteborg og Bohus, Aelvsborg, Skaraborg, Vaermland m.fl.) og Norge (amter: Smaalenene, Akershus, Hedemarken m.fl.), 4) Preussen/Østrig videre ned (Brandenburg, Schlesien), Rusland/England kun som abstrakt model. Samme datastruktur som Danmarks amter (amter_1825.json / amter1851.json) så alt (manpower, skat, forsyning) virker ens.

## Gammel liste gennemgået (2026-10-10)

| # | Punkt | Status |
|---|---|---|
| 2 | Slagmarkens grafik 100 gange bedre | Ikke taget som egen opgave. Der er lavet: græs, træer, skygger, røg, bagte figurer (under rettelse). Mangler: terrænteksturer, vand, bygninger i slag, vejr/lys, effekter. |
| 3 | Campaign map endnu bedre grafik | Delvist (3D-huse, broer, byer, nye bygningsbilleder). Mangler en samlet grafikrunde. |
| 4 | Se slag, sårede, dræbte, dræbte fjender pr. kompagni (kort og OOB) | Enhedskortet viser slag, faldne, sårede, fanget og de seneste slag. Mangler: dræbte fjender (TotalEnemyKilled vises ikke), og samme tal i Kamporden-boksene. |
| 5 | Erfaring, leder, stats, ildmetoder | Enhedskortet har erfaring, leder, seks færdigheder og øvelser; ildmetoder trænes efter forskning. Mangler: erfaring pr. kampsituation (Docs/Erfaring.md). |
| 6 | Fane og uniform som billede af en soldat i de rigtige farver | Uniformbillede findes (1851-billeder) og farvevalg gemmes; 3D-farver virker først når de bagte figurer er rettet. Mangler: fanen på kortet og soldaten i valgte farver i kortet; 1825-billeder. |
| 7 | Sårede og hvornår de kommer tilbage | Gjort: enhedskortet viser antal på lazaret og hvornår halvdelen er tilbage. Mangler: erfaring bevares for de sårede (puljeregel). |
| 8 | Opgradér batteri til ridende batteri | Ikke lavet: opgradering findes til riflet kanon, ikke fodbatteri til ridende batteri (heste, mandskab på heste, pris, uddannelsestid). |
| 9 | Forskning: landbrug, smede m.fl. mere effektive | Gjort: civil forskning udvidet (landboreformer, mergling, smede og redskaber, kvæg, dampmaskiner med mere). |

## Jernbane som indkomst (2026-10-11)
- Tog og togstrækninger bidrager endnu ikke til økonomien. Forslag: passagerindtægt (efter byernes befolkning og linjelængde), fragtindtægt (varer fra overskuds- til underskudsbyer), drift/vedligehold som udgift, og nedsat indtægt når linjen bruges til tropper eller er afbrudt (station/bro). Kode: Campaign1851Network.cpp, Campaign1851Army.cpp (ResetTroopTrains), økonomi i Campaign1851Resources.cpp.

## Ønsker 2026-10-13 (til planlægning)

- **Erobre faner:** en fane kan erobres; fanebærer med egen animation (bærer, falder, taber fanen); en ny soldat samler fanen op igen. Erobret fane tæller i rapporten (F) og i moral/prestige. Kræver animationer til fanebærer (hold fanen, falde, samle op).
- **Nye lade-animationer** til gevær (forladning/slagrør/bagladning, stående/knælende/liggende) og kanon/morter.
- **Køb af våben og udstyr fra andre** (andre nationer og private; pris, leveringstid, kvalitet; kobles til handel/diplomati).
- **Bygninger der selv fremstiller udstyr:** mængde pr. måned (geværer, kanoner, ammunition, sadler, vogne) efter bygning, arbejdere og råvarer.
- **Kavaleri og dragoner med sabler:** dragoner har gevær, når de sidder af, og sabel, når de sidder op igen (test af skift); sabel i hånden i charge.
- **Fanebærer-animationer** (se erobre faner).
- **Rapport efter slaget:** ammunition og mad med (ikoner mangler), nationsemblemer ved totalerne, sejrspoint og sejrsgrad (uafgjort, delvis, sejr, stor sejr; taberen trækker sig ved stor sejr).
- **Dækning og stilling i træfsandsynlighed:** se Docs/Kamp-AI-Design-v2.0.md (knælende/liggende rammes sværere og rammer bedre; dækning i terræn).

## Fund fra 4-mod-1-testene (2026-10-13)

- **4 mod 1 med AI-officer (HoldReserve, SkirmishAttack):** virker: ordre ANGRIB fra bataljons-HQ til alle fire kompagnier, march i kolonne, linje ved udfoldningsgrænsen (195 m), et kompagni "i reserve", et "går udenom egen ild", et "ildbasen: holder og skyder", flanke med sidetrin, salver. Fejl fundet og rettet: kompagnierne endte oven i hinanden (se historikken).
- **4 mod 1 + rytteri (SkirmishArms):** rytteriet angriber aldrig. Eskadronen afviser charge hele slaget ("Ingen eksisterende åbningsregel opfyldt", unit-loggen DK-SKIRMISH-SQN), fordi den ingen angrebsordre får fra bataljonskæden, og fordi reglen kun åbner for linje-angreb under en eksplicit offensiv mission. Fjendens kompagni går derfor aldrig i karré, og der er ingen flanke-/bagcharge på et kompagni, der holdes fast af egen ild.
- **Manglende AI (til planlægning):** (1) rytteri-tasking: en eskadron under en bataljon skal vælge et fastholdt fjendtligt kompagni (rammet af egen ild inden for de sidste sekunder) som mål, ride om på flanke/bagside og chargere, når afstand og vinkel er rigtige og ingen egne står i banen; (2) trusselsvurdering i AI-officeren: rangér fjendtlige enheder efter fare (størrelse, afstand, retning, kavaleri, artilleri) og vælg derefter, hvilke egne enheder der holdes i reserve, og hvornår de sættes ind (fx når ildbasen svækkes eller flanken åbner); (3) sidestep for at komme fri af egen ild er der (loggen: "går udenom egen ild", "sidetrin"), men skal testes systematisk med flere kompagnier i ildlinjen.

## Kanonbatteriet: animationer, forstilling og heste (2026-10-13)

- **Findes:** forstilling/afprotsning som logik og tider (12 s ned, 15 s op, ARTILLERY-DEPLOY består), skubning af kanon uden forspand, marchstilling med kanonerne efter hinanden og pjecen bagud.
- **Mangler:** (1) selve kanonen som prop: drejning ved sigte, rekyl ved skud, hjul der drejer under kørsel; (2) en synlig **forstilling** (lille tohjulet vogn) med to til seks heste foran, som kobles til kanonens hale ved opprotsning og af ved afprotsning, samt kusk; (3) besætningens animationer (punkt 25-31 i Docs/Manglende-Animationer.md): lade (svabre, ladning, kugle, sætte for), sigte, fyre af og træde tilbage, skubbe kanonen, spænde for/af, morterbetjening; (4) heste til forstilling (kræver at hestene er bagt/rigget, se Codex-opgaven "bakeall").
- **Rækkefølge:** kanonens bevægelser og enkel forstilling med heste først (ingen menneskelig animation nødvendig), derefter besætningens animationer (leveres, eller HY-Motion på de enkle), til sidst koblingsanimationen.

- **Lade-animationer til gammeldags gevær:** stående, knælende og liggende i to våbentyper (forlader og bagladning), delt i faser med markører så de kan strækkes til epokens ladetid; se Docs/Manglende-Animationer.md afsnit 6. Kobling til ladetid og lyd kommer bagefter.

## Næste opgaver efter test af animationer og formationer (2026-10-22)

Brugeren tester først animationer og formationer i slaget. Derefter i denne rækkefølge, med målet at kampagnekortet nærmer sig referencebilledet (Codex' byillustrationer i `Reference/Campaign1851/Scenery/DK/Towns/`):

1. **Lys:** gør kortet og bygningerne belyste med ægte sollys, bløde skygger og skylys. Erstat de sorte, uigennemsigtige jordskygger med bløde, halvgennemsigtige. Tjek ydelsen med 250.000 træer.
2. **Farver:** dæmpet, jordnær palet (halmgul, olivengrøn, brændt tegl) via farvekorrektion og blødere farver i kortfliserne.
3. **Byernes opbygning:** huse langs gader og omkring et torv ved kirken, med gårdspladser, haver, stakitter og brønde i stedet for spredte rækker.
4. **Jorden tæt på:** detaljeteksturer (græs, stub, plovfurer), læhegn i 3D langs markskel (brug brikken til jorddiger) og bløde vejkanter.
5. **Flere gårdtyper efter region** (research: `Docs/Bygninger-DK-Research-2026-10-21.md`): tre- og tolænget gård (overalt, blandet med firlænget i Østdanmark), parallelgård (Nordjylland), vestjysk langhus (Syd- og Vestjylland), haubarg (marsken i Slesvig), niedersachsenhaus (gesten i Holsten og Lauenburg). Rækkefølge: billeder fra Codex, godkendelse, modeller fra Blender-agenten, valg efter region i koden.
6. **Mere detaljerede bygninger tæt på:** strå- og tegltag med struktur og sprossevinduer; de nuværende modeller bruges længere væk.
7. Bagefter de svenske, norske og preussiske byggestile.

Enhedskameraet er lavet 2026-10-22 (G / Shift+G / FØLG).

## Formationer og animations-QA (2026-10-22)

- ~~Kolonnens sving~~ (løst 2026-10-22: marchkolonnen går i en bue på 3 m).
- ~~Ankomst med ny front~~ (løst 2026-10-22: række for række og intet skub fra stående mænd). Tilbage: mænd passerer hinanden ned til 7–22 cm undervejs.
- **KOLONNE stillestående** ligner en linje, til kompagniet marcherer (slangen dannes først i bevægelse).
- **Linjens svingning:** den stive linje drejer om midten, og fløjen farer 14 m/s rundt ved march-start (se svingningsplanen fra 2026-10-21).
- **Copilots P1/P2-punkter (ikke gjort):**
  - svingning af linjen om et flydende omdrejningspunkt (forsøgt 2026-10-21, stoppet);
  - overgang til og fra karré;
  - underenheder (sektioner);
  - fodfart mod animation;
  - drejeanimationer;
  - lokal undvigelse af forhindringer;
  - lidt menneskelig uregelmæssighed i formationen.
- **Animations-QA (brugerønske):** samme metode som formationerne, med testforløb kørt af kommandoer, målinger hvert sekund og grænseværdier. Målinger:
  - håndflade til gevær/fanestang i alle klip;
  - fodens glidning mod jorden;
  - mundingens retning mod målet ved skud;
  - ladefasernes rækkefølge og at manden står stille;
  - faldne der ligger på jorden og ikke inde i en kammerat;
  - tid til geleddet er lukket.

## Større omorganisering efter tab (eksercerreglementet 1863)
Ved store tab sammendrog officererne kompagniet ved et ophold i kampen til færre delinger (eller sammenlagde kompagnier i bataljonen). Mangler: et "omorganiser"-trin, når et kompagni er under ca. 50 % og står stille, og senere sammenlægning af kompagnier i bataljonen.

## Kendte fejl og næste trin (2026-10-23)
- **Sammenrykning under march:** to mænd i bageste geled kan gå 9 cm fra hinanden i op til 6 s, når en flankemand lige er sendt et geled tilbage.
- **Lys-forsøg** (plan punkt 1): lit materiale på de modellerede bygninger, sol og skylys, bløde skygger; FPS-måling før og efter.
- **Preussens bygninger** (Codex-kvote); derefter modeller for SE/NO/PR og valg af bygningssæt pr. land på kortet.
- **Dæmpet palet** i `build_map.py` og ny maling af fliserne (plan punkt 2, anden del).

## Udskudte opgaver 2026-10-20

Slaget:
- **Fanebæreren skal afprøves målrettet:** tæt kamera forfra og fra siden, både i linje og i kolonne. Med `-Strategy1864DrillTest=...:KILLCOLOURS` og billeder hvert sekund kontrolleres:
  - at han står foran forreste geled og forrest i kolonnen;
  - at han bruger klippene `ColourWalk`/`ColourIdle`, med stangen i hånden og 12° bagud;
  - at fanen ligger på jorden, mens den er nede;
  - at afløseren smider sit gevær og rejser fanen;
  - at moralen giver −5 og +3 i loggen.
  Faldet og afløsningen er allerede set i loggen (2026-10-19).
- **Fanebærerens plads (brugerens beslutning, 2026-10-20):**
  - I linjen står han ikke foran linjen, men som nr. 1 i 1. geled i højre forreste hjørne.
  - Linje til kolonne sker altid ved højre om. Fanebæreren bliver derfor forreste mand i venstre række i teten og går først i slangen.
  - Fra kolonne til linje bliver teten igen højre fløj, og han står igen som nr. 1 i 1. geled.
  - Konsekvens for koden: eksercitsen skal altid gå ud fra højre fløj. I dag vælges den fløj, der ligger nærmest målet (`DrillEnterFlank` i `PlanColumnDrill`).
  - Eksercitsen efter brugerens skitse (`Reference/Drill/Linje_Kolonne_Linje_Brugerskitse_2026-10-20.png`):
    - Linjen gør højre om, og kolonnen går ud fra højre fløj som en slange.
    - Ved målet kommer kolonnen ind fra den nye linjes venstre ende og går langs linjens akse, til teten når højre ende. Så gør alle venstre om, hvor de står.
    - Ligger målet så kolonnen ellers ville komme ind fra højre, slår kolonnen et sving rundt, så den altid kommer ind fra venstre.
  - **Ny beslutning (2026-10-20), erstatter "nr. 1 i 1. geled":**
    - Fanebæreren (F) og en officer (O) står uden for gitteret.
    - I linjen står F lige til højre for højre fløj ud for 1. geled, og O står yderst til højre.
    - I kolonnen går O forrest foran midterrækken, og F går lige bag ham.
  - **Faner (valgmulighed 3):**
    - Én rigtig bataljonsfane, båret af fanekompagniet. Den kan erobres, påvirker moralen og tæller i rapporten.
    - De øvrige kompagnier får et mindre kendemærke. Den historiske praksis tjekkes af `research`.
    - Alle kompagnier får en synlig officer med sabel. Billeder til DK/SE/NO/PR laves af Codex 23 (2026-10-20); 3D-figuren laves derefter af `model3d` med Hunyuan.
  - Fejl set i brugerens billede 2026-10-20: fanebæreren stod inde i forreste geled midt i linjen, og fanedugen gik ind i soldaterne. Et smidt gevær stod på skrå i stedet for at ligge på jorden.
- **Svingning med flydende omdrejningspunkt (kun linje, ikke kolonne):** første forsøg blev rullet tilbage 2026-10-20, fordi det blev værre. Før næste forsøg:
  - planlægges målet ved ankomst bedre (det sidste ben fik ikke sit startpunkt, så linjen drejede stadig på stedet til sidst);
  - afprøves små drejninger under march for sig.
  Referencen ligger i `R:\Onedrive\AI\Kompagni_3Geledder_Wheeling_Floating_Pivot_Samlet.zip`.
- **Krav (brugeren, 2026-10-21): ingen mand må forsvinde fra sin plads og dukke op et andet sted.** Når et hul lukkes efter tab, skal man se manden gå derhen.
  - Årsag i dag: `StepUpFiles` (`StrategyInfantryVisualComponent.cpp:971`) sætter den nye plads direkte (`Slot.X`). Kompagniet er stift, så manden hopper ca. 90 cm frem.
  - Løsning: animér det som "slut op mod fanen" (`CloseFrom`/`CloseTo`, `UpdateCloseRanks`). Manden går frem med gangklippet og front mod fjenden, og bølgen går fil for fil.
  - Det samme gælder, når en ny fanebærer træder ud af geleddet, og alle andre steder hvor en plads ændres.
- **Fejl set i videooptagelse 2026-10-21:** efter ankomsten i linje forsvinder alle soldaterne i ca. 20 s, mens kameraet er tæt på; kun fanebæreren ses. De dukker op igen ved næste tab. Mistanke: de bagte crowd-figurers grænser opdateres ikke efter eksercitsen, så de bliver skjult (culling) ved et tæt kamera. Se `Saved/Codex/gap_check.png`.
- **Enhedskamera (brugerønske 2026-10-21):** lavet 2026-10-22: G følger, Shift+G følger bag enheden, FØLG-knap i enhedspanelet. Afventer brugerens spiltest.
- **Video af kamp (brugerønske 2026-10-21):** to kompagnier i ildkamp, tæt på: ladning, sigte, skud, mænd der falder og rækker der fyldes op. Brug `-Strategy1864Record` og `-Strategy1864FollowCam`.
- **Tab pr. geled:** geværild forfra skal ramme forreste geled oftest, 2. geled en gang imellem og 3. næsten aldrig (ca. 78/20/2 %, skøn). Referencen er `Kompagni_1864_3Geledder_Historisk_Tab_Reform_Samlet.zip`. Rullet tilbage sammen med svingningen.
- **Artilleriets besætning:** referencen er `Kanon_Animationer_Begge_Rettet_Samlet.zip`.
  - Fodartilleri: fremrulning, svabring, ladning og indstødning med ladestokken i løbet, retning og skud.
  - Kørende artilleri: under kørsel er heste og limber foran og mundingen bagud; derefter aftrækning, skud og påspænding igen.
  - Kræver planlægning (figurer, klip, limber og heste); se også afsnittet om kanonbatteriet ovenfor.
- **Rekvisitter:** kanonkugle, visker og ladestok er lavet i Blender (`SourceAssets/Artillery/Props/`), men ikke importeret. Mangler:
  - kuglen skal have flere segmenter og en tydeligere jerntekstur;
  - viskerens uldhoved har et spejlet mønster og ingen totter;
  - import i Unreal med `unreal-integration`.
- **Kavaleriet:** slange i kolonne og formation som infanteriet, samt animationer til dragoner og husarer.

Kampagnen:
- **Kortudvidelsen:** se planen og `Docs/Kortfliser.md`.
  - Codex skal male de nye fliser med samme maler som Danmark, med ETOPO 2022-højder og uden søm. Kørslen er planlagt til 2026-10-20 kl. 22.17.
  - Derefter følger Codex 21: flise-terræn i C++ med LOD og flisecache, gitre over 2048, kameraet og save v41.
  - Til sidst import af fliserne i Unreal og kortbilleder.
  - Tallene i `Docs/Kortudvidelse-Research-2026-10-20.md` er skøn og skal kontrolleres mod kilderne.
- **Floder på det nye kort:** Elben (og andre floder i de nye områder) er kantede polylinjer og et hydrologisk skøn. Brug Natural Earth-floddata (`ne_10m_rivers_lake_centerlines`, kræver download) og glat forløbene.
- **Portrætter til SE/NO/PR:** SE er færdig (42) og NO næsten (40). Mangler:
  - PR (42) og NO Minister_08–09; Codex er planlagt til kl. 22.17;
  - oprydning af `_qa_*`-filerne;
  - import med `Content/Python/import_foreign_portraits.py`;
  - afprøvning i spillet af, at hver nation får sine egne portrætter.
- **Officerskarrierer og kampagneloggen:** bygget og valideret statisk, men ikke afprøvet i spillet (save v40, loggen i `Saved/CampaignLogs/`).

Værktøj:
- **Build via PowerShell** afbrød 2026-10-20 uden fejltekst (UBA). Kald `UnrealBuildTool.dll` direkte med `Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe` fra `Engine/Source`; det virker.
