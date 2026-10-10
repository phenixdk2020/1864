# Backlog: status for de 20 opgaver (campaign-kortet)

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

## After action report (ønske 2026-10-09, senere)

Efter hvert slag (3D og kampagne) en rapport pr. side, enhed og samlet: døde, sårede (og hvor mange der kommer tilbage via lazaret), fanger, erobret udstyr (kanoner, våben, heste, vogne, faner), tabt udstyr, ammunition brugt, moral- og cohesion-forløb, officerstab, tid i kamp. Tallene føres tilbage til kampagnens hær (mænd, udstyr, lazaret, krigsfanger). Kræver tabsregnskab pr. enhed i slaget (i dag kun CurrentStrength/InitialStrength) og en rapportskærm ved AFSLUT SLAGET.

## Uniformbilleder i 1825 (2026-10-10)

Indkaldelsesvinduet (Indkald en ny enhed) og enhedskortet viser nu 1851-uniformbillederne også i 1825 (de var skjult i 1825 og viste kun en streg-figur). Billederne er fra 1851 og passer ikke historisk til 1825 (glatløbet musket, andre uniformer): se på det, og lav eller skaf 1825-billeder (linje, jæger, dragon, artilleri, rytteri) pr. våben.
