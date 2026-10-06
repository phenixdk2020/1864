# PROJECT 1864 — Designmanual v00.02.08 — Strategisk landudvikling

## 20.16 Udvikling på strategikortet

Strategikortet skal ikke kun være et sted, hvor hære flyttes. Landet skal kunne udvikles økonomisk, logistisk og militært gennem langsigtede investeringer. Udvikling sker primært på **regionalt/by-niveau** med tydelige projekter, kapaciteter og forbindelser frem for fri city-builder-placering af hundredvis af bygninger.

> **KERNEPRINCIP:** Spilleren vælger retning og prioritet. Befolkning, arbejdskraft, kapital, råstoffer, transport og byggetid bestemmer, hvor hurtigt resultatet faktisk opstår.

### 20.16.1 Infrastruktur: veje, broer, jernbane og telegraf

Infrastruktur skal være synlig på strategikortet og have direkte militær betydning.

| Infrastruktur | Strategisk effekt |
| --- | --- |
| Lokal vej / jordvej | Basal civil og militær bevægelse; stærkt påvirket af regn, mudder og trafik. |
| Forbedret hovedvej | Hurtigere march, højere supply-throughput og mindre congestion/slitage. |
| Bro | Gør floder og vandløb til konkrete chokepoints; kan forstærkes, ødelægges og repareres. |
| Jernbane | Meget høj transportkapacitet for tropper, ammunition, fødevarer og industri mellem betjente knudepunkter. |
| Jernbanestation/terminal | Lastning, aflæsning, lager og kapacitetsbegrænsning; en jernbane uden terminalkapacitet er ikke uendelig throughput. |
| Telegraf | Hurtigere strategiske ordrer og rapporter mellem forbundne stationer/HQ'er. |
| Havn | Import/eksport, søtransport, flådeforsyning og tungt materiel. |
| Depot/lager | Buffer mellem produktion/transport og felthær; kan erobres eller ødelægges. |

Veje og jernbaner er **forbindelser**, ikke generelle regionsbuffs. Et regiment får kun fordelen, hvis det faktisk bruger infrastrukturen. Flaskehalse, ødelagte broer, stationer med for lav kapacitet og fjendtlig interdiction kan bryde et ellers stærkt netværk.

Udbygning skal ske i niveauer eller konkrete projekter, fx `Forbedr Aalborg–Randers hovedvej`, `Udvid stationskapacitet`, `Byg bro`, `Reparer jernbane`. Projektet har cost, materialebehov, arbejdskraft og byggetid.

### 20.16.2 Gårde, fødevarer og foder

Landbrug er den brede produktionsbase. En region har landbrugsareal, jordkvalitet, arbejdskraft, sæson, lagerkapacitet og adgang til marked/transport.

- Gårde producerer korn/fødevarer og foder.
- Høst er sæsonafhængig; mobilisering eller kamp i en region før/under høst kan reducere output.
- Hære kan købe eller rekvirere lokale forsyninger, men overdreven rekvisition reducerer senere civil produktion og lokal loyalitet/stabilitet.
- Dårlige veje kan skabe lokal fødevaremangel, selv når national produktion er tilstrækkelig.
- Lader/magasiner og regionale kornlagre kan øge buffer og reducere spild.
- Slag, plyndring, nedbrænding eller længerevarende besættelse kan skade gårde og lagerkapacitet.

Spilleren skal normalt investere i **regional landbrugskapacitet, lager, dræning/forbedringer og transport**, ikke placere hver enkelt bondegård manuelt.

### 20.16.3 Heste, stutterier og remount-system

Heste er en selvstændig strategisk ressource, fordi kavaleriet, artilleriet, ambulancerne og store dele af forsyningstjenesten er afhængige af dem.

Der skelnes mindst mellem:

- **ride-/kavaleriheste**,
- **artilleri-/trækheste**,
- øvrige transport-/arbejdsheste,
- kvalitet/fitness og eventuelt alder som aggregeret state.

Hesteforsyning består af flere lag:

1. eksisterende civil og militær hestebestand,
2. remount-depoter og militære reserver,
3. køb på hjemmemarkedet,
4. rekvisition i kontrollerede områder,
5. udenlandsk import,
6. langsigtet avl via stutterier/hesteopdræt.

Et **stutteri/hesteavlsprogram** kan forbedre fremtidig mængde og kvalitet, men skal have lang lead time. I en kort 1864-kampagne kan spilleren derfor ikke bestille 5.000 nye artilleriheste og få dem næste måned. Her bliver remount-depoter, eksisterende besætninger, køb og transport de vigtigste mekanismer. I en længere 1848–1871-kampagne bliver egentlig avl og udvikling af hestebestanden strategisk relevant.

Heste kan gå tabt gennem kamp, sygdom, udmattelse, dårlig fodring og transport. En hær kan derfor have nok mænd og kanoner, men stadig mangle mobilitet.

### 20.16.4 Våbenindustri og arsenaler

Militær industri skal bestå af **specialiserede kapaciteter** frem for én generisk `arms factory`-værdi.

| Kapacitet | Produktion/funktion |
| --- | --- |
| Arsenal/våbendepot | Lager, inspection, distribution, repair og ombygning. |
| Gevær-/våbenværksted | Rifler, musketter, reservedele og våbenreparation. |
| Artilleriværk/foundry | Kanoner, lavetter, metaldele og større reparationsarbejde. |
| Ammunitionsværk | Patroner/projektiler og komponenter efter våbentype. |
| Krudtværk | Sortkrudt; høj strategisk værdi og risiko ved sabotage/brand. |
| Maskinværksted | Tooling, maskindele og industriel kapacitet, som understøtter flere produktionslinjer. |
| Vogn-/sadelmagerkapacitet | Supply-vogne, limbers, hjul, seletøj og reparation. |
| Tekstil/læderproduktion | Uniformer, telte, støvler og seletøj. |

Produktionskæden skal være fysisk forståelig, fx:

`jern + kul + arbejdskraft + maskinkapacitet -> bearbejdede dele -> våben/artilleri -> arsenal -> depot -> enhed`

En fabrik kan kun producere modeller, den har **tooling/design/adoption** til. Omstilling fra én våbentype til en anden tager tid og kan midlertidigt sænke output. Det betyder, at teknologisk viden alene ikke giver en moderne hær.

Reparation, konvertering og nyproduktion konkurrerer om den samme industrielle kapacitet. Erobrede våben kan derfor være værdifulde, men kan også skabe et logistisk problem, hvis de kræver andre reservedele eller ammunition.

### 20.16.5 Andre relevante strategiske udviklinger

Følgende investeringer passer naturligt ind i samme system:

- **miner og råstofudvinding** — jern, kul, træ og building materials,
- **savværker og tømmerkapacitet** — broer, jernbane, vogne og feltarbejder,
- **værksteder/foundries** — civil og militær metalbearbejdning,
- **hospitaler og lazaret-kapacitet** — højere treatment/recovery capacity,
- **militære depoter/magasiner** — ammunition, fødevarer, materiel og reserveheste,
- **kasernen/mobiliseringscentre** — assembly, training og udrustning,
- **officersskoler/stabsskoler** — langsigtet leader development,
- **befæstninger/kystbatterier** — permanente defensive projekter,
- **havneudvidelser** — større handel, transport og naval throughput,
- **telegrafstationer** — command/reporting network,
- **jernbanedepoter/værksteder** — rolling stock, repair og operational railway capacity.

### 20.16.6 Projektmodel og byggetid

Alle større udviklinger bør bruge samme generiske projektmodel:

`Forslag -> finansieret -> materialer/arbejdskraft allokeret -> under bygning -> delvist operationel -> færdig -> vedligehold/reparation`

Et projekt har mindst:

- placering/region eller forbindelse mellem to steder,
- type og niveau,
- byggeomkostning,
- nødvendig arbejdskraft,
- råvarer/materialer,
- eventuelle maskiner/importerede komponenter,
- estimeret byggetid,
- aktuel progress,
- condition,
- maintenance cost,
- output/capacity ved completion.

Krig kan accelerere visse nødløsninger, men ikke ophæve fysikken. En midlertidig bro eller feltdepot kan oprettes hurtigt; et nyt jernværk eller en jernbanelinje tager væsentligt længere tid.

### 20.16.7 Strategisk UI og kortvisualisering

Når en region/by vælges, skal spilleren kunne se **hvad området faktisk kan** og hvorfor.

Eksempel på regionalt panel:

`Nordjylland`

`Befolkning | Arbejdskraft | Fødevarer | Heste | Industri | Lager | Transport`

Under dette vises aktive anlæg/projekter og forbindelser:

- Aalborg arsenal — kapacitet / lager / repair queue,
- regional food production og reserve,
- remount depot / tilgængelige heste,
- vej- og jernbaneforbindelser med throughput,
- aktive byggeprojekter med progress og forventet completion,
- flaskehalse og advarsler.

Strategikortet skal også visuelt ændre sig, når landet udvikles: forbedrede veje/jernbaner, større stationer, nye depoter, fabriks-/byvækst og nye befæstninger bør kunne aflæses uden at åbne et regneark.

### 20.16.8 Spillerens strategiske valg

Systemet skal skabe reelle trade-offs. Eksempler:

- investér i **nye våben** eller reparér/konvertér eksisterende lagre,
- byg **jernbane** til fronten eller forbedr flere billigere hovedveje,
- reserver heste til **artilleri** eller **kavaleri/transport**,
- brug penge på **hærens mobilisering** eller langsigtet industriel kapacitet,
- opbyg **krudt/ammunitionslager** tæt på fronten med højere risiko, eller hold det sikkert længere bagude,
- rekvirér lokale heste/fødevarer nu og accepter senere økonomisk/politisk skade,
- udvid et arsenal eller køb moderne våben i udlandet,
- reparér en strategisk bro hurtigt eller finansier en mere permanent forbedring.

Målet er, at en stærkere hær skal være resultatet af **år med institutioner, industri, landbrug, transport og lagerstyring** — ikke kun af at have nok penge til at trykke på en recruit-knap.
