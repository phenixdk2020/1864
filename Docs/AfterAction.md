# Rapport efter slaget

Implementeret 2026-10-10. Ingen build eller spiltest er kørt som del af ændringen.

## Brug

Ved et afgørende udfald fryses 3D-slaget og rapporten åbnes. VIS RAPPORT genåbner den; LUK lukker vinduet og beholder pausen. FORRIGE/NÆSTE bladrer mellem rækker. AFSLUT SLAGET skriver kampagneresultatet og vender tilbage til kampagnen. Afsluttes slaget tidligt, beregnes ét udfald før rapporten fryses; første klik viser rapporten, næste afslutter. I et selvstændigt testslag forlader FORLAD SLAGET testen efter rapportvisningen.

Kampagnen viser samme rapport efter indlæsning af BattleResult og kan genåbne seneste rapport. Automatiske afgørelser og tilbagetrækninger får en rapport med deres eksisterende spilestimater; umålte felter vises som —. Rapporten gemmes som UTF-8 i `Saved/Reports/battle_<timestamp>.txt` (automatisk rapport tilføjer slag-id) og logges med `PROJECT1864-REPORT`.

## Regnskab

Pr. side og kampenhed: mænd ved start, faldne, sårede, fanger, erobret/tabt materiel, skud brugt, sekunder i kamp og største moraltab. Materiel: K kanoner, M mortérer, G geværer, H heste, V vogne, F faner. Tekstfil/JSON indeholder også salver, skud pr. salve, tabsårsager og officerer såret/fanget. Officerer kan **aldrig dø i slag**; aldersrelateret død uden for slag følger det eksisterende karrieresystem.

Tab registreres ved den faktiske skade-/forbrugshændelse. Som spilestimat fordeles personelskader med ca. 1/3 faldne og 2/3 sårede; akkumuleret afrunding undgår, at små salver kun dræber. Fanger registreres ved fysisk overløb af en flygtende enhed. Efterladt materiel tælles som tab, og erobret materiel tildeles én gang ved kontakt/erobring. Evakuerede artillerister registreres ikke som faldne. Stabes organisatoriske styrker lægges ikke til deres underordnede; uindtrådte reserver udelades.

Tid i kamp tælles under Engaged/UnderFire og i ti sekunder efter skud eller skader. Sidens kamptid er summen af enhedernes kamptid, ikke slagets vægurstid; sidens moraltab er den største målte nedgang. Dette er målinger, ikke et fuldt moral-/cohesion-tidsforløb. Lazaretprognosen 88–96 % er et spilestimat; faktisk tilbagevenden følger kampagnens eksisterende pleje.

## Kampagne og historik

`PROJECT1864-BattleResult-1` udvides med `reportVersion: 1`, `ledger`, `reportText`, tab pr. kategori, materiel og `officerParticipants`. Enhedsresultater har startMen, killed, wounded, prisoners, kills (faktiske fjendtlige faldne tilskrevet skadehændelsen), heldField, highestMoraleLoss, ammoUsed og materiel. Identitet, medlemskab, dubletter, summer og numeriske grænser kontrolleres før anvendelse. Et ugyldigt resultat efterlader slaget ventende. Ældre resultater uden reportVersion følger den tidligere vej.

Faldne, sårede og fangne trækkes fra mænd; sårede overføres til regimentets Sick/lazaret, fanger til de eksisterende puljer og PrisonersByNation (nationen, der holder dem). Erobret materiel går på lager; deployerede kanoner/mortérer/heste/vogne trækkes fra enheden. Allerede udleveret materiel trækkes ikke igen fra reservebeholdningen. Ammunition reduceres med den brugte andel af startbeholdningen, og moral med det målte største tab.

Ved AFSLUT SLAGET eksporteres resultatet. Når kampagnen accepterer det, tilføjes én tjenestepost pr. deltagende kampagneregiment: slag, sted, scenariedato, mænd ved start, faldne/sårede/fangne, fjendtlige faldne, materiel og holdt felt/tilbagetrækning. TotalKilled, TotalWounded, TotalCaptured og TotalEnemyKilled opdateres. Enhedskortets historik viser postens tal; materiel og feltstatus findes i dens tooltip.

Campaign1851Career.cpp tilføjer én dateret karrierepost pr. deltagende kampagneofficer: deltog, såret, fanget, udmærkelse. Udmærkelse er et spilestimat: mindst ti tilskrevne fjendtlige faldne eller erobring af en kanon, mortér eller fane, og officeren er ikke fanget. Seneste post vises på officerskortet; tooltip viser hele karrieren. Automatisk afgørelse registrerer deltagelse for tilknyttede officerer, uden at opfinde sår eller udmærkelser.

Gemmeversion **32** gemmer seneste rapport, fangepuljer og officerskarrierer. Tjenesteposternes eksisterende tekstformat udvides med fire felter; gamle poster med otte felter læses stadig. Dato gemmes eksplicit for nye poster, så 1825 og 1851 ikke blandes ved visning.

## Kort test — kræver særskilt tilladelse til spilstart

Tilføj `-Strategy1864TestReport -Strategy1864DebugReport` til en eksisterende slagstart. Testflaget vælger en lille skirmish, hvis ingen kampagneanmodning har forrang. Efter 15 simulerede sekunder påføres seks danske og otte fjendtlige personelskader, en fjendtlig enhed flygter og tages til fange, én dansk officer såres og én fjendtlig officer fanges. Et uafgjort udfald fryser rapporten. Flaget har ingen virkning på almindelige slag uden eksplicit aktivering.

Kontrollér rækker og sidetotaler, officerer, paginering, LUK/VIS RAPPORT, tekstfil og log. Kampagnelinket kontrolleres separat med et slag startet fra kampagnen: afslut, kontrollér mænd/Sick, fanger, materiel, tjenesteposter, karriere og gem/indlæs. `-Strategy1864DebugReport` logger hele tekstfilen. Eksisterende `-Strategy1864AutoFinish=N` kan afslutte en kampagnetest automatisk ved udfald eller tidsgrænsen.

## Statisk kontrol og afgrænsning

UE 5.8-kilder er kontrolleret for JSON-felter, FMath, dato-/filfunktioner, WeakObjectPtr, HUD-tekststørrelse og pause-/level-API. `git diff --check` og statisk kontrol af nye deklarationer/definitioner, include-stier, klammer og officersregler er udført. Den gentagelige statiske kontrol k?res med `python Tools/validate_after_action.py`. Dette er ikke en compiler- eller runtime-verifikation.

Forternes eksisterende resultatvej bevares; et fuldt separat tabsregnskab for fortbesætninger, der ikke er almindelige taktiske enheder, og et egentligt moral-/cohesion-tidsforløb kræver senere udvidelse. Automatisk afgørelse har ingen simulerede salver eller kamptid og viser derfor ikke opdigtede målinger.
