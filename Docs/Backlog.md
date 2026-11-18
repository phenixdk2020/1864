# Backlog: status for de 20 opgaver (campaign-kortet)

Resterende byarbejde: Befolkningsstyret lokal kulissevækst i 1851-layoutet er ikke bekræftet. De øvrige klasser får sokler, men deres eksisterende bylog har endnu ikke sokkel-/reservationsoptælling. Afventer visuel kontrol af Bogense: sokler, veje/vand, Textile_Mill og kaserne. Dynamiske nye veje eller bygninger over allerede reserverede vækstfelter kræver yderligere kontrol.

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
- Bygget 2026-11-03, mangler spiltest: rytteri mod svækket karré og hele udfaldet i et rigtigt slag.
- Bygget 2026-10-14, mangler spiltest: stabsafstande.
- ALT-vejpunkter, destinationsboks, 30°-sving og figurskalaskift; kontrollér hop i grafikken samt flag og HQ under march, standsning og sving.
- En belejring er ikke fremprovokeret i en kørt test. Testflag og kørselsvejledning er tilføjet 2026-10-08: [Siege-Test.md](Siege-Test.md); afventer build og afprøvning.
- Søtransport af tropper er endnu ikke lavet.
- Testbeløbet i statskassen (5.000.000 rd.) skal ned på 150.000 før rigtigt spil.

## Slaget 2026-11-05 til 11-15: bygget, åbne punkter (detaljer i TASKS.md)

- **Karré:** linje ↔ karré og kolonne ↔ karré er bygget (målt 43/49 og 25/52 cm). Allan skal se `Saved/QA/2026-11-15-karre-sving/karre-sving.mp4` og `Saved/QA/2026-11-15-kolonne-karre/kolonne-karre.mp4`. Tilbage: kompagnier med tab falder tilbage til glidning, fjendens karré under rytteritrussel afvises fra linje, og en svingende kolonne afvises.
- **Ildkampen:** kun første geled skyder (`FrontRank`). Allan skal afgøre, om to-geleds ild skal være standard. Hele kompagniet skuldrer og rykker sammen efter hvert tab, og røgen er store hvide skiver.
- **Kanonbatteriet:** besætning med 8 poster pr. kanon, rekyl og forstilling er bygget. Tilbage: ladegrebene tæt på, vanter, hjul der drejer, og tab og moral ved kanonen (Allan 2026-11-15, kortlagt, ikke lavet): de overlevende overtager ikke de faldnes poster, rytteri overløber ikke kanonerne, og et batteri på flugt ses ikke.
- **Rytteri:** hove mod jorden og gangartsskift er ikke bedømt, halen er stiv, og rytterfigurerne mangler VAT-bagning.
- **Officerer:** hvileklippet står cirka 8 cm for lavt, dødsklippet hopper, og officeren er ikke set i karré.
- **Kolonne til linje:** 2. kompagni venter 10 s; årsagen er ikke fundet.

## Senere (ikke prioriteret endnu)
- Sverige-Norge med eget kort, og andre landes kort (Preussen, Østrig).
- Søtransport af regimenter mellem havne.
- Jernbanemobilisering koblet til køreplaner på kortet.

## Bygget 2026-10 (kampagne og slag), ikke afprøvet i et rigtigt slag

Overskriften omfatter også implementerede/mergede ændringer; merge alene er ikke bevis for build eller test.

- Udført siden sidste opdatering: særskilte 1825-data; datadrevne events med spillerpåvirkelige betingelser; militære/civile forskningsfaner; navne, uniformpalette og våbenombygning (Events.md og UnitCustomisation.md). Skov i sigtelinjen, skjul og dækning er merget og bygget. Runtime-forløb og balance mangler; uniformmaterialernes asset-forberedelse og visuelle kontrol udestår.
- Implementeret: karré med fire 90°-ildzoner og bajonetter, fartafhængig rytteritrussel, kavalerichok og afvisning ved stabil karré. Afprøvning af hele regelforløbet mangler.
- Bygget 2026-10-14, mangler spiltest: kompakte figurer og naturligere gang.
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

Designgrundlaget for kamp-AI'en ligger i `Docs/Kamp-AI-Design-v2.0.md` (ordre med latency og betingelser, kontakter med alder og tillid, mission/reaktion med SuspendedMission, combat slots og lanes, ildfelter, artilleribeskyttelse, kavaleri, epoke- og udstyrsprofiler som data, Auto-toggle pr. kommandonode, ydelsesbudget, debug og acceptscenarier T1-T10). Rækkefølge: fase 1 kontrakter, fase 2 vertikal skive (1 mod 1, bataljon, debug, T1-T3 og T7, auto-toggle), playtest, derefter artilleri, kavaleri og morter, til sidst brigade/division og betingede ordrer.

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

- [ ] Visuel kontrol af kortvisningsbjælke, skansebillboards, Københavns to broer og indkaldelsespreview i spillet ved forskellige zoomniveauer og vinduesstørrelser (ikke startet som del af opgaven).

## Naboerne inddeles i amter som Danmark (2026-10-10)

Alle nabolande skal have samme inddeling som Danmark: amter/distrikter med grænser på ejergrid, befolkning, rekrutter, skat og byer. Rækkefølge: 1) syd (Mecklenburg-Schwerin, Preussen/Pommern; Codex-opgaven "south" er i gang: Aemter, Regierungsbezirke, Kreise), 2) Hannover, Oldenburg, Hamborg, Luebeck, Bremen, 3) Sverige (laen: Malmoehus, Kristianstad, Blekinge, Halland, Goeteborg og Bohus, Aelvsborg, Skaraborg, Vaermland m.fl.) og Norge (amter: Smaalenene, Akershus, Hedemarken m.fl.), 4) Preussen/Østrig videre ned (Brandenburg, Schlesien), Rusland/England kun som abstrakt model. Samme datastruktur som Danmarks amter (amter_1825.json / amter1851.json) så alt (manpower, skat, forsyning) virker ens.

## Gammel liste gennemgået (2026-10-10)

| # | Punkt | Status |
|---|---|---|
| 2 | Slagmarkens grafik 100 gange bedre | Ikke taget som egen opgave. Der er lavet: græs, træer, skygger, røg, bagte figurer (under rettelse). Mangler: terrænteksturer, vand, bygninger i slag, vejr/lys, effekter. |
| 3 | Campaign map endnu bedre grafik | Delvist (3D-huse, broer, byer, nye bygningsbilleder). Mangler en samlet grafikrunde. |
| 4 | Se slag, sårede, dræbte, dræbte fjender pr. kompagni (kort og OOB) | Mangler: dræbte fjender pr. kompagni og samme tal i Kamporden-boksene. |
| 5 | Erfaring, leder, stats, ildmetoder | Enhedskortet har erfaring, leder, seks færdigheder og øvelser; ildmetoder trænes efter forskning. Mangler: erfaring pr. kampsituation (Docs/Erfaring.md). |
| 6 | Fane og uniform som billede af en soldat i de rigtige farver | Uniformbillede findes (1851-billeder) og farvevalg gemmes; 3D-farver virker først når de bagte figurer er rettet. Mangler: fanen på kortet og soldaten i valgte farver i kortet; 1825-billeder. |
| 7 | Sårede og hvornår de kommer tilbage | Gjort: enhedskortet viser antal på lazaret og hvornår halvdelen er tilbage. Mangler: erfaring bevares for de sårede (puljeregel). |

## Jernbane som indkomst (2026-10-11)
- Bygget 2026-10-13, mangler spiltest: passager- og fragtindtægt og balance. Mangler: drift/vedligehold som udgift og nedsat indtægt når linjen bruges til tropper eller er afbrudt (station/bro). Kode: Campaign1851Network.cpp, Campaign1851Army.cpp (ResetTroopTrains), økonomi i Campaign1851Resources.cpp.

## Ønsker 2026-10-13 (til planlægning)

- **Erobre faner:** en fane kan erobres; fanebærer med egen animation (bærer, falder, taber fanen); en ny soldat samler fanen op igen. Erobret fane tæller i rapporten (F) og i moral/prestige. Kræver animationer til fanebærer (hold fanen, falde, samle op).
- **Nye lade-animationer** til gevær (forladning/slagrør/bagladning, stående/knælende/liggende) og kanon/morter.
- **Køb af våben og udstyr fra andre** (andre nationer og private; pris, leveringstid, kvalitet; kobles til handel/diplomati).
- **Bygninger der selv fremstiller udstyr:** mængde pr. måned (geværer, kanoner, ammunition, sadler, vogne) efter bygning, arbejdere og råvarer.
- **Kavaleri og dragoner med sabler:** dragoner har gevær, når de sidder af, og sabel, når de sidder op igen (test af skift); sabel i hånden i charge.
- **Fanebærer-animationer** (se erobre faner).
- **Rapport efter slaget:** Bygget 2026-10-13, mangler spiltest: sejrsgrader og kampagnevirkning i et rigtigt kampagneslag. Nationsemblemer ved totalerne mangler.
- **Dækning og stilling i træfsandsynlighed:** se Docs/Kamp-AI-Design-v2.0.md (knælende/liggende rammes sværere og rammer bedre; dækning i terræn).

## Fund fra 4-mod-1-testene (2026-10-13)

- **Manglende AI (til planlægning):** sidestep for at komme fri af egen ild er der (loggen: "går udenom egen ild", "sidetrin"), men skal testes systematisk med flere kompagnier i ildlinjen.

## Kanonbatteriet: animationer, forstilling og heste (2026-10-13)

- **Findes:** forstilling/afprotsning som logik og tider (12 s ned, 15 s op, ARTILLERY-DEPLOY består), skubning af kanon uden forspand, marchstilling med kanonerne efter hinanden og pjecen bagud.
- **Mangler:** drejning ved sigte og hjul der drejer under kørsel; nærkontrol af besætningens animationer (punkt 25-31 i Docs/Manglende-Animationer.md): lade (svabre, ladning, kugle, sætte for), sigte, fyre af og træde tilbage; morterbetjening og koblingsanimationen.
- **Rækkefølge:** nærkontrol af besætningens animationer, til sidst koblingsanimationen.

- **Lade-animationer til gammeldags gevær:** stående, knælende og liggende i to våbentyper (forlader og bagladning), delt i faser med markører så de kan strækkes til epokens ladetid; se Docs/Manglende-Animationer.md afsnit 6. Kobling til ladetid og lyd kommer bagefter.

## Næste opgaver efter test af animationer og formationer (2026-10-22)

Brugeren tester først animationer og formationer i slaget. Derefter i denne rækkefølge, med målet at kampagnekortet nærmer sig referencebilledet (Codex' byillustrationer i `Reference/Campaign1851/Scenery/DK/Towns/`):

1. **Lys:** gør kortet og bygningerne belyste med ægte sollys, bløde skygger og skylys. Erstat de sorte, uigennemsigtige jordskygger med bløde, halvgennemsigtige. Tjek ydelsen med 250.000 træer.
2. **Farver:** dæmpet, jordnær palet (halmgul, olivengrøn, brændt tegl) via farvekorrektion og blødere farver i kortfliserne.
3. Stakitter og brønde afventer egnede scenery-modeller.
4. **Jorden tæt på:** detaljeteksturer (græs, stub, plovfurer), læhegn i 3D langs markskel (brug brikken til jorddiger) og bløde vejkanter.
6. **Mere detaljerede bygninger tæt på:** strå- og tegltag med struktur og sprossevinduer; de nuværende modeller bruges længere væk.
7. Bagefter de svenske, norske og preussiske byggestile.

## Formationer og animations-QA (2026-10-22)

- **Ankomst med ny front:** mænd passerer hinanden ned til 7–22 cm undervejs.
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
- **Bygninger som på referencebillederne (brugeren 2026-10-23: "ærgerligt at vi ikke kan lave denne grafik"):** prøve med to bygninger (det svenske gule byhus og den danske firlængede gård) i to udgaver:
  - billede-til-3D fra referencebilledet (Hunyuan3D/TRELLIS i Pinokio, agent `model3d`), reduceret til 2–5.000 trekanter;
  - vores model med referencebilledet projiceret på og bagt ind i en tekstur (agent `blender`).
  Begge vises på kortet med lyset slået til (lys-forsøget), side om side med referencebilledet og de nuværende modeller. De detaljerede modeller bruges tæt på, de nuværende på afstand (LOD). Kører efter Preussens billeder (ét GPU-job ad gangen).
- **Huse oven på vejene:** Bygget 2026-10-28, mangler spiltest: særskilt kontrol af huse på veje og vand i B/C-byerne.
- **Opfyldning i filen skal ske langsommere (brugeren 2026-10-23):** i dag efter 0,5 s (efter reglementet 1863). Ønske: langsommere skridt frem i hullet, så det kan ses.
- **Sammenrykning mod fanen hvert 20.–30. sekund, tilfældigt (brugeren 2026-10-23):** i dag 3 s efter sidste tab, højst 8 s. Ønske: sammenrykningen sker med et tilfældigt interval på 20–30 s under kamp. Testes med en ildkamp.
- **Sammenrykning under march (kendt fejl, skal løses):** to mænd i bageste geled kan gå 9 cm fra hinanden i op til 6 s, når en flankemand lige er sendt et geled tilbage. Løsning: flankemanden går først bagud og derefter ind, eller pladserne i bageste geled tildeles efter hvor han står efter skridtet tilbage.
- **Lys-forsøg** (plan punkt 1): lit materiale på de modellerede bygninger, sol og skylys, bløde skygger; FPS-måling før og efter.
- **Preussens bygninger** (Codex-kvote); derefter modeller for SE/NO/PR og valg af bygningssæt pr. land på kortet.
- **Dæmpet palet** i `build_map.py` og ny maling af fliserne (plan punkt 2, anden del).

## Skud fra mand til mand (brugeren 2026-10-28, idé, venter til QA'en er færdig)
- **I dag:** `ResolveHits` (`StrategyCombatComponent.cpp`) slår en terning pr. skud, men skuddene er anonyme. `KillSoldiers` vælger bagefter ofrene i den side, der vender mod skytterne.
- **Ønske:** hver skytte sigter på en bestemt fjende og rammer eller rammer ikke. Flere skytter må gerne sigte på samme mand. Skytte 1 kan ramme ved siden af og skytte 2 ramme. Målet vælges oftest over for skytten, men også på naboer, fanebærer og officer, og ved skrå ild flytter det sig til siden.
- **Hvert skud** giver ét af tre udfald:
  - træf på manden, der blev sigtet på;
  - vildfaren kugle, der rammer naboen eller manden bagved (lille chance);
  - forbi.
  Rammer et skud en mand, der allerede falder i samme salve, er kuglen spildt.
- **Balance:** spildte dobbelttræf giver færre tab end i dag. Træfchancen kalibreres, så de samlede tab svarer til periodens tal, og det måles før og efter.
- **Én figur = én mand:** standarden er allerede 1. Knapperne 2 og 5 (`StrategyHUD.cpp`, `-Strategy1864FieldLOD`) fjernes, hvis en FPS-måling med det største slag (bataljon mod bataljon, gerne en brigade) holder på det nye grafikkort. Ellers bruges LOD på afstand, men hver mand findes stadig.
- **Logning til QA:** hvert skud skrives som fx "DK-C1 mand 1 → SE-C1 mand 7: træf/forbi/vildfaren, afstand, chance". Det hører under QA-sporet for adfærd i slagene: falder den mand, der blev ramt, og står han over for skytten?

## Officerer og kavaleri (brugeren 2026-10-28, venter til video-QA'en er færdig)
- **Officerens sabel:** Bygget 2026-11-05, mangler spiltest: overgange mellem trukket sabel i formation og sabel i skeden under march.
- **De to ridende officerer:** Bygget 2026-11-05, mangler spiltest: galop og rytterens sæde i sadlen.
- **Kavaleriet:** gennemgå opførslen (march, formation, charge, afvisning ved karré, tilbagetog) og animationerne for heste og ryttere (dragoner og husarer, sabel i charge). Video-QA'en (Tools/QA) udvides til rytteri med Test 3, så fejlene kan måles og ses.

## Udskudte opgaver 2026-10-20

Slaget:
- **Fanebæreren skal afprøves målrettet:** tæt kamera forfra og fra siden, både i linje og i kolonne. Med `-Strategy1864DrillTest=...:KILLCOLOURS` og billeder hvert sekund kontrolleres:
  - at han står ved højre fløj i linjen og bag officeren i kolonnen;
  - at han bruger klippene `ColourWalk`/`ColourIdle`, med stangen i hånden og 12° bagud;
  - at fanen ligger på jorden, mens den er nede;
  - at afløseren smider sit gevær og rejser fanen;
  - at moralen giver −5 og +3 i loggen.
  Faldet og afløsningen er allerede set i loggen (2026-10-19).
- **Faner (valgmulighed 3):**
  - Én rigtig bataljonsfane, båret af fanekompagniet. Den kan erobres, påvirker moralen og tæller i rapporten.
  - De øvrige kompagnier får et mindre kendemærke. Den historiske praksis tjekkes af `research`.
- **Svingning med flydende omdrejningspunkt (kun linje, ikke kolonne):** første forsøg blev rullet tilbage 2026-10-20, fordi det blev værre. Før næste forsøg:
  - planlægges målet ved ankomst bedre (det sidste ben fik ikke sit startpunkt, så linjen drejede stadig på stedet til sidst);
  - afprøves små drejninger under march for sig.
  Referencen ligger i `R:\Onedrive\AI\Kompagni_3Geledder_Wheeling_Floating_Pivot_Samlet.zip`.
- **Ingen mand må hoppe ved pladsændringer:** kontrollér de øvrige overgange, hvor en plads ændres.
- **Fejl set i videooptagelse 2026-10-21:** efter ankomsten i linje forsvinder alle soldaterne i ca. 20 s, mens kameraet er tæt på; kun fanebæreren ses. De dukker op igen ved næste tab. Mistanke: de bagte crowd-figurers grænser opdateres ikke efter eksercitsen, så de bliver skjult (culling) ved et tæt kamera. Se `Saved/Codex/gap_check.png`.
- **Enhedskamera (brugerønske 2026-10-21):** lavet 2026-10-22: G følger, Shift+G følger bag enheden, FØLG-knap i enhedspanelet. Afventer brugerens spiltest.
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
  - Tallene i `Docs/Kortudvidelse-Research-2026-10-20.md` er skøn og skal kontrolleres mod kilderne.
- **Floder på det nye kort:** Elben (og andre floder i de nye områder) er kantede polylinjer og et hydrologisk skøn. Brug Natural Earth-floddata (`ne_10m_rivers_lake_centerlines`, kræver download) og glat forløbene.
- **Portrætter til SE/NO/PR:**
  - oprydning af `_qa_*`-filerne;
  - Bygget 2026-10-20, mangler spiltest: afprøvning i spillet af, at hver nation får sine egne portrætter.
- **Officerskarrierer og kampagneloggen:** bygget og valideret statisk, men ikke afprøvet i spillet (save v40, loggen i `Saved/CampaignLogs/`).

Værktøj:
- **Build via PowerShell** afbrød 2026-10-20 uden fejltekst (UBA). Kald `UnrealBuildTool.dll` direkte med `Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe` fra `Engine/Source`; det virker.

## Fund fra video-QA (2026-10-28, `Saved/QA/2026-10-28-oversigt.md`)
- **Opsamling af fanen:** manden knæler ca. 1 m fra stangen og rører den ikke (`fane`, t=16–18 s). Rettelsen fra 2026-10-22 virker ikke i denne situation.
- **Døde stables:** 10 tab på én gang fra flanken lægger ligene oven i hinanden med fanen nedenunder (`fane`, t=42 s). Hver død skal have sin egen plads på jorden.
- **Kolonnen går gennem de døde:** de marcherende træder ind i en falden mand (`tab-i-kolonne`, t=31 og 46 s).
- **Den blå marchrute** ses i alle optagelser. Den bør skjules med `-Strategy1864NoHud` og tones ned i spillet.
- **`linje-ild`:** Bygget 2026-11-03, mangler spiltest: kontrollér, at scenariet når frem til ild, så salver, ladning og røg kan bedømmes.
- **Video-QA for rytteri og kanoner** (Test 3 og 4) og for animationer: nærkontrol af hove, gangartsskift og ladegreb; se TASKS.md.

## Byer: opfølgning 2026-10-28

- [ ] København: undersøg kyst-/vandmasken og bygninger i vand. Ikke rettet sammen med Bogense.
- [ ] Bogense, 1825: kontrollér land/vand og vejtilslutninger ved 0,7 km zoom.

## Klasse B: regionalbyer — 2026-10-29

- 1825-klassen er 5.000–7.999 indbyggere efter den godkendte C-grænse på 5.000 og den eksisterende storbygrænse på 8.000. Omfatter Helsingør 6.600, Odense 7.900, Aalborg 6.600, Aarhus 6.000, Randers 5.900, Rendsborg 7.500 og Glückstadt 5.700. Befolkningerne er projektets afrundede skøn i `Tools/Map1851/cities_1825.json`, ikke folketællinger fra 1825. Udenlandske byer og Bornholm følger fortsat deres eksisterende spor.
- [ ] Bygget 2026-11-12, mangler spiltest: alle B-byer, især kystbyer; kontrollér sammenhængende vejtilslutninger, land/vand, byggeri uden nedrivning samt vækst og gem/indlæs uden for Odense. Loggens `overlaps=0` er ikke en uafhængig runtime-måling.
