# Broer

## Hvor der er broer
- **Faste broer:** hvor en vej mellem to byer krydser vand på 20–900 m, er der en bro, fx over havneløbet i København.
- **Pontonbroer:** hvor en færge krydser et sund på højst 1,2 km, fx Alssund, kan der lægges en pontonbro.
- **På kortet:** broerne vises tæt på kortet som et lille **BRO**-skilt, som man kan klikke på.
  - Røde skilte er sprængte broer.
  - "(bro)" er et sted, hvor en pontonbro kan lægges. Det vises kun helt tæt på kortet.

## Broerne i 1851 (ved spillets start)
| Bro | Type | Længde |
|---|---|---|
| Knippelsbro, København | Fast bro (historisk; lagt over kanalen, hvor kortet har den) | ca. 840 m på kortet |
| Broen ved København (vejen vestpå) | Fast bro | 40 m |
| Broen ved Nykøbing F (Guldborgsund) | Fast bro | 360 m |
| Limfjorden (Aalborg–Nørresundby) | Sted for pontonbro (færge) | 325 m |
| Alssund ved Sønderborg | Sted for pontonbro (færge) | ca. 1,3 km |

- **Samme bro på flere veje:** hvis flere vejforbindelser går over den samme bro, er det én bro. Sprænges den, afskæres dem alle.
- **Kortets kystlinje er forenklet:** byens historiske broer lægges derfor over det vand, kortet har tættest på deres rigtige plads.
- **Langebro** falder sammen med Knippelsbro på kortet.
- **Broer over floderne:** hvor en vej mellem to byer krydser en flod eller kanal, er der også en bro, fx Broen over Gudenå ved Randers (se Hydro1851.md).

## På kortet
- **Faste broer:** et stendæk med rækværk i begge sider.
- **Pontonbroer:** et mørkt trædæk.
- **Sprængt:** to stumper.
- **Under bygning:** to ender.

## Handlinger
| Handling | Pris | Tid | Virkning |
|---|---|---|---|
| Spræng | 500 rd. | Straks | Vejen afskæres for begge sider. Marcher, tog og kolonner må finde en anden vej |
| Genopbyg | 4.000 rd. | 20 dage (dobbelt i frost) | Vejen åbnes igen |
| Læg pontonbro | 25.000 rd. | 45 dage | Færgen erstattes af en bro. Det går hurtigere over sundet, men fjenden kan også gå over uden både, hvad flåden end gør, medmindre broen sprænges |

## Slagmarken
Broer inden for slagmarken kommer med i `bridges` (navn, x, y, længde og tilstand). En sprængt bro vises som et hul i vejen.

## Gemning (v27)
`bridge|id|tilstand|dage` for de broer, der er ændret.
