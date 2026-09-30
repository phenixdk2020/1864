# Råmaterialer, udstyr og nye enheder

Vinduet **MATERIEL** åbnes med knappen ved siden af SLAGMARK.

## Råmaterialer
Alle tal er spillets skøn.

| Materiale | Enhed | Lager 1851 | Landet giver pr. md. | Importpris |
|---|---|---|---|---|
| Jern | t | 60 | 0 (maskinværksted +4) | 40 rd. |
| Kul | t | 80 | 0 (kulmine +30) | 12 rd. |
| Tømmer | læs | 200 | 30 (savværk +40) | 6 rd. |
| Krudt | tønder | 300 | 5 (krudtværk +40) | 25 rd. |
| Klæde | uniformer | 3.000 | 150 (klædefabrik +300) | 6 rd. |
| Læder | sæt | 2.500 | 100 (garveri +200, sadelmageri +120) | 4 rd. |

**Import:**
- KØB-knapperne køber i udlandet.
- I krig er alt 50 % dyrere.
- Uden herredømmet til søs kan intet importeres.

**Våbenværkerne** bruger råmaterialer. Mangler der noget, arbejder de kun så meget, som lageret tillader.

| Værk pr. md. | Laver | Bruger |
|---|---|---|
| Geværværksted | 150 geværer | 3 t jern, 1 t kul, 5 læs tømmer |
| Arsenal | 60 geværer og 1 kanon | 4 t jern, 2 t kul, 3 læs tømmer |
| Kanonstøberi | 2 kanoner | 8 t jern, 6 t kul |
| Krudtværk | 40 tønder krudt | 2 t kul |

**Intendanturen på AUTO** køber selv:
- jern og kul til tre måneders forbrug;
- klæde og læder til to bataljoner;
- krudt op til 200 tønder.

## Indkald en ny enhed
Man vælger fire ting:
- **Type:** linjebataillon, jægerkorps, dragonregiment, batteri eller ridende batteri.
- **Garnison:** en by med færdig kaserne. Der trænes enheden.
- **Hører under:** generalkommandoen.
- **Øvelser:** programmet den starter med.

Hver type kræver:

| Type | Mand | Geværer | Kanoner | Heste | Uniformer | Læder | Pris × |
|---|---|---|---|---|---|---|---|
| Linjebataillon | 760 | 760 | 0 | 14 | 760 | 760 | 1,0 |
| Jægerkorps | 760 | 760 | 0 | 12 | 760 | 760 | 1,15 |
| Dragonregiment | 560 | 560 | 0 | 600 | 560 | 1.120 | 1,6 |
| Batteri | 150 | 0 | 8 | 110 | 150 | 300 | 1,3 |
| Ridende batteri | 180 | 0 | 6 | 230 | 180 | 460 | 1,5 |

- Rekrutterne tages fra amtets mandskab.
- Manglende geværer købes i udlandet, og manglende heste købes i amterne.
- Enheden får en chef og en kaptajn for hvert kompagni eller eskadron. De er nyansatte.

## Gemning
`raw|jern|kul|tømmer|krudt|klæde|læder` i Economy-linjerne.

## Morterer og vogne
- **Morterbatteri:** 120 mand, 6 morterer, 12 vogne, 72 heste og 120 uniformer.
- **Fart:** et batteri med morterer marcherer med 80 % fart, når der er to vogne pr. morter. Er der for få vogne, går det med 50 %.
- **I slag** tæller en morter 80 % af en kanon.
- **Lager:** 12 morterer og 150 vogne i 1851.
- **Produktion:** arsenalet støber én morter om måneden, og vognfabrikken bygger 20 vogne (1 t jern og 10 læs tømmer).
- **Indkøb:** en morter koster 900 rd. i udlandet, en vogn 60 rd. i landet. Begge dele er dyrere i krig.
- **Units.json** har felterne `mortars` og `wagons`.
