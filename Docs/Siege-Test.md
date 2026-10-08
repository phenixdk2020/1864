# Kampagnetest af belejring

Dato: 2026-10-08. Implementeret og gennemgået statisk; **ikke bygget eller kørt**. Backlogpunktet er derfor stadig ikke afprøvet i spillet.

Kør følgende i PowerShell efter næste build (kommandoen er ikke kørt her):

```powershell
& "I:/Spil/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "R:/Onedrive/cx2/siege/Game1864.uproject" /Game/Maps/Campaign1851 -game -windowed -ResX=1920 -ResY=1080 -CampaignNation=DK -CampaignSeed=1864 -CampaignDeviation=0 -CampaignWorks=2 -CampaignFortsComplete -CampaignTestSiege=Fredericia:45 -log
```

Flaget starter en ny kampagne, hvis der ikke indlæses en kampagne eller returneres fra et slag. Som ved andre nye spil overskrives Autosave. Det bruger det valgte scenarie (1825 som standard); scenariedata ændres ikke. Fredericias historiske værk bygges kun tidligt via det særskilte testflag `-CampaignWorks=2`. Byen skal allerede have garnison eller færdig skanse; forkert navn, udenlandsk/besat by og manglende forsvar afvises i loggen. `:<days>` er testens løbetid i kampagnedage (1–365, standard 45), ikke en ændring af belejringsreglerne. Testen pauser ved slutningen og lukker ikke spillet.

Testen anvender eksisterende krigstilstand, erklærer krig hvis nødvendigt og opretter et preussisk korps gennem `SpawnCorps`. Det opstilles 8,9 km syd for byen med sammenlignelig styrke og uden egne kanoner, så den almindelige startkontrol starter belejringen på næste tick. Testflaget vælger højeste hastighed (6), også hvis `-CampaignSpeed` angiver noget langsommere, og aktiverer samme automatiske slagafgørelse som `-CampaignAutoBattles`. Andre krigskorps og normale kampagnesystemer er stadig aktive; dette er en integrationstest, ikke en isoleret balanceprøve. Testen tvinger hensigten `SiegeTown`, og afprøver derfor ikke AI'ens oprindelige målvalg.

## Forventet log

Søg i `Saved/Logs/Game1864.log` efter `CAMPAIGN-1851|siege|`:

- `opstilling`: korps-id/navn, by, styrker og forsyninger før start.
- `døgn`: én status pr. behandlet kampagnedøgn, også efter storm eller opløsning. `belejrer=1`, forløb i dage mod 35, garnisonens tilstedeværende mænd, skansernes garnison/kanoner og hver skanses forsvar, proviant og skud. Regimentsproviant og ammunition vises som summer, ikke som fælles lager eller gennemsnit. Fjendtlige korps har ikke et forsyningsfelt.
- Skanser mister 6 skud pr. kanon og 0,3 dags ekstra proviant dagligt; forsvar falder hver tiende dag. Andre forsyningssystemer kan ændre tallene samme døgn.
- Storm senest efter 35 dage, eller tidligere ved brud/forsvar 1 eller styrkeforhold mindst 1,3. Slagloggen skal indeholde byens forsvarere og skanser. Slagudfaldet før også belejringspræfikset og viser stemning og aktuel besættelse efter udfaldet.
- `faldet=1` betyder faktisk besættelse, ikke blot dansk nederlag. Besættelse sker i det normale marchforløb efter forsvarernes tilbagetog; et nederlag kan derfor først vise `faldet=0`. `prestige=ikke implementeret` er bevidst: der findes ikke national prestige i kampagnedata. Stemningen påvirkes af slagets eksisterende `PoliticalShock` (+3 ved sejr, −1 ved uafgjort, −3 ved nederlag; begrænsninger og andre systemer kan påvirke den).
- Til sidst `test afsluttet; kampagnen er sat på pause`.

## Rettede sikre fejl

1. Stormslaget blev dannet ved belejringskorpsets position ca. 9 km væk, men `CreateBattle` medtager kun forsvarere inden for 8 km. Stormen flytter nu korpset frem til byen før slagoprettelse.
2. Marchankomst kunne besætte en by med færdige skanser før kontakt/belejring blev registreret på næste tick. Skansen og en planlagt belejring tæller nu som forsvar ved ankomst.
3. Belejringsstart fastholder nu korpset 8,9 km fra byen, også efter en dagsmarch der har passeret startafstandens grænse.
4. Et nyt almindeligt angrebsmål nulstiller en gammel belejringshensigt.
5. Manuel undsætning var blokeret af `EngageableCorps` for alle belejrende korps. Tropper uden for den belejrede positions 10 km kan nu angribe; tropper inde i positionen kan stadig ikke bruge denne vej som udfald. Automatisk kontakt med undsætning findes allerede.
6. Indlæsning af en aktiv belejring rydder nu marchruten og byankomsttilstanden. Eksisterende `siege|korpsindeks|by|belejrer|startdag` bevares; intet gemmeformat er ændret.

## åbne punkter til leadens afprøvning/afklaring

- **120 dage og “belejringen er hævet”:** Ingen af disse regler/stier findes i den læste implementering. Reglen i `Siege1851.md` og koden er storm efter 35 dage. Der er ikke indført en ny 120-dagesregel uden en spec.
- **Forsyning:** Trænkolonner kan fortsat levere til skanser under belejring. Der er ingen samlet blokade/kapitulation ved tom proviant og ingen fjendtlig korpsproviant. Afklar design før en blokaderegel indføres.
- **Ophævelse:** Et afgjort slag rydder belejringstilstanden, men der er ingen særskilt nyhed om ophævet belejring. Fred/invalid by eller et korps uden mænd har heller ikke en udtrykkelig ophævelsessti i `DailySieges`. Dette kræver klarhed om ønsket adfærd.
- **Garnisonstab:** Bombardement rammer regimenter når byen; forternes udskilte kompagnier har separat mandskab og rammes ikke direkte af denne løkke. Afklar tabsfordeling for at undgå dobbeltregistrering.
- **Målvalg:** AI'en vælger kun belejring, hvis der ikke samtidig findes et attraktivt stormmål. Testflaget omgår dette valg med en eksplicit hensigt.
- **Gem/indlæs:** Gem midt i belejringen og indlæs uden testflag. Kontroller identisk `SiegeTown`, `bSieging`, startdag, skanseforsvar og forsyninger; derefter fortsat storm på den oprindelige frist. Selve kommandolinjetestens logmål og slutdag er midlertidig testinstrumentering og gemmes ikke. Kampagnens eksisterende belejringsfelter gemmes allerede i War-linjerne.
- **Undsætning:** Før danske enheder frem fra uden for 10 km mod korpset; kontroller både automatisk kontakt inden for 5 km og manuelt angreb. Sejr skal ophæve belejringen via slagudfaldet. Test desuden nederlag/tilbagetog, besættelse og eventuel befrielse.
