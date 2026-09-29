# Udenrigs: diplomati og handel

Vinduet **UDENRIGS** er menu 9. Nationerne står i `Data/Campaign1851/Nations1851.json`: Danmark, Sverige-Norge, Preussen, Østrig, Storbritannien, Frankrig og Rusland.

## Forholdet (−100 … 100)
- **Udgangspunkt:** `relation` i JSON-filen. Kampagnens seed varierer det med op til ±20 × afvigelsen.
- **Hver måned:**
  - Forholdet glider 2 % tilbage mod udgangspunktet.
  - Preussen og Østrig: forholdet falder, når spændingen er over 25, med (spænding − 25) / 40.

## Handlinger
| Handling | Pris | Krav | Virkning |
|---|---|---|---|
| Gesandt | 5.000 rd. | 90 dage mellem to gesandter til samme land | Forholdet stiger op til +8 (mindre, jo bedre forholdet er i forvejen) |
| Handelstraktat | 10.000 rd. | Forhold ≥ 20, ikke i krig med landet | + `tradeValue` rd. om året og forholdet +5. Byer med mindst 4.000 indbyggere vokser 0,05 %-point mere om året pr. traktat |
| Alliance | 20.000 rd. | Kun Sverige-Norge; handelstraktat og forhold ≥ 60 | Ved krig kommer 3 svensk-norske bataljoner (760 mand) til København. Sverige-Norge betaler dem. Spændingen stiger med 4 |
| Garanti | 15.000 rd. | Storbritannien, Frankrig eller Rusland; forhold ≥ 50 | Hver garanti tager en femtedel af stigningerne i spændingen (højst 70 % dæmpning). I krig kan der komme en fredskonference |

## Øresundstolden
- Indtil april 1857 giver den 60.000 rd. om året. Det er spillets skøn for den del, der går til udviklingsbudgettet.
- Københavnstraktaten afskaffer tolden. Derefter betaler søfartsnationerne en kapitalisering på 30.000 rd. om året i 20 år.

## Krig og fred
- **Fredskonferencen i London:** samles efter 60 dages krig, hvis mindst én garantimagt har forhold ≥ 40.
- **SLUT FRED:** besatte danske byer afstås. De bliver udenlandske (`bForeign`), og deres amter betaler ikke længere. Fjendens korps og slagene forsvinder, og spændingen sættes til 30.

## Gemning (v23)
Gemmes som `Diplomacy`-linjer:
- `state|sundAfskaffet|kapitaliseringsår|konferencedag|krigsstartdag`
- `nation|id|forhold|handel|alliance|garanti|allieredeKommet|gesandtdag`
- `ceded|by`

## Test
`-CampaignDiplomacy=GB:0;GB:0;SE:1;RU:3 -CampaignOpenWindow=foreign`
