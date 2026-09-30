# Forskning og doktriner

Vinduet **FORSKNING** er menu 10.

## Forskning som træ
Vinduet FORSKNING viser et træ i stil med Hearts of Iron.
- **Grene:** fem søjler.
- **Niveauer:** rækkerne er niveau I, II, III osv.
- **Streger** går fra et projekt til det, det åbner for.
- **Rækkefølge:** et projekt kræver kun det ovenover i sin gren. Der er ingen årstal.
- **Start:** klik på en boks. Der åbnes en boks med symbolet, hvad projektet gør, pris og tid, hvad det kræver først, og knappen START (eller hvorfor det ikke kan startes endnu).
- **Symboler:** hver gren har sit symbol: rødt kors, stjerneskanse, telegraf, krydsede geværer og kanon.
- **Ét ad gangen:** der kører ét projekt ad gangen, og det betales hver måned. Står Krigsministeriet på AUTO, vælger det selv.

| Gren | I | II | III |
|---|---|---|---|
| Sanitet og forsyning | Sanitetsvæsenet (tab −20 %) | Konserves og feltbagerier (6 dages proviant) | Militærhospitaler (syge 40 % hurtigere tilbage) |
| Befæstning | Fæstningsbyggeri (dækning +10) | Kasematter og blendinger (dækning +10) | |
| Samfærdsel | Felttelegrafen (indkaldelse × 1,25) | Jernbanemobilisering (× 1,25) | |
| Hæren | Stabsskolen (kampværdi + 5 %) | Bagladegeværet (infanteri × 1,25) | |
| Artilleriet | Riflede kanoner (× 1,4) | | |

## Doktriner
- Et skift koster 5.000 rd. og −0,05 i moral for alle enheder.
- Derefter følger 60 dages omstilling. Imens er kampkraften −10 %, og der kan ikke skiftes igen.

| Niveau | Valg | Virkning |
|---|---|---|
| Strategisk | **Fæstningen** (start) | Dækning +10 %-point. Slag uden skanser −5 % |
| | Felthæren | Slag uden skanser +5 % |
| Operativ | **Koncentration** (start) | +8 % med 3 eller flere enheder i slaget, ellers −5 % |
| | Forsvar i dybden | Tab −20 %, kampkraft −3 % |
| Taktisk | Ildkamp | +10 % i slag ved skanser |
| | **Bajonetangreb** (start) | +10 % mod Østrig og Forbundet, −15 % mod Preussen |
| | Spredt orden | Tab −15 %, +3 % |

## Til 3D-slagene
`Units.json` og `BattleRequest_N.json` får to nye felter:
- `"doctrine": { strategic, operational, tactical, changing, lossFactor, fortCoverBonusPercent, gunFactor, infantryFactor }`
- `"research": [id, …]`

## Gemning (v23)
Gemmes som `Research`-linjer:
- `state|projekt|måneder|d0|d1|d2|omstillingSlutDag`
- `done|id`

## Test
`-CampaignResearchDone=sanitation,conserves -CampaignResearch=fortress -CampaignDoctrine=2:2 -CampaignOpenWindow=research`
