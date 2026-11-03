# Backlog: status for de 20 opgaver (campaign-kortet)

Historik 2026-10-29 (organiske købstæder, klasse B): odenseB-0_7km.jpg, aarhusB-0_7km.jpg og Town_B_Regional-referencerne gennemset. Seks parallelle husrækker erstattet af indfaldsveje til et centralt torv, buede hoved-/sidegader og korte tværgyder. Ved vand bevares eksisterende vej/bro med en landfast torvforbindelse, hvor terrænet tillader det. Kirken reserveres ved torvet før sidegader og huse; købmænd prioriteres i centrum. Blandede husmodeller følger gadetangenten med små vinkel-/facadeforskydninger og flere huller i udkanten. Roterede fodaftryk kontrolleres mod hinanden, veje, vand, bane, broer og garnisonsfelt. Fast kandidatorden fra byindeks, uafhængig af kulissernes random stream og gemte byggepladser. Kantbanken følger forlængede udfaldsgader med væksthuse og op til seks frie byggegrunde; nye InTown-byggerier bruger eksisterende reservationskontrol uden husrydning. Befolkningsmål uændret: Odense 49, Aarhus 36, Helsingør 40 huse, hver med én kirke og mål om seks frie grunde. Faktiske antal afhænger af gyldigt terræn og eksisterende byggeri og er ikke bekræftet i spil. Samme husskala og sokler kun på skråninger; Bogense/klasse C, 1851 og LaneLinesKm uændret. UE 5.8 API-headere, CRLF og diff kontrolleret statisk; 1.000 matematiske rotations-/pivottilfælde (4.000 hjørner) kontrolleret uden kompilering. Ingen build, spilstart eller commit. Afventer ny log, visuel kontrol af alle tre byer samt vækst/save-load.

Historik 2026-10-29 (Bogense-sokler og bydiameter): Set bogense4.jpg. Sokkelgrænsen brugte kortets km-skala (0,05 UE-enheder); nu 50 cm i verdensrum med aktørens Z-skala. Terrænhjørner alene bestemmer forskellen, uden -0,3-offset. Soklen dækker laveste hjørne til gulvet med 1 cm overlap i hver ende; lys sten RGB 205/203/195. extent-m bruger aktive huse/kirke og placerede InTown-byggepladser, uden kandidatbank eller gader; km gange 1000 giver geografiske meter. Layout, LaneLinesKm og 1851 bevaret. UE 5.8-header og CRLF/diff kontrolleret statisk; intet build, spilstart eller commit. Afventer ny log og visuel kontrol; 300-600 m er ikke bekræftet.

Historik 2026-10-29 (voksende kantgrunde): InTown rydder ikke kulisser. Småbyer deler en fast rækkefølge af kantgrunde mellem befolkningshuse og byggepladser; de første cirka fire ubesatte grunde holdes frie til byggeri. Husene fortsætter tætte gadefronter på kantgrundene. Kandidater og husvalg afledes af terræn, befolkning og gemte byggepladskoordinater; ingen nye save-felter. InTown søger ved udfaldsveje uden for bebyggelsen og derefter i en ydre ring. Log: reserved-plots og edge-sites. UE 5.8-headere, CRLF og diff kontrolleret statisk; intet build, spilstart eller commit. Afventer visuel kontrol, save/load-rundtur og kontrol af vejnet/ydre kandidatbank (64 mulige kantgrunde, begrænset af terræn). Kandidatgader vises kun ved optagne eller aktuelt reserverede kantgrunde. Øvrige byklasser har endnu ikke delt kantgrundsvækst.

Historik 2026-10-29 (sokler og faste byfelter): Huse og kirker placeres vandret efter h?jeste fodaftrykshj?rne; gr? kubesokkel g?r til laveste hj?rne ved mindst 0,5 m h?jdeforskel i kortets enheder. Engine BasicShapes/Cube og Content/Materials/M_FlatColor er kontrolleret p? disk. 1825-byer under 5.000 indbyggere har nu fire r?kker over to parallelle gader (fem felter pr. r?kke), kirke og torv; m?let er 18?20 huse efter aktuel befolkning, begr?nset af gyldige felter. Hele kapacitetsplanen reserveres ved start, s? v?kst/indl?sning kun ?ndrer instanser p? faste felter uden at flytte eksisterende huse. Kulissefelter er afledte data og ?ndrer ikke save-formatet. Fire mulige byggefelter ved kanten reserveres, n?r terr?n og eksisterende infrastruktur tillader det; InTown-byggeri med radius h?jst 0,3 km tager f?rste ledige, passende felt. Garnisonens eksisterende s?rskilte felt og byggepladsvisning bevares. Loggen for dette layout indeholder houses/church/plinths/reserved-plots/pop. LaneLinesKm er bevaret i TownLane-l?kken. Kun statisk kontrol mod UE 5.8-headere samt CRLF/diff-kontrol; intet build, spilstart eller commit.

Resterende byarbejde: Befolkningsstyret lokal kulissev?kst er implementeret for det nye sm?bylayout, ikke de ?vrige byklasser/1851-layoutet. De ?vrige klasser f?r sokler, men deres eksisterende bylog har endnu ikke sokkel-/reservationsopt?lling. Sm?byens byggelog er et ?jebliksbillede ved anl?ggelse, ikke en ny statuslinje ved hvert befolkningstrin. Kystbyer kan have f?rre end 20 gyldige husfelter og f?rre end fire gyldige byggefelter; kirke/torv bevares ved delvis plan som f?r. Hvis selve kirke/torv ikke kan placeres inden for s?geomr?det, logges fejl som f?r. Afventer visuel kontrol af Bogense: sokler, anden gade, veje/vand, reserverede felter, Textile_Mill og kaserne samt v?kst og save/load. Store/strategiske/kystbundne byggerier bruger fortsat deres eksisterende placeringsregler. Dynamiske nye veje eller bygninger over allerede reserverede v?kstfelter kr?ver yderligere kontrol.


Historik 2026-10-29: Sm?bylayout i 1825 ?ndret til ti huse med u?ndret skala og langsiden mod gaden. Fittede modelgr?nser (inkl. importrotation og forskudt pivot) bestemmer ber?rende gavle. Kirke og lukket torv reserveres f?rst; hele planen s?ges samlet p? land op til 1 km fra den u?ndrede mark?r, med fire mulige gaderetninger. Vand, landeveje, bane og broer kontrolleres f?r placering; hovedveje oml?gges uden om husr?kkerne. Loggen har `church=0|1` og `shift-m`. Hvis ingen komplet plads findes, bevares den gyldige kirke/torv-placering med flest frie husfelter, og manglende huse logges som advarsel. Kun hvis selve kirken og torvet ikke kan st? p? land inden for 1 km, logges en fejl med `church=0`. UE 5.8-headerkontrol og statisk geometrikontrol udf?rt; intet build, spilstart eller commit. Afventer visuel kontrol af Bogense/Middelfart: kirke/torv, 10 huse, ber?rende gavle, mindre udstr?kning og vejtilslutninger. 1851-layoutet er bevaret.

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
- **Status 2026-11-03:** (1) rytteri-tasking og (2) trusselsvurdering/reserve er **implementeret, men ikke bygget og ikke testet** (bataljonsplanen `StrategyBattalionPlanComponent`, feltofficerens eskadronmission, reserve efter doktrin: altid ved mindst 4 kompagnier, styrkeforholdet styrer kun indsættelsen, bygget OK og tests kører; se historikken 2026-11-03 og accept-ID'erne `RESERVE-*`/`CAV-*` i `Docs/TestFlags.md`). Punkt (3) er uændret.
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
3. ~~**Byernes opbygning:** huse langs gader og omkring et torv ved kirken, med gårdspladser, haver.~~ Stakitter og brønde afventer egnede scenery-modeller. (2026-10-28: C/B/A, frie bagarealer og QA-log; visuel kontrol udestår.)
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
- **Bygninger som på referencebillederne (brugeren 2026-10-23: "ærgerligt at vi ikke kan lave denne grafik"):** prøve med to bygninger (det svenske gule byhus og den danske firlængede gård) i to udgaver:
  - billede-til-3D fra referencebilledet (Hunyuan3D/TRELLIS i Pinokio, agent `model3d`), reduceret til 2–5.000 trekanter;
  - vores model med referencebilledet projiceret på og bagt ind i en tekstur (agent `blender`).
  Begge vises på kortet med lyset slået til (lys-forsøget), side om side med referencebilledet og de nuværende modeller. De detaljerede modeller bruges tæt på, de nuværende på afstand (LOD). Kører efter Preussens billeder (ét GPU-job ad gangen).
- ~~**Huse oven på vejene (brugeren 2026-10-23):** nogle byhuse står på gaderne og landevejene. Årsag: byens gader reserverer ikke plads (`Take`), og husrækken står kun 0,12 km fra gadens midte, mens et hus fylder ca. 0,1 km i radius på kortet. Løsning: reservér gadens punkter som landevejene gør, og flyt husrækkerne længere ud (ca. 0,17 km).~~ (2026-10-28: reservation før husplacering, modelbaserede afstande og segmentkontrol af veje, gader, broer og jernbaner; spiltest udestår.)
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
- **Officer til fods leder kolonnen:** en synlig officer foran kolonnen ved fanebæreren (skitse fra 2026-10-22: `SSSSSSS F O -->`). Midlertidig figur med sabel er foreslået, indtil den rigtige officersfigur findes.
- **Officerens sabel:** han trækker sablen og står med den, når kompagniet står i formation. Når kolonnen begynder at marchere, stikker han den i skeden. Det kræver fire klip (træk sabel, stå med draget sabel, stik i skeden, march med sablen i skeden) og overgange efter kompagniets tilstand.
- **De to ridende officerer:** hestene og rytterne er ikke animerede og ser dårlige ud. De skal have gang, skridt og trav, holde stille og vende, og rytteren skal sidde rigtigt på hesten.
- **Kavaleriet:** gennemgå opførslen (march, formation, charge, afvisning ved karré, tilbagetog) og animationerne for heste og ryttere (dragoner og husarer, sabel i charge). Video-QA'en (Tools/QA) udvides til rytteri med Test 3, så fejlene kan måles og ses.

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

## Fund fra video-QA (2026-10-28, `Saved/QA/2026-10-28-oversigt.md`)
- **Opsamling af fanen:** manden knæler ca. 1 m fra stangen og rører den ikke (`fane`, t=16–18 s). Rettelsen fra 2026-10-22 virker ikke i denne situation.
- **Døde stables:** 10 tab på én gang fra flanken lægger ligene oven i hinanden med fanen nedenunder (`fane`, t=42 s). Hver død skal have sin egen plads på jorden.
- **Kolonnen går gennem de døde:** de marcherende træder ind i en falden mand (`tab-i-kolonne`, t=31 og 46 s).
- **Den blå marchrute** ses i alle optagelser. Den bør skjules med `-Strategy1864NoHud` og tones ned i spillet.
- **`linje-ild` mangler ildkamp:** kompagniet står i linje uden fjender i skudhold i 120–180 s. Scenariet skal have en ordre, der fører til ild (fx `-Strategy1864SkirmishAttack`), før salver, ladning og røg kan bedømmes.
- **Video-QA for rytteri og kanoner** (Test 3 og 4) og for animationer: samme ramme i `Tools/QA`.

## Byer: opf?lgning 2026-10-28

- [ ] K?benhavn: unders?g kyst-/vandmasken og bygninger i vand. Ikke rettet sammen med Bogense.
- [ ] Bogense, 1825: afprøv klasse C i scenery-skala (10 huse, kirke 1,5 × husradius, 24 m gade, ca. 1,03 km bykerne); kontrollér land/vand, vejtilslutninger og læsbarhed ved 0,7 km zoom. Større klasser afventer denne vurdering.

- Historik 2026-10-28: Klasse C i 1825 skaleret til kortets scenery: husradius 80 m (synlig bredde ca. 130 m), kirkeradius 120 m, 10 huse i to rækker, gade 24 m; beregnet fuld udstrækning 1.033 m. Målt fra modelbounds × fit × instansskala: bøgekrone 154–267 m, mølle 227 × 316 m før variation, gårdanlæg 452–619 m før variation. Byområdet friholdes også for trækroner og gårdenes fulde bounds. Afventer visuel spiltest ved 0,7 km zoom; ingen build, spilstart eller commit.

## Klasse B: regionalbyer ? 2026-10-29

- 1825-klassen er 5.000?7.999 indbyggere efter den godkendte C-gr?nse p? 5.000 og den eksisterende storbygr?nse p? 8.000. Omfatter Helsing?r 6.600, Odense 7.900, Aalborg 6.600, Aarhus 6.000, Randers 5.900, Rendsborg 7.500 og Gl?ckstadt 5.700. Befolkningerne er projektets afrundede sk?n i `Tools/Map1851/cities_1825.json`, ikke folket?llinger fra 1825. Udenlandske byer og Bornholm f?lger fortsat deres eksisterende spor.
- Testby: Odense, 55,396? N / 10,389? ? (projektets bykoordinat). Indlandsby, men i klassens ?vre st?rrelsesende; terr?nets fladhed og ?ens indvirkning skal vurderes i spillet.
- Implementeret: s?rskilte B-klassedata; 48 faste kernepladser i seks r?kker (otte pr. r?kke), tre parallelle 24 m gader med to tv?rforbindelser ved enderne, kirke og torv samt to k?bmandshuse ved torvet. Almindelige huse har C's radius 80 m; k?bmandshuse 96 m; kirke 144 m (1,2 ? C). Synligt husm?l: 30 + max(0, befolkning ? 5.000) / 150, heltalsdivision: 34?49 i de nuv?rende B-byer, Odense 49 inklusive v?kstplads. Terr?nhindringer kan reducere antallet.
- Seks frie kantgrunde s?ges i en deterministisk bank; voksende befolkning aktiverer flere huse i samme r?kkef?lge. Byggeri bruger ledige kantgrunde og m? ikke rydde eksisterende huse. B's v?kst kontrollerer ogs? rektangul?re overlap og lokale gader. Fundamenter f?lger samme h?ldningst?rskel som C. Samme logfelter: houses, church, plinths, reserved-plots, edge-sites, pop, street-points, overlaps, extent-m og shift-m.
- Bogenses C-layoutblok er kontrolleret ordret u?ndret (SHA-256); C's husm?l og fire reservationer er bevaret. `LaneLinesKm` og 1851-grenen er bevaret. Klasseoplysninger er afledte visningsdata, s? intet nyt saveformat.
- [ ] Brugerens n?ste build/spiltest: Odense og alle B-byer, is?r kystbyer; kontroll?r torv/kirke, sammenh?ngende vejtilslutninger, land/vand, frie kantgrunde, v?kst, byggeri uden nedrivning og samme layout efter gem/indl?s. Loggens `overlaps=0` er ikke en uafh?ngig runtime-m?ling. Kontroll?r ogs? Bogense mod `Saved/QA/2026-10-28-byer/bogense5.jpg`. Den gamle C-testnote ovenfor er overhalet af brugerens godkendelse.
- Historik 2026-10-29: Klasse B udvidet med C-systemets kortskala, torv/kirke, terr?nfundamenter og deterministiske kantv?kst; C-layoutet bevaret. Statisk kontrol alene; ingen build, spilstart eller commit. Historik f?rt her efter opgavens filbegr?nsning.
