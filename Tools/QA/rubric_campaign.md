# Tjekliste til QA af kampagnekortet og menuerne

Bruges af `Tools/QA/campaign_qa.py`. Billederne er skærmbilleder af kortet i tre afstande (12, 4 og 1,5 km), af alle vinduer og af startmenuen. `crop_`-billeder er udsnit i fuld opløsning af et skærmbillede (venstre/højre, øverst/nederst), så teksten kan læses. Hvert punkt har et id, som fundene skal bruge i feltet `kind`.

## Kortet

| id | Hvad der er forkert | Hvordan det ser ud |
|---|---|---|
| `house_on_road` | Huse står på vejene eller gaderne | Et hus, en gård eller et træ står oven på et vejbånd eller en bygade (kendt fejl i backloggen). |
| `floating` | Ting svæver eller er sunket | Bygninger, træer eller markører over eller under terrænet, eller kun halvt synlige. |
| `overlap` | Ting oven i hinanden | To bygninger i hinanden, en by-markør oven i et navn, mærkater der dækker hinanden. |
| `empty_town` | By uden bygninger | En dansk by, der kun er en markør uden huse, når man er tæt på (4 km eller nærmere). |
| `wrong_style` | Forkert stil for egnen | Bygninger der ikke passer til stedet (fx byhuse midt på marken, hedegårde på øerne). |
| `colour` | Farver der skærer | Skarpe, mættede farver, der bryder med den dæmpede kortstil; tydelige skel mellem kortfliser. |
| `terrain` | Fejl i terrænet | Huller, takker, flimrende flader, kyst eller vand på forkert sted, sømme mellem fliser. |

## Menuer og vinduer

| id | Hvad der er forkert | Hvordan det ser ud |
|---|---|---|
| `text_overflow` | Tekst flyder ud | Tekst går ud over sin knap, celle eller vinduets kant, eller er skåret af midt i et ord. |
| `text_overlap` | Tekst oven i tekst | To tekster eller en tekst og et ikon oven i hinanden. |
| `unreadable` | Ulæselig | For lille, for lav kontrast mod baggrunden, eller sløret. |
| `clipped_window` | Vinduet skåret af | Vinduet går ud over skærmkanten, eller dets indhold er skåret af uden rullebjælke. |
| `missing_text` | Manglende tekst | Tomme felter, hvor der burde stå noget; overskrift uden indhold. |
| `placeholder` | Pladsholder eller fejltekst | Engelske udviklertekster, `None`, `NaN`, `-1`, tekniske id'er, mærkelige tegn (fx `?` i stedet for æ/ø/å). |
| `other` | Andet | Alt andet, der ser forkert ud. |

## Projektets valg (ikke fejl)

- Kortet er bevidst uden dynamisk lys. Lyset er malet ind (skyggeside mod sydøst), og bygningerne har en fast skygge.
- Bygninger og træer ses kun tæt på (zoomet ind). Fra 12 km er der kun by-markører og navne.
- Nabolandenes byer (Sverige, Norge, de tyske stater) har endnu ingen bygninger. Det er planlagt, ikke en fejl.
- Statskassen viser et testbeløb (5.000.000 rd.).
- Teksten i spillet er dansk. Engelske tekster er en fejl (`placeholder`).

## Alvor

- **3**: ses straks og ødelægger billedet, eller teksten kan ikke læses.
- **2**: ses ved nærmere eftersyn.
- **1**: småting.

## Regler for bedømmerne

- Rapportér kun det, der faktisk kan ses på billedet. Hvert fund skal have billedets navn og et sted i billedet.
- Skriv beskrivelsen på dansk. Sæt `confidence` ærligt mellem 0 og 1. Ingen fund er et gyldigt svar.
