# Manglende animationer (2026-10-10)

Til dem, der skal lave eller finde animationerne. Alle skal kunne bages til VAT (vertex animation textures, se `Docs/Performance-Battle.md`), så de skal være rene skeletanimationer uden fysik, uden kamera og uden ekstra objekter undtagen riflen (hænger på hånden).

## 0. Tekniske krav (gælder alle)

| Punkt | Krav |
|---|---|
| Skelet | Mixamo-kompatibelt `mixamorig:` (41 knogler), samme som de 65 eksisterende klip (`DK_Livgarden_1864_Apose_textured_skeleton.fbx`). Hips som rod. Klippene hænges på det delte Skeleton i Unreal. |
| Format | FBX, 30 fps, kun skeletanimation (ingen mesh i filen er nødvendig). Navn `A_<Navn>` som i dag. |
| Rod | Gå/løb/kravle: ingen rodbevægelse (in place), fremdrift laves i spillet. Ligge/falde: rod følger kroppen ned til jorden. |
| Loop | Gå, løb, kravle, stå, grave og lignende loops uden hop (første = sidste frame). Død, fald og overgange er enkeltklip, og sidste frame er sluttilstanden. |
| Længde | Loops 1,0–2,0 s. Overgange 0,5–1,5 s. Dødsfald 1,0–2,5 s. |
| Gevær | Gevær i hånd er del af klippet. Der er brug for varianter med gevær på skulder (marchstilling), gevær ved siden (ved foden) og gevær i anslag. |
| Hænder | Hold fingrene så de passer til riflens greb (bajonet og lade ligger i fast hånd). |
| Variation | Gerne to til tre varianter af gang, løb og skyd (lidt forskellig takt og arm), så mænd ikke gaar i takt. |

## 1. Det der findes i dag (65)

Stå, gå med og uden gevær (flere), løb (flere), start/stop gang, vend (90/180), kryds-gang, bagud-gang, stå-knæl-ligge og tilbage (alle overgange), knæl-idle, ligge-idle, kravle fremad og bagud, ligge-skyd, ligge-lad, skyd stående, sigte, lad stående og siddende, bajonetstød, angrebsløb (A_Charge), død forfra / bagfra / hovedskud / ligge (A_Prone_Death) / faldende / døende, gå-til-døende og løb-til-døende, knæl ramt-til-ryg.

## 2. Mangler - infanteri

| # | Animation | Bruges til | Bemærkning |
|---|---|---|---|
| 1 | **Grave / skovle (loop)** | BYG, skanser og brystværn | Med skovl, to varianter (grave, kaste jord). |
| 2 | **Bære tømmer / jordkurv (loop)** | BYG og byggeri | Gå med byrde. |
| 3 | **Såret, halte-gang (loop)** | Sårede der går til lazaret | Hånd mod arm eller ben, langsom. |
| 4 | **Såret, ligge og vride sig (loop)** | Sårede på marken | Efter død-klip, men før de er døde. |
| 5 | **Bære en såret (to-mands)** | Sanitetsfolk | Kan udelades i første omgang. |
| 6 | **Hænder op / overgive sig** (overgang + loop) | Fanger | Gevær tabes. |
| 7 | **Gå som fange (loop)** | Fangetransport | Hænder foran eller bag kroppen. |
| 8 | **Marchgang, gevær på skulder (loop, tæt takt)** | Kolonne på march | Findes delvist (`A_Walking_with_rifle`); en dedikeret strammere parade-gang mangler. |
| 9 | **Hvile, sidde/ligge på jorden (loop)** | Hvilende enheder, bivuak | Gevær på knæene. |
| 10 | **Skyd knælende** (separat fra stående) | Knæl-ild | Findes som overgange, men ikke som egen ild-loop. |
| 11 | **Løb i spredt orden, krumbøjet (loop)** | Skyttekæde, læg jer ned | Lavere løb. |
| 12 | **Kast sig ned (overgang)** | SPRED / læg jer ned | Fra stå eller løb til ligge. |
| 13 | **Rejse sig fra ligge (overgang)** | Efter SPRED | Findes som `A_Prone_To_Crouch`, men ikke direkte op i løb. |
| 14 | **Fikser bajonet** | Før angreb og karré | 1-2 s. |
| 15 | **Parade (blokér) og stød i nærkamp, to varianter** | Melee | Udover `A_Bayonet_Stab`. |
| 16 | **Ramt i nærkamp og falde (to varianter)** | Melee | Død med bajonetstød. |
| 17 | **Karré: knæl med gevær skråt frem (loop)** | Karré, 1. række | Gevær mod ryttere. |
| 18 | **Karré: stå, sigte og skyd (loop)** | Karré, 2. række | Eksisterende sigt/skyd kan bruges, men uden vending. |
| 19 | **Modtage ordre / saluttere** | Officerer og befalingsmænd | Valgfri. |

## 3. Mangler - befalingsfolk og specialfigurer

| # | Animation | Bruges til |
|---|---|---|
| 20 | **Fanebærer: stå, gå, løb med fane (loops)** | Kompagnifane |
| 21 | **Trommeslager: gå og spille (loop), stå og spille** | Tromme i kolonne |
| 22 | **Hornblæser: stå og blæse, gå og blæse** | Signaler |
| 23 | **Officer: pege, kikkert (to klip), kommandere med sabel løftet, stå** | Kompagnichef og stab |
| 24 | **Ordonnans / kurér: løbe og rende (se også hest)** | Ordrer |

## 4. Mangler - artilleri

Kanonbesætning (menige ved kanon) og kanon som bevægeligt objekt (kan animeres som rigid, ikke skelet).

| # | Animation | Bemærkning |
|---|---|---|
| 25 | **Lade kanon: svabre, lægge ladning, lægge kugle, sætte for (4 faser)** | Hver fase 1-2 s, kan være ét langt klip med markører. |
| 26 | **Sigte (håndtag, rette pjæce)** | Loop. |
| 27 | **Fyre af (stikke tønde, træde tilbage) og tilbageløb** | Besætning træder tilbage ved rekyl. |
| 28 | **Skubbe kanonen (to-tre mand, loop)** | Flytte kanon uden forspand. |
| 29 | **Spænde heste for / sidde op på forspand** | Ridende artilleri og fodartilleri. |
| 30 | **Kanonen kører (hjul drejer, rekyl-animation af selve kanonen)** | Prop-animation. |
| 31 | **Morter: lade fra munding, tænde, træde tilbage** | Morterbesætning. |

## 5. Mangler - kavaleri (heste findes kun som statisk mesh i dag)

Der findes ingen hesteanimationer. Hest og rytter skal laves som to animationer på to skeletter (eller ét hest+rytter-skelet).

| # | Animation | Bemærkning |
|---|---|---|
| 32 | **Hest: stå, gå, trav, kanter (canter), galop (loops)** | Fire gangarter med tydeligt forskellig fart (cm/s kan målsættes af mig). |
| 33 | **Hest: vende (venstre/højre)** | Gerne som blend, ellers to klip. |
| 34 | **Hest: stejle, falde (såret), ligge død** | Stejle bruges ved charge mod karré. |
| 35 | **Hest: græsse / hvile** | Bivuak. |
| 36 | **Rytter: sidde (idle), trav, galop (loops)** | Følger hestens knogler. |
| 37 | **Rytter: sabelhug højre, sabelhug venstre, parade** | Charge og melee. |
| 38 | **Rytter: lanse i vandret anslag (loop i galop)** | Lansere. |
| 39 | **Rytter: skyde med pistol/karabin (stillesidende)** | Dragoner og husarer. |
| 40 | **Rytter: sidde af, sidde på (overgang)** | Dragoner (stig af og kæmp til fods). |
| 41 | **Rytter: falde af hesten, ligge** | Tab. |
| 42 | **Rytter: sidde på heste i kolonne, fire i bredden (loop med anden afstand)** | Marchgang. |

## 6. Mangler - civile og forsyning (senere)

Vognfører, sygetransport, officersbud, hesteføring, læssende arbejdere (forsyningsvogne), arbejdere ved belejring (grave løbegrave, fylde kurve, trække kanoner). Ikke nødvendige i den første udgave.

## 7. Prioriteret rækkefølge

1. **Straks (3D-slag):** 12 (kast sig ned), 11 (spredt løb), 14 (fikser bajonet), 15-16 (melee), 17-18 (karré), 1 (grave), 3-4 (sårede), 20 (fane).
2. **Kavaleri (hel gruppe, uden dem er rytteri kun statisk):** 32, 33, 34, 36, 37, 38, 40, 41.
3. **Artilleri:** 25-28.
4. **Fanger og sanitet:** 6, 7, 5.
5. **Resten:** 21-24, 29-31, 35, 39, 42, 8-10, 13, 19.

Når et sæt er leveret, kører jeg import og bagning (se `Docs/Performance-Battle.md` og `Content/Python/bake_all_vat.py`, når Codex har afleveret den), og kobler klippene til situationerne i `StrategyInfantryVisualComponent` og kavaleri/artilleri-komponenterne.
