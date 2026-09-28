# Veje og jernbaner 1851 (UE-CAMPAIGN v00.00.26)

Designmanual 20.16.1: infrastruktur består af **forbindelser**, ikke regionsbonusser. Kortet har 105 forbindelser mellem nabobyer. De kommer fra `tools/map1851/build_map.py` og står under `links` og `railways` i `Data/Campaign1851/Denmark1851_Map.json`.

## Forbindelser

| Felt | Indhold |
|---|---|
| `a`, `b` | Byerne i hver ende |
| `roadKm`, `ferryKm` | Km på land og over vand. `ferry` er navnet på overfarten. |
| `km` | Vejens forløb (projicerede km) |
| `rail`, `railKm` | Hvor en jernbane mellem byerne kan gå. Den følger terrænet med ca. 4 gange så stor vægt på stigning som en vej, og den krydser aldrig vand. Feltet mangler, hvor det ikke kan lade sig gøre. |
| `chaussee` | Vejen var brolagt/makadamiseret chaussé i 1851. Listen er omtrentlig. |

## Jernbaner i 1851

| Linje | Åbnet |
|---|---|
| København–Roskilde | 27.6.1847 |
| Christian VIII's Østersø-Jernbane, Altona–Elmshorn–Neumünster–Kiel | 18.9.1844 |
| Glückstadt–Elmshorn | 20.7.1845 |
| Rendsborg–Neumünster | 18.9.1845 |
| Berlin–Hamborg, gennem Lauenborg (Büchen) | 15.12.1846 |
| Lübeck–Ratzeburg–Mölln–Büchen | Under anlæg. Åbner 15.10.1851 i spillet. |

## Projekter

| Projekt | Pris | Byggetid | Drift |
|---|---|---|---|
| Chaussé (forbedr vejen) | 2.500 rd./km | 40 dage + 2,5 dage/km | 40 rd./km/år |
| Jernbane | 12.000 rd./km + station i hver ende uden station (19.100 rd.) | 150 dage + 6,5 dage/km | 150 rd./km/år |

- Byggeriet betales som bygningerne:
  - 20 % udbetales ved start.
  - Resten går som dagløn.
  - Arbejdet går i stå uden penge.
- Arbejdet er jordarbejde, så det går i vintertakt 0,3 fra december til februar.
- Jernbaneprisen er **statens andel**. Banerne i 1850'erne blev bygget af koncessionsselskaber med privat (mest engelsk) kapital. Staten gav jord, en aktiepost og rentegaranti.
- Én igangværende opgave pr. forbindelse.
- En jernbane rydder huse og træer langs sporet, når arbejdet går i gang.

## Rejsetid

| Hvordan | Tid |
|---|---|
| Landevej | 20 km pr. dagsmarch |
| Chaussé | 27 km pr. dagsmarch |
| Færge | + ½ dag |
| Jernbane | 1 dag til lastning + 300 km/dag |

Tiderne vises i byens panel. Hærenes march skal senere bruge de samme tal.

## På kortet

- **Jernbane:** mørk banedæmning med hvide streger (klassisk kortsignatur), tog der pendler, og en station i hver by.
- **Under anlæg:** jorddæmningen vokser først. Sporet følger efter de første 60 % af tiden. En arbejdsvogn kører ved fronten, og der er en fremdriftsring midt på linjen.
- **Chaussé:** lys stenbelægning oven på landevejen.
