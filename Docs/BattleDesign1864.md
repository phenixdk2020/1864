# 3D-slagene: designbeslutninger

Dette dokument samler de beslutninger, der er truffet om slagene. Kampagnens kort, byer, økonomi og rekruttering er holdt ude. Teksten er brugerens egen opsummering fra 1. oktober 2026. Den gemmes her, så den følger koden i Game1864.

- **Unity** er den praktisk afprøvede battle-prototype og facit for den adfærd, der allerede er testet.
- **Unreal** (`Source/Strategy1864`) er den permanente implementation. Den bevarer det, der er valideret i Unity, men med en renere arkitektur og mere detaljeret simulation.

Unity er reference, indtil den tilsvarende Unreal-funktion er bygget **og testet** (se Designmanualen).

## 1. Grundidé
Slagene er en egentlig taktisk simulation, ikke en visuel minispil-del. Kommandostruktur, formation, terræn, våben, moral, sigtelinje, officerer og fysisk placering har betydning.

```
DIVISION → BRIGADE → REGIMENT → MAJOR A / MAJOR B → kompagnier
```

- HQ'erne er fysiske enheder på slagmarken.
- En ordre delegeres ned gennem kæden: spiller → division → brigade → regiment → major → kompagni.
- Det giver plads til officerernes evner, ordreforsinkelse, kurerer, tab af HQ og lokalt initiativ.

## 2. Én bevægelsesautoritet
Der må kun være én fysisk bevægelsesautoritet ad gangen. Ellers sker der det, Unity viste:
- kompagnier går over broen og tilbage;
- HQ'er pendler frem og tilbage;
- enheder standser før målet;
- Line og Column skifter igen og igen;
- en ordreknap forbliver blå.

Missionen bestemmer, hvor enheden skal hen. Formation, bro og undvigelse bestemmer, hvordan den kommer derhen, men må ikke ændre slutmålet.

## 3. Ordrer og ordrestatus
- **Ordrerne:** ANGRIB HER · FORSVAR HER · RYK FREM · TILBAGETRÆK · SAML · STOP/HOLD.
- **Blå knap:** ordren udføres fysisk lige nu. Rød: alt er faldet på plads. FORSVAR HER kan stadig være den gældende hensigt.
- **To ting holdes adskilt:** mission/hensigt og fysisk udførelse.
- **Afsluttet:** et kompagni regnes som fremme inden for ca. 0,50 m af sin endelige plads.

## 4. Valg og RTS-styring
- **Valg:** venstreklik vælger, og venstreklik med træk giver boksvalg.
- **Modifikatorer:** Shift og Ctrl bruges senere.
- **Fjender:** de vælges ikke sammen med egne enheder.
- **HQ'er og kavaleri:** de kan vælges på slagmarken eller i kampordenen.
- **Ved ordre fra højere HQ:** valget forsvinder ikke, når målet placeres.

## 5. Front er en del af ordren
- **Klik:** destination.
- **Klik og træk:** destination og front.
- **Rækkefølge:** fronten fastlægges, før kompagniernes pladser beregnes. At rette den bagefter gav forkerte linjer i Unity.

## 6. Infanteri 1:1
- **Grundregel:** én synlig soldat er én mand. 190 mand vises som ca. 190 soldater.
- **Kavaleri:** gardehusarer 120 og dragoner 140.
- **Visningsgrad:** `UStrategyInfantryVisualComponent` kan vise 1:1, 1:2, 1:5 og 1:10.
- **Simulationen:** visningsgraden må aldrig ændre den simulerede styrke eller formation.

## 7. Formationer
- **LINE, COLUMN og SQUARE.**
- **Front:** et kompagni på 190 mand i QA-modellen har ca. 48 m front i 3 geledder. Antallet af geledder skal senere kunne afhænge af nation, periode og doktrin.
- **Udfoldning til linje:** ved nærmeste fjendes maksimale rækkevidde + 35 m, i testen ca. 135 m.

## 8. Afstand mellem kompagnier
- **Afstand:** nominel midte-til-midte-afstand 72 m, mindst 68 m.
- **Ved konflikt:** hellere forskydning til siden end at stable kompagnier.
- **Skudfelt:** et kompagni, der spærrer for et andet, kan træde til siden.

## 9. Rækkevidde, kegler og sigtelinje

| Ildpolitik | QA-afstand |
|---|---|
| CLOSE | 35 m |
| MEDIUM | 70 m |
| LONG | 100 m |
| Ildkegle | ±35° |

- **Visning:** den aktive afstand vises tydeligt, de andre svagt. HOLD viser kun de svage.
- **Under test:** alle fjendens kegler er synlige fra starten.
- **Kun QA-grafik:** keglerne giver ikke AI'en viden. Skud kræver kendt mål, sigtelinje, rækkevidde og kegle.

## 10. Ild og ammunition
- **Kampkernen:** ammunition, ladning, rækkevidde, træfsikkerhed, kegle, sigtelinje, tab, moralchok og chok på samholdet.
- **Salver uden træf:** en salve kan give 0 træf og bruger stadig ammunition og ladetid.
- **Styrke:** træf trækkes fra `CurrentStrength`.
- **Moral og samhold:** to adskilte værdier.

## 11. Skydeøvelse (forskning)

| Niveau | Øvelse |
|---|---|
| 0 | Front Rank Fire |
| 1 | Two-Rank Fire |
| 2 | Fire by Rank |
| 3 | Controlled Volley |
| 4 | Independent Fire |
| 5 | Advanced Fire Drill |

- **Bevidst forenkling:** progressionen er en ahistorisk gameplay-abstraktion.
- **Fire by Rank:** geled 1 skyder og lader, så geled 2, så geled 3, og forfra. Soldaterne bytter ikke plads.
- **Gamle former bevares:** forskning erstatter ikke de gamle former.
- **Forskning er ikke dygtighed:** træning, ilddisciplin, underofficerer, træthed, moral og våben afgør, hvor godt øvelsen udføres.
- **Status:** implementeret i v00.02.79.

## 12. Stående, knælende og liggende
- **Påvirker:** målprofil, bevægelse, ladning, skydeøvelse og dækning.
- **Bagladere** fungerer bedre liggende end forladere.

## 13. Karré
- **Formationen:** en fysisk formation mod kavaleri. Den tager tid at danne og er dårlig til at bevæge sig.
- **Som mål:** et attraktivt mål for artilleri og infanteri.
- **Ild:** fire sider med hver 25 % ild og egen ladetid. Ikke 360° med hele kompagniet.
- **Røg:** retningsbestemt, fra den side der skyder.

## 14–16. Kavaleri

| Situation | Formation |
|---|---|
| Normal linje | 4 geledder |
| Chok | 4 geledder |
| Marchkolonne | 4 i bredden |
| Bro/defilé | 2 i bredden |

- **Omformering:** rytterne flytter sig fysisk til nye pladser. Fuld chokfart først, når omformeringen er færdig.
- **Ved broer:** kavaleriet rider mod broen i normal formation og går først i 2 i bredden ca. 36 m fra broen.
- **Rækkefølgen:** nær bred → bro → fjern bred → plads at komme ud på → tidligere formation genoprettes.

## 17. Floder og ruter
- **Vand:** åbent vand er en hård forhindring.
- **Broruter:** kræver et reelt skift af bred.
- **Samme bred:** bevægelse følger bredden.

## 18–20. Kavaleriets adfærd
- **Gennem infanteri:** kavaleri rider ikke gennem fjendtligt infanteri.
- **Selvstændig AI:** SCREEN → OPPORTUNITY → CHARGE.
- **Chokretning:** front er dårligst, flanke stærkere, ryg stærkest.
- **Chokkets styrke:** senere påvirkes chokket af tab, moral, samhold, hestenes tøven, terræn, forsvarsild og formation. Det kan ende i FALTER, ABORT eller ROUT.
- **Infanteriets svar:** infanteri skyder kun mod kavaleri med sigtelinje, rækkevidde og kegle. Det samme gælder karré-truslen.

## 21. Kavaleri tilknyttet Major A/B
- **Under angreb:** midlertidigt `CurrentCommandParent`. `OrganicParent` forbliver uændret.
- **To og to:** to kavalerienheder og to majorer parres med laveste samlede rejsevej.
- **Bagefter:** tilbage til det tidligere HQ som reserve.
- **FORSVAR HER:** reserve og flankesikring ca. 150 m bag og 45 m til siden. QA-værdier.

## 22. Dragoner
- **Afsidning:** mounted → SID AF → kæmper til fods → STIG OP → mounted.
- **Fordeling:** ca. 75 % skytter og 25 % hesteholdere.
- **STIG OP på afstand:** gå tilbage til hestene, saml og sid op. Ingen teleportering.
- **Til fods:** HOLD/CLOSE/MED/LONG og karabin med ladetid og ammunition.

## 23. HQ'er følger slaget
- **Følge:** HQ'er følger formationen i passende afstand bagved. Det er administration og overtager ikke missionen.
- **Kommandozoner (QA):** major 320/450 m, regiment 800/1100 m, brigade 1350/1850 m, division 2100/2850 m.
- **Senere:** zonerne påvirker ordreforsinkelse, rapporter og koordination. De giver ikke en magisk skadestraf.

## 24–25. Officers-AI
- **Slås til og fra:** ON/OFF pr. kommandoniveau.
- **Hvad ON betyder:** klar til at udføre og videregive missioner inden for sin myndighed. Det betyder ikke "find selv en fjende".
- **Prioritet:** direkte spillerordrer går forud.
- **Doktriner:** DEF/BAL/OFF.
- **Officerprofiler:** initiativ, taktik, nerve og flere.
- **ANGRIB HER for et kompagni:** en afgrænset placering. Kompagniet ankommer, beholder fronten og kæmper gennem ildpolitikken. Ingen ny jagt.

## 26. Terræn og dækning
- **Ét terrænsystem:** bakke, ryg, sænkning, hældning, kam, bagskråning, død vinkel og højdefordel.
- **Samme sandhed:** artilleri og infanteri bruger den samme sigtelinje.
- **Højde:** forbedrer observation, men ophæver aldrig en fysisk sigtelinje.

## 27. Sortkrudtsrøg
- **Simulation:** røgen har tæthed og levetid.
- **Effekt:** den påvirker sigtelinje og træfsikkerhed.
- **Visning:** retningsbestemt.

## 28–30. Specialister, stillinger og befæstninger
- **Specialister:** et kompagni detacherer skytter, skarpskytter og arbejdshold efter behov.
- **Detacherede mænd:** de trækkes fra moderenheden og tælles ikke to gange. At komme tilbage kræver tid og samhold.
- **Forsvarsstillinger:** retning, kapacitet, tilstand, ejer, besætter, gennembrud og erobring.
- **Typer:** hurtig dækning, markarbejder, løbegrave, brystværn, skanser og kanonstillinger.
- **Storm på skanser:** stiger, ingeniører, arbejdshold og gennembrud.

## 31–34. Artilleri og morterer
**Batteriet:**
- kanoner, besætning, kuske, heste, ammunition og tilstand;
- LIMBERED → DEPLOYING → DEPLOYED → LIMBERING, plus flytning med håndkraft;
- skud kræver kanoner, besætning, ammunition, sigtelinje, rækkevidde, sideretning og opstilling.

**Ammunition** (endelige historiske værdier er ikke låst):
- kugler, granater, shrapnel og kardæsk.

**Skader og erobring:**
- træf rammer besætning, heste eller kanoner;
- en kanon kan være operational, disabled eller destroyed;
- disabled kan repareres, destroyed kan ikke;
- et batteri kan blive ABANDONED eller CAPTURED, og erobrede kanoner kan genbruges.

**Morterer:**
- en egen evne, der rammer bagskråning, død vinkel, løbegrave og skanser uden direkte sigtelinje;
- TRANSPORT → EMPLACING → DEPLOYED → PACKING.

## 35–36. Forsyning og projektiler
- **Forsyningsvogne:** fysiske vogne med ammunition, kuske, heste, tilstand og last.
- **Genforsyning:** kræver nærhed, den rigtige ammunition og en passende tilstand. Den stopper ved bevægelse eller under ild.
- **Projektiler:** visningen viser det træf, simulationen har beregnet, aldrig et andet.
- **Kamera:** P følger det seneste projektil, F9 viser banen.

## 37–38. Fog of War og SPEJD HER
- **Viden om fjenden:** Unknown → Suspected → Contact → Identified → Fresh → Stale. Sidst kendte position har faldende sikkerhed.
- **SPEJD HER:** RECON → CONTACT → SCREEN → REPORT, med forsinket melding op gennem kæden.
- **Status:** aktiveres først, når FOG og sigtelinje er autoritative.

## 39. Kamporden, HUD og zoom
- **Kampordenen:** ét samlet træ. Et klik vælger, dobbeltklik fokuserer. Rullebjælke. AI vises som ON/OFF.
- **Zoomtrin:** Close, Medium, Operational, Strategic og Very Far.
- **På afstand:** symboler erstatter 1:1-modellerne, uden at simulationen stopper.

## 40–42. Modeller og animationer
- **Livgarden:**
  - `DK_Livgarden_1864_Apose_textured_skeleton.fbx`;
  - geværet med og uden bajonet;
  - 65 Mixamo-animationer.
- **Fælles skelet:** `SK_Human_1864`. Animationerne deles, og uniformer, mesh og materialer udskiftes.
- **Mangler:** en god stående ladning af forladeren.
- **Kavaleri:** rytter og hest er separate, synkroniserede meshes. Rytteren kan blive skudt af en hest, der løber videre, og en hest kan falde under en rytter, der styrter.

## 43. Unreal-porten er ikke et redesign
Unity er facit for det, der er testet. Unreal implementerer det med rene, permanente systemer:
- formation, ordrer, officers-AI og ildkontrol;
- skydeøvelse, kamp, stilling og terræn;
- kavaleri, artilleri, forsyning, specialister og befæstninger;
- visning, animationstilstand og udstyr.

## 44. Testet eller kun implementeret

| Område | Unity | Unreal |
|---|---|---|
| Infanteriets bevægelse, linje og kolonne | Testet visuelt | Kerne implementeret |
| 1:1-infanteri | Testet | Ny rigtig skelet-visning lavet |
| Afstand mellem kompagnier | Testet og justeret | Implementeret |
| Tidlig udfoldning til linje | Testet og justeret | Implementeret |
| Angreb og forsvar | Testet mange gange | Implementeret |
| Ordrestatus blå/rød | Testet og rettet | Implementeret |
| Valg bevares | Testet | Implementeret |
| Front ved træk | Testet | Implementeret |
| Flod og bro | Testet og fejlrettet | Implementeret |
| Kavaleriformationer | Testet | Kerne implementeret |
| Kavaleri 2 i bredden over bro | Testet og justeret | Implementeret |
| Dragoner sidder af | Testet | Kerne porteret |
| Dragonernes ild | Testet | Kerne porteret |
| Karré | Testet og fejlrettet | Implementeret |
| Fjendens kegler | Testet gentagne gange | Implementeret (QA) |
| Kavaleri-AI og screen | Testet | Kerne implementeret |
| Højere HQ | Testet | Implementeret |
| Midlertidig tilknytning af kavaleri | Testet | Implementeret |
| Artilleri | Ikke fuldt implementeret | Stor kerne implementeret |
| Morterer | Design | Kerne implementeret |
| Forsyningsvogne | Design | Kerne implementeret |
| Avanceret terræn og død vinkel | Delvist | Kerne implementeret |
| Skydeøvelse som forskning | Ny beslutning | Implementeret v00.02.79 |
| Livgarden som rigtigt skelet-mesh | Ikke denne model | Importeret. Kører i duellen i Game1864 (1. okt. 2026) |

Hele Unreal-porten må ikke kaldes testet endnu.

## 45. Næste milepæle (fra 1. oktober 2026)
- **Allerede kørt i Game1864:**
  - Livgarden og svensk infanteri med fælles skelet, rigtige soldater, march i kolonne og linje i 3 geledder;
  - salver, røg, død og faldne der bliver liggende (testbanen `Strategy1864_Duel`).
- **Mangler:**
  - stående, knælende og liggende testet;
  - FireByRank vist geled for geled på den enkelte soldats plads, så geled 1, 2 og 3 skyder og lader hver for sig i stedet for hele kompagniet på én gang.

## 46. Afklaringer (1. oktober 2026)
- **Geledder:** infanteriet står i 3 geledder. Det giver et kompagni på 190 mand ca. 48 m front. I 2 geledder ville fronten være ca. 70 m og linjen meget lang.
- **Modstander:** de første fjender er svenske, med den svenske 3D-model. Preussiske og østrigske tropper kommer senere.
- **Fra forskning til slag:** en ny evne (fx en skydeøvelse eller karré) skal først forskes i kampagnen og dernæst trænes af den enkelte enhed, før den kan bruges i slaget. `battleRules` angiver, hvad der er forsket. Enhedens træning afgør, om den kan det.
- **Vand:**
  - Små vandløb og bække må krydses, men langsomt og i uorden.
  - Større vandløb og floder kan kun krydses ad en rigtig bro eller en pontonbro, som pionererne skal bygge.
  - En sprængt bro skal bygges op igen eller erstattes af en pontonbro.
