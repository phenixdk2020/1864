# PROJECT 1864 — B-280–B-291 Charge, Melee, Regiments-HQ og næste våbenarter

**Status:** AKTIV implementeringssekvens — v00.00.09f28 har nu første fulde Regiment-control MVP  
**Branch:** `channel-test`

## B-280 — Infantry CHARGE

**Status:** MVP IMPLEMENTERET i v00.00.09f25 / fortsat QA

CHARGE er en eksplicit ordre/state. Enheden lukker gennem normal fire-distance til fysisk kontakt uden at ranged AI stopper fremrykningen ved Close/Medium/Long.

MVP:
- `CHARGE [V]` + enemy target-pick;
- HOLD FIRE under charge approach;
- Line i åbent terræn, bridge-routing må midlertidigt kræve Column;
- ekstra charge-speed/cohesion-cost;
- transition til melee-manager ved fysisk kontakt.

## B-281 — Infantry melee completion

**Status:** MVP IMPLEMENTERET / fortsat QA

Eksisterende melee-core er koblet til CHARGE. FRONT/FLANK/REAR og charge momentum findes som første testlag.

Videre QA/arbejde:
- bedre contact frontage;
- tydelig engaged state;
- break-off / winner / loser / rout;
- mere visuel melee feedback;
- ingen formation overlap/teleport gennem modstanderen.

## B-282 — Under-fire reaction

**Status:** MVP IMPLEMENTERET i v00.00.09f26 / fortsat QA

AI ON company under movement, som modtager bekræftet fjendtlig ild fra valid enemy inden for egen Long range, suspenderer movement, deployer Line, vender mod truslen og besvarer ild når fire policy tillader det.

Regler:
- parent mission suspenderes, men slettes ikke;
- HOLD FIRE respekteres;
- bridge passage har prioritet og reaktion deferred til passagen er fri;
- AI OFF er direkte spiller-authority.

## B-283 — Second battalion

**Status:** MVP IMPLEMENTERET i v00.00.09f27

Struktur:
- Major A → 1. Bataljon → 4 kompagnier;
- Major B → 2. Bataljon → 4 kompagnier;
- total 8 danske company-scale infanterienheder.

Begge Majorer bruger samme `PrototypeRegimentHierarchy09F27` controller, HUD, authority-model, slot-planner og dynamic HQ movement.

## B-284 — Regimental HQ / Oberstløjtnant

**Status:** MVP IMPLEMENTERET i v00.00.09f28

Et fysisk selectable Regiments-HQ er oprettet med **Oberstløjtnant** som chef.

Kanonisk prototype-hierarki:
- Kaptajn → kompagni;
- Major → bataljon (4 kompagnier);
- Oberstløjtnant → regiment (2 bataljoner / 8 kompagnier);
- Oberst → brigade;
- Generalmajor → division.

## B-285 — Oberstløjtnant commands Majors

**Status:** MVP IMPLEMENTERET i v00.00.09f28

Regiments-HQ giver mission intent til de to Majorer, aldrig direkte company destinationer.

Missionsordrer:
- FORSVAR HER;
- ANGRIB HER;
- RYK FREM;
- TILBAGETRÆK;
- HOLD;
- SAML.

Majorer oversætter regimentsintention til company roller/slots via samme F27 core.

## B-286 — Regiment AI tactical allocation

**Status:** FØRSTE MVP IMPLEMENTERET i v00.00.09f28 / vigtig QA-gate

F28 kan fordele de to bataljoner som:
- begge side om side;
- én fremme + én regimentsreserve;
- én fremme + én flankebataljon.

Valget afhænger i første MVP af mission, doctrine og kendt enemy count. Reserve/flanke er ikke obligatorisk.

Fortsat arbejde:
- bedre terrain evaluation;
- known/stale enemy information;
- battalion strength/cohesion i beslutningen;
- løbende reassessment uden at churn'e eksisterende missioner;
- flank safety og route feasibility.

## B-287 — Authority chain

**Status:** MVP IMPLEMENTERET gennem hele kæden i v00.00.09f28 / fortsat QA

Authority:

`Oberstløjtnant order → Major mission → company mission`

Regler:
- manuel company ordre → AI OFF / spiller-authority;
- AI ON efter manuel ordre → lokal Kaptajn-authority, ikke gammel Major-destination;
- kun en NY Major-ordre reclaim'er kompagniet;
- en NY Regimentsordre reclaim'er Majoren og giver ny bataljonsmission;
- lokal under-fire reaction suspenderer parent mission, men sletter den ikke;
- direkte HQ movement sætter det pågældende HQ AI OFF.

## B-288 — Order delay/courier preparation

**Status:** PLANLAGT

F28 sender endnu regimentsmission direkte til Majorerne. Arkitekturen skal senere udvides med fysisk order lifecycle/courier delay mellem Oberstløjtnant og Majorer og senere højere HQ.

Senere couriers skal kunne blive delayed, lost eller intercepted. Command relationship line og konkret order route er separate overlays.

## B-289 — Dynamic HQ advance / Command Zone

**Status:** MVP IMPLEMENTERET Major-niveau i v00.00.09f25 og Regiments-niveau i v00.00.09f28 / fortsat QA

Major bands:
- `IN COMMAND` — 0–320 m;
- `EXTENDED` — 320–450 m;
- `OUT OF COMMAND` — over 450 m.

Regimental HQ bands:
- `IN COMMAND` — 0–800 m til Major;
- `EXTENDED` — 800–1100 m;
- `OUT OF COMMAND` — over 1100 m.

HQ flytter frem i bounds under fremrykning og tilbage ved senere withdrawal logic. Command distance påvirker command/reaction/coordination, ikke musket accuracy direkte.

## REGIMENT-CONTROL GATE

**F28 er første version der indeholder hele gaten. Nu skal den QA-stabiliseres.**

Gaten kræver:
1. infantry CHARGE/melee er brugbar;
2. under-fire reaction er brugbar;
3. 2 bataljoner / 2 Majorer / 8 kompagnier fungerer samtidigt;
4. fysisk Oberstløjtnant-HQ fungerer;
5. Oberstløjtnant → Major → Kaptajn authority fungerer uden stale-order regressions;
6. Regiments-AI kan give bataljonerne angreb/forsvar og vælge brugbar reserve/flanke-disposition;
7. Major- og Regiments-HQ positionering/command reach fungerer;
8. manual override/reclaim virker på både company- og Major-niveau.

Når denne gate er stabil, fortsætter vi **ikke straks til Brigade/Division**.

## B-290 — Dragoons

**Status:** NÆSTE STORE SYSTEM efter stabil F28 Regiment-control gate

Dragoons skal bygges og testes før højere HQ.

Første scope:
- mounted movement;
- mounted formationer;
- `SID AF` / dismount;
- dismounted fire;
- `SID OP` / remount;
- mounted charge/melee;
- evt. dismounted melee;
- horses/horse holders som tactical state;
- Dragoon casualty/state model;
- Kaptajn/Major/Regiments-AI skal kunne vurdere mounted vs dismounted anvendelse;
- samme authority-, destination-, command-zone- og senere order-delay-system som infanteriet.

Dragoon AI skal testes både direkte og gennem hele regimentskæden.

## B-291 — Kanonbatteri / Artilleri

**Status:** PLANLAGT umiddelbart efter Dragoon-baseline

Første scope:
- batteri som taktisk enhed;
- limbered movement;
- unlimber/deploy;
- facing/fire arc;
- range bands;
- ammunitionstyper;
- reload/cadence;
- crew/cannon casualties;
- solid shot/shell/shrapnel/canister hvor perioden tillader det;
- struktur-/building damage;
- battery AI for position selection, target priority, displacement og ammunition policy;
- samme HQ/authority/command-zone-system som øvrige våbenarter.

Efter Dragoons + Artilleri testes Combined Arms under Regimental HQ. Først derefter prioriteres Brigade-/Divisions-HQ.
