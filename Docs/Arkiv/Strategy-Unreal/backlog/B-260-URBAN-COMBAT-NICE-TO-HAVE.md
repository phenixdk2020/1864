# B-260–B-269 — Urban combat / building occupation (Nice to Have)

**Status:** NICE TO HAVE / SENERE FASE  
**Prioritet:** Lavere end nuværende kernearbejde med movement, pathfinding, formationsadfærd, combat QA og Officer AI.  
**Formål:** Fastholde designidéerne om kamp i bygninger og forsvar af byer, uden at de trækkes ind i den aktive prototype før de vigtigere systemer er stabile.

## B-260 — Besæt bygning

**Status:** IDÉ / NICE TO HAVE

- En bygning kan besættes af en del af en enhed.
- Enhedens fulde styrke forsvinder ikke ind i bygningen; kun op til bygningens kapacitet går ind.
- Resten af kompagniet forbliver som støtte/hovedstyrke udenfor.
- Kompagniet forbliver som udgangspunkt én kommanderet enhed, men kan internt opdeles i fx `BuildingDetachment`, `SupportDetachment` og `MainBody`.
- Senere kan en egentlig `Detachér deling`-funktion gøre dele af enheden selvstændige.

## B-261 — Bygningskapacitet og firing slots

**Status:** IDÉ / NICE TO HAVE

- `OccupancyCapacity` og `FiringCapacity` er to separate værdier.
- Antallet af mænd inde i huset er ikke lig med antallet, der kan skyde samtidigt.
- Hver facade får definerede `FiringSlots` ved vinduer, døre, huller og evt. andre åbninger.
- Hvert slot kan have position, facing, fire arc, etage, cover, exposure og aktiv/inaktiv state.
- Antallet af aktive skytter afhænger af hvilken side fjenden befinder sig på.
- Reservefolk inde i bygningen kan lade, erstatte faldne og bemande andre facader.

## B-262 — Firing-slot rotation

**Status:** IDÉ / NICE TO HAVE

- Soldater ved vinduer kan følge samme kampcyklus som normal infanteriild: `AFVENT -> SIGT -> SKYD -> LAD`.
- Ved begrænset antal firing slots kan soldater rotere frem og tilbage mellem skydepladser og reserve.
- Bygningens kampkraft bestemmes primært af brugbare firing positions mod den aktuelle fjenderetning, ikke kun af samlet occupancy.

## B-263 — Bygningsskade, artilleri og brand

**Status:** IDÉ / NICE TO HAVE

Bygninger bør senere have separate states for mindst:

- `Structural Integrity`
- `Fire`
- `Cover`
- `Occupancy`
- `FiringSlots`

Artilleri og ild kan:

- reducere structural integrity,
- ødelægge eller skabe firing slots,
- reducere cover,
- antænde bygningen,
- give casualties på besætningen,
- udløse delvist eller totalt kollaps.

Materialetype skal have betydning, fx træ, bindingsværk, mursten og sten.

## B-264 — Ruiner

**Status:** IDÉ / NICE TO HAVE

- Ødelagte bygninger forsvinder ikke bare.
- En bygning kan blive til `RUIN`.
- Ruiner kan stadig give cover og enkelte firing positions.
- Kapacitet og beskyttelse ændres efter graden af ødelæggelse.
- Mursten/murbrokker kan blokere eller indsnævre gader og påvirke pathfinding.

## B-265 — Evakuering af bygning

**Status:** IDÉ / NICE TO HAVE

- Officer AI skal kunne evakuere en bygning ved alvorlig brand, kollapsrisiko, omringning eller dårlig tactical position.
- Soldater forlader via definerede exit points og forsøger at genforene sig med `MainBody`.
- Spilleren kan senere få en eksplicit `EVAKUER BYGNING`-ordre.

## B-266 — Forsvar by

**Status:** IDÉ / NICE TO HAVE

En senere ordre `FORSVAR BY` kan lade Officer AI opdele en by i taktiske sektorer, fx:

- yderste forsvarslinje,
- strongpoints,
- hovedgader,
- sidegader,
- torv/centrum,
- reserveposition,
- fallback-positioner,
- vigtige udgange/broer/stationer.

AI-officeren fordeler tropper efter bygningernes firing arcs, cover, kapacitet, forventet fjenderetning og reservebehov.

## B-267 — Byforsvar i dybden

**Status:** IDÉ / NICE TO HAVE

- Byforsvar skal ikke være én capture-cirkel.
- Forsvaret kan udvikle sig fra ydre huse til indre stillinger og strongpoints.
- En enhed kan trække sig fra ét hus/én gade til næste forberedte stilling.
- Gader kan fungere som fire lanes.
- Flere facader kan skabe crossfire uden en kunstig damage-bonus; fordelen kommer fra flere reelle skytter med LOS.

## B-268 — Byforberedelse og barrikader

**Status:** IDÉ / NICE TO HAVE

Ved tid til forberedelse kan et byforsvar senere opbygge:

- simple barrikader,
- vogne/tønder/træ på tværs af gader,
- sandsække/jord,
- forstærkede firing positions,
- alternative fallback-stillinger.

Ingeniører og officer/staff skill kan senere påvirke hastighed og kvalitet.

## B-269 — Bykontrol og storm

**Status:** IDÉ / NICE TO HAVE

- En by bør have flere vigtige control points frem for én universel capture-zone.
- Eksempler: torv, kirke, rådhus, station, bro og centrale vejkryds.
- `STORM BYGNING` og mere detaljeret bykamp kan komme senere uden krav om fuld indendørs FPS-pathfinding.
- Byen betragtes først som reelt kontrolleret, når centrale punkter og organiserede strongpoints er neutraliseret eller opgivet.

## Afhængigheder før dette tages op

Dette backlogområde må først prioriteres, når følgende kerneområder er væsentligt mere stabile:

1. formation-level pathfinding omkring almindelige hard blockers,
2. bridge/river routing,
3. Line/Column movement og facing,
4. combat range / fire eligibility / casualties,
5. Officer AI mission execution,
6. cover/LOS-baseline,
7. artilleri-baseline og objekt-/bygningsskade.

Det er derfor bevidst klassificeret som **Nice to Have** og ikke som aktivt prototype-scope.
