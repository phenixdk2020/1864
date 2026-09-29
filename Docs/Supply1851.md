# Forsyning (Campaign 1851): første levering

Denne levering dækker F-1 til F-4, F-7 og F-11 i *Backlog-Forsyning.md*. Alle tal er skøn til spillet.

## På enheden
- Hver enhed har **proviant** (dage), **foder** (dage) og **ammunition** (andel af en fuld ladning: 60 patroner pr. mand, 120 skud pr. kanon, altså to kampdage).
- Ved spillets start har alle 4 dages proviant, 2 dages foder og fuld ammunition.
- **Garnison** (holder i en by i monarkiet): Proviant og foder fyldes op af hærens faste budget. Ammunition fyldes kun op, hvis byen har et depot med ammunition.
- **I felten eller på march** spiser enheden af det, den har med. Den fyldes op fra et **depot inden for 25 km** (en dagsmarch). Er der intet depot, køber den i en **by, den passerer**: byen sælger omkring en ration pr. tiende indbygger om dagen, til 0,18 rd. pr. ration.

## Mangel
- **Ingen proviant:** samhørighed −4 og moral −0,02 om dagen. Der går 0,4 % af mandskabet om dagen til sygdom og desertering.
- **Intet foder** (rytteri og artilleri): samhørighed −3 om dagen. 1 % af hestene dør om dagen.

## Depoter
| Bygning | Proviant | Foder | Ammunition (bataljonsladninger) |
|---|---|---|---|
| Garnisonens *Depot og magasin* | 40.000 | 20.000 | 30 |
| *Kornmagasin* | 60.000 | 60.000 | – |
| *Arsenal* | – | – | 200 |

- Hver måned køber intendanturen halvdelen af det, depoterne mangler.
  - Proviant koster 0,12 rd. pr. ration og foder 0,10 rd.
  - Ammunition koster 600 rd. pr. ladning, 40 % mindre med eget arsenal eller egen ammunitionsfabrik.
- I budgettet står det som "Forråd til depoterne".

## Skanser
- Skanserne har et magasin: 120 skud pr. kanon, 100 patroner pr. mand og proviant til 14 dage.
- Besætningen spiser af magasinet. Det fyldes fra et depot inden for 25 km eller ved opkøb i en nærliggende by.
- Tallene står i *Fortifications.json* (`roundsPerGun`, `cartridgesPerMan`, `foodDays`).

## Data til 3D-slagene: `Saved/Battle/Units.json`
Filen skrives hver måned og ved hver gemning. Hver enhed har:

```json
{ "id": "B4", "name": "4. Bataillon", "arm": "Linjeinfanteri", "lat": 55.39, "lon": 10.36, "place": "Odense",
  "men": 760, "maxMen": 760, "horses": 14, "guns": 0,
  "foodDays": 4, "fodderDays": 2, "ammoFraction": 1, "cartridgesPerMan": 60, "roundsPerGun": 0,
  "battleFactors": { "reloadTime": 1.0, "accuracy": 0.98, "...": 0 } }
```
