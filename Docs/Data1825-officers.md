# Officerer og regering i 1825

Dateret 8. oktober 2026. Scenariets udgangspunkt er 1. juli 1825. Dette er en kildebaseret delvis rekonstruktion af personer og styreform, med eksplicitte spilskøn. Det er ikke en fuldstændig officersrangliste eller en ny kamporden.

## Filer og skalaer

Kildedata: `Tools/Map1851/Officers_1825.json` og `Ministers_1825.json`. Identiske runtime-kopier ligger i `Data/Campaign1851`. Kør `python Tools/Map1851/sync_officers1825.py` efter dataændringer; det ændrer ingen 1851-filer. Der findes ingen særskilt Resources-kopi af officersdata i dette projekt.

`ActiveScenario().Id == "1825"` vælger disse data. 1851 beholder `Officers1851.json`, navnelister, terningeslag, generaler, kommandotilknytning og ministerpuljer. Officerernes erfaring anvender ingen scenariefaktor. Skalering af tropper og befolkning tilhører andre dele af opgaven og er ikke ændret her.

De ti evner angives i 0–100: føring, inspiration, initiativ, taktik, stab, disciplin, aggressivitet, nerve, politisk vægt og forsigtighed. Indlæsning afrunder `værdi / 10` og begrænser til 1–10, som det eksisterende kamp- og gemmesystem kræver. 0 bliver derfor intern minimumsværdi 1. Erfaring forbliver 0–100. Alle profiler er vurderinger; kilderne indeholder ingen numeriske evneværdier. Høj aggressivitet er en stil, ikke automatisk en fordel.

## Navngivne officerer

| Person | Født / alder i 1825 (år minus fødselsår) | Rang og rolle | Grundlag |
|---|---|---|---|
| Frantz Christopher Bülow | 1769 / 56 | Generalmajor, generaladjudant og kongens militære rådgiver | Generalmajor 1809; først generalløjtnant 1828. Placeringen i GK1 er en spilforenkling af den centrale ledelse. |
| Frederik Rubeck Henrik Bülow | 1791 / 34 | Infanterikaptajn; reserve i spillet | Kaptajn 1815, først major 1836. Reserveplaceringen er et skøn; konkret 1825-kompagni er ikke verificeret. |
| Christian Julius de Meza | 1792 / 33 | Kaptajn i artilleriet, lærer; starter på studieorlov | Kaptajn 1821, stipendium og udenlandsrejse 1825–1827. Tilbagekomst sat til 1. juli 1827 som skøn; kilden fastslår ikke dagen. |

Frantz Bülows profil er 60/50/40/40/75/80/30/65/95/80, erfaring 68. Stor politisk vægt og administrativ indflydelse er skøn ud fra hans nærhed til kongen; taktisk evne er vurderet forsigtigt. Frederik Bülows profil er 65/65/60/60/45/65/60/65/30/55, erfaring 42: vurderet som en aktiv yngre infanterikaptajn med erfaring fra Englandskrigene. De Mezas profil er 50/35/55/65/70/65/30/65/35/75, erfaring 35. Den bygger på hans uddannelses- og lærerbaggrund, uden at tilskrive ham sejrene i 1848–1850 på forhånd. Studieorlov bruger `Away = 3`, så han ikke kan udnævnes før hjemkomsten; den gemmes gennem de eksisterende Away/AwayUntil-felter.

De øvrige territorialkommandanter er fiktive: Hans Frederik von Rosen (1766, 59 år, GK2), Christian Wilhelm von Holck (1772, 53 år, GK3), Carl Ludvig von Schack (1768, 57 år, GK4) og Johan Georg von Rantzau (1770, 55 år, GK5). Alle er generalmajorer, erfaring 60. Disse navne påstår ingen forbindelse til konkrete historiske officerer; femkommandostrukturen er den eksisterende kampagnes abstraktion. De er markeret `historical: false`, og deres særskilte profiler står i JSON.

Regiments-/bataljonschefer og kompagnichefer genereres med tidsmæssigt passende danske og tyske navne fra helstaten. Rang følger den eksisterende enhedsfunktion: major ved bataljon/garde, oberst ved kavaleriregiment, kaptajn ved batteri og kompagni. De fire reserveofficerers grader bevares. Der findes ikke et verificeret fuldt garnisonsregister fra 1825 i denne leverance; alle genererede personer er fiktive.

| Genereret rolle | Alder | Erfaring | Basisprofil i ovenstående rækkefølge |
|---|---|---|---|
| General | 50–65 | 50–70 | 60/50/40/55/65/75/35/65/60/75 |
| Enhedschef | 38–57 | 32–58 | 55/50/45/50/55/75/40/60/40/70 |
| Kaptajn | 28–46 | 20–45 | 50/50/50/50/45/70/45/55/25/65 |

Hver genereret evne får ±10 før konvertering, med fast kampagnefrø. Kaptajnsprofilen gælder også batterichefer. Fredstjeneste giver relativt høj disciplin, men lavere erfaring og initiativ end en krigsveteran. Navnene er ikke garanteret unikke; identitet og gemning bruger officerens id. Løn og rekrutteringspriser er bevaret som balanceværdier, ikke dokumenterede 1825-lønninger; den faktiske løn beregnes allerede efter rang i hærsystemet.

## Frederik VI og enevældens regering

Frederik VI (født 1768, 57 år) er regent. Regeringsnavnet er »Frederik VI og gehejmestatsrådet«. Folkestemningen kan ikke afsætte kongen eller danne en Ejder-/skandinavistisk regering før 22. marts 1848. Helstat er her spillets kode for kongelig tjeneste, ikke medlemskab af et parti. De eksisterende opinionsandele er fortsat spilvariabler, ikke valgresultater. Regenten skifter til Christian VIII 3. december 1839 og Frederik VII 20. januar 1848. Efter enevældens ophør genbruges den eksisterende minister- og regeringssimulation.

Der fandtes ikke otte konstitutionelle fagministerier i 1825. Ressorternes UI-navne og delegation bevares af hensyn til betjeningen; de repræsenterer opgaver fordelt mellem kongen, gehejmestatsrådet og kollegierne.

| Spilressort | Startperson | Født / alder | Historisk rolle / antagelse | Dygtig / sparsom / forsigtig (0–100) |
|---|---|---|---|---|
| Indenrigs | Frederik Julius Kaas | 1758 / 67 | Præsident for Danske Kancelli, justitsminister og statsrådsmedlem | 75/65/85 |
| Offentlige arbejder | Otto Joachim Moltke | 1770 / 55 | Kancellipræsident for hertugdømmerne; tilknytning til alle offentlige arbejder er spillets fordeling | 70/75/85 |
| Krig | Frantz Christopher Bülow | 1769 / 56 | Kongens generaladjudant og militære rådgiver; ikke krigsminister i senere forstand | 65/45/80 |
| Transport | Hans Christian Holm | 1778 / 47 | Fiktiv kongelig embedsmand; ingen jernbanestyrelse | 60/75/80 |
| Intendantur | Jens Frederik Wichmann | 1775 / 50 | Fiktiv embedsmand ved militær forsyning og regnskaber | 65/80/75 |
| Udenrigs | Ernst Schimmelmann | 1747 / 78 | Udenrigsminister 1824–1831 | 70/65/90 |
| Marine | Steen Andersen Bille | 1751 / 74 | Den ældre Bille, ledende søofficer; marineposten er en funktionel forenkling | 75/70/80 |
| Finans | Johan Sigismund von Møsting | 1759 / 66 | Finansminister og præsident for Rentekammeret; central kongelig rådgiver | 85/85/90 |

Hvert ressort får desuden én navngiven fiktiv udskiftningskandidat; fødselsår, rolle og profil står i JSON. De er kongelige rådgivere, ikke yngre 1851-politikere. Evner konverteres til 1–10; 75 bliver således 8. Ministeralder og rolle er kildemetadata: den eksisterende ministerstruktur viser ikke alder og gemmer ikke disse felter. Bülow i officerskorps og regering repræsenterer samme historiske persons to funktioner; spillet forbinder ikke afskedigelser på tværs af de to systemer.

Begrænsning: dette rekonstruerer startåret. Automatisk død, pension og historisk succession for alle navngivne embedsmænd/officerer er ikke modelleret; de kan fortsætte længere end deres historiske levetid, ligesom i 1851-systemet. De senere konstitutionelle ministerier er heller ikke en fuld datokorrekt kronologi.

## Portrætter og gemning

Der findes ingen person-til-portræt-liste. `SCampaign1851Overlay::PortraitFor` vælger ved navnets hash blandt indlæste eksisterende teksturer. Officerer vælges efter alder: Ung under 38 (30 pladser), Ældre 38–49 (30), Gammel fra 50 (15). Ved manglende aldersportrætter bruges Officer/General-puljen. Ministre bruger Minister-puljen (25 pladser). Ændrede navne og fødselsår giver derfor automatisk en 1825-fordeling. Disse generiske billeder er illustrationer, ikke historiske lighedsportrætter; ingen nye eller omdøbte uassets er nødvendige.

SaveOfficers/RestoreOfficers bevarer id, navn, fødselsår, rang, generalstatus, evner, erfaring, enhed, kommando og orlov. SavePolitics/RestorePolitics bevarer regeringsnavn, strømning og de konkrete ministre med deres evner. Ingen ændring af saveversion eller felternes skala. Eksisterende gemninger bevarer deres allerede genererede officerer/ministre. Manuel indlæsning fra et andet scenarie genåbner nu kampagnekortet med det gemte scenaries data før restore; det samme gælder tilbagekomst fra slag. Det undgår 1825-kandidatpuljer på et indlæst 1851-spil og omvendt.

## Kilder (tilgået 8. oktober 2026)

- [K.C. Rockstroh: Frantz Bülow, Dansk Biografisk Leksikon](https://biografiskleksikon.lex.dk/Frantz_B%C3%BClow): fødselsår, grader og militær/politisk rolle.
- [Frederik Rubeck Henrik Bülow, Dansk biografisk Lexikon, bind III, side 284–285](https://runeberg.org/dbl/3/0287.html): født 1791 (foregående side), kaptajn 1815 og major 1836.
- [K.C. Rockstroh: Christian de Meza, Dansk Biografisk Leksikon](https://biografiskleksikon.lex.dk/Christian_de_Meza): kaptajnsudnævnelse, undervisning og studieophold.
- [Frederik Julius Kaas, Dansk Biografisk Leksikon](https://biografiskleksikon.lex.dk/Frederik_Julius_Kaas): justitsminister fra 1813 og gehejmestatsråd fra 1814.
- [Harald Jørgensen: Otto Joachim Moltke, Dansk Biografisk Leksikon](https://biografiskleksikon.lex.dk/Otto_Joachim_Moltke): kancellipræsident og statsrådsmedlem fra 1824.
- [Ernst Schimmelmann, Danmarkshistorien](https://danmarkshistorien.lex.dk/Ernst_Schimmelmann%2C_1747-1831): udenrigsminister 1824–1831.
- [Ministre og kollegier, Gyldendal og Politikens Danmarkshistorie](https://gyldendalogpolitikensdanmarkshistorie.lex.dk/Ministre_og_kollegier): kollegiestyre, kongens rådgivere og Møstings finansrolle og indflydelse.
- [Johan Sigismund Møsting, Dansk biografisk Lexikon, bind XII, side 117](https://runeberg.org/dbl/12/0119.html): født 1759; supplerende biografisk identifikation.
- [Arktisk Institut: Østgrønlandske stednavne, Steen Andersen Bille](https://arktiskinstitut.dk/fileadmin/files/arktiskinstitut/pdf/Oestgroenlandske_stednavne/OEstgroenlandske_Stednavne_Version_15.pdf): den ældre Bille, født 1751, viceadmiral i 1825. Præcis udnævnelsesdag er ikke anvendt.
- [Frederik 6., Dansk Biografisk Leksikon](https://biografiskleksikon.lex.dk/Frederik_6.): kongens personlige styre og regeringstid.

## Kontrol

JSON-parsing, identiske kilde-/runtime-filer, id- og kommandoreferencer, evnegrænser, fødselsår, orlovsdato og otte ministerpuljer er kontrolleret statisk. 1851-datafilerne er uændrede. Save-/restore-kode og scenarieskift er gennemgået; Unreal er hverken bygget eller startet. Faktisk indlæsning og UI i Unreal udestår.
