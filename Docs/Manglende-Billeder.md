# Bygningsbilleder - status (2026-10-10)

Alle 53 bygninger i `Data/Campaign1851/Buildings1851.csv` har nu **hver sit eget billede** (B01-B20 og B21-B61, 768 px, importeret som 512 px kort i `/Game/Campaign1851/Buildings`). Ingen to bygninger deler billede. Originalerne ligger i `Reference/Campaign1851/Buildings/Strategy1864_Original_20` og `Strategy1864_Original_B21_B61`.

## Kobling

Billedet for hver CSV-bygning hedder `T_Bld_<Nøgle>` (fx `T_Bld_Shipyard`); garnisonsmodulerne `T_Module_Stables`, `T_Module_Depot`, `T_Module_Infirmary`; kasernen `T_Barracks_Infantry`. Bygninger, som endnu ikke har en definition i `Campaign1851ConstructionSite.cpp` (fx Mobiliseringsdepot, Officersskole, Krudtværk, Tinghus, Godsbanegård, Banevogterhus, Stenbro, Skibsværft, Marinestation, Vognfabrik, Sadelmageri, Garveri, Kulmine, Herregård, Proprietærgård, Husmandssted, Bondegård), har billedet klar og venter på bygningen.

## Billeder uden bygning (kan bruges til nye bygninger eller moduler)

B03 Kommandobygning, B06 Infanteribygning, B07 Kanonbygning, B09 Officerbolig, B10 Mandskabsbolig, B11 Portnerbolig, B14 Vandmølle, B19 Markedshal. Findes som `T_Bld_Command_Building` og så videre (B03-B19), undtagen vandmøllen (`T_Bld_Watermill`).

## Billeder der stadig mangler

Ingen bygningsbilleder. Resten af de manglende billeder er uniformbilleder til 1825 (Docs/Backlog.md) og billeder til bygninger fra nye forskningsemner (kommer, når Docs/Research-Tree.md er færdigt og bygningerne er besluttet).
