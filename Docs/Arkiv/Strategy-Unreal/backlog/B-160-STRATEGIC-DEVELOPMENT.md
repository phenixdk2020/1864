# PROJECT 1864 — Backlog B-160–B-169: strategisk landudvikling

**Status: BESLUTTET / PLANLAGT**  
**Designbaseline: v00.02.08**

Dette backlog-supplement konkretiserer den allerede besluttede økonomi-/udviklingsretning og kobler den til synlige handlinger på strategikortet.

| ID | Status | Emne | Beslutning / note |
| --- | --- | --- | --- |
| B-160 | BESLUTTET | Regionale udviklingsprojekter | Landudvikling styres primært som projekter på region/by/forbindelse-niveau frem for fri city-builder-placering. Projekter bruger penge, materialer, arbejdskraft og byggetid. |
| B-161 | BESLUTTET | Veje, broer og jernbane | Veje/jernbane er konkrete forbindelser med throughput, congestion, condition og weather-effekt. Broer er chokepoints, kan ødelægges/repareres og har strategisk værdi. |
| B-162 | BESLUTTET | Telegraf og stationer | Telegrafnet og stationer påvirker ordre-/rapporteringshastighed. Jernbanestationer/terminaler har loading/unloading/warehouse capacity og kan være flaskehalse. |
| B-163 | BESLUTTET | Gårde og harvest cycle | Fødevarer og forage produceres regionalt efter jord, arbejdskraft, sæson og vejr. Mobilisering, kamp, rekvisition og ødelæggelse kan reducere output og lager. |
| B-164 | BESLUTTET | Heste og remount-system | Rideheste, artilleri-/trækheste og øvrige arbejdsheste er konkrete strategiske ressourcer. Remount-depoter, køb, rekvisition og import er centrale i korte kampagner. |
| B-165 | BESLUTTET | Hesteopdræt/stutterier | Langsigtet avl kan udvikle mængde/kvalitet i længere kampagner, men har flerårig lead time. Heste må ikke kunne instant-produceres som ammunition. |
| B-166 | BESLUTTET | Specialiseret våbenindustri | Militær industri opdeles mindst i arsenal/depot, geværværksted, artilleriværk/foundry, ammunitionsværk, krudtværk, maskinværksted og vogn-/sadelmagerkapacitet. |
| B-167 | BESLUTTET | Tooling og produktionsomstilling | En fabrik kan kun producere våbenmodeller, den er tool'et/adopteret til. Omstilling mellem modeller tager tid, koster kapacitet og kan midlertidigt sænke output. |
| B-168 | BESLUTTET | Synlig udvikling på strategikortet | Forbedrede veje/jernbaner, stationer, depoter, industri, byvækst og befæstninger skal så vidt muligt kunne aflæses visuelt på strategikortet. |
| B-169 | PLANLAGT | Regional development UI | Region/by-panel viser population, workforce, food, horses, industry, stockpiles, transport, aktive projekter, bottlenecks, condition og forventet completion. |

## Designværn

- Udvikling skal skabe strategiske trade-offs, ikke micromanagement af hver enkelt gård eller fabrik.
- Kort kampagne og lang kampagne skal bruge samme datamodel, men med forskellig relevans af langtidssystemer som stutterier og nye industrikomplekser.
- Infrastruktur er fysisk/geografisk. En enhed får ikke automatisk en `+20 % movement`-buff, fordi regionen har en god vej; den skal faktisk bevæge sig på den relevante forbindelse.
- Krig kan skade økonomisk og logistisk kapacitet gennem occupation, bombardment, sabotage, kamp, rekvisition og tab af arbejdskraft.
- Produktion skaber faktiske varer/lagre. Penge alene må ikke teleportere våben, ammunition, heste eller transportmateriel til enheden.
