# Erfaring: optjening og praktisk bonus (2026-10-10)

Beskriver hvordan det virker i koden i dag (`Campaign1851Army.cpp`, `Campaign1851Battles.cpp`, `Campaign1851Officers.cpp`, `Strategy1864/Combat/StrategyConditionComponent.cpp`, `AI/StrategyOfficerProfileComponent.cpp`) og forslag til det vi mangler.

## 1. Enhedens erfaring (0-100)

**Optjening i kampagnen (i dag):**

| Kilde | Hvor meget | Loft |
|---|---|---|
| Grunduddannelse (indkaldt enhed) | op til +20 over uddannelsen | 40 |
| Marchdage | +0,02 pr. dag | 70 |
| Et slag | +5 (flad, uanset udfald og tab) | 100 |
| Nye rekrutter blandes ind | vægtet middel med rekrutternes erfaring | |

Navngivning: under 20 rekrutter, 20-39 øvede, 40-59 erfarne, derefter veteraner (stjerner = erfaring / 20).

**Færdigheder (0-100, seks stk.) er noget andet end erfaring:** ladegreb, skydning, eksercits, feltøvelse, udholdenhed, bajonet. De trænes med øvelsesprogrammet (Øvelser) og påvirker slaget gennem `BattleFactors`: ladetid 1,30 til 0,85, nøjagtighed 0,70 til 1,25, deployhastighed 0,75 til 1,20, skyttekæde 0,70 til 1,20, træthed 1,30 til 0,75, storm 0,75 til 1,25.

**Hvad erfaring gør i 3D-slaget:**
- Nøjagtighed og effektivitet: ×0,85 (ingen erfaring) til ×1,20 (fuld), ganget med træthedsfaktor (`GetAccuracy...` i `StrategyConditionComponent.cpp`).
- Moralchok: ×1,10 (ingen) til ×0,70 (fuld). Veteraner knækker langsommere.
- Artilleri: erfaring påvirker skydenøjagtighed (`StrategyArtilleryFireMissionComponent.cpp`).
- Overføres som `experience` til slagets enhedsdata (`Saved/Battle/Units.json`).

**Hvad der IKKE sker i dag:** slaget skriver ikke erfaring tilbage pr. situation; kun +5 flad bonus efter slaget. Ingen forskel på at kæmpe hårdt og at stå i reserve. Ingen nedgang ved lang inaktivitet. Ingen "veteranegenskaber" der er synlige for spilleren.

## 2. Officerens erfaring (0-100) og egenskaber

**Optjening:** 0,05 pr. dag på march, 0,01 pr. dag ellers; +6 ved sejr og +3 ved nederlag (både chef og general). Startværdi 5-20 (almindelig officer), 50-80 (general).

**Bruges til:**
- **Officerens rating** (0-100) = vægtet middel af ti egenskaber (føring, stab, inspiration og så videre) + 10 pct af erfaringen. Ratingen vises og afgør, hvem der er bedst til en post.
- **Rangstigning:** hver rang kræver en mindste erfaring (`RankExperience`).
- **I 3D-slaget** (officerprofilen): stressreaktion = 70 pct sindsro + 30 pct erfaring (dårlig officer reagerer 1,30× langsommere/mere panisk, god 0,75×), beslutningsstabilitet = 40 pct sindsro + 30 pct erfaring + 20 pct disciplin + 10 pct taktik. Det er det, designdokumentets afsnit 23 beskriver (reaktionstid, vurderingsfejl).
- Chefens føring og inspiration påvirker enhedens samhold og moral på march og i kamp.

**Beslutning:** officerer kan ikke dø i kamp (kun såres eller fanges), så erfaring forsvinder ikke ved dødsfald.

## 3. Praktisk bonus: hvordan det omsættes (forslag til det der mangler)

| Hvad | Forslag |
|---|---|
| Erfaring pr. kampsituation | Slaget returnerer pr. enhed: tid under ild, hits givet, hits modtaget, om den holdt sig, charges, carré. Erfaring = f(tid i kamp, sejr, tab, moral holdt). En enhed i reserve får næsten intet; en under hård ild får meget (+3 til +12). |
| Færdighed efter brug | Skydning vokser af antal skud afgivet i slaget, bajonet af nærkamp og charge, feltøvelse af tid i skyttekæde/under dækning, eksercits af formationsskift, udholdenhed af march. Op til loft pr. træningsniveau. |
| Veterantræk (synlige) | Ved 40+ "Erfarne": hurtigere ladning. Ved 60+: kolde under ild (lavere moralchok). Ved 80+: kan karré og bajonet uden tøven. Vises som mærker på enhedskortet. |
| Tab koster erfaring | Mister en enhed mange mænd, falder erfaringen med blandingen af nye rekrutter (allerede i `Manpower.cpp`), og fyldes op med rekrutter. Slag med >40 pct tab giver en ekstra straf. |
| Officerens bonus til enheden | Chefens erfaring og føring giver enheden en bonus på ladetid (op til -5 pct) og samhold (+10 pct) og hurtigere ordremodtagelse (reaktionstid); en uerfaren chef giver straf. Vises som "Chef: +x pct" på kortet. |
| Udmærkelser | Officerer og enheder kan få en udmærkelse efter et slag (fx Dannebrogordenen, fanen) med fast bonus til moral og rekruttering. Gemmes i historikken. |
| Veteranreserve | Hjemsendte veteraner kommer tilbage ved mobilisering (højere erfaring end rekrutter). |
| Inaktivitet | Erfaring falder langsomt i garnison uden øvelser (−0,01 pr. dag over 60) for at give en grund til at øve. |

## 4. Tal vi bør kalibrere

Erfaring +5 pr. slag er lille ift. 0-100. Forslag: et middelslag +8, et hårdt +15, en sejr med små tab +4, en flugt −5 (men ikke under 0).

## 5. Afhængigheder

- After action report (Docs/AfterAction.md, under udvikling) skal levere tallene (tid under ild, skud, hits, tab).
- Kamp-AI (Docs/Kamp-AI-Design-v2.0.md, afsnit 20 og 23): officerkvalitet og moral/cohesion læser de samme tal.
- Enhedens og officerens historik (Backlog): hver erfaringsændring skrives med årsag.
