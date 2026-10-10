# Enhedstilpasning

Enhedskortet har **OMDØB**, **UNIFORM** og **OPGRADÉR**. Navnefeltet
erstatter navnet ved første indtastning; **GEM NAVN** eller Enter gemmer,
**TILBAGE**, X eller Escape annullerer. Tomme navne afvises. Navnet må
indeholde højst 80 tegn og følger enheden i lister, kamporden, kortetiketter
og 3D-slagets HQ og kompagnier. Det oprindelige navn bevares særskilt, så
omdøbning ikke ændrer kavaleritypen.

Uniformpanelet har 12 farver for hver af jakke, bukser og hovedbeklædning
samt **D**, som fjerner den pågældende tilsidesættelse. Valg lagres straks
i kampagnens tilstand og kommer med ved næste gemning. Den flade figur
viser den valgte palette. Farverne er periodens repræsentative farver,
ikke dokumenterede farvestofspecifikationer. Hattenes snit er integreret
i de nuværende soldatermeshes; kasket/shako/hjelm kan derfor ikke vælges
uafhængigt. Der tilbydes ingen knapper uden en understøttet model.

## Materialer til 3D-farver

De eksisterende importerede materialer bruger faste teksturer. Farverne
overføres allerede som lineære RGBA-værdier via `Units.json`, som
`BattleRequest_N.json` refererer til, og indlæses i
`FStrategyUniformOverrides`/`FStrategyUniformColors`. De anvendes på
soldatermeshes, også når formationen opretter flere efter BeginPlay.
Præsentationssnapshot indeholder jakke, bukser og hovedbeklædning.

**Asset-forberedelsen er endnu ikke kørt.** For synlige farveændringer
på de teksturerede infanterimodeller skal
`Content/Python/prepare_campaign_uniforms.py` køres i Unreal Editors
Python-konsol. Scriptet tilføjer parametrene `CoatColor`, `TrouserColor`,
`HeadgearColor` og tre `Override…`-kontakter til de eksisterende
materialgrafer. Det gemmer de berørte assets uden at åbne materialeeditorer.
Det starter ingen editor på egen hånd. API-referencen er
[Epic MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary).

Garmentmaskerne anvender skønnede højdebånd i modellens uskinnede lokale
koordinater og bevarer hud, støvler og lyse remme efter teksturens farver.
Maskerne kræver visuel kontrol på infanteri, jægere og garde; ved behov bør
de erstattes med malede teksturmasker. Farvetilpasset infanteri bruger nu også VAT, når modellerne og
`M_CrowdVAT_All` er bagt med `Content/Python/bake_all_vat.py`. Instansdata
indeholder de tre RGB-farver og override-kontakter; det fusionerede gevær
får ikke uniformfarver. Bagescriptet forbereder også nærfigurernes
uniformmaterialer og overgangsmasker. Manglende VAT-assets giver skeletfallback med
`PROJECT1864-CROWD`-log. Masker/overgange er ikke visuelt verificeret.
Ryttere/stab/ordonnanser bruger samme VAT-farvedata og n?rmaterialer.
Artilleri modtager samme farvetilstand; synlig
farvning afhænger af, at deres besætnings-/soldatermeshes og materialer
understøtter parametrene.

## Våben

Håndvåben følger flintlås → perkussion → Minié-riffel → bagladegevær.
Fod- og ridende batterier følger glatløbet → riflet kanon; morterer har
ingen ombygning. 1825 starter med flintlåsvåben, 1851 med perkussion.
Perkussionslås er kendt i 1851. Forskning åbner køb/ombygning; den udstyrer
ikke automatisk hele hæren. Forskningslisten er udvidet i slutningen, så
gemte projektindekser fortsat peger på samme emner.

Ombygning forudsætter stilstand uden kamp eller rekrutuddannelse og den
nødvendige forskning. Geværer betales for hele etablissementet, inklusive
fraværende mænd og plads til erstatningsmandskab; kanoner pr. faktisk
kanon. Lageret bruges først, resten købes automatisk. Ombygning koster
desuden 1 rd. pr. gevær eller 40 rd. pr. kanon. Indkøb koster 6/12/18 rd.
pr. håndvåben efter trin og 400 rd. pr. kanon. Alle priser og kampfaktorer
er spilbalanceringsskøn.

Ombygning tager 10 dage for håndvåben og 14 for kanoner. Fremskridtet
står stille på march og i kamp. Undervejs anvendes det gamle våben med
halv træfsikkerhed og dobbelt ladetid. Efter afslutning overføres det nye
våbens rækkevidde, træfsikkerhed og ladetid til 3D-slaget; automatisk kamp
anvender også ombygningens ulemper og våbenfordelene. Ombygning betales
kun én gang og lagres med resterende dage i gemmeformat **29**.

Deling arver våben, farver og igangværende ombygning. Flytning af mænd,
kompagnier og kanoner mellem enheder og sammenlægning kræver samme
våben og afsluttet ombygning. Den beholdte enheds farver bliver gældende
for en sammenlagt enhed.

## Afprøvning i Unreal, når det ønskes

- I begge scenarier: omdøb en oprindelig og en ny enhed; kontroller kort,
  lister, kamporden, gem/indlæs og navn i 3D-slaget. Skriv W/A/S/D/K/F i
  navnefeltet og kontroller, at kamera og genveje ikke reagerer.
- Vælg forskellige farver for alle tre dele, nulstil én med D, gem/indlæs
  og del enheden. Efter materialeforberedelsen kontrolleres levende og
  faldne infanterifigurer tæt på og på afstand samt garder/jægere.
- Forsøg opgradering uden forskning og uden penge. Gennemfør derefter
  hvert våbentrin; kontroller lager/ledger, 10/14 dage, pause på march og
  i kamp, gem/indlæs midt i ombygningen samt rækkevidde/ladetid i 3D.
- Kontroller ridende batterier og afvisning af morteropgradering. Forsøg
  at sammenlægge eller flytte mellem forskellige våbenniveauer.
- Indlæs et ældre v28-spil: standardfarver og scenariets startvåben skal
  anvendes. Kontroller, at igangværende forskning beholder sit emne.

Ingen build, editorstart eller spiltest er foretaget under denne ændring.
