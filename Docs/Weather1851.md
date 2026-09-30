# Vejr og føre (backlog 4)

Dagens vejr beregnes fra kampagnens seed. Derfor skal intet gemmes: samme seed giver samme vejr.

## Temperaturen
Temperaturen er summen af:
- det danske månedsgennemsnit, glidende mellem månederne;
- vinterens hårdhed det år (−3 til +2 °C, november–marts);
- perioder på flere dage (svingninger over 9 og 23 dage);
- dagens egen variation (±1,5 °C).

## Vejret
Reglerne prøves i denne rækkefølge:
1. **Storm:** 5 % af dagene i oktober–marts, ellers 2 %.
2. **Tø:** mildt vejr (over 1 °C) efter en kold periode.
3. **Regn eller sne:** med månedens chance for nedbør (38–58 %). Det er sne under 0,5 °C.
4. **Frost:** under −1 °C.
5. Ellers er det **klart**.

## Føret (fart på vejene)
| Vejr | Landevej | Chaussé | Tværs over marken |
|---|---|---|---|
| Regn (marts–april, oktober–november) | 0,70 | 0,85 | 0,61 |
| Regn (ellers) | 0,85 | 0,93 | 0,81 |
| Sne | 0,60 | 0,80 | 0,48 |
| Tø | 0,50 | 0,75 | 0,35 |
| Storm | 0,80 | 0,90 | 0,74 |
| Frost | 0,95 | 0,98 | 0,94 |

- **Jernbanen:** 0,75 i sne og storm.
- **Færger:** 0,25 i storm.
- **Trænkolonner:** kører med føret opløftet i 1,2 og er derfor lidt langsommere end marcherende tropper.

## Isvinter
- Hvis der har været mindst 8 dage under −2 °C inden for 14 dage, bærer de smalle farvande (Slien, Alssund).
- Fjenden kan så gå over, selv når flåden behersker farvandet.

## Hvor vejret ses
- **Kalenderen:** årstid, vejr og temperatur.
- **Slag-anmodningen:** felterne `weather`, `temperatureC` og `ice`.
