# Skanser (feltbefæstninger), Campaign 1851

## Historisk ramme
- Dybbøl-stillingen (1864) havde **10 skanser, 7 store og 3 små, med ca. 66 kanoner og 11 morterer**.
  Den stærkeste, skanse IV, havde **12 kanoner**. Da den blev stormet, fungerede kun 4 af dem.
- Omkring **5.000 mand** stod i forsvarsværkerne. Under bombardementet trak de fleste sig tilbage:
  skanse III var den 18. april ifølge en kilde forsvaret af kun 19 mand.
- Kilder: [Battle of Dybbøl](https://en.wikipedia.org/wiki/Battle_of_Dybb%C3%B8l),
  [Naturstyrelsen, Dybbøl Banke](https://eng.naturstyrelsen.dk/experience-nature/explore-denmark-s-nature-with-our-guides/dybboel-heights/attractions),
  [Military history Denmark](https://english.military-history-denmark.dk/?page_id=81).
- Besætningstallene nedenfor er **skøn**.

## I spillet
Skanser bygges på monarkiets land uden for byerne og mindst 350 m fra hinanden. Knappen **SKANSER** under SPILMENU
åbner panelet. Vælg type og klik på kortet. Fronten vender mod syd og kan drejes i skansens panel.

| | Lille skanse (lunette) | Stor skanse (lukket) |
|---|---|---|
| Pris / tid | 9.000 rd. / 45 dage | 28.000 rd. / 90 dage |
| Kanoner (start–maks.) | 2–4 | 6–12 |
| Infanteri (plads) | 120 | 250 |
| Kanonerer | 7 pr. kanon | 7 pr. kanon |
| Brystværn / grav | 2,5 m / 2,0 m | 3,2 m / 2,8 m |
| Vedligehold | 300 rd./år | 800 rd./år |

- **Kanoner:** +2 ad gangen for 3.600 rd. og 15 dage.
- **Byggetid:** Jordarbejde går næsten i stå i frost.
- **Forsvarsniveauer.** Stor skanse koster det dobbelte.

| Niveau | Indhold | Dækning | Pris (lille) | Tid |
|---|---|---|---|---|
| 1 | Brystværn og grav | 40 % | (i anlægget) | |
| 2 | Palisader og ulvegrave | 55 % | 3.000 rd. | 20 dage |
| 3 | Bombesikkert blokhus | 70 % | 6.000 rd. | 35 dage |
| 4 | Traverser og bombesikre magasiner (+0,3 m brystværn) | 85 % | 9.000 rd. | 50 dage |

## Data til 3D-slagene: `Saved/Battle/Fortifications.json`
Filen skrives, hver gang en skanse påbegyndes, bliver færdig, opgraderes eller drejes, og når et spil indlæses.

```json
{
  "format": "PROJECT1864-Fortifications-1",
  "date": "1851-07-01T00:00:00.000Z", "seed": 42,
  "fortifications": [{
    "id": 1, "name": "Skanse 1 ved Sønderborg", "type": "redoubt | lunette",
    "lat": 54.912, "lon": 9.735, "mapKmX": -16.9, "mapKmY": 323.8, "nearTown": "Sønderborg",
    "facingBearingDeg": 270,
    "built": true, "buildProgress": 1,
    "guns": 12, "maxGuns": 12, "gunType": "glatløbet fæstningskanon, 24-pund", "gunners": 84,
    "defenceLevel": 4, "defenceName": "...", "coverPercent": 85,
    "parapetHeightM": 3.5, "ditchDepthM": 2.8,
    "palisade": true, "blockhouse": true, "traverses": true,
    "infantryCapacity": 250, "garrison": 250, "sizeM": 84,
    "gunPlatforms": [{ "xM": 28.6, "yM": -6.3, "yawDeg": -35.5, "armed": true }]
  }]
}
```

**Felterne:**
- `facingBearingDeg`: fronten, i grader fra nord med uret (270 = vest).
- `gunPlatforms`: i skansens eget koordinatsystem, i meter fra centrum. `x` peger mod fronten, `y` til højre.
  `yawDeg` er kanonens skudretning i forhold til fronten. Kanonstillingerne fyldes op forfra: først fronten, så flankerne, så bagsiden.
- `sizeM`: skansens omtrentlige bredde. Kortet tegner skanserne større end virkeligheden, men dataene er i rigtig størrelse.
- `garrison`: infanteri i skansen nu. Den er fuld, når skansen er færdig. Senere hænger den sammen med regimenterne.

**Test:** `-CampaignBuildFort=54.912,9.735,stor,270;54.904,9.732,lille,250 -CampaignFortsComplete`.
