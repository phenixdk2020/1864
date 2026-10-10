# Kamporden ved sit åbningssted

Opdateret 10. oktober 2026. Kontrolleret ved gennemlæsning af kodevejene; spil og editor er ikke startet.

## Fælles regler

Konteksten fastlægges ved åbning: den standsede enheds aktuelle by (`Town`), byen fra garnisonskortet eller den marcherende/fritstående enheds øverste hærformation. Amtet gemmes som vinduestilstand ved åbningen. Valg, opdeling, samling og drag-and-drop erstatter aldrig konteksten. HELE HÆREN er et udtrykkeligt skift til den samlede oversigt; genåbning fra en anden enhed/by fastlægger en ny kontekst.

Venstre side grupperer levende, standsede enheder med `Formation == 0` efter deres aktuelle `Town`, ikke deres `Home` eller generalkommando. En garnisonsby genkendes på sådanne enheder eller positiv `GarrisonCapacity`, inklusive billet efter [Garrison.md](Garrison.md). Kontekstbyen vises først, derefter de øvrige garnisonsbyer i samme amt, hver under I GARNISON med bynavn. Tomme garnisoner med kapacitet viser »Ingen enheder i garnison her«. Listen kan rulles med musehjulet; +/- viser kompagnier, eskadroner og batterisektioner, som fortsat kan trækkes særskilt.

En garnisonsoverskrift, som er etableret i vinduet, bevares også, hvis dens sidste enhed trækkes ud og byen mangler kapacitet. HELE HÆREN eller genåbning nulstiller denne midlertidige liste over kendte garnisonsbyer.

Højre side viser hærtræerne med levende, standsede enheder på kontekstbyens `Town`. Enheder og befolkede underformationer ved andre byer medtages ikke. Ved åbning fra march/fri mark vises enhedens eget hærtræ, inklusive eventuelle nye træer udskilt fra det i dette vindue; åbningshær og amt ændres ikke. Ældre formationer uden Army-rod bruger det eksisterende syntetiske FELTHÆREN-træ med enhedens egen øverste formationsgren. En marcherende enhed uden formation viser ingen hærrod. Boksen »Træk herover / Ny felthær med bataljonsstab« er altid til stede.

Kun garnisonen i kontekstbyen er aktiv som kilde. Andre garnisoner i samme amt vises med grå tekst og tooltip »Du skal staa i <by> for at tage enheder ud«; hverken bataljoner eller deres underenheder kan vælges eller trækkes derfra. Foldning med +/- er fortsat tilladt som læsning. Uden bykontekst (march, fri mark eller HELE HÆREN) er garnisonslisterne skrivebeskyttede.

Retur til »Slip her« eller en garnisonsoverskrift kræver, at enheden står standset i destinationsbyen, og at denne er kontekstbyen (eller enhedens eget hjemsted). Controlleren kontrollerer også kilden, destinationsbyen og hele kapacitetsplanen før opdeling/flytning og afviser med toast. Der er ingen automatisk retur til en anden by eller et fort ved pladsmangel. Et samlet valg returneres samlet eller afvises samlet.

»Træk herover« opretter Army -> Bataljon-HQ -> enheder. Bataljonsstaben bruger det eksisterende laveste staff-niveau `Detachment`, navngives `<bataljonsnavn> HQ`, vises som Bataljonsstab og får enhedens chef i chefpladsen, hvis denne er besat. Chefvælgeren bruger enhedens eksisterende rangkrav (fx Major for fodfolk). Flere bataljoner fra samme regiment får et fælles Regiment-HQ over deres egne bataljonsstabe; tilhørsforholdet findes via eksisterende Regiment-forælder eller scenariets oprindelige bataljonsnavn, uafhængigt af spillerens omdøbning. Eksisterende bataljonsstabe med deres officerer bevares. Et træk af en hel HQ-gren bevarer grenen under den nye Army-rod. OPRET REGIMENT/BRIGADE/DIVISIONS-HQ kan fortsat indsætte højere niveauer. Når sidste enhed forlader et HQ, opløses tomme HQ'er og tomme hære.

## Gennemlæste forløb

| Tilfælde | Kodevej og fast kontekst | Venstre side | Højre side og resultat |
|---|---|---|---|
| Åbnet fra garnisonsenhed | `OpenOOB → SetOOBContext → SetOOBPlace(Town)`; første valgte enhed bestemmer stedet. | I GARNISON med bynavn og byens garnisonsenheder; andre garnisonsbyer i samme amt får egne overskrifter. | Kun hære ved den valgte by samt ny-hær-boksen. Intet globalt FELTHÆREN-træ uden lokale feltformationer. |
| Åbnet fra felthær | En standset enhed med gyldig `Town` giver bykontekst; en enhed på fri mark giver hærrod og åbningsamt. | Garnisoner i åbningsamtet, også tomme med kapacitet. | Hære ved byen eller den fritstående enheds eget hærtræ. |
| Åbnet fra marcherende enhed | `SetOOBContext` undersøger `IsMarching` før `Town` og følger Formation-kæden til roden. | Garnisonsbyer i amt ved åbningen; ingen venstreliste hvis ingen kvalificerer. | Eget hærtræ; en eventuel gammel `Town` vælger ikke et andet sted. Retur afvises mens enheden marcherer. |
| Åbnet med HELE HÆREN | `OOBFocusClear → ClearOOBView`, eller åbning fra oversigtsmenuen. | Alle garnisonsbyer med egne overskrifter. | Den samlede hærorganisation samt ny-hær-boksen. Drag ændrer ikke denne tilstand. Det gamle filter er kun tilladt i denne tilstand. |
| Efter træk ud | `TreeDrop`, `NewFormation/99999 → CreateArmyWithUnitHQs → MoveRegimentToFormation → RevealOOBArmy`. To valgte regimenter ved samme sted flyttes sammen; Company-kilden bruger `SplitOffCompany`. | De flyttede enheder forlader garnisonslisten; en tom by med kapacitet beholder overskrift og tom-hint. | Ny hær ved kontekstbyen vises med bataljonsstabe og eventuelt fælles Regiment-HQ. Ingen overgang til »De valgte enheder«. |
| Efter træk tilbage | `TreeDrop/Garrisons → ReturnRegimentsToGarrison → PlanGarrisonReturn`, eller eksisterende Company-/Formation-retur. | Returnerede enheder vises under destinationens byoverskrift. Andre destinationer afvises. | `PruneEmptyFormations` opløser tomme Army-træer, også når de kun indeholder tomme HQ'er. Ny-hær-boksen bevares. |
| By uden garnison | `SetOOBPlace`; ingen egne garnisonsenheder og ingen kapacitet. | Ingen overskrift for byen. Har amtet heller ingen garnisonsbyer, er venstre side tom. Under træk fra feltet vises »Slip her / tilbage i garnison« med bykontekst eller for en enhed, der står i eget hjemsted; retur kræver plads i selve byen. | Kun lokale feltstyrker samt ny-hær-boksen. |
| Amt med to garnisonsbyer | `PaintOOBChart` finder byerne via `AmtId`, kapacitet/enheder og placerer kontekstbyen først. | To separate I GARNISON-overskrifter. Hver liste kræver præcis `Town == overskriftens by`; samme kommando eller `Home` sammenblander dem ikke. Tomme lister bevares; lange lister kan rulles. | Kun hære ved selve kontekstbyen, ikke alle hære i amtet. |

## Bevarede funktioner og statisk kontrol

Dragmarkering på kilde og mål, officerernes plusknapper, HQ-knappernes tegnings-/hitorden og den permanente ny-hær-boks er bevaret. Klikkæden i controlleren har ikke fået nye `else if`-grene; den nye returhandler ligger selvstændigt i `TreeDrop`.

Nye anvendelser af TArray/TSet, svage objektpointere, afrunding og Slate-klip er kontrolleret mod Unreal 5.8-headerne under `I:/Spil/Epic Games/UE_5.8/Engine/Source`. Kontekstfelterne er midlertidig UI-tilstand; eksisterende gemte `Town`/`Formation` anvendes fortsat, og gemmeformatet ændres ikke. Scenariedata for 1825 og 1851 er ikke ændret. Den faktiske gengivelse og interaktion er endnu ikke spiltestet.
