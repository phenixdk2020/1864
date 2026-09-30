# Nationer, vækst og AI (Campaign 1851)

Designgrundlag: designmanualen, afsnittene *Multi-nation AI parity*, *Government, ministre og granular
AI-delegation* og *Historisk plausibel replay variation*.

## Nationer
`Data/Campaign1851/Nations1851.json`: Danmark, Sverige-Norge, Preussen og Østrig.
- **Hvem der styrer** et land er kun en indstilling (`Controller`: spiller eller AI), ikke en særlig slags land.
- **Danmark** har kort: amter, byer, veje, jernbaner, hær og officerer. Alle tal kommer derfra.
- **De andre lande** vokser på en **abstrakt model**, indtil de får deres eget kort:
  - befolkning,
  - udviklingsbudget (befolkning × skat pr. indbygger),
  - jernbane-km,
  - hær,
  - industriindeks.

  Deres ministerier fordeler budgettet efter regeringens vægte. Milepæle skrives i beslutningsloggen.
- **Sverige næste gang:** Sæt `onMap` til `true`, når landets kortdata findes. Så kører den samme AI, som styrer Danmark på AUTO.

## Seed og historisk afvigelse
- **Seed:** Hvert nyt spil får sit eget seed. Det kan styres med `-CampaignSeed=N`.
- **Afvigelse:** Den vælges i menuen under *Nyt spil*: 0, 10, 20 eller 35 %. Standard er 20 %. Den kan styres med `-CampaignDeviation=20`.
- Afvigelsen varierer, deterministisk ud fra seedet:
  - hvert amts vækst (op til 1,5 × afvigelsen),
  - regeringernes vægte pr. ressort og deres forsigtighed,
  - åbningsdatoen for jernbaner, der er under anlæg i 1851 (op til ca. 8 måneder ved 35 %),
  - officerskorpset (navne og evner), plus generalernes evner ±1 med sandsynlighed = afvigelsen,
  - regimenternes erfaring (± 15 × afvigelsen),
  - støj på AI'ens prioriteringer.
- **Samme seed giver samme verden.** Gemte spil gemmer seed og afvigelse.

## Vækst (månedlig)
- **By:** 1,4 %/år × amtets variation, plus:
  - jernbanestation +0,6,
  - chaussé +0,15,
  - færdige civile bygninger.
- **Land:** 0,8 %/år × amtets variation, plus civile bygningers landeffekt.
- **Skat:** Skatten følger befolkningen (amtets by- og landbefolkning).
- **Private investorer** bygger selv købmandsgårde, bryggerier, teglværker, savværker, maskinværksteder, klædefabrikker og kroer i voksende byer. Chancen er dobbelt så stor med station. Det koster ikke staten noget: ingen dagløn og ingen drift.

## Civile bygninger
De ligger i byens bygningsliste under fanen **CIVILE**. Priser og byggetid står i `Buildings1851.csv`, effekterne i `Campaign1851Nations::CivilEffect`:

| Bygning | Vækst by / land (%-point) | Indtægt rd./år | Privat |
|---|---|---|---|
| Skole | 0,15 / 0,05 | – | |
| Rådhus | 0,10 / – | 500 | |
| Posthus | 0,15 / 0,02 | 300 | |
| Sygehus | 0,30 / 0,03 | – | |
| Toldbod | 0,15 / – | 900 | |
| Fyrtårn | 0,05 / – | 300 | |
| Købmandsgård | 0,30 / 0,10 | 500 | ja |
| Bryggeri og brænderi | 0,20 / 0,05 | 700 | ja |
| Teglværk | 0,20 / – | 400 | ja |
| Savværk | 0,10 / 0,02 | 250 | ja |
| Maskinværksted | 0,40 / – | 1.200 | ja |
| Klædefabrik | 0,60 / 0,05 | 2.500 | ja |
| Kro | 0,10 / 0,05 | 150 | ja |

Indtægterne bogføres hver måned som *Erhverv, told og post*.

## Statsrådet (vinduet STATSRÅD)

**Fire ressorter:**
- Indenrigs: civile bygninger.
- Offentlige arbejder: chausséer og jernbaner.
- Krigsministeriet: øvelser, ledige poster og ansættelse af officerer.
- Transport: troppetog.

**Tre måder at styre et ressort på:**
- **MANUEL:** Spilleren bestemmer selv.
- **RÅDGIVER:** Ministeriet anbefaler med begrundelse, og spilleren trykker UDFØR.
- **AUTO:** Ministeriet handler selv.

**Rammer:**
- Ministerierne bruger kun penge over **mindste kassebeholdning**, som spilleren sætter.
- Hver måned må de kun binde en del af overskuddet. Delen afhænger af vægt og forsigtighed.
- Øvelser må højst koste en fjerdedel af den månedlige skat.

**Hvordan AI'en vælger:**
- Den vælger det, der giver mest afkast pr. rigsdaler: indtægt plus skat af den vækst, handlingen giver.
- Hver beslutning logges med årsager, pris og status (`CAMPAIGN-1851|ai|...` i loggen).
- AI'en bruger de samme handlinger som spilleren og får ingen gratis ressourcer.

**Test:** `-CampaignDelegate=auto|advisory`, `-CampaignOpenWindow=council`, `-CampaignSpeed=6`.

## Landene 1851–1866 og uniformerne
Listen over lande følger `Reference/Units/MultiView/PROJECT_1864_Lande_og_Uniformer_1851-1866.xlsx`. Den står også i designmanualens afsnit 5.1 og i designsupplementet "Lande og uniformer 1851–1866".

| Id | Land | Gruppe | Rolle |
|---|---|---|---|
| DK | Danmark | Norden | Kernefraktion (spilleren) |
| SE | Sverige | Norden | Kernefraktion; alliance mulig |
| NO | Norge | Norden | I union med Sverige |
| PR | Preussen | Stormagter | Kernefraktion |
| AT | Østrig | Stormagter | Historisk 1864-fraktion |
| GB, FR, RU | Storbritannien, Frankrig, Rusland | Stormagter | Garantimagter |
| HAN, MEC, OLD, BRA | Hannover, Mecklenburg-Schwerin, Oldenburg, Braunschweig | Tyske stater | Nordtyske fraktioner |
| NL, BE | Nederlandene, Belgien | Vesten | Vestlige fraktioner |
| SH | Slesvig-Holstens oprørshær | Tyske stater | Særfraktion, inaktiv (opløst 1851) |

- **Felter pr. land:** `role`, `period`, `group`, `priority`, `infantry`, `cavalry` og `artillery` (uniformstyperne). En inaktiv fraktion har `active: false`.
- **Skøn:** befolkning, hær, jernbane-km, forhold og handelsværdi for de nye lande er spillets skøn.
- **Statsrådet** viser landene med en hær på mindst 30.000 mand og nævner de mindre stater på én linje.
- **UDENRIGS** har faneblade: STORMAGTER, NORDEN, TYSKE STATER og VESTEN.
