# PROJECT 1864 — B-190–B-199: Officer stats og AI decision model

**Status: BESLUTTET / AKTIV IMPLEMENTERING**  
**Designbaseline: v00.02.08**  
**Target prototype: P0A v00.00.09 TEST**

## Kerneprincip

Officer-AI skal afledes af officerens konkrete profil og den information, han faktisk har. Officer stats må ikke blot være kosmetiske ratings eller generiske combat-bonuses. De bruges som input til reaction delay, valg mellem lokale handlinger, risikovillighed, ordreloyalitet, stressreaktioner og beslutningsstøj.

Alle første prototype-stats ligger på **0–100**. Værdierne i v00.00.09 er QA-profiler og er **ikke historiske vurderinger**.

## B-190 — Leadership

**BESLUTTET.** Leadership beskriver evnen til at få en formation til at reagere samlet og fastholde en beslutning under pres.

Påvirker bl.a. execution quality, cohesion-relaterede reaktioner, evnen til at fastholde en plan og senere rally/ordreudførelse. Leadership må ikke være en skjult global +accuracy bonus.

## B-191 — Inspiration

**BESLUTTET.** Inspiration beskriver officerens evne til at løfte morale og få tropper til at holde ud i en kritisk situation.

I v00.00.09 bruges den primært i AI-beslutningstærskler ved lav morale. Senere kobles den til rally, morale recovery, rout resistance og personlige battlefield events.

Leadership og Inspiration er separate: en dygtig organisator kan være mindre inspirerende, og en karismatisk officer kan være svagere til stab/planlægning.

## B-192 — Tactical Skill

**BESLUTTET.** Tactical Skill er kvaliteten af lokale taktiske valg.

Den påvirker bl.a. target/position-prioritering, engagement distance, valg af formation/terræn, reserve- og støttevalg samt vurdering af lokale odds. Tactical Skill bestemmer ikke, hvad officeren ved; fog-of-war/knowledge-state ligger separat.

## B-193 — Initiative

**BESLUTTET.** Initiative styrer hvor hurtigt og villigt officeren reagerer uden at vente på en ny ordre.

Den påvirker AI reaction interval, hvor hurtigt et nyt lokalt problem får en beslutning og villighed til at udnytte en mulighed inden for mission/autonomy.

Høj Initiative er ikke altid positivt: kombineret med høj Aggressiveness og lav Discipline kan det skabe forhastede handlinger.

## B-194 — Staff / Command Skill

**BESLUTTET.** Staff/Command Skill beskriver planlægning, koordinering og ordrebehandling.

I v00.00.09 påvirker den reaction/decision efficiency. Senere bliver den central for order delay, acknowledgement/execution, brigade/division coordination, reserve management, staff work og logistics coordination.

## B-195 — Discipline / Obedience

**BESLUTTET.** Discipline/Obedience beskriver hvor tæt officeren følger mission og commander intent.

- Høj værdi: færre unødvendige afvigelser og højere mission adherence.
- Lav værdi: større risiko for at lokale ønsker/personality overtager den givne mission.

Dette er ikke det samme som autonomy. Autonomy er den **tilladte frihedsgrad**; Discipline/Obedience beskriver, hvordan officeren bruger den frihed.

## B-196 — Aggressiveness ↔ Caution

**BESLUTTET.** Aggressiveness er et personality-axis på 0–100:

- 0 = meget forsigtig,
- 50 = balanceret,
- 100 = meget aggressiv.

Det er ikke en kvalitetsscore. Det påvirker bl.a. engagement threshold, pursuit, lokal withdrawal, modangreb, reservebrug og hvor tæt enheden søger mod fjenden.

## B-197 — Experience som separat baggrundsværdi

**BESLUTTET.** Officer Experience holdes separat fra kerne-skills.

Experience repræsenterer faktisk tjeneste-/kampbaggrund og skal især reducere decision noise, gøre reaktioner mere konsistente og senere kunne udvikles gennem service record og kamp. Experience må ikke bare duplikere Tactical Skill eller Leadership.

## B-198 — Første AI-beregninger

**AKTIV v00.00.09.** Første prototype bruger officerprofilen i en lille, synlig decision model.

- **Reaction delay** afledes primært af Initiative + Staff/Command Skill og modificeres af Composure under pres.
- **Decision quality** afledes primært af Tactical Skill + Experience.
- **Mission adherence** påvirkes af Discipline/Obedience.
- **Risk bias** påvirkes af Aggressiveness/Caution.
- **Morale-oriented choices** påvirkes af Leadership + Inspiration.
- **Stress stability** påvirkes af Composure/Nerve.

Difficulty kan lægge en separat AI-efficiency/noise-faktor ovenpå, men må ikke omskrive officerens stats.

## B-199 — Composure / Nerve

**BESLUTTET.** Composure/Nerve beskriver hvor køligt officeren bevarer overblik og beslutningsevne under pres.

Det er en selvstændig stat og må ikke blandes sammen med Leadership, Inspiration eller enhedens morale.

### Høj Composure

En officer med høj Composure:

- reagerer mere stabilt under kraftig beskydning og tab,
- har mindre decision noise når morale/cohesion falder,
- skifter sjældnere plan i panik,
- vurderer withdrawal/rally mere rationelt,
- får mindre stress-relateret reaction delay,
- har mindre risiko for tunnel vision og overreaktion.

### Lav Composure

En officer med lav Composure kan under pres:

- reagere for sent eller for pludseligt,
- skifte mål eller plan unødigt,
- trække sig tidligere end situationen kræver,
- blive for aggressiv i et desperat modangreb afhængigt af personality,
- overse bedre lokale muligheder,
- få større decision noise og længere/ustabil reaction time.

Lav Composure betyder ikke automatisk fejhed. Kombinationen med Aggressiveness, Leadership, Experience og aktuelle battlefield conditions afgør reaktionen.

### Eksempler på kombinationer

- **Høj Leadership + lav Composure:** inspirerer og organiserer godt, men kan miste overblik under kaos.
- **Lav Inspiration + høj Composure:** kølig og rationel, men ikke særlig god til at løfte troppernes morale.
- **Høj Aggressiveness + lav Composure:** risiko for impulsive eller desperate angreb.
- **Høj Aggressiveness + høj Composure:** kontrolleret offensiv officer, som tager bevidste risici.
- **Lav Aggressiveness + høj Composure:** metodisk og forsigtig uden nødvendigvis at være passiv eller panisk.

## Første kerneprofil i v00.00.09

Officerprofilen består derfor af otte centrale dimensioner:

1. Leadership
2. Inspiration
3. Tactical Skill
4. Initiative
5. Staff / Command Skill
6. Discipline / Obedience
7. Aggressiveness ↔ Caution
8. Composure / Nerve

Officer Experience gemmes separat som baggrunds-/stabilitetsværdi.

## Designværn

- Stats er AI-input og command-egenskaber, ikke skjulte generiske combat buffs.
- Officerens personality og skills skal kunne give både styrker og svagheder.
- Ingen enkelt stat må dominere hele AI'en.
- Kombinationsvirkninger er vigtigere end simple `stat > 70 = god` regler.
- QA-profiler i v00.00.09 må ikke præsenteres som historiske ratings.
