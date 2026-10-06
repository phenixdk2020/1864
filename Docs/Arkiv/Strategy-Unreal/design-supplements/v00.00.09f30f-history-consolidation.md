# v00.00.09f30f — Implementation & Fix History Consolidation

## Formål
F30F er en dokumentationsversion. Den ændrer ikke tactical gameplay fra F30E. Formålet er at gøre projektets implementeringshistorik og kendte fejlrettelser læsbar fra én autoritativ kilde.

## Autoritative historikkilder
- `VERSION.txt` — aktiv buildstatus og kondenseret lineage.
- `docs/IMPLEMENTATION-AND-FIX-HISTORY.md` — fuld konsolideret implementation/fix-history.
- `docs/archive/VERSION-through-v00.00.09f29r.txt` — historisk snapshot gennem F29R.
- versionerede designsupplementer i `docs/design-supplements/`.
- Git commit history.

## Regel
En ændring beskrives som "implementeret", når den er committed på den aktive testbranch. Det er ikke det samme som runtime-verificeret. TEST-builds markeres først som verificeret efter Unity-compile/runtime QA.

## F30F runtime
Runtime code svarer til F30E bortset fra build marker/version metadata. Ingen combat-, movement-, AI- eller formationregel ændres i F30F.
