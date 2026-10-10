# Forskningstræ: Danmark 1825–1864

Opdateret 10. oktober 2026. Der kan forskes i ét militært og ét civilt projekt samtidig. Betaling sker hver måned; skift af projekt kasserer fremdriften. Åbningsår giver en mulighed, ikke en obligatorisk historisk begivenhed. Omkostninger, varighed og bonusser er spilbalanceestimater.

30 nye civile emner supplerer de ni eksisterende. Bankvæsen, sparekasser og forsikring er samlet i ét emne; håndværk og maskinfabrikker ligeledes. Chaussébyggeri og dræning dækkes allerede af `roads` og `marl`.

## År og historisk grundlag

Nye emner har samme åbningsår i begge scenarier. Eksisterende emner er fortsat straks åbne i 1851, og jernbane/perkussion er fortsat kendt ved denne start. I 1825 er Landbohøjskolen korrigeret til 1858, jernbaneanlæg til 1844 (helstatens Holsten), felttelegraf til 1854 og bagladegevær til 1860. Disse ændringer påvirker ikke 1851-scenariets eksisterende emner.

Frimærker 1851, folkehøjskole 1844, stænderforsamlinger 1834, grundlov 1849, næringsfrihed 1857 og toldreform 1863 er historiske holdepunkter. De brede tekniske emners øvrige år er forsigtige adoptionsestimater, ikke påstande om opfindelsesår eller en ensartet udbredelse i landet. Ældre praksis er forskningsbar fra 1825 som udbredelse og organisering. Forudsætninger repræsenterer investerings- og uddannelsesforløb, ikke en uundgåelig historisk kausalitet.

Sukkerroer 1860 er et alternativhistorisk, beskedent dyrkningsforsøg med +1 % landindtægt. Dansk industriel roesukkerproduktion begyndte først i 1872 og gives derfor ikke som fabriksteknologi her. Mejeriemnet gælder herregårdsmejerier, ikke de senere andelsmejerier. Politiske forskningsemner opbygger administration; de ændrer ikke styreform eller gennemtvinger grundlov/events.

Kilder: [PostNord: frimærker 1851](https://www.postnord.dk/postnord-i-danmark/postnords-historie/posten-samfundet-og-kulturen/), [folkehøjskoler 1844](https://danmarkshistorien.lex.dk/H%C3%B8jere_bondeskoler%2C_1842-1864), [næringsloven 1857](https://danmarkshistorien.lex.dk/N%C3%A6ringsfrihedsloven%2C_29._december_1857), [Danmark 1814–1900 og toldreform 1863](https://lex.dk/Danmarks_historie_1814-1900), [dansk roesukker fra 1872](https://trap.lex.dk/Sukkeret_-_Lollands_hvide_guld), [Landbohøjskolen 1858](https://universitetshistorie.ku.dk/leksikon/v/veterinaerskolen/), [Det tekniske Institut 1843](https://forbiblioteker.kb.dk/samlinger/fysiske-samlinger/laan-tryksager/skoleblade-og-skoleaarsskrifter), [telegrafhistorie: åbning 1854](https://www.kb.dk/e-mat/dod/113414050345-bw.pdf).

## Alle forskningsemner

Niveauet er vinduets niveau: en forudsætning i det andet spor tæller ikke med. Omkostningen er rd./måned; totalen er uden afbrudte projekter.

## Civile emner

### Samfærdsel

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `roads` | Vej- og kanalbyggeri | I | 1830 | Straks | 1200 | 12 | 14400 | — | Ingeniørkunst på vejene: chausséer anlægges 15 % billigere |
| `railway` | Jernbaneanlæg | I | 1844 | Kendt | 2500 | 18 | 45000 | — | Muliggør at bygge jernbaner på kortet (uden den kan en bane ikke anlægges). I 1851 kendes den allerede |
| `telegraph` | Felttelegrafen | I | 1854 | Straks | 1500 | 12 | 18000 | — | Indkaldelsen går 25 % hurtigere; meldinger på timer |
| `steamships` | Dampskibsfart | I | 1825 | 1825 | 900 | 12 | 10800 | — | Byernes skat +2 %; indkaldelse +5 % |
| `ports` | Havneudvidelser | II | 1830 | 1830 | 1200 | 14 | 16800 | `steamships` | Byernes skat +3 % |
| `lighthouses` | Fyrtårne og sømærker | III | 1835 | 1835 | 700 | 10 | 7000 | `ports` | Byernes skat +2 % |
| `ferries` | Faste færgeruter | IV | 1840 | 1840 | 900 | 12 | 10800 | `lighthouses` | Indkaldelse +8 % |
| `postage` | Postvæsen og frimærker | II | 1851 | 1851 | 700 | 8 | 5600 | `roads` | Byernes skat +2 %; indkaldelse +5 % |
| `civiltelegraph` | Civil telegraf | III | 1854 | 1854 | 1500 | 12 | 18000 | `postage` | Byernes skat +2 %; indkaldelse +8 % |
| `railoperation` | Jernbanedrift | II | 1847 | 1847 | 1800 | 18 | 32400 | `railway` | Byernes skat +3 %; indkaldelse +10 % |

### Næringsliv

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `landreform` | Landboreformer | I | 1828 | Straks | 1000 | 12 | 12000 | — | Udskiftning og fæstebøndernes frikøb: skatten fra landet +6 % |
| `marl` | Mergling og dræning | I | 1828 | Straks | 1200 | 12 | 14400 | — | Landbruget mergler og dræner jorden: skatten fra landet +8 % |
| `agrischool` | Landbohøjskolen | II | 1858 | Straks | 2000 | 18 | 36000 | `marl` | Uddannede forpagtere og bedre sædskifte: skatten fra landet yderligere +7 % |
| `smithy` | Smede og redskaber | I | 1826 | Straks | 800 | 8 | 6400 | — | Bedre smedjer og værktøj i byerne: geværværksteder og støberier yder 15 % mere |
| `steam` | Dampmaskiner | II | 1830 | Straks | 2500 | 18 | 45000 | `smithy` | Dampkraft i værkstederne: værkerne yder yderligere 25 %, og skatten fra byerne +5 % |
| `credit` | Kreditforeninger | I | 1830 | Straks | 1000 | 10 | 10000 | — | Kredit til håndværk og handel: skatten fra byerne +5 % |
| `livestock` | Staldfodring og kvægavl | I | 1825 | 1825 | 600 | 10 | 6000 | — | Landets skat +3 %; stutterier og remontedepoter +10 % heste |
| `dairy` | Herregårdsmejerier | II | 1840 | 1840 | 1000 | 12 | 12000 | `livestock` | Landets skat +4 % |
| `crops` | Kartofler og kornsædskifte | I | 1825 | 1825 | 500 | 8 | 4000 | — | Landets skat +3 % |
| `forestry` | Ordnet skovbrug | II | 1825 | 1825 | 700 | 12 | 8400 | `crops` | Landets skat +2 %; værkernes materielproduktion +5 % |
| `fishing` | Fiskeri og salteri | II | 1830 | 1830 | 600 | 10 | 6000 | `crops` | Byernes skat +2 %; båret proviant +1 dag |
| `machines` | Håndværk og maskinfabrikker | III | 1843 | 1843 | 1500 | 16 | 24000 | `steam` | Geværværksteder, støberier og vognværker +10 % produktion |
| `ironfoundry` | Jernstøberi | II | 1840 | 1840 | 1100 | 12 | 13200 | `smithy` | Værkernes materielproduktion +8 % |
| `textiles` | Mekaniske spinderier | III | 1845 | 1845 | 1200 | 14 | 16800 | `ironfoundry` | Byernes skat +3 % |
| `beettrials` | Forsøgsdyrkning af sukkerroer | III | 1860 | 1860 | 700 | 12 | 8400 | `dairy` | Landets skat +1 %; forsøg, ikke en dansk sukkerindustri |
| `breweries` | Bryggerier og maltning | II | 1847 | 1847 | 1000 | 12 | 12000 | `credit` | Byernes skat +3 % |
| `tradefreedom` | Næringsfrihed | IV | 1857 | 1857 | 1200 | 12 | 14400 | `textiles` | Byernes skat +4 %; administrativ reform uden tvunget politisk begivenhed |
| `customs` | Toldreform | II | 1863 | 1863 | 1300 | 12 | 15600 | `savings` | Byernes skat +3 % (netto af ændrede toldsatser) |
| `savings` | Bankvæsen, sparekasser og forsikring | I | 1825 | 1825 | 600 | 10 | 6000 | — | Byernes skat +2 % |

### Samfund og oplysning

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `schools` | Almueskoler | I | 1825 | 1825 | 500 | 12 | 6000 | — | Indkaldelse +5 % gennem læsekyndighed og registre |
| `civilhospitals` | Hospitaler og lazaretter | I | 1830 | 1830 | 900 | 12 | 10800 | — | Syge soldater vender 10 % hurtigere tilbage |
| `folkhighschool` | Folkehøjskolen | II | 1844 | 1844 | 800 | 12 | 9600 | `schools` | Landets skat +2 %; folkeoplysning og landbrugsviden |
| `statistics` | Statistik og folketælling | II | 1834 | 1834 | 600 | 8 | 4800 | `schools` | Landets og byernes skat +2 %; indkaldelse +5 % |
| `assemblies` | Stænderforsamlinger | III | 1834 | 1834 | 800 | 10 | 8000 | `statistics` | Landets skat +2 %; lokal økonomisk administration |
| `constitution` | Grundlov og Rigsdag | IV | 1849 | 1849 | 1400 | 18 | 25200 | `assemblies` | Byernes skat +3 %; administrativ kapacitet, ændrer ikke automatisk styreform |
| `justice` | Retsvæsen og handelsret | I | 1830 | 1830 | 700 | 10 | 7000 | — | Byernes skat +2 % |
| `tradeschools` | Håndværkerskoler | II | 1843 | 1843 | 900 | 12 | 10800 | `justice` | Værkernes materielproduktion +8 % |
| `choleraprevention` | Koleraforebyggelse | II | 1853 | 1853 | 1000 | 12 | 12000 | `civilhospitals` | Sygdomstilfælde −10 %; koleraens ekstra smitte halveres |
| `firebrigades` | Organiseret brandvæsen | II | 1830 | 1830 | 700 | 10 | 7000 | `justice` | Byernes skat +2 % gennem sikrere handel og værksteder |

## Militære emner

### Sanitet og forsyning

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `sanitation` | Sanitetsvæsenet | I | 1840 | Straks | 800 | 12 | 9600 | — | Ambulancer og feltlazaretter: tab i slag −20 % |
| `conserves` | Konserves og feltbagerier | II | 1835 | Straks | 1000 | 10 | 10000 | `sanitation` | Enhederne bærer 2 dages proviant mere (6 i stedet for 4) |
| `hospitals` | Militærhospitaler | III | 1845 | Straks | 1200 | 12 | 14400 | `conserves` | Syge og sårede kommer 40 % hurtigere tilbage |

### Befæstning

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `fortress` | Fæstningsbyggeri | I | 1830 | Straks | 1200 | 12 | 14400 | — | Ingeniørkorpsets skole: skansernes dækning +10 %-point |
| `casemates` | Kasematter og blendinger | II | 1840 | Straks | 2000 | 15 | 30000 | `fortress` | Skansernes dækning yderligere +10 %-point |

### Samfærdsel

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `railmob` | Jernbanemobilisering | I | 1850 | Straks | 1500 | 12 | 18000 | `telegraph` | Køreplaner for indkaldelsen: yderligere 25 % hurtigere |
| `pontoon` | Pontonnerkorpset | II | 1830 | Straks | 1500 | 12 | 18000 | `railmob` | Pontonbroer koster 40 % mindre og lægges på halv tid. I slaget: pionererne kan slå en bro over en å |

### Infanteriet

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `breech` | Bagladegeværet | III | 1860 | Straks | 4000 | 24 | 96000 | `minie` | Åbner betalt ombygning til bagladegeværer på enhedskortet; ladetid × 0,35 efter ombygningen |
| `square` | Karré-eksercitsen | I | 1826 | Straks | 600 | 8 | 4800 | — | Kompagniet danner karré på 30 % kortere tid, og hver side skyder 30 % i stedet for 25 %. Kampværdi +2 % |
| `skirmish` | Kædelinjer og jægertaktik | II | 1830 | Straks | 1000 | 12 | 12000 | `square` | Spredt orden i kornet og bag hegnene: dækningen der × 1,25, skyttekamp +15 %, tab −10 % |
| `tworank` | To-geleds ild | I | 1826 | Straks | 500 | 6 | 3000 | — | De to forreste geledder skyder sammen. Skal derefter indøves i regimenterne (eksercits/skydeøvelser) |
| `firebyrank` | Geledild | II | 1828 | Straks | 800 | 8 | 6400 | `tworank` | Geledderne skyder på skift, så ilden aldrig hører op. Skal indøves i regimenterne |
| `volley` | Kommanderet salve | III | 1830 | Straks | 900 | 8 | 7200 | `firebyrank` | Hele kompagniet på kommando: den tunge salve, der ryster fjenden. Skal indøves i regimenterne |
| `independent` | Fri ild | IV | 1840 | Straks | 1000 | 10 | 10000 | `volley` | Hver mand skyder, når han har ladt og sigtet: hurtigere ild, svagere salver. Skal indøves i regimenterne |
| `percussion` | Perkussionslås | I | 1830 | Kendt | 900 | 8 | 7200 | — | Åbner betalt ombygning af flintlåsvåben på enhedskortet |
| `minie` | Minié-riffel | II | 1849 | Straks | 2000 | 12 | 24000 | `percussion` | Åbner betalt ombygning til riflede håndvåben med længere rækkevidde |
| `column` | Kolonneeksercits | I | 1825 | 1825 | 600 | 8 | 4800 | — | Kampværdi +2 % ved bajonetdoktrin eller angrebskolonne; kolonnen er en eksisterende formation |

### Artilleriet

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `riflegun` | Riflede kanoner | I | 1855 | Straks | 3000 | 18 | 54000 | — | Åbner betalt ombygning af batterier til riflede kanoner |

### Kavaleriet

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `recon` | Rytterspejdning | I | 1826 | Straks | 700 | 8 | 5600 | — | Rytteriet ser 50 % længere på kortet. I slaget: ordren SPEJD HER, når fjenden ikke længere ses overalt |
| `carbine` | Dragonernes ildkamp | II | 1835 | Straks | 1200 | 12 | 14400 | `recon` | Afsiddede dragoner skyder til 45/90/130 m i stedet for 35/70/100 m og lader på 5 s i stedet for 7. Kampværdi +2 % med rytteri |
| `shock` | Rytterchokket | III | 1830 | Straks | 1500 | 14 | 21000 | `carbine` | Rytteriet reformerer 25 % hurtigere, og et angreb i flanke eller ryg ryster 20 % mere. Kampværdi +3 % med rytteri |
| `remount` | Remonte og hestepleje | II | 1830 | 1830 | 800 | 10 | 8000 | `recon` | Stutterier og remontedepoter +20 % heste |

### Kommando

| Id | Emne | Niveau | Åbner 1825-spil | Åbner 1851-spil | Rd./md. | Måneder | Total rd. | Kræver | Effekt |
|---|---|---|---:|---|---:|---:|---:|---|---|
| `staff` | Stabsskolen | I | 1830 | Straks | 1000 | 18 | 18000 | — | Uddannede stabsofficerer: kampværdi +5 %. I slaget: kommandozonerne 15 % større |
| `genstaff` | Generalstaben | II | 1845 | Straks | 2000 | 18 | 36000 | `staff` | Brigade- og divisionsstabe: ordrer udføres 25 % hurtigere, kommandozonerne yderligere 15 % større, +5 % i slag med 3 enheder eller flere |

## Effektkoblinger

- `RuralTaxFactor` og `UrbanTaxFactor` indgår i `TaxPerYear`: indkomstbonusser gælder den relevante befolkning i ikke-besatte amter. Procenter lægges sammen i hver faktor; de er produktivitets- og administrationsbonusser, ikke ændrede politiske skattesatser.
- `WorksOutputFactor` indgår i `MonthlyMateriel` efter råvarebegrænsningen: geværværksteder, arsenaler, kanonstøberier og vognværker. Nye produktionsbonusser lægges sammen, derefter multipliceres de med smede/damp. Ingen gratis materiel eller ny fabriksbygning.
- Kvægavl/remonte lægges sammen (+10/+20 %) og påvirker kun heste fra færdige `Stud_Farm` og `Remount_Depot`; ikke opkøb eller startlager.
- `FoodCap` giver fiskeriets ekstra proviantdag gennem den eksisterende forsyningsmekanik.
- `CallInFactor` giver civil transport og uddannelse hurtigere indkaldelse. Nye bonusser lægges sammen og multipliceres med eksisterende telegraf/jernbanemobilisering og stemning. Der loves ingen ændring af enhedernes marchhastighed eller automatisk transport over vand.
- `DailyHealth` giver civile hospitaler ×1,10 tilbagekomst, multipliceret med militærhospitaler. Koleraforebyggelse giver ×0,90 sygdom og ændrer kolerafaktoren fra 10 til 5,5: den ekstra smitte over normalniveau halveres. Enhedskortets tilbagekomstoversigt bruger samme faktor.
- `DanishQualityFactor` giver kolonneeksercits ×1,02 med bajonetdoktrin eller angrebskolonne. Eksisterende våbenemner åbner betalt ombygning pr. enhed; færdig forskning uddeler ikke gratis våben.

## Doktriner

Eksisterende strategiske og operative valg samt taktiske indeks 0–2 bevares. Skift benytter den eksisterende pris og 60 dages omstilling, moral −0,05 (minimum 0,30) og kampværdi ×0,90 under omstillingen.

| Niveau | Valg | Effekt |
|---|---|---|
| Strategisk | Fæstningen | Skanser +10 procentpoint dækning; ×0,95 kampværdi uden skanser |
| Strategisk | Felthæren | ×1,05 kampværdi uden skanser |
| Operativ | Koncentration | ×1,08 med mindst tre enheder, ellers ×0,95 |
| Operativ | Forsvar i dybden | Tab ×0,80; kampværdi ×0,97 |
| Taktisk | Ildkamp | Kampværdi ×1,10 ved skanser |
| Taktisk | Bajonetangreb | Kampværdi ×1,10, mod Preussen med tændnålsgevær ×0,85 |
| Taktisk | Spredt orden (kædelinje) | Kampværdi ×1,03; tab ×0,85; 3D-skyttekamp ×1,10 og åben orden |
| Taktisk | Karréforsvar (nyt indeks 3) | Kræver `square`; kampværdi ×1,02; tab ×0,95 |
| Taktisk | Angrebskolonne (nyt indeks 4) | Kræver `column`; kampværdi ×1,05 uden skanser, ×0,95 ved skanser; offensiv AI, nærild og frit angreb i 3D |

Karré og kolonne er eksisterende 3D-formationer. De nye valg er doktrinbonusser, ikke tvungne formationer. Karréforsvarets tabsfaktor eksporteres i eksisterende `battleRules`; kampværdi beregnes i kampagnens autoresolution. Perkussion, Minié, bagladere, riflede kanoner, felttelegraf, feltsanitet og kædelinjer var allerede implementeret og er ikke duplikeret. Felttelegrafens eksisterende civile sporklassifikation er bevaret.

## Layout og statisk kontrol

Tre lige brede civile grene, kort på 68 px og rækketrin på 82 px. Emner står lodret inden for niveauet; musehjul og OP/NED ruller én række. Kun fuldt synlige kort får klikfelter. Niveauet står på hvert kort, og detaljepanelet viser forudsætning og fuld effekt.

| Opløsning | Vindue | Civil kortbredde | Synlige rækker | Maks. rulning (rækker) |
|---|---|---:|---:|---:|
| 1600×900 | 1560×760 | 456.0 px | 6 | 13 |
| 1920×1080 | 1860×930 | 556.0 px | 8 | 11 |

Kontrollen beregner geometrien ved Slate-skala 1 og alle rulningspositioner: ingen overlappende kort eller kort uden for indholdsområdet. Tekst bruger eksisterende `PaintTextFit`; dialogtitlen tilpasses også. Dette er statisk kontrol, ikke en visuel spiltest. Spil/editor er ikke startet.

## Gemte spil

Version 31 tilføjer `civil|id|måneder` til `UCampaign1851SaveGame::Research`. Gamle `state`- og `done`-linjer læses stadig; en gammel gemning uden civil-linje starter uden aktiv civil forskning. Alle 31 eksisterende id’er og indeks er bevaret, nye emner er tilføjet sidst. Doktrinindeks 0–2 bevarer deres betydning; nye 3–4 tilføjes sidst. Effekterne afledes af færdige forsknings-id’er og kræver ikke nye bonusfelter.
