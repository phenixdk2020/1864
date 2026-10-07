# Portrætter: hvad der mangler (100 stk.)

| Pulje | Antal | Filer |
|---|---|---|
| Officerer, unge | **30** | `T_Portrait_Officer_Ung_00.png` til `_29.png` |
| Officerer, ældre | **30** | `T_Portrait_Officer_Aeldre_00.png` til `_29.png` |
| Officerer, gamle | **15** | `T_Portrait_Officer_Gammel_00.png` til `_14.png` |
| Ministre | **25** | `T_Portrait_Minister_00.png` til `_24.png` |

Alle: **512 × 640 (4:5)**, hoved og skuldre, set **forfra**, mørk ensfarvet baggrund, 1850'erne (dansk guldalder-maleri), samme lys og
stil på hele sættet. Alle er mænd. Et billede viser en type, ikke en bestemt person: spillet giver hver officer eller minister ét billede ud
fra navnet og beholder det hele karrieren.

Filerne ligger i `Reference/Campaign1851/Portraits/`. Importer med `Tools/Campaign/Import-Billeder.bat`.

## Rang: billedet er personen, spillet tegner graden
Officerernes billeder viser **ingen rang**: en mørkeblå frakke med rød krave og sølvknapper, **uden epauletter, stjerner og ordener**.
Spillet tegner rangmærket (epaulette med stjerner) over billedets nederste hjørne. Ved forfremmelse skifter mærket, ansigtet er det samme.
Kaptajn til general er derfor af samme slags billede; kun alderen adskiller dem.

## Officerer: tre aldersgrupper
Spillet vælger gruppe efter officerens alder: **under 38 år = Ung**, **38 til 49 = Ældre**, **50 og derover = Gammel**.

| Gruppe | Antal | Udseende |
|---|---|---|
| Ung (28-37 år) | 30 | glatbarberet, overskæg eller små bakkenbarter, mørkt eller lyst hår, glat ansigt |
| Ældre (38-49 år) | 30 | bakkenbarter, fuldskæg, hageskæg, begyndende gråt hår, første rynker |
| Gammel (50-65 år) | 15 | gråt eller hvidt hår, kindskæg eller fuldskæg, rynker, strengt blik |

Giv alle ansigterne i en gruppe forskellige træk (hårfarve, hårlinje, skæg, næse, øjenbryn, kæbe), ellers ligner de hinanden.
Er der færre end angivet i en gruppe, bruger spillet dem, der findes, og gentager dem.

De gamle `T_Portrait_Officer_00` til `_11` og `T_Portrait_General_00` til `_08` bruges som reserve, indtil alder-billederne findes.
Dem med store epauletter passer ikke til systemet.

## Ministre: 25 stk.
Sort kjole, hvid høj flip, sort halsbinde. Ingen uniform. 35-70 år. Variér alder, skæg, hår og ansigtsform.

| Fil | Beskrivelse |
|---|---|
| `_00` | ældre, gråt hår, kindskæg, alvorlig |
| `_01` | midaldrende, glatbarberet, sidedeling, skarp |
| `_02` | ung (ca. 40), overskæg, åbent blik |
| `_03` | tyk, rund, bakkenbarter, jovial |
| `_04` | tynd, høj pande, runde briller, lærd |
| `_05` | ældre, skaldet, hvidt fuldskæg |
| `_06` | midaldrende, mørkt hår, hageskæg |
| `_07` | stram, tilbagestrøget hår, tykke bakkenbarter |
| `_08` | ældre, gråt krøllet hår, overskæg og kindskæg |
| `_09` | yngre (ca. 35), glatbarberet, ivrig |
| `_10` | midaldrende, lysebrunt hår, rundt fuldskæg |
| `_11` | gammel, tyndt hvidt hår, trætte øjne |
| `_12` | midaldrende, rødligt hår, kraftige bakkenbarter |
| `_13` | høj og mager, mørkt hår, tynd moustache |
| `_14` | ældre, bred, hvidt hår, glatbarberet, tung hage |
| `_15` | yngre (ca. 38), lyst hår, pincenez |
| `_16` | midaldrende, skaldet, mørkt fuldskæg |
| `_17` | ældre, spids næse, grå bakkenbarter, skarpt blik |
| `_18` | lav og buttet, krøllet mørkt hår, overskæg |
| `_19` | midaldrende, glat hår, hageskæg og overskæg |
| `_20` | gammel, tyndt gråt hår, lange kindskæg, brille |
| `_21` | yngre (ca. 40), mørkt hår, kraftig hage, alvorlig |
| `_22` | midaldrende, tilbagestrøget hår, tynd kæbe, tankefuld |
| `_23` | ældre, lysegrå hår, runde kinder, venlig |
| `_24` | midaldrende, rødbrunt fuldskæg, stort pandehår |

Prefikset på alle er `T_Portrait_Minister`.
