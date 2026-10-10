# Slaglyd

Slagmodulet har syntetiske lyde, ikke historiske optagelser. Gameplay, ammunition, tab og kampens tilfældighedsforløb ændres ikke.

## Generering og import

Kør fra projektroden:

```powershell
python Tools/Battle/make_battle_sounds.py
# Fallbacken kan også vælges eksplicit:
python Tools/Battle/make_battle_sounds.py --pure-python
```

Scriptet bruger ingen netværksadgang. Numpy er valgfri; standardbibliotekets math/random/struct/wave giver samme signal uden numpy. Fast seed gør generering reproducerbar. Output er 17 mono-WAV-filer i `Reference/Battle/Audio`, 44.100 Hz, signed PCM 16 bit, DC-korrigeret og peak-normaliseret til -3 dBFS. Skud og sammenstød er under to sekunder; rumlen er otte sekunder og hovsløjfen fire. Musketsalverne består af 20 henholdsvis 60 overlappende skud med 0–0,35 og 0–1,2 sekunders spredning.

Import kræver Unreal Editors Python-plugin. Følgende kommando starter editorens kommandolinjeproces og må kun køres efter udtrykkelig tilladelse:

```powershell
& "I:/Spil/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "R:/Onedrive/cx2/audio/Game1864.uproject" -run=pythonscript -script="R:/Onedrive/cx2/audio/Content/Python/import_battle_sounds.py" -unattended -nosplash
```

Importscriptet opretter eller erstatter `USoundWave`-assets i `/Game/Battle/Audio`, sætter `looping` på rumlen og hovlyden og gemmer assets. WAV-filerne alene kan ikke afspilles af runtime-systemet. `Config/DefaultGame.ini` inkluderer lydmappen i cooking, fordi lydsystemets referencer er bløde stier.

## Afspilning

`UStrategyBattleAudio` er en komponent på `AStrategyGameMode`. Den indlæser lydassets i BeginPlay; der foretages ingen synkrone indlæsninger under tick eller skud. Manglende assets giver en advarsel og springes over. Projektets nuværende lokale slag bruger autoritativ game mode; netværksklienter kræver særskilt præsentationsdistribution senere.

Infanteriets visuelle salveoprindelse bruges som affyringspunkt: førre end otte skud vælger en af seks musketvarianter, 8–40 vælger lille salve, flere vælger stor salve. `bUsesRifleAudio` på kampkomponenten vælger riffelknald for små riffelsalver; der er endnu ingen fælles våbenklassifikation i slagmodulet. Volumen følger kvadratroden af antallet af skud med begrænsning. Pitch varierer 0,93–1,07 via en separat RNG.

Begge artilleriets affyringsforløb spiller én af fire kanonvarianter pr. affyret kanon, med 0,05–0,2 sekunders forskydning. Morterens affyring spiller `mortar_thump`. Den eksisterende bajonetchokfunktion og kavaleriets sammenstød spiller `bayonet_clash`. Kavaleriets charge-start og -stop styrer en hovsløjfe, som følger enheden; højst fire charges har lyd samtidig. Charge-lyde starter kun inden for 800 meter.

Runtime-attenuation bruger inverse afstandsfald, en indre radius på 150 meter og samlet rækkevidde cirka 2,5 km for musketer og 6 km for artilleri (Unreal-koordinater i cm). Fra 1,4 til 1,5 km fades skud over i fjern rumlen; over 1,5 km erstattes individuelle skud. Rumlen samles i 1 km store regioner, højst fire samtidig, og stopper efter otte sekunder uden ny ild. Kameraets højde tæller med i lytterafstanden. Udgang af afstandsområdet og ophør af lyd ryddes op af tick.

Over 400 meter forsinkes lyden med afstand / 343 m/s, højst fire sekunder. Køen har 96 faste pladser og bruger reel tid; pause fryser ventetiden. En fælles `USoundConcurrency` bruger højst 24 stemmer, `StopQuietest` og afstandsbaseret volume scaling. Komponentens 24 faste stemmepladser stopper den svageste stemme før en stærkere ny spawn; svagere anmodninger droppes. Regioner og hovsløjfer tæller med i samme budget. Der oprettes ingen timere, attenuation- eller concurrency-objekter pr. skud; kun den nødvendige audio-component spawn.

Pitch følger controllerens `SimulationSpeed`. Pause pauser eksisterende lydkomponenter og blokerer nye lyde. Ved x10 er lydstyrken nul og nye lyde springes over. Lydløs tilstand blokerer ligeledes nye lyde. Faste køer og budgetter prioriterer overbelastning ved at droppe lyde frem for at ændre gameplay.

## Indstillinger og kontrol

HUD-indstillinger har lydstyrke i trin på 10 procent samt lydløs tilstand. De gemmes som `BattleMasterVolume` og `BattleMute` under `[/Script/Strategy1864.Settings]` i GameUserSettings.ini, som de øvrige slagindstillinger. Standardvolumen er 80 procent.

- `-Strategy1864NoSound`: deaktiverer hele slaglydsystemet, også preload.
- `-Strategy1864DebugAudio`: logger `PROJECT1864-AUDIO` med clip, kameraafstand i meter og volumen før attenuation/concurrency.

Statisk kontrol og WAV-validering kan køres uden Unreal. Manuel lytte-/spiltest efter import bør dække små/store salver, begge artillerimåltyper, morter, bajonet og charge; flyt kameraet over 400 m og 1,5 km, test pause/x0,5/x2/x10, mute og mange samtidige batterier. Afpræv også gemte indstillinger ved næste start. Import, build og disse spiltests er ikke udført som del af implementeringen.
