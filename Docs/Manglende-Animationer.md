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

## 5. Mangler - kavaleri (opdateret 2026-11-03: hest og rytterklip findes som kilder, import og test mangler)

Hest og rytter er to skeletter (`SK_Horse_1864` og det fælles menneskeskelet). Hesteklippene (`Reference/Animations/Horse/`, genereret af `Tools/Animation/horse_gaits_blender.py`) og rytterklippene (`Tools/Animation/build_cavalry_rider_set.py`, `build_cavalry_sabre_set.py`) er lavet og bruges af `StrategyCavalryVisualComponent`. **Intet af det er importeret i Unreal eller set i spil endnu.**

| # | Animation | Status |
|---|---|---|
| 32 | **Hest: stå, gå, trav, kanter (canter), galop (loops)** | **Findes** (`A_Horse_Stand/Walk/Trot/Canter/Gallop`, på stedet, 30 fps). Fodens jordhastighed i klippet: 0 / 60 / 179 / 357 / 757 cm/s; spillets tærskler er 80/350/650/900. Skridtlængden er for kort til spillets fart (gå er ca. 45 cm pr. fodisæt mod ca. 1,5 m i virkeligheden), så afspilningshastigheden begrænses og fødderne glider; klippene bør regenereres med længere skridt. |
| 33 | **Hest: vende (venstre/højre)** | Mangler som klip. Erstattet af en hældning (op til 14 grader) i koden. |
| 34 | **Hest: stejle, falde (såret), ligge død** | **Findes** (`A_Horse_Rear` 2,37 s, `A_Horse_Fall` 1,97 s, falder mod venstre og bliver liggende på sidste billede). Rear bruges ved afvist chok, Fall ved tab. Ingen separat "ligge død". |
| 35 | **Hest: græsse / hvile** | Mangler. |
| 36 | **Rytter: sidde (idle), trav, galop (loops)** | **Findes** (`A_Combat_CavWalk`, `CavTrotRise` lettende trav, `CavCanter`, `CavGallop`; idle er den holdte "Reload sitting"-pose). Tidsat til hesteklippene, samme billedantal. |
| 37 | **Rytter: sabelhug højre, sabelhug venstre, parade** | **Delvist:** `CavSabreDraw` (træk), `CavSabreSlash` (kombo, første sving bruges), `CavSabreFight`; plus `CavCharge` (stående i bøjlerne). Hug venstre og parade mangler som egne klip. |
| 38 | **Rytter: lanse i vandret anslag (loop i galop)** | Mangler. |
| 39 | **Rytter: skyde med pistol/karabin (stillesidende)** | Mangler. |
| 40 | **Rytter: sidde af, sidde på (overgang)** | Mangler. |
| 41 | **Rytter: falde af hesten, ligge** | **Findes** (`A_Combat_CavFall`, 60 billeder, kastes til højre og lander på ryggen). |
| 42 | **Rytter: sidde på heste i kolonne, fire i bredden (loop med anden afstand)** | Mangler som eget klip; kolonnen bruger gangartsklippene. |

Kendte begrænsninger: sablen er en statisk rekvisit, der følger rytterens placering og ikke hånden under træk og hug; rytterens ben er "Reload sitting"-sædets og er ikke tilpasset en hest; rytterklippene er kun set som pind-figurer.

## 6. Mangler - civile og forsyning (senere)

Vognfører, sygetransport, officersbud, hesteføring, læssende arbejdere (forsyningsvogne), arbejdere ved belejring (grave løbegrave, fylde kurve, trække kanoner). Ikke nødvendige i den første udgave.

## 7. Prioriteret rækkefølge

1. **Straks (3D-slag):** 12 (kast sig ned), 11 (spredt løb), 14 (fikser bajonet), 15-16 (melee), 17-18 (karré), 1 (grave), 3-4 (sårede), 20 (fane).
2. **Kavaleri (hel gruppe, uden dem er rytteri kun statisk):** 32, 33, 34, 36, 37, 38, 40, 41.
3. **Artilleri:** 25-28.
4. **Fanger og sanitet:** 6, 7, 5.
5. **Resten:** 21-24, 29-31, 35, 39, 42, 8-10, 13, 19.

Når et sæt er leveret, kører jeg import og bagning (se `Docs/Performance-Battle.md` og `Content/Python/bake_all_vat.py`, når Codex har afleveret den), og kobler klippene til situationerne i `StrategyInfantryVisualComponent` og kavaleri/artilleri-komponenterne.

## 6. Lade-animationer til gammeldags gevær (2026-10-13)

Behov: lade **stående, knælende og liggende**, hver i to våbentyper. Et klip pr. stilling og våbentype, delt i faser med markører (så spillet kan strække hver fase til epokens ladetid og sætte lyd på de rigtige øjeblikke). Skelet og krav som i afsnit 0 (Mixamo-skelet, 30 fps, in place, geværet i hånden som del af klippet).

| # | Klip | Faser (markører) | Længde | Findes i dag |
|---|---|---|---|---|
| L1 | `A_Load_Muzzle_Standing` (forlader) | 1 sænk gevær, kolbe ved foden · 2 tag patron og bid den op · 3 hæld krudt i løbet · 4 kugle i munding, riv papir af · 5 træk ladestok, stød ned (2 stød), sæt tilbage · 6 løft gevær, fænghætte på (percussion) eller pandekrudt og luk pandejern (flintlås) · 7 anslag | 18-20 s i alt ved ca. 3 skud i minuttet (fasefordeling 1:1,5:2:2,5:5,5:3:1,5 pct. af tiden); strækkes til ladetiden | delvist: `A_Combat_ReloadStanding` (ét generisk klip) |
| L2 | `A_Load_Muzzle_Kneeling` | samme syv faser, knæ i jorden, kolben på låret; ladestokken trækkes kortere | som L1 | nej (kun overgange og knæl-idle) |
| L3 | `A_Load_Muzzle_Prone` | samme faser liggende på siden/ryggen: vend på siden, lad, vend tilbage i anslag; ladestokken kortere | som L1 | delvist: `A_Prone_Reloading` |
| L4 | `A_Load_Breech_Standing` (bagladning, Dreyse) | 1 sænk gevær · 2 åbn kammer (træk bolt tilbage) · 3 læg patron ind · 4 luk og spænd · 5 anslag | 4-6 s i alt (ca. 6-8 skud i minuttet) | nej |
| L5 | `A_Load_Breech_Kneeling` | samme fem faser knælende | som L4 | nej |
| L6 | `A_Load_Breech_Prone` | samme fem faser liggende (kan lades liggende) | som L4 | nej |
| L7 | `A_Load_Sitting` | beholdes (`A_Reload_sitting`) til siddende ryttere og dragoner | | ja |

Koblingen i spillet: ladetid pr. epoke og pr. stilling styrer, hvor hurtigt klippet afspilles (faserne strækkes ens), og lydmarkørerne sætter lyd på krudt, ladestok, hane og skud. Indtil klippene findes, bruges de nuværende generiske klip.
