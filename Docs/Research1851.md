# Forskning og doktriner

Vinduet **FORSKNING** er menu 10.

## Forskning
- Der kører ét projekt ad gangen, og det betales hver måned.
- Skifter man projekt, tabes det arbejde, der allerede er gjort på det gamle.
- Er der ikke penge i kassen, står projektet stille.
- Står Krigsministeriet på **AUTO** i Statsrådet, vælger det selv det første åbne projekt, som kassen kan bære over reserven.

| Id | Projekt | Fra | Måneder | Rd./md. | Kræver | Virkning |
|---|---|---|---|---|---|---|
| sanitation | Sanitetsvæsenet | 1852 | 12 | 800 | | Tab i slag −20 % |
| fortress | Fæstningsbyggeri | 1852 | 12 | 1.200 | | Skansernes dækning +10 %-point |
| conserves | Konserves og feltbagerier | 1853 | 10 | 1.000 | | 6 dages proviant båret (før 4) |
| telegraph | Felttelegrafen | 1854 | 12 | 1.500 | | Indkaldelse ×1,25 |
| staff | Stabsskolen | 1855 | 18 | 1.000 | | Kampværdi ×1,05 |
| railmob | Jernbanemobilisering | 1856 | 12 | 1.500 | telegraph | Indkaldelse yderligere ×1,25 |
| riflegun | Riflede kanoner | 1858 | 18 | 3.000 | | Kanoner (felt og skanse) ×1,4 |
| breech | Bagladegeværet | 1860 | 24 | 4.000 | staff | Infanteri ×1,25 |

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
