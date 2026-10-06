# PROJECT 1864 — Designmanual v00.02.00 — Feature Architecture

### F32 – Markets, Trade & Foreign Procurement

Formål. Forbinde indenlandsk økonomi, diplomati, flåde og strategiske mangler.

Indhold.

- Supply/demand og priser på strategiske varer.
- Trade agreements, tariffs og contracts.
- Foreign procurement af våben, maskiner, kul, jern og skibe.
- Blockade/war disruption.

Afhængigheder. F27-F31, F20, F22

Exit-kriterier.

- Import/eksport og blokade skaber konkrete pris-, lager- og produktionsændringer.

### F33 – Research, Technology & Adoption

Formål. Skelne mellem viden og faktisk implementeret militær kapacitet.

Indhold.

- Research/knowledge.
- Trials og doctrine development.
- Industrial tooling og mass production.
- Unit-level adoption og training.

Afhængigheder. F29-F32, F37, F38

Exit-kriterier.

- En teknologi kan være kendt uden at være masseproduceret; adoption kan følges i faktiske enheder.

### F34 – State Budget, Tax & Debt

Formål. Give langsigtede økonomiske trade-offs.

Indhold.

- Taxes, tariffs, loans, interest og credit.
- Military/civil budget priorities.
- War finance, requisitions og political tolerance.

Afhængigheder. F26-F32, F22

Exit-kriterier.

- Staten kan finansiere en krig på flere måder, og gæld/skatter giver vedvarende økonomiske konsekvenser.

### F35 – Civil & Private Economic Development

Formål. Lade økonomien udvikle sig uden total spiller-mikro.

Indhold.

- Private investment heuristics.
- Demand-driven workshops/warehouses.
- Urbanisation og regional specialisation.

Afhængigheder. F26-F34

Exit-kriterier.

- Mindst dele af civiløkonomien kan vokse/reagere autonomt på profit, infrastruktur og efterspørgsel.

### F36 – Recruitment, Conscription & Mobilisation

Formål. Implementere nation-specifikke manpower-institutioner.

Indhold.

- Volunteer enlistment.
- Conscription/session rules.
- Active/reserve/cadre systems.
- Mobilisation pipeline og assembly.

Afhængigheder. F26, F03, F31, F34

Exit-kriterier.

- Klik på mobilisation skaber en tidslig proces; enheden bliver ikke kampklar før personel, udstyr og organisation er samlet.

### F37 – Unit Training, Readiness & Experience

Formål. Gøre fredstidstræning og veteraner til langsigtet militær kapital.

Indhold.

- Training dimensions: drill, marksmanship, fire discipline, fieldcraft, endurance m.fl.
- Garrison training, live-fire, manoeuvres.
- Readiness separat fra training.
- Replacement integration og veteran dilution.

Afhængigheder. F03, F31, F36

Exit-kriterier.

- To enheder med samme headcount kan have tydeligt forskellig træning/readiness/experience, som mærkes i kamp.

### F38 – Leader Development & Military Education

Formål. Skabe et officerskorps, der udvikles over år.

Indhold.

- Academies og staff courses.
- Career assignments og mentorship.
- Exercises/manoeuvres.
- Promotion by seniority/merit/politics.
- Combat learning og personality interaction.

Afhængigheder. F04, F37, F34

Exit-kriterier.

- Officerer forbedres gennem plausibel tjeneste/uddannelse, og fredstidsvalg påvirker senere command quality.

### F39 – Casualties, Wounded, Disease & Recovery

Formål. Erstatte simple casualty-tal med persistent personelstatus.

Indhold.

- Killed/wounded/missing/captured/deserted/sick.
- Wound severity og recovery timelines.
- Return-to-unit og invalidation.
- Disease model.

Afhængigheder. F12, F17, F36

Exit-kriterier.

- Efter et slag kan samme wounded-pool udvikle sig til returnees, invalids eller deaths over tid.

### F40 – Medical Service & Evacuation

Formål. Gøre sanitet til en reel logistisk og organisatorisk kapacitet.

Indhold.

- Battlefield collection og casualty stations.
- Ambulance/transport capacity.
- Field hospitals, doctors og supplies.
- Overcrowding, water og mortality modifiers.

Afhængigheder. F07, F17, F39, F46

Exit-kriterier.

- Evakuerings- og hospitalssystemet kan målbart reducere dødelighed og antallet af sårede, der efterlades.

### F41 – POW, Missing, Desertion & Repatriation

Formål. Bevare menneskelig state efter en enhed forlader slagmarken.

Indhold.

- Unwounded POW og wounded POW.
- Missing resolution over time.
- Prisoner pools, officers/enlisted, health.
- Exchange, parole/repatriation framework.

Afhængigheder. F17, F39, F40, F22

Exit-kriterier.

- En savnet soldat kan senere blive identificeret som POW, hospitalised, returned eller killed; POWs er ikke automatisk døde manpower.

### F42 – Regimental Lineage & Unit Chronicle

Formål. Gøre navngivne enheder til historiske institutioner.

Indhold.

- Established/lineage/name changes.
- Commanders, battles, casualties, service history.
- Persistent campaign chronicle.
- Tradition og veteran continuity.

Afhængigheder. F03, F17, F37, F38

Exit-kriterier.

- En enhed kan åbnes og vise både historisk pre-game lineage og alle væsentlige hændelser fra spillerens campaign.

### F43 – Colours, Standards & Battle Honours

Formål. Give regimentsidentitet et fysisk og historisk symbol.

Indhold.

- Fane/standard som data + 3D object.
- Colour guard og capture/loss state.
- Historical og dynamic battle honours.
- Prestige/tradition effects.

Afhængigheder. F10, F17, F42

Exit-kriterier.

- Fanen kan vises på slagmarken og dens udstedelse, skade, tab eller hæder gemmes permanent i kronikken.

### F44 – Decorations & Personal Service Records

Formål. Følge navngivne personers militære liv gennem kampagnen.

Indhold.

- Promotions, wounds, captivity, commands og battles.
- Commendations og decorations.
- Retirement, death og invalid status.

Afhængigheder. F04, F17, F38-F42

Exit-kriterier.

- En officers service record overlever stillingsskift og kan bruges historisk i UI og events.

### F45 – Unit Traits & Earned Perks

Formål. Lade regimenter/bataljoner udvikle særlige evner gennem faktisk adfærd.

Indhold.

- Training traits.
- Experience traits.
- Historical/regimental traits.
- Trait decay/dilution efter massive replacements.

Afhængigheder. F37, F42

Exit-kriterier.

- Traits optjenes gennem træning/erfaring og kan svækkes, hvis den institutionelle base ændres markant.

### F46 – Formation Specialisations & Support Perks

Formål. Give brigader/divisioner/korps organisatoriske kapaciteter som Grand Tactician-lignende perks, men fysisk forankret.

Indhold.

- Flying Column.
- Ambulance Service/Corps.
- Skilled Cartographers.
- Pontoon Train.
- Sappers & Miners.
- Artillery/logistics/recon specialisations.
- 2–3 experience levels og readiness.

Afhængigheder. F07, F15, F37-F40

Exit-kriterier.

- En specialisering kræver relevant udstyr/personel/træning; tab af support assets reducerer dens effekt.

### F47 – National Development Integration & Balance

Formål. Samle økonomi, manpower, uddannelse, handel og krig i én stabil loop.

Indhold.

- Cross-system telemetry.
- AI use of economy/training/recruitment.
- Historical benchmark scenarios.
- Long-campaign balance og anti-snowball mechanics.

Afhængigheder. F26-F46, F18

Exit-kriterier.

- En flerårig testkampagne kan køres uden deadlocks/exploits, og militære resultater kan forklares gennem de underliggende nationale systemer.
