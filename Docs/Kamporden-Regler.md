# Kamporden ved sit åbningssted

Opdateret 10. oktober 2026. Kontrolleret ved gennemlæsning af kodevejene; spil og editor er ikke startet.

## Fælles regler

Konteksten fastlægges ved åbning: den standsede enheds aktuelle by (`Town`), byen fra garnisonskortet eller den marcherende/fritstående enheds øverste hærformation. Amtet gemmes som vinduestilstand ved åbningen. Valg, opdeling, samling og drag-and-drop erstatter aldrig konteksten. HELE HÆREN er et udtrykkeligt skift til den samlede oversigt; genåbning fra en anden enhed/by fastlægger en ny kontekst.

Venstre side grupperer levende, standsede enheder med `Formation == 0` efter deres aktuelle `Town`, ikke deres `Home` eller generalkommando. En garnisonsby genkendes på sådanne enheder eller positiv `GarrisonCapacity`, inklusive billet efter [Garrison.md](Garrison.md). Kontekstbyen vises først, derefter de øvrige garnisonsbyer i samme amt, hver under I GARNISON med bynavn. Tomme garnisoner med kapacitet viser »Ingen enheder i garnison her«. Listen kan rulles med musehjulet; +/- viser kompagnier, eskadroner og batterisektioner, som fortsat kan trækkes særskilt.

En garnisonsoverskrift, som er etableret i vinduet, bevares også, hvis dens sidste enhed trækkes ud og byen mangler kapacitet. HELE HÆREN eller genåbning nulstiller denne midlertidige liste over kendte garnisonsbyer.

Højre side viser hærtræerne med levende, standsede enheder på kontekstbyens `Town`. Enheder og befolkede underformationer ved andre byer medtages ikke. Ved åbning fra march/fri mark vises enhedens eget hærtræ, inklusive eventuelle nye træer udskilt fra det i dette vindue; åbningshær og amt ændres ikke. Ældre formationer uden Army-rod bruger det eksisterende syntetiske FELTHÆREN-træ med enhedens egen øverste formationsgren. En marcherende enhed uden formation viser ingen hærrod. Boksen »Træk herover / Ny felthær uden ekstra HQ« er altid til stede.

Retur bruger garnisonsmodellens fælles kapacitetsplan, nuværende by før hjemsted og derefter nærmeste gyldige by/fort. Et samlet regimentsvalg ved samme by returneres samlet eller afvises samlet. Overskrifter og returzone er returhandlere, ikke genveje til at flytte alle enheder under en generalkommando. Enheder fra en anden garnisonsby blandes ikke ind under kontekstbyens overskrift. En udskilt hær fra en anden by bliver heller ikke en hær ved kontekstbyen.

## Gennemlæste forløb

| Tilfælde | Kodevej og fast kontekst | Venstre side | Højre side og resultat |
|---|---|---|---|
| Åbnet fra garnisonsenhed | `OpenOOB → SetOOBContext → SetOOBPlace(Town)`; første valgte enhed bestemmer stedet. | I GARNISON med bynavn og byens garnisonsenheder; andre garnisonsbyer i samme amt får egne overskrifter. | Kun hære ved den valgte by samt ny-hær-boksen. Intet globalt FELTHÆREN-træ uden lokale feltformationer. |
| Åbnet fra felthær | En standset enhed med gyldig `Town` giver bykontekst; en enhed på fri mark giver hærrod og åbningsamt. | Garnisoner i åbningsamtet, også tomme med kapacitet. | Hære ved byen eller den fritstående enheds eget hærtræ. |
| Åbnet fra marcherende enhed | `SetOOBContext` undersøger `IsMarching` før `Town` og følger Formation-kæden til roden. | Garnisonsbyer i amt ved åbningen; ingen venstreliste hvis ingen kvalificerer. | Eget hærtræ; en eventuel gammel `Town` vælger ikke et andet sted. Retur afvises mens enheden marcherer. |
| Åbnet med HELE HÆREN | `OOBFocusClear → ClearOOBView`, eller åbning fra oversigtsmenuen. | Alle garnisonsbyer med egne overskrifter. | Den samlede hærorganisation samt ny-hær-boksen. Drag ændrer ikke denne tilstand. Det gamle filter er kun tilladt i denne tilstand. |
| Efter træk ud | `TreeDrop`, `NewFormation/99999 → CreateFormation(Army) → MoveRegimentToFormation → RevealOOBArmy`. To valgte regimenter ved samme sted flyttes sammen; Company-kilden bruger `SplitOffCompany`. | De flyttede enheder forlader garnisonslisten; en tom by med kapacitet beholder overskrift og tom-hint. | Ny hær ved kontekstbyen vises direkte med regimenterne, uden ekstra HQ. Ingen overgang til »De valgte enheder«. |
| Efter træk tilbage | `TreeDrop/Garrisons → ReturnRegimentsToGarrison → PlanGarrisonReturn`, eller eksisterende Company-/Formation-retur. | Returnerede enheder vises under destinationens byoverskrift. Konteksten ændres ikke ved en alternativ destination. | `PruneEmptyFormations` opløser tomme Army-træer, også når de kun indeholder tomme HQ'er. Ny-hær-boksen bevares. |
| By uden garnison | `SetOOBPlace`; ingen egne garnisonsenheder og ingen kapacitet. | Ingen overskrift for byen. Har amtet heller ingen garnisonsbyer, er venstre side tom. Under træk fra feltet vises stadig »Slip her / tilbage i garnison«; modellen afgør, om retur er mulig. | Kun lokale feltstyrker samt ny-hær-boksen. |
| Amt med to garnisonsbyer | `PaintOOBChart` finder byerne via `AmtId`, kapacitet/enheder og placerer kontekstbyen først. | To separate I GARNISON-overskrifter. Hver liste kræver præcis `Town == overskriftens by`; samme kommando eller `Home` sammenblander dem ikke. Tomme lister bevares; lange lister kan rulles. | Kun hære ved selve kontekstbyen, ikke alle hære i amtet. |

## Bevarede funktioner og statisk kontrol

Dragmarkering på kilde og mål, officerernes plusknapper, HQ-knappernes tegnings-/hitorden og den permanente ny-hær-boks er bevaret. Klikkæden i controlleren har ikke fået nye `else if`-grene; den nye returhandler ligger selvstændigt i `TreeDrop`.

Nye anvendelser af TArray/TSet, svage objektpointere, afrunding og Slate-klip er kontrolleret mod Unreal 5.8-headerne under `I:/Spil/Epic Games/UE_5.8/Engine/Source`. Kontekstfelterne er midlertidig UI-tilstand; eksisterende gemte `Town`/`Formation` anvendes fortsat, og gemmeformatet ændres ikke. Scenariedata for 1825 og 1851 er ikke ændret. Den faktiske gengivelse og interaktion er endnu ikke spiltestet.
