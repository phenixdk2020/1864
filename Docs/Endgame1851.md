# Valg af nation og kampagnens slutning (backlog 19 og 20)

## Spil som
**Nyt spil** i spilmenuen (M) har valget DANMARK eller SVERIGE-NORGE.

**Sverige-Norge** er en forberedelse:
- Landet spilles på den abstrakte model. I Statsrådet sætter spilleren regeringens prioriteter pr. ressort (0,1–3).
- AI'en styrer Danmark på kortet, med alle ressorter på AUTO.
- Kortet, kassen og vinduerne er stadig Danmarks, indtil Sverige får sit eget kort.

Test: `-CampaignNation=SE`.

## Slutningen
**Hvornår:** kampagnen slutter 1. januar 1867 eller med freden efter en krig. Spillet pauser og viser **Kampagnens udfald**. Luk vinduet for at spille videre.

**Pointene:**

| Del | Point |
|---|---|
| Monarkiet bevaret | 40 × andelen af monarkiets byer fra 1851, der ikke er afstået |
| Befolkningens vækst | Væksten i procent, fra −10 til 20 |
| Finanserne | (kasse − gæld) / 100.000, fra −15 til 15 |
| Krigen | Krigsstilling × 15. Freden bevaret giver +10 |
| Stemningen | Stemning / 10 |

**Bedømmelsen:**

| Point | Bedømmelse |
|---|---|
| 80 eller mere | Storslået |
| 60 eller mere | Hæderligt |
| 40 eller mere | Tåleligt |
| Under 40 | Katastrofe (som i 1864) |

## Balance
Testbeløbet i statskassen (5.000.000 rd.) er stadig sat. Det skal ned på 150.000 før rigtigt spil, og så bør priserne efterprøves.

Test: `-CampaignOpenWindow=end`.
