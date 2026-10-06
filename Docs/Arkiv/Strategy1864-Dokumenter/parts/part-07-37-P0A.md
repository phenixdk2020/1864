# PROJECT 1864 — Designmanual v00.02.07

## 37. Implementeringsstatus – P0A Unity 3D Battle Prototype

P0A er den første spilbare tekniske vertical slice. Formålet er ikke endelig grafik eller historisk balance, men at bevise at den valgte regimentsmodel kan styres i realtid i 3D og at authoritative mandskab, formation, range, combat, morale og rout kan holdes adskilt fra den grafiske 1:10-repræsentation.

### 37.1 Spilbar scope v00.00.07

| System | P0A implementering |
| --- | --- |
| Battle setup | 2 danske regimenter mod 2 preussiske regimenter på et kompakt proceduralt 3D-kort. |
| Authoritative strength | Danmark: 1. Regiment 620 og 5. Regiment 585. Preussen: 8th Regiment 610 og 18th Regiment 560. |
| 3D representation | Ca. 1 grafisk soldat pr. 10 faktiske soldater; ca. 240 simple 3D-soldatmodeller samlet. |
| Camera | RTS-kamera med WASD/piletaster, Q/E rotation og musehjulszoom; kamera-input er uafhængigt af battle timeScale. |
| Selection & orders | Klik/Shift+klik på danske regimenter, højreklik for movement eller attack order. |
| Formation | Line og column kan skiftes under slaget. |
| Range | Valgte regimenter kan vise effektiv skudafstand som ring på terrænet. |
| Combat | Automatisk volleyild med distancefaktor, regimentskvalitet, casualties og sortkrudtsrøg. |
| Unit state | Strength, morale, cohesion og rout er simulerede regimentsværdier. |
| Enemy AI | Simpel preussisk AI; ét regiment angriber direkte, ét andet bruger en indledende flankerende waypoint. |
| Time | Pause samt 1x/2x/3x og en løbende battle clock. |
| Outcome | Battle manager afgør dansk sejr/nederlag, låser slaget i pause-state efter resultat og tillader restart med R. |

### 37.2 Arkitektur der nu er praktisk bevist

- Det faktiske mandskabstal er authoritative. Antallet af 3D-modeller følger styrken, men modellerne er ikke selvstændige combat-agenter.
- Regimentet er den simulerede kampentitet; visual soldiers følger formation slots og kan senere erstattes af historiske modeller/animationer uden at omskrive kampberegningen.
- Tactical orders og enemy AI bruger samme `Regiment`-interface, hvilket giver et naturligt grundlag for senere brigade-/officer-AI.
- Range, fire interval, accuracy og movement speed er datafelter og kan senere flyttes til historiske weapon/unit databases.
- Battlefield-generation er adskilt fra unit simulation og kan senere erstattes af håndlavede eller geodata-baserede maps.

### 37.3 Bevidste prototypebegrænsninger

- Ingen NavMesh/formation pathfinding eller obstacle avoidance endnu.
- Ingen order delay, courier-system, commander intent eller selvstændig brigadechef-AI endnu.
- Ingen bataljons-/kompagniunderopdeling i den aktive 3D-simulation endnu.
- Ingen ammunition, wounded/killed-split, sanitet, POW eller persistent campaign aftermath endnu.
- Ingen historiske uniformsmodeller, faner, animationer, lyd eller endelig UI.
- Våbenparametre er prototype-tuning og må ikke betragtes som endeligt historisk balancegrundlag.

### 37.4 Foreslået næste build – P0B

P0B må først påbegyndes, når P0A v00.00.07 er compile-clean og runtime-valideret i Unity 6000.6.0f1.

1. Indfør bataljoner som underformationer i regimentet.
2. Implementér order delay, acknowledgement og brigadechefens selvstændige reaktioner ud fra skills/personality.
3. Tilføj rigtig terrain cover, line-of-sight og skov-/hegn-/højderygseffekter.
4. Tilføj ammunition, reload/fire discipline samt killed/wounded/missing som separate casualty states.
5. Udskift direkte movement med NavMesh/formation pathfinding.
6. Tilføj historiske uniformer, faner og animationssystem.
7. Tilføj mindst ét artilleribatteri og et simpelt casualty evacuation-/ambulance-hook som første support-system.

### 37.5 Repository- og Unity-layout

Fra P0A **v00.00.03** er GitHub-repositoryet `phenixdk2020/Strategy` selv det aktive Unity-projekt. Unity Hub skal derfor clone og åbne repository-roden direkte.

```text
Strategy/
├── Assets/
├── Packages/
├── ProjectSettings/
├── docs/
├── README.md
└── VERSION.txt
```

Den tidligere placering `prototype/P0A-Unity-Battle/` er fjernet for at undgå, at Unity Hub kloner repositoryet men ikke finder `ProjectSettings/ProjectVersion.txt` i projektroden. Tekniske P0A-noter ligger fremover under `docs/prototypes/`.

Fra P0A **v00.00.04** er Unity editor-baseline `6000.6.0f1` (Unity 6.6) med changeset `f7f8ed4d1e24`. Baseline er valgt efter faktisk testmiljø: Unity Hub på testmaskinen har 6000.6.0f1 installeret, mens den tidligere 6000.3.17f1 ikke var installeret og derfor udløste `Missing Editor Version`. Projektet skal som udgangspunkt følge den editor-version, der bruges til aktiv udvikling og compile-test, medmindre en senere releasebeslutning fastlåser en anden supporteret Unity-linje.

### 37.6 Unity 6.6 built-in module dependencies

Ved første compile-test i Unity 6000.6.0f1 viste projektet CS1069-fejl for `UnityEngine.GUIStyle` og `UnityEngine.ParticleSystem`. Årsagen var, at prototypeprojektets `Packages/manifest.json` oprindeligt var tomt, mens Unity 6.6 kræver, at relevante built-in moduler er aktiveret eksplicit.

Fra P0A **v00.00.05** indeholder `Packages/manifest.json` derfor:

```json
{
  "dependencies": {
    "com.unity.modules.audio": "1.0.0",
    "com.unity.modules.imgui": "1.0.0",
    "com.unity.modules.particlesystem": "1.0.0",
    "com.unity.modules.physics": "1.0.0"
  }
}
```

Modulerne understøtter følgende prototypefunktioner:

- **IMGUI** — `GUIStyle`, `GUI.Box` og det midlertidige battle-HUD.
- **Particle System** — sortkrudtsrøg ved volleyild.
- **Physics** — raycasts, colliders, selection og ordreinput.
- **Audio** — `AudioListener` på RTS-kameraet og senere lydeffekter.

Dette er en projektkonfigurationsrettelse, ikke en ændring af battle-design eller simulation.

### 37.7 C# compile cleanup i Unity 6.6

Den efterfølgende compile-test fandt én blokkerende C#-fejl og to warnings i prototypekoden. Fra P0A **v00.00.06** er de ryddet op:

- **CS0136 i `Regiment.cs`** — `column` blev deklareret både i line- og column-grenen af `GetFormationPosition()`. Column-formationens lokale variabel hedder nu `columnIndex`.
- **CS0618 i `PrototypeBootstrap.cs`** — obsolete `Object.FindFirstObjectByType<BattleManager>()` er erstattet af `Object.FindAnyObjectByType<BattleManager>()`, da rækkefølge ikke er relevant for bootstrap-checket.
- **CS0414 i `Regiment.cs`** — den private `holdPosition`-state var kun skrevet til og aldrig læst. Feltet og de overflødige assignments er fjernet; hold-ordren repræsenteres fortsat korrekt ved `hasDestination = false` og `forcedTarget = null`.

Rettelserne ændrer ikke den tilsigtede battle-adfærd. De fjerner compile-blockeren og teknisk gæld, som blev synlig ved den første faktiske Unity 6.6-kompilering.

### 37.8 Release-QA-regel for Unity-prototyper

Før en Unity-prototype markeres som spilbar skal følgende kontrolleres:

1. `Assets/`, `Packages/` og `ProjectSettings/` findes i den mappe Unity Hub åbner.
2. `ProjectSettings/ProjectVersion.txt` indeholder en syntaktisk gyldig Unity-version og revision.
3. Repository-roden kan identificeres af Unity Hub som et Unity-projekt.
4. Den valgte Unity-version er installeret på den aktive testmaskine, eller installationen er dokumenteret som et eksplicit krav.
5. Alle Unity built-in moduler, som scripts refererer til, er eksplicit til stede i `Packages/manifest.json`.
6. Projektet compile-testes i den valgte Unity-version, og blokkerende compiler-fejl rettes før Play-test.
7. Obsolete API-warnings og kendte kodewarnings ryddes, når de opdages i den aktive baseline, så warnings ikke skjuler senere regressions.
8. Eventuelle editor-, package-, module- eller compile-ændringer dokumenteres i `VERSION.txt` og designmanualens implementeringsstatus.
9. Pause, 1x/2x/3x, kamera, selection/orders, formationer, combat/rout, victory/defeat og restart gennemføres som en fast P0A smoke-test.

P0A er en systems-prototype og ikke den endelige P3 Tactical Vertical Slice fra roadmapet. Den reducerede prototype bruges til at opdage arkitektur- og kontrolproblemer tidligt, før order-delay, bataljoner, historiske assets og persistent campaign state kobles på.

### 37.9 Statisk QA-hardening – P0A v00.00.07

Inden P0B er hele den aktuelle P0A-runtimekode samt editor-bootstrap blevet gennemgået statisk med Unity `6000.6.0f1` som target. Følgende klare runtime-/input-risici er rettet:

- **Battle-end state** — efter victory/defeat er slaget nu terminalt pauset. Space og 1/2/3 ignoreres, så en afsluttet kamp ikke kan sættes i gang igen uden state-reset. `R` er fortsat den eksplicitte restart.
- **RTS camera og timeScale** — pan og rotation bruger `Time.unscaledDeltaTime`, så kameraet bevæger sig med samme real-time hastighed under pause, 1x, 2x og 3x.
- **Kamera-input** — WASD og piletaster læses direkte med `Input.GetKey` i stedet for via navngivne `Horizontal`/`Vertical` Input Manager-akser. Det reducerer afhængigheden af ikke-versionerede Input Manager-defaults i den nuværende minimale repository-baseline.
- **Mousewheel zoom** — zoom anvender et fast step pr. scroll-input i stedet for at gange scroll-delta med frame delta; zoomoplevelsen er dermed ikke frame-rate-afhængig.
- **MainCamera null guard** — `PlayerCommander` forsøger at reacquire `Camera.main` og springer input-frame over, hvis kameraet ikke findes, frem for at dereferere `null`.
- **BattleManager singleton guard** — en dublet-manager kan ikke længere overskrive den aktive `Instance`; `Instance` ryddes desuden ved destruction af den aktive manager.
- **HUD camera lookup** — `Camera.main` caches én gang pr. `OnGUI`-pass og genbruges for alle regimentslabels.
- **Register guard** — `BattleManager.Register` ignorerer `null` og dubletter.

Den bredere statiske gennemgang fandt ingen yderligere klare compiler errors eller obsolete API-kald i de fem runtime-scripts og editor-startup-scriptet. Den tidligere `FindAnyObjectByType`-ændring er i tråd med Unity 6 API'et, og de aktiverede built-in moduler matcher de namespaces/types, som P0A faktisk anvender.

Dette er **statisk QA**, ikke en påstand om runtime-validering. P0A v00.00.07 må først betegnes compile-clean/spilbar baseline, når den efterfølgende test i Unity 6000.6.0f1 er gennemført uden blokkerende Console-fejl og det fulde 2-mod-2 smoke-test-loop fungerer.

Repositoryet har fortsat en minimal Unity-generated metadata-baseline: `ProjectSettings/` indeholder på GitHub aktuelt kun `ProjectVersion.txt`, mens scene, `.meta`-filer, `Packages/packages-lock.json` og flere editor-genererede settings kan opstå lokalt ved åbning. De må ikke slettes blindt. Efter næste Unity-test inspiceres lokal `git status`, før der træffes en separat beslutning om hvilke generated metadata der skal versionsstyres for en mere reproducerbar baseline.
