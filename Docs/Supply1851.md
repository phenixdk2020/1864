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

## Trænkolonner (F-5, v00.00.42)
- **Antal:** Hæren har 6 trænkolonner i 1851. Hver er 20 vogne og 80 heste, og en ny koster 2.500 rd. (knappen **KØB KOLONNE** i SKANSER-panelet).
- **Last:** En kolonne laster op til 12.000 rationer, 4.000 foderrationer og 6 ammunitionsladninger på det nærmeste depot med forråd. Den kører ad vejene med 25 km om dagen, i vinterføre kun 60 %.
- **Aflevering:** Kolonnen afleverer det, enheden eller skansen mangler, og kører hjem med resten. Er enheden marcheret videre, følger kolonnen efter.
- **Automatisk:** Enheder uden depot i nærheden bestiller selv en kolonne i tide, altså når proviant er under køreturen + 1 dag. **SEND FORSYNING** i hær- og skansepanelet sender en med det samme.
- **På kortet:** Kolonnerne ses som hestevogne, når man er zoomet ind.
## Byggematerialer (backlog 4, v00.00.44)
- Teglværker leverer mursten for 600 rd. om måneden og savværker tømmer for 300 rd. til materialelageret i deres by. Det gælder også private værker, som staten køber af.
- En by kan højst have 5.000 rd. på lager.
- Lageret betaler op til halvdelen af nyt byggeri inden for 30 km, sammen med materialer fra nedrivning.
- Lageret står i bypanelet og i FORSYNING-vinduet.

## Forsyningsvinduet og forsyningskortet (backlog 5, v00.00.44)
- **Menuknappen FORSYNING:** viser statens lager (geværer, kanoner, heste, trænkolonner, forråd pr. måned), depoter (proviant, foder, ammunition, materialer), kolonner undervejs og enheder i felten. Enhederne står efter forsyning, de dårligst forsynede først, med farve og SEND-knap.
- **Forsyningskort (tasten F):** viser en grøn ring om hvert depot med forråd (rækkevidde 25 km) og en farvet prik pr. enhed: grøn = forsynet, gul = under 2 dage, rød = under 1 dag. Trænkolonnerne vises som brune prikker.