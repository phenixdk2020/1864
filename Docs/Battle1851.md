# Slag: kampagnen ↔ 3D-slagene (backlog 9)

## I kampagnen
- **Hvornår et slag opstår:** Et fjendtligt korps kommer inden for 5 km af danske tropper eller en færdig skanse.
- **Hvad spillet gør:** Det pauser og viser **slagpanelet** med de danske styrker (enheder og skanser, inden for 8 km), fjenden og et skøn over chancen for dansk sejr. Der er tre valg:
  1. **UDKÆMP I 3D:** skriver `Saved/Battle/BattleRequest_N.json`. Kampagnen venter på `Saved/Battle/BattleResult_N.json` og læser det ind, når det dukker op. Filen omdøbes derefter til `.read`.
  2. **AFGØR AUTOMATISK:** kampagnen regner selv. Det kan også vælges, mens den venter på 3D.
  3. **TRÆK TILBAGE:** de danske enheder går nordpå, og skanserne opgives.

### Den automatiske afgørelse
- **Dansk styrke** er summen af:
  - hver enhed: mand til stede × kvalitet × forsyning. Kvaliteten er træfsikkerhed, ladehastighed, stormangreb, moral og samhørighed. Forsyningen er ammunition og proviant.
  - hver kanon: 60 mand.
  - hver skanse: mand inde × (1 + 2 × dækning), plus reserven × (1 + reservens dækning), plus kanoner × 60 × ammunition.
- **Fjendens styrke:** mand × kvalitet + kanoner × 60. Kvaliteten er Preussen 1,3 (tændnålsgeværet), Østrig 1,05 og Forbundet 0,9.
- **Doktrin og forskning** (*Research1851.md*):
  - Den danske kampkraft ganges med doktrinens faktor, ×1,05 med stabsskolen og ×0,9 under en omstilling.
  - Infanteriet ganges med 1,25 med bagladegeværet.
  - Kanonerne ganges med 1,4 med riflede kanoner.
  - Skansernes dækning øges med fæstningsdoktrinen og fæstningsbyggeriet.
  - Egne tab ganges med sanitetsvæsenets og doktrinens faktor.
- **Chance for sejr:** r² / (1 + r²), hvor r = dansk styrke / fjendens styrke.
- **Tab:**
  - Taberen mister 15–25 % og vinderen 5–10 %. Ved uafgjort mister begge 8–14 %.
  - Enhederne bruger 30–70 % af ammunitionen og får +5 i erfaring.
  - Moral og samhørighed falder, mest for taberen.
- **Følger:**
  - **Dansk nederlag:** skanserne er taget, kanonerne er tabt, og kompagnierne går til deres bataljoner. Enhederne trækker sig tilbage til nærmeste danske by nordpå, og korpset går videre efter 3 dage.
  - **Dansk sejr:** korpset trækker sig og hviler 20 dage. Under 4.000 mand opløses det.

## BattleRequest_N.json (kampagnen skriver)
```json
{
  "format": "PROJECT1864-BattleRequest-1",
  "battleId": 3, "date": "1864-02-02T10:00:00.000Z", "season": "Vinter", "snow": true,
  "lat": 54.52, "lon": 9.56, "nearTown": "Slesvig",
  "danishUnitIds": ["J1", "B12"], "fortIds": [1, 2],
  "unitsFile": "Units.json", "fortsFile": "Fortifications.json",
  "enemy": { "name": "Preussisk I. Korps", "nation": "PR", "men": 25400, "guns": 101,
             "rifle": "Dreyse tændnålsgevær (bagladeriffel)", "quality": 1.3 },
  "resultFile": "BattleResult_3.json"
}
```
- `battlefieldFile`: slagmarkens terræn (se Battlefield1851.md), bygget 8 km omkring slaget.
- Enhedernes og skansernes fulde data står i `Units.json` og `Fortifications.json`, som skrives i samme øjeblik.
- `Units.json` indeholder mand til stede, forsyning og kampværdier.
- `Fortifications.json` indeholder kanoner, dækning, kompagnier og magasin.

## BattleResult_N.json (dit 3D-slag skriver)
```json
{
  "battleId": 3,
  "outcome": "danish_victory | enemy_victory | draw",
  "units": [ { "id": "J1", "losses": 140, "ammoUsed": 0.6 }, { "id": "B12", "losses": 95, "ammoUsed": 0.4 } ],
  "forts": [ { "id": 1, "captured": false }, { "id": 2, "captured": true } ],
  "enemyLosses": 2300
}
```
- `losses`: mand tabt (døde, sårede, fangne), pr. enhed-id fra anmodningen.
- `ammoUsed`: andel af en fuld ladning (0–1).
- `captured: true`: skansen er taget. Kanonerne er tabt, og kompagnierne går til deres bataljoner.
- `outcome` styrer tilbagetog og fjendens hvil på samme måde som den automatiske afgørelse.

**Test:** `-CampaignWarTest -CampaignAutoBattles` afgør alle slag automatisk og lader tiden løbe.
