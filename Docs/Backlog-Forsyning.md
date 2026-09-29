# Backlog: Forsyning (proviant, foder, ammunition)

Status: **forslag, afventer godkendelse**. Nummereret i foreslået rækkefølge.
Designmanualens ramme: logistik og supply er fysiske strømme. Intet teleporteres; det skal køres, bæres eller sejles.

## Ideen i korte træk
- Hver enhed har **dages forsyning** hos sig: proviant til mændene, foder til hestene og ammunition.
- Forsyningen hentes fra **depoter** og fyldes op ad **forsyningslinjer**: vej med trænvogne, jernbane eller skib.
- Uden forsyning falder moral og samhørighed. Folk bliver syge og deserterer, og marchen bliver langsommere.
- Skanserne har et **ammunitionslager**, og det går med til dine 3D-slag.

| # | Opgave | Indhold | Afhænger af |
|---|---|---|---|
| **F-1** | Forsyning på enheden | Hver bataljon, eskadron og batteri får proviant, foder og ammunition i dage. Ved start: 4 dage proviant, 2 dage foder og ammunition til 2 kampdage. Forbruget pr. mand og hest og dag regnes efter tidens rationer. Hærpanelet får en linje "Forsyning: 3½ dag". | – |
| **F-2** | Depoter og magasiner | Garnisonens *Depot og magasin* (findes allerede) og det civile *Kornmagasin* bliver depoter med lager: proviant, foder og ammunition i rationer. Lageret købes fra amtet og fyldes op over tid. Arsenalet og ammunitionsfabrikken laver ammunition. | F-1 |
| **F-3** | Forsyningsrækkevidde | En enhed i garnison eller inden for 1 dagsmarch af et depot med lager fyldes op dagligt. Ellers lever den af det, den har med. | F-1, F-2 |
| **F-4** | Følger af mangel | Første dag uden proviant: samhørighed −5 og moral −3 om dagen, derefter værre, med sygdom (tab af mand) og langsommere march. Uden foder: rytteri og artilleri sløves, og heste dør. Uden ammunition kan en enhed ikke kæmpe (slaget får tallet). | F-1 |
| **F-5** | Trænkolonner | Vognkolonner kører fra depot til en enhed eller en skanse ad vejene. De ses på kortet, tager tid, kan blive forsinket i vinterføre og kan senere angribes. Nye kolonner købes (vogne og heste). | F-2, F-3 |
| **F-6** | Jernbane og skib | Forsyning med tog (bruger troppetogene eller egne godsvogne) og med skib mellem havne. Det er hurtigt, men kun mellem stationer og havne. | F-5 |
| **F-7** | Skansernes lager | Hver skanse får ammunition (skud pr. kanon, patroner pr. infanterist) og proviant til besætningen. Lageret fyldes af trænkolonner, og **Fortifications.json** får tallene. Skanser med bombesikre magasiner (niveau 4) mister mindre under bombardement. | F-5, skanser |
| **F-8** | Forsyningskort | Kortet kan farves grønt, gult og rødt efter, hvor mange dage der er til nærmeste depot. Forsyningslinjer og depoternes lager vises, og der kommer et vindue med alle depoter. | F-3 |
| **F-9** | Intendanturen (AI) | Nyt ressort i Statsrådet: *Intendanturen* (kvartermesteren). På AUTO eller RÅDGIVER køber den forråd og sender trænkolonner til enheder og skanser med lav forsyning, med begrundelse. | F-5, Statsrådet |
| **F-10** | Årstider og forbrug i fred | Vinteren gør vejene langsomme og foderet dyrere. Skydeøvelser bruger ammunition fra depotet. I fredstid koster forrådet penge hver måned: en ny post "Forråd" i budgettet. | F-2 |
| **F-11** | Data til 3D-slagene | Pr. enhed: ammunition (skud pr. mand, pr. kanon), dage siden sidste forsyning og træthed. Pr. skanse: magasinets indhold. Formatet dokumenteres som *Fortifications.json*. | F-1, F-7 |

## Forslag til første levering (MVP)
**F-1 → F-2 → F-3 → F-4 → F-7 (uden trænkolonner) → F-11.**
Det giver forsyning, depoter og følger af mangel. Skanserne får et ammunitionslager, og dine slag får tallene.
Trænkolonner (F-5), jernbane og skib (F-6), forsyningskortet (F-8) og Intendanturen (F-9) kommer bagefter.

## Tal (skøn til spillet; kilder tilføjes, når de findes)
- **Proviant:** 1 ration pr. mand pr. dag (ca. 1,2 kg: brød, kød, gryn, brændevin). 0,12 rd. pr. ration.
- **Foder:** 1 ration pr. hest pr. dag (ca. 8 kg havre og hø). 0,10 rd. pr. ration.
- **Ammunition:**
  - Infanteri: 60 patroner pr. mand med sig og 100 i depotet.
  - Kanoner: 120 skud pr. kanon.
  - Én kampdag bruger cirka 40 patroner og 60 skud.
- **Med sig:** 4 dages proviant i tornyster og bagage, 2 dages foder.
- **Trænvogn:** ca. 800 kg last, 25 km om dagen. Firspand.
