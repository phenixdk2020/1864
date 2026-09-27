# Asset-spec — Dansk linjeinfanterist (1864)

Reference: `Reference/Units/Infantry_Linje_Turnaround.webp`, `Infantry_Linje_APose.webp`
Kode der bruger assetet: `AInfantryCompany` (`Source/Game1864/Public/InfantryCompany.h`)

## 1. Grundkrav fra designmanualen

- **1:1:** én simuleret soldat = én synlig figur. LOD må aldrig reducere antallet, kun detaljen (F30E/F30L).
- Kompagni = 190 mand i 3 geledder, ca. 48 m front (64 rotter × 75 cm).
- Faldne bliver liggende på slagmarken ("Vis faldne 1:1").
- Samme figur skal kunne bære alle regimenter → farver styres af materialet, ikke af separate teksturer.

## 2. Historik der skal afklares før modellen låses

| Punkt | Billedet viser | Tjek |
|---|---|---|
| Hovedbeklædning | Shako med kokarde og pompon | Brugte linjeinfanteriet i 1864 shako eller kasket/kabudse? |
| Årstid | Waffenrock uden kappe | Vinterfelttoget feb.–apr. 1864 → kappe som alternativ outfit |
| Gevær | Ligner flintlås | Skal være slaglås (percussion) |

## 3. Opdeling i dele (separate meshes / material-slots)

| Del | Separat mesh? | Grund |
|---|---|---|
| Krop + ansigt + hænder | Basis | Ansigtsvariation via slot 12 |
| Waffenrock | Ja | Kappe-variant senere |
| Bukser | Del af krop | Farve via custom data |
| Hovedbeklædning | Ja | Shako ↔ kasket, officer-variant |
| Krydsremme + patrontaske + bajonetskede | Ja | Officerer/tamburer uden |
| Tornyster + rulle + feltflaske | Ja | Kan droppes (march vs. kamp) |
| Gevær med bajonet | Ja, static mesh | Sættes på socket, skal kunne skifte hånd |

## 4. Trekantbudget og LOD

| LOD | Afstand (ca.) | Trekanter | Indhold |
|---|---|---|---|
| LOD0 | < 25 m | 15–25k | Fuld detalje, knapper, remme i geometri |
| LOD1 | 25–60 m | 5–8k | Remme bages til normal map |
| LOD2 | 60–150 m | 1,5–2,5k | Silhuet + farvefelter |
| LOD3 | > 150 m | 300–600 | Impostor-agtig, stadig 1:1 |

Over ca. 175 m skifter semantic zoom til NATO-counters (F29Q), men figurerne forbliver i scenen.

## 5. Materiale — per-instance custom data

Materialet læser `PerInstanceCustomData` (sættes af `AInfantryCompany::ApplyLivery`):

| Index | Betydning |
|---|---|
| 0–2 | Frakke (RGB, lineær) |
| 3–5 | Bukser |
| 6–8 | Opslag: krave + ærmeopslag |
| 9–11 | Passepoil (kantning) |
| 12 | Variant 0–1: ansigt, skæg, slid/snavs |

Teksturen skal derfor have en **maske-tekstur** (R = frakke, G = bukser, B = opslag, A = passepoil) oven på en neutral grå albedo, så farven multipliceres ind i materialet.

## 6. Skelet, sockets og animation

- Rig til **UE5 Manny-skelettet** (A-positur), så Unreals animationer kan retargetes.
- Sockets: `hand_r_musket`, `hand_l_support`, `back_musket` (over skulder), `head_hat`.
- Første animationssæt: stå, marchere, lade, sigte + skyde, falde (fremad/bagud), liggende død.
- Crowd-rendering: animationerne bages med **AnimToTexture** (aktiveret i projektet) til vertex-animation på instanced static meshes.

## 7. Pipeline

1. Billede → 3D (multi-view med front/side/ryg) eller manuel modellering i Blender.
2. Opdel i delene fra afsnit 3, retopologi, UV'er.
3. Bag normal-, AO- og maskekort.
4. Rig til Manny, lav LOD'er.
5. Importér til `Content/Units/Infantry/`, bag VAT med AnimToTexture.
6. Sæt `SoldierMesh` + `SoldierMaterial` på `AInfantryCompany`.
