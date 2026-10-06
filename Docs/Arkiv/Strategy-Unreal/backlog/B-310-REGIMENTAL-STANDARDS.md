# B-310–B-319 — Regimentsfaner / standards

## Status

**BESLUTTET / FØRSTE BATTLE-MVP IMPLEMENTERET i v00.00.09c TEST**

Regimentsfaner skal være et fælles identitets- og command-visual både på Battle map og Grand Campaign. De første runtime-faner er læsbare QA-placeholders; endelige mønstre, farver, insignier og antal faner pr. regiment skal research-valideres pr. land og enhed.

## B-310 — Battle map

Hvert regiment får en synlig standard ved formationens center/command group.

Første MVP:

- standarden følger regimentets position og facing,
- Danmark og Preussen har tydeligt forskellige placeholder-designs,
- hvert regiment får en lille stabil farveaccent/ribbon så flere regimenter kan skelnes,
- standarden ændrer ikke simulation/combat stats,
- `STANDARD-DIAG` logger oprettelsen.

Senere:

- rigtig flag bearer/escort,
- historisk korrekt regimentsfane,
- flere faner hvor OOB/regler kræver det,
- animation/cloth,
- morale/command relation,
- risiko for at fane mistes eller erobres i melee/rout,
- captured standards som battle/campaign result.

## B-311 — Campaign map

På campaign-kortet følger standard-symbolik semantic zoom:

- tæt zoom: formation/army token kan vise fysisk/national standard,
- mellemzoom: regiment/brigade identity kan vises som lille flag/colour på unit card/token,
- lang zoom: NATO/APP-6-lignende symbol er primær, mens flag bruges som national/formation identity accent,
- individuelle regimentsfaner vises kun når formationen er unfolded/inspected; et army/corps-token må ikke vise dusinvis af små faner samtidig.

## B-312 — Stable identity

Flag/standard-data knyttes til stable RegimentId/FormationId og må ikke være afhængig af GameObject-navn i den endelige datamodel.

Minimum fremtidig data:

- `StandardId`
- nation
- regiment/formation owner ID
- historical pattern/art reference
- issue date/version
- captured/lost state
- bearer state senere

## B-313 — Fog of war

Fjendtlige standards må ikke blive et gratis identifikationssystem gennem fog of war. På afstand kan spilleren først se ukendt/fjendtlig formation; præcis regiment-identitet kræver passende observation/intelligence.

## B-314 — Semantic zoom

Regimentsfaner er en del af Battle semantic zoom:

1. tæt: fysisk 3D-standard og flag bearer,
2. mellem: simplificeret banner/formation marker,
3. lang: NATO/APP-6-symbol + lille national/formation identity accent,
4. meget lang: højere-echelon aggregation.

Samme princip gælder campaign-laget.
