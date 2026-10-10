# Garnison, billet og kaserne

Opdateret 10. oktober 2026.

## Fund i den tidligere kode

`bHasPlot` er alene et geometrisk resultat: søgningen finder en jævn, fri landgrund ved byen. En by som Køge kan mangle en sådan grund uden at mangle huse til indkvartering. Flaget må derfor begrænse kasernebyggeriet, ikke adgangen til garnisonsoversigten.

`RaiseTownOk` godkendte både færdige kaserner og byer, som en enhed havde som `Home`. Det repræsenterer scenariets eksisterende kaserner, også når de ikke er tegnet som byggeprojekter. Reglen bevares nu med `ArmyAtStart` som grundlag; en nyrekrutteret enheds `Home` kan ikke gøre en billetby til en kaserneby. Starthæren vælges allerede særskilt fra `Army_1825.json` eller `Army1851.json`.

Tilbagevenden krævede tidligere en garnisonsby eller et bygget fort i samme amt, alternativt inden for byens radius/én kilometer fra fortet. Der var ingen regel om, at destinationen skulle være enhedens egen `Home`, ingen kapacitetskontrol og ingen ændring af `Town` ved opløsning af en formation.

## Oversigten

GARNISON er altid aktiv på den indenlandske bys kort, også uden byggegrund. Den viser alle levende, standsede enheder med `Town` sat til byen, herunder enheder i feltformationer. Hver række viser navn, mandtal, våbenart, moral, proviant, relevant foder og ammunition. VÆLG åbner det almindelige enhedsvalg. KAMPORDEN FOR BYEN åbner stedvisningen via `SetOOBPlace(town)`.

Mandtal og ledig plads vises samlet. Der tælles hele `Men`, ikke alene fredens fremmødte soldater; hjemsendte skal også kunne indkaldes. Lange lister har sider, tilpasset skærmhøjden. TILBAGE I GARNISON er aktiv for enheder i formation, når de fælles tilbagevendingsregler tillader det; ellers vises status og eventuel forklaring i et værktøjstip.

## Kapacitet og billet

- Uden kaserne: `max(0, min(indbyggere / 40, 1.000))` mand, med heltalsdivision. Eksempel: 2.500 indbyggere giver plads til 62 mand.
- Ny, færdig kaserne: 800 mand, svarende til bygningstabellens bataljon.
- Scenariets eksisterende kasernebyer: mindst 800 mand, ellers summen af starthærens fulde `MaxMen` med hjemsted i byen. Kapaciteten ændres ikke af senere tab, opdeling, omdøbning eller flytning.
- Udenlandske eller besatte byer har ingen tilgængelig kapacitet.

Billet er indkvartering i byens huse og kræver hverken byggegrund eller en bestemt mindste bystørrelse. Positiv moralgenopretning er 5 % langsommere (faktor 0,95); moralens målniveau og fald ændres ikke. Garnisonens proviant- og foderforråd fyldes til normal kapacitet divideret med 1,15. Ammunition kommer fortsat fra eksisterende depotlagre.

Forsyningsunderholdet har et tillæg på 15 % af 30 dages proviant til fremmødte soldater og foder til heste, beregnet med forsyningssystemets eksisterende priser. Tillægget indgår både i den viste månedsdrift og den faktiske månedsbetaling. Nye enheders særskilte underhold ganges desuden med 1,15, når de er i billet. Disse balanceværdier er spilestimater, ikke dokumenterede historiske satser.

En march kan fortsat bringe en feltstyrke til en by, selv om hele styrken ikke kan indkvarteres; oversigten kan derfor vise overbelægning. Ledig plads vises da som nul. Kapacitetskontrollen gælder indtræden i garnison og rekruttering; der oprettes ikke en ny kapacitetsbegrænsning på selve marchordrerne.

## Tilbagevenden

Enheden skal være standset og ude af kamp. Den nuværende by foretrækkes, derefter `Home`, derefter nærmeste by med plads i samme amt eller inden for byens radius. Billetbyer er gyldige på samme vilkår som kasernebyer. `Home` ændres ikke; det er fortsat hjemsted for rekruttering og erstatningsmandskab.

En hel formation planlægges før nogen ændring. Alle enheder reserverer plads i en fælles kapacitetsberegning; plads tælles ikke dobbelt, når en enhed allerede står i byen. Kan en enhed ikke placeres, ændres ingen enheder, og formationen bevares. Ved succes sættes `Formation` til nul, `Town` til destinationen og enheden placeres med kortets eksisterende `PlaceInTown`.

Den tidligere fortmulighed bevares, når ingen by kan rumme enheden: et bygget fort i samme amt eller inden for én kilometer tillader tilbagevenden uden tildeling af en by. Enheden bliver på sin position med `Town = INDEX_NONE`; dette fordeler ikke automatisk kompagnier til fortet og bruger ikke billetplads.

## KASERNE og rekruttering

KASERNE er en særskilt sektion i samme fane. Byggefunktionerne vises kun med byggegrund, mindst 2.500 indbyggere og en ubesat by. Uden grund står: »Ingen byggegrund ved <by>: kysten eller terrænet levner ingen jævn, fri plads«. En for lille by får »Kræver over 2.500 indb.«. Som for de øvrige eksisterende bygningskrav accepterer kontrollen præcis minimumstallet, selv om etiketten siger »over«.

Minimum kommer fra `min_population` på `Garrison_Barracks` i `Data/Campaign1851/Buildings1851.csv`, indlæses som et bygningskrav og bruges i garnisonsmodultabellen. Generatoren `Tools/Campaign/building_costs.py` skriver også feltet. Priser, tid og de øvrige eksisterende tabelværdier er uændrede. Kravet bruges ved nyt, betalt byggeri; indlæsning af eksisterende projekter får ikke et nyt befolkningskrav.

Billet giver aldrig adgang til at oprette enheder. `RaiseTowns` og enhedsvælgeren kræver fortsat færdig kaserne eller scenariets eksisterende kaserneby. Besatte byer udelukkes. En tom vælger forklarer »ingen by med kaserne« og hvorfor billet ikke kan rekruttere. Både enhedsvælgeren og den ældre bataljonsknap kontrollerer ledig kapacitet, før ressourcer trækkes.

## Gemmekompatibilitet og kontrol

Ingen ny kampagnetilstand gemmes. Kapacitet, billetstatus og tillæg beregnes fra eksisterende indbyggertal, byggeprojekter, scenariets starthær og enhedernes gemte `Town`/`Formation`. Gemmeversionen er uændret. Gamle spil indlæses uden at få slettet enheder eller afvist kaserner; eksisterende overbelægning vises, og nye garnisonsplaceringer kræver plads.

Statisk kontrol: alle eksisterende CSV-værdier sammenlignet med HEAD, generatorens output sammenlignet med tabellen, Python-syntaks kontrolleret, panelhøjden beregnet for 720–1.440 pixels og diff kontrolleret for whitespacefejl. Nye/anvendte container-, matematik-, streng- og Slate-signaturer kontrolleret mod Unreal 5.8-headerne. Der er ikke bygget eller startet spil/editor; den faktiske UI og gameplay skal derfor stadig verificeres ved en senere godkendt spiltest.
