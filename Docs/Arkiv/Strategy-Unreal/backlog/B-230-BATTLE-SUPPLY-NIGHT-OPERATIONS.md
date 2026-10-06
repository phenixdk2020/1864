# PROJECT 1864 — B-230–B-239: Battle supply, mørke og overnight resupply

**Status: BESLUTTET / PLANLAGT EFTER P0A v00.00.09**  
**Designbaseline: v00.02.08**

Dette supplement fastlægger, hvordan forsyning under slag, mørke, natlig reorganisering og flerdagsslag skal fungere. Systemet bygger videre på de allerede besluttede supply-vogne, ammunition som konkret beholdning, strategiske supply lines, fysisk courier/order delay og simulationstid.

## B-230 — Battle supply er fysisk og begrænset

**BESLUTTET.** Enheder må ikke genopfylde ammunition magisk under kamp.

Resupply kræver mindst:

- en kompatibel supply source: ammunition wagon, caisson, field train, depot eller anden relevant node,
- faktisk ammunition i kilden,
- en brugbar rute mellem source og recipient,
- nok tid til transport og distribution,
- at recipienten ikke er fuldstændigt afskåret,
- kompatibel ammunitionstype/kaliber.

Supply source og rute kan trues, afbrydes, erobres eller ødelægges.

## B-231 — Forward ammunition og field trains

**PLANLAGT.** Ikke alle forsyningsvogne skal stå helt fremme ved skydelinjen.

Logistikken opdeles senere i mindst:

- field train / større reserve bag fronten,
- forward ammunition wagons/caissons tættere på formationerne,
- unit-level carried ammunition,
- artillery caissons/ammunition reserve.

Jo tættere supply flyttes på fronten, desto hurtigere resupply men desto større risiko for capture/destruction.

## B-232 — Resupply under aktiv kamp

**BESLUTTET.** Resupply kan ske under et slag, men langsommere og mere risikabelt end under roligere perioder.

Effektiviteten påvirkes bl.a. af:

- distance til supply source,
- road/terrain,
- enemy fire/interdiction,
- unit movement,
- morale/cohesion,
- Staff/Command Skill og organisatorisk kvalitet,
- availability af wagons/horses/drivers,
- ammunition priority.

En formation i voldsom ildkamp kan derfor få kun delvis eller forsinket resupply.

## B-233 — Nightfall er phase transition, ikke hard battle end

**BESLUTTET.** Mørke afslutter normalt den organiserede dagskamp, men behøver ikke afslutte selve tactical battle instance.

Battle state kan skifte:

`DAYLIGHT -> DUSK -> NIGHT -> DAWN -> DAYLIGHT`

Ved skumring reduceres normal formation combat kraftigt på grund af dårlig LOS, command/control og formation integrity. Spilleren/AI kan stadig udføre begrænsede night actions.

## B-234 — Night phase actions

**PLANLAGT.** Under natfasen kan formationer bl.a.:

- holde stilling,
- trække sig,
- repositionere begrænset,
- etablere/forbedre hasty fieldworks,
- samle og evakuere sårede,
- reorganisere formationer,
- hvile og reducere fatigue,
- modtage ammunition/food/forage/medical supply,
- sende couriers/reports,
- føre patrols/screens,
- gennemføre begrænset night attack/infiltration, hvis ordre/officer/doctrine tillader det.

Night attack er muligt, men har markant dårligere observation, coordination, formation control og higher friendly-friction risk. Det skal ikke være standardadfærd.

## B-235 — Overnight resupply

**BESLUTTET.** Natten giver et naturligt resupply-vindue, men kun hvis enheden reelt er forbundet til egne linjer.

En unit kan få overnight resupply, hvis:

- dens supply route til egen field train/depot er åben eller kan genetableres,
- supply source har beholdning,
- wagons/horses/transport capacity findes,
- fjenden ikke kontrollerer eller effektivt interdict'er ruten,
- enheden kan modtage/distribuere forsyningen.

Afskårne enheder får **ingen automatisk natlig ammunition**. De må leve af deres carried stocks, local capture/salvage eller genetablere kontakt.

## B-236 — Partial resupply og prioritet

**BESLUTTET.** Overnight resupply er ikke automatisk 100 %.

Distribution bestemmes af:

- available stock,
- transport throughput,
- distance,
- priority,
- formation need,
- staff/logistics efficiency,
- enemy interdiction.

Spilleren kan senere prioritere fx:

`CRITICAL | HIGH | NORMAL | LOW`

Artilleri og infanteri bruger separate ammunition pools; én overskydende ressource kan ikke konverteres magisk til en anden.

## B-237 — Flerdagsslag

**BESLUTTET.** Hvis begge sider stadig er i kontakt efter natten, kan samme battle fortsætte næste dag med persistent state:

- casualties,
- ammunition,
- morale/cohesion,
- fatigue,
- damaged/captured guns/wagons,
- fieldworks,
- officer casualties/state,
- positions,
- supply connectivity.

Der må ikke ske en skjult fuld reset ved midnat.

## B-238 — Daylight og auto-pause

**PLANLAGT.** Sunrise/sunset/dusk skal senere beregnes ud fra dato, geografisk position og relevant weather/light model frem for et universelt klokkeslæt.

UI kan auto-pause ved vigtige phase transitions:

- Dusk begins,
- Night established,
- Dawn begins.

Ved skumring kan spilleren få valg som:

`Hold positions | Withdraw | Continue night operations | Reorganize/resupply | Prepare dawn attack`

Auto-pause skal kunne konfigureres som assistance-setting senere.

## B-239 — Night/supply acceptance criteria

Senere prototype skal mindst verificere:

1. Unit ammunition falder ved fire og kan ikke regenerere uden supply source.
2. Intakt supply route + stock kan resupply en unit.
3. Afskåret unit modtager ingen automatic overnight resupply.
4. Delvis throughput giver partial, ikke full, resupply.
5. Dusk ændrer light/combat/AI state uden at slette battle state.
6. Normal line combat reduceres markant i darkness.
7. Night withdrawal/reorganisation/resupply kan fortsætte simulationen.
8. Casualties/ammo/fatigue/positions persisterer gennem natten.
9. Dawn kan starte næste dags kamp fra samme persistent battlefield state.
10. x20 accelererer natfasen uden at ændre de authoritative supply-regler.

## Scopeværn

Dette implementeres ikke i den første P0A v00.00.09 Officer AI-gate. Officer AI, pause/time controls og eksisterende combat-regression skal først valideres. Systemet designes dog nu, så Officer AI senere kan tage hensyn til ammunition, night posture og supply connectivity i sine decisions.
