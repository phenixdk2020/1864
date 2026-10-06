# Billeder til spillet: hvad der skal laves, og hvordan de kommer ind

Alle kampagnens billeder (bygninger, uniformer, portrætter) lægges som PNG i `Reference/Campaign1851/...`
med præcis det filnavn, der står herunder. Kør derefter `Tools/Campaign/Import-Billeder.bat`
(Unreal skal være lukket). Spillet bruger det, der ligger i mappen; mangler en fil, vises "(billede kommer)".

## Fælles regler, så alle billeder ligner hinanden

- **Samme stil på alle:** samme maleriske stil, samme lys (fra øverst til venstre), samme farvemætning.
- **Altid forfra.** Bygninger set lige på facaden (ingen perspektiv fra oven), personer forfra og lige op, ikke i A-pose.
- **Gennemsigtig baggrund** (PNG med alfa), hvor intet andet er nævnt. Ingen skygge på jorden uden for figuren.
- **Marginer:** ca. 6 % luft hele vejen rundt.
- Størrelserne er det, spillet viser dem i, dobbelt så store (skarpt på høje skærme).

## 1. Bygninger (`Reference/Campaign1851/Buildings/`): 512 × 512, forfra

Filnavn = `T_Bld_<nøgle>.png`. Spillet viser dem som 56 × 56 på listen og 190 × 190 på bygningens kort.

Militære: `Arsenal`, `Field_Hospital` (Lazaret), `Coastal_Battery` (Kystbatteri), `Powder_Magazine` (Krudtmagasin),
`Star_Fort` (Skanse), `Telegraph_Office` (Telegrafstation), `Harbor_Warehouse` (Havnepakhus),
`Grain_Warehouse` (Kornmagasin), `Rifle_Workshop` (Geværværksted), `Cannon_Foundry` (Kanonstøberi),
`Ammunition_Works` (Ammunitionsfabrik), `Stud_Farm` (Stutteri), `Remount_Depot` (Remontedepot).

Civile: `Schoolhouse` (Skole), `Town_Hall` (Rådhus), `Post_Office` (Posthus), `Hospital` (Sygehus),
`Harbor_Building` (Toldbod), `Lighthouse` (Fyrtårn), `Merchant_House` (Købmandsgård), `Brewery` (Bryggeri og brænderi),
`Brickworks` (Teglværk), `Sawmill` (Savværk), `Machine_Workshop` (Maskinværksted), `Textile_Mill` (Klædefabrik), `Inn` (Kro).

Garnisonens bygninger, samme størrelse: `T_Barracks_Infantry.png`, `T_Module_Stables.png`, `T_Module_Depot.png`,
`T_Module_Infirmary.png`.

Eksempel: `T_Bld_Schoolhouse.png`, `T_Bld_Cannon_Foundry.png`.

## 2. Uniformer (`Reference/Campaign1851/Uniforms/`): 512 × 768 (2:3), helfigur forfra

Én soldat, lige op og ned, våbnet ved siden af, hænderne nede. Vises som 150 × 230 på enhedskortet.

`T_Uniform_Infantry.png` (linjeinfanteri), `T_Uniform_Guard.png` (Livgarden), `T_Uniform_Jager.png` (jæger),
`T_Uniform_Cavalry.png` (dragon), `T_Uniform_Hussar.png` (husar), `T_Uniform_Artillery.png` (artillerist, fod),
`T_Uniform_HorseArtillery.png` (ridende artilleri).

## 3. Portrætter (`Reference/Campaign1851/Portraits/`): 512 × 640 (4:5), hoved og skuldre forfra

Spillet giver hver person ét af puljens billeder ud fra navnet, så det er typer, ikke ligheder. Jo flere forskellige,
jo færre ens ansigter. Højst 12 pr. slags (00 til 11); færre end 12 virker også.

- `T_Portrait_Officer_00.png` ... `_11.png`: kaptajn eller major, mørkeblå uniform med rød krave.
- `T_Portrait_General_00.png` ... `_11.png`: general, epauletter og ordener.
- `T_Portrait_Minister_00.png` ... `_11.png`: minister i sort kjole og hvidt halstørklæde.

Portrættet vises i en ramme (178 × 224 på officerskortet, 190 × 238 på ministerkortet), derfor 4:5.

## 4. Flag (`Content/Units/Flags/`)

Flagene bruges som tekstur på flagstangen i slaget og som billede på enhedskortet. Filnavn `T_Flag_<land>.png`, kvadratisk,
1024 × 1024, flaget udfladet og forfra: `DK`, `PR` (Preussen), `AT` (Østrig), `SE` (Sverige). Flere (fx `DE`, `NO`) kan føjes til;
giv besked, så kobles de på.

## Sådan kommer billederne ind

1. Læg PNG-filerne i mapperne ovenfor (overskriv de gamle).
2. Luk Unreal.
3. Dobbeltklik `Tools/Campaign/Import-Billeder.bat`. Den importerer alle billederne som UI-teksturer.
4. Start spillet. Billederne er i spillet.

Flagene importeres som resten af enhedsmaterialet (spørg, når de er klar).
