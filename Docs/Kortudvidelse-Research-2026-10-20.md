# Kortudvidelse: research til Sverige, Norge og Preussen (2026-10-20)

Data fra agenten `research`. Punkt 1–4 er skrevet fra hukommelsen, uden at kilderne er åbnet, så ALLE tal er skøn, indtil de er kontrolleret. Kun punkt 5 (højdedata) er hentet live. I data bruges `populationStatus: "skøn"`, indtil et tal er verificeret mod kilden. Koordinater er ca. ±0,03°.

## Kilder til kontrol
- **Sverige:** *Historisk statistik för Sverige, del 1 Befolkning 1720–1967*, SCB 1969; Tabellverket via https://www.scb.se/hitta-statistik/historisk-statistik; *Sveriges administrativa indelning*, Riksarkivet.
- **Norge:** SSB historisk befolkningsstatistik (folketællinger 1825, 1835, 1845, 1855), https://www.ssb.no; Digitalarkivet, https://www.digitalarkivet.no.
- **Preussen:** Dieterici, *Die Bevölkerung der preussischen Monarchie* (Preußisches Statistisches Bureau, 1840'erne–50'erne), via https://gdz.sub.uni-goettingen.de; GenWiki, https://wiki.genealogy.net.
- **Hær:** Curt Jany, *Geschichte der preußischen Armee* (1928–33).

## 1. Distrikter (skøn)
**Totaler:**
- Sverige: 2,77 mio. (1825), 3,48 mio. (1850).
- Norge: 1,05 mio. (1825), 1,33 mio. (1845), 1,49 mio. (1855).
- Preussen: ca. 12,3 mio. (1825), ca. 16,3 mio. (1849).

### Sverige: län | sæde | pop 1825 | pop 1850
```
Stockholms stad|Stockholm|76000|93000
Stockholms|Stockholm|110000|135000
Uppsala|Uppsala|80000|98000
Södermanland|Nyköping|120000|145000
Östergötland|Linköping|180000|215000
Jönköping|Jönköping|135000|160000
Kronoberg|Växjö|110000|130000
Kalmar|Kalmar|165000|190000
Gotland|Visby|35000|43000
Blekinge|Karlskrona|85000|105000
Kristianstad|Kristianstad|140000|180000
Malmöhus|Malmö|190000|250000
Halland|Halmstad|100000|120000
Göteborg och Bohus|Göteborg|150000|195000
Älvsborg|Vänersborg|210000|250000
Skaraborg|Mariestad|170000|200000
Värmland|Karlstad|190000|250000
Örebro|Örebro|130000|165000
Västmanland|Västerås|100000|120000
Kopparberg|Falun|135000|165000
Gävleborg|Gävle|110000|145000
Västernorrland|Härnösand|105000|140000
Jämtland|Östersund|50000|65000
Västerbotten|Umeå|75000|95000
Norrbotten|Luleå|55000|75000
```

### Norge: amter (amtsæde, skøn; befolkning pr. amt skal hentes fra SSB)
Smaalenene (Moss), Akershus (Christiania), Hedemarken (Hamar/Elverum), Christians (Lillehammer), Buskerud (Kongsberg/Drammen), Jarlsberg og Larvik (Tønsberg), Bratsberg (Skien), Nedenes (Arendal), Lister og Mandal (Kristiansand), Stavanger, Søndre Bergenhus (Bergen), Nordre Bergenhus (Fjaler/Sogn), Romsdal (Molde), Søndre Trondhjem (Trondheim), Nordre Trondhjem (Steinkjer), Nordland (Bodø), Tromsø, Finnmarken (Vardø).

### Preussen: provins | hovedstad | Regierungsbezirke
```
Ostpreußen|Königsberg|Königsberg;Gumbinnen
Westpreußen|Danzig|Danzig;Marienwerder
Brandenburg|Potsdam|Potsdam;Frankfurt/Oder (Berlin eget politiområde)
Pommern|Stettin|Stettin;Köslin;Stralsund
Posen|Posen|Posen;Bromberg
Schlesien|Breslau|Breslau;Liegnitz;Oppeln
Sachsen|Magdeburg|Magdeburg;Merseburg;Erfurt
Westfalen|Münster|Münster;Minden;Arnsberg
Rheinprovinz|Koblenz|Koblenz;Düsseldorf;Köln;Trier;Aachen
Hohenzollern|Sigmaringen|kun 1851 (preussisk fra 1850)
```
- **Provinsen Preussen:** Ost- og Westpreußen blev slået sammen 1829–1878. Scenariet 1825 har to provinser, 1851 én provins med de samme fire Regierungsbezirke.
- **Rhinprovinsen og Westfalen** er adskilt fra kerneområdet; Hannover, Braunschweig og Hessen-Kassel ligger imellem.

## 2. Byer (tusinder, ca. 1825 / ca. 1850, skøn): by | lat | lon | p1825 | p1850 | distrikt
### Sverige
```
Stockholm|59.33|18.07|76|93|Stockholm
Göteborg|57.71|11.97|26|26|Göteborg o. Bohus
Malmö|55.60|13.00|8|13|Malmöhus
Norrköping|58.59|16.19|10|15|Östergötland
Karlskrona|56.16|15.59|10|11|Blekinge
Gävle|60.67|17.14|5|9|Gävleborg
Uppsala|59.86|17.64|4|6|Uppsala
Örebro|59.27|15.21|4|7|Örebro
Jönköping|57.78|14.16|5|6|Jönköping
Lund|55.70|13.19|4|6|Malmöhus
Kristianstad|56.03|14.16|4|6|Kristianstad
Linköping|58.41|15.62|4|5|Östergötland
Karlstad|59.40|13.50|3|5|Värmland
Västerås|59.61|16.55|3|4|Västmanland
Falun|60.61|15.63|4|4|Kopparberg
Kalmar|56.66|16.36|4|5|Kalmar
Visby|57.64|18.29|3|4|Gotland
Landskrona|55.87|12.83|3|4|Malmöhus
Ystad|55.43|13.82|3|4|Malmöhus
Uddevalla|58.35|11.94|3|4|Göteborg o. Bohus
Helsingborg|56.05|12.69|2|4|Malmöhus
Halmstad|56.67|12.86|3|4|Halland
Nyköping|58.75|17.01|3|4|Södermanland
Eskilstuna|59.37|16.51|2|4|Södermanland
Borås|57.72|12.94|2|4|Älvsborg
Härnösand|62.63|17.94|2|3|Västernorrland
Sundsvall|62.39|17.31|1|2|Västernorrland
Umeå|63.83|20.26|1|2|Västerbotten
Luleå|65.58|22.15|1|1|Norrbotten
Östersund|63.18|14.64|0|1|Jämtland
Växjö|56.88|14.81|2|3|Kronoberg
```
### Norge
```
Christiania|59.91|10.75|24|29|Akershus
Bergen|60.39|5.32|19|25|S. Bergenhus
Trondhjem|63.43|10.39|10|14|S. Trondhjem
Kristiansand|58.15|8.00|6|9|Lister o. Mandal
Drammen|59.74|10.20|6|10|Buskerud
Stavanger|58.97|5.73|4|9|Stavanger
Fredrikshald|59.12|11.39|4|5|Smaalenene
Arendal|58.46|8.77|4|5|Nedenes
Larvik|59.05|10.03|3|4|Jarlsberg o. Larvik
Skien|59.21|9.61|3|4|Bratsberg
Kongsberg|59.67|9.65|5|4|Buskerud
Kristiansund|63.11|7.73|3|4|Romsdal
Moss|59.43|10.66|2|3|Smaalenene
Fredrikstad|59.21|10.94|2|3|Smaalenene
Tønsberg|59.27|10.41|2|3|Jarlsberg o. Larvik
Porsgrunn|59.14|9.66|2|3|Bratsberg
Molde|62.74|7.16|1|2|Romsdal
Ålesund|62.47|6.15|1|2|Romsdal
Tromsø|69.65|18.96|1|2|Tromsø
Hammerfest|70.66|23.68|0|1|Finnmarken
Vardø|70.37|31.11|0|1|Finnmarken
Bodø|67.28|14.40|0|1|Nordland
Hamar|60.79|11.07|0|1|Hedemarken
Lillehammer|61.12|10.47|0|1|Christians
Haugesund|59.41|5.27|0|1|Stavanger
Risør|58.72|9.23|2|2|Nedenes
Grimstad|58.34|8.59|1|2|Nedenes
Mandal|58.03|7.46|2|2|Lister o. Mandal
Flekkefjord|58.30|6.66|1|2|Lister o. Mandal
```
### Preussen
```
Berlin|52.52|13.40|220|420|Potsdam/Berlin
Breslau|51.11|17.03|90|110|Breslau
Köln|50.94|6.96|60|95|Köln
Königsberg|54.71|20.51|65|80|Königsberg
Danzig|54.35|18.65|55|65|Danzig
Magdeburg|52.13|11.63|40|55|Magdeburg
Stettin|53.43|14.55|27|50|Stettin
Aachen|50.78|6.08|33|50|Aachen
Elberfeld|51.26|7.15|30|45|Düsseldorf
Barmen|51.27|7.20|25|40|Düsseldorf
Düsseldorf|51.23|6.78|25|35|Düsseldorf
Posen|52.41|16.93|30|40|Posen
Potsdam|52.40|13.06|28|40|Potsdam
Halle|51.48|11.97|25|35|Merseburg
Erfurt|50.98|11.03|22|30|Erfurt
Frankfurt/Oder|52.35|14.55|20|30|Frankfurt
Koblenz|50.36|7.59|15|25|Koblenz
Münster|51.96|7.63|17|25|Münster
Trier|49.76|6.64|12|17|Trier
Krefeld|51.34|6.56|8|40|Düsseldorf
Bonn|50.73|7.10|11|16|Köln
Görlitz|51.15|14.99|12|18|Liegnitz
Liegnitz|51.21|16.16|12|15|Liegnitz
Stralsund|54.31|13.08|13|20|Stralsund
Tilsit|55.07|21.88|10|15|Gumbinnen
Memel|55.71|21.14|8|13|Königsberg
Bromberg|53.12|18.01|6|13|Bromberg
Essen|51.46|7.01|5|10|Düsseldorf
Dortmund|51.51|7.47|4|10|Arnsberg
Minden|52.29|8.92|9|11|Minden
Wesel|51.66|6.62|10|13|Düsseldorf
```
Krefelds tal for 1850 er et meget groft skøn.

## 3. Militær (skøn)
- **Preussen:**
  - Korps-HQ: Garde Berlin/Potsdam; I Königsberg; II Stettin; III Berlin/Frankfurt (Oder); IV Magdeburg; V Posen; VI Breslau; VII Münster; VIII Koblenz.
  - Fredsstyrke: ca. 120–130 tusind (1825), ca. 130–140 tusind (1850).
  - Fæstninger: Magdeburg, Stettin, Danzig, Königsberg, Küstrin, Glogau, Neisse, Koblenz/Ehrenbreitstein, Wesel, Köln, Saarlouis, Posen (fra 1828), Graudenz, Kolberg, Torgau, Spandau, Pillau, Silberberg.
- **Sverige:**
  - Fæstninger: Karlsborg (58.53, 14.51; påbegyndt 1819), Vaxholm (59.40, 18.33), Karlskrona (flådebase), Göteborg/Nya Älvsborg (57.69, 11.87), Carlsten/Marstrand (57.89, 11.58), Landskrona, Kristianstad, Stockholm (livgarder).
  - Styrke: indelta ca. 30–35 tusind og værvede ca. 5–8 tusind. Indelningsverket betyder regionalt fordelte regimenter, ikke faste garnisoner.
- **Norge:**
  - Fæstninger: Fredriksten (59.12, 11.39), Kongsvinger (60.19, 12.00), Akershus (59.91, 10.74), Fredrikstad, Oscarsborg (59.68, 10.60; bygget i 1840'erne, ikke i 1825), Fredriksvern/Stavern (flåde), Kristiansten (Trondheim), Bergenhus, Vardøhus.
  - Styrke: ca. 12 tusind linjetropper plus landvern.

## 4. Nabolande inden for udsnittet: navn | hovedstad | befolkning ca. 1850 i mio. (skøn)
```
Kongeriget Hannover|Hannover|1.8
Mecklenburg-Schwerin|Schwerin|0.54
Mecklenburg-Strelitz|Neustrelitz|0.10
Oldenburg|Oldenburg|0.28
Braunschweig|Braunschweig|0.27
Nederlandene|Amsterdam/Haag|3.1
Belgien|Bruxelles|4.4
Luxembourg|Luxembourg|0.19
Kongeriget Sachsen|Dresden|1.9
Sachsen-Weimar|Weimar|0.26
Hessen-Kassel|Kassel|0.75
Hessen-Darmstadt|Darmstadt|0.85
Nassau|Wiesbaden|0.43
Bayern (kant)|München|4.5
Østrig/Böhmen|Prag|4.4
Kongrespolen|Warszawa|4.8
Storfyrstendømmet Finland|Helsingfors|1.6
Rusland (Baltikum)|Skt. Petersborg|2.0
```

## 5. Højdedata (hentet live)
- **ETOPO 2022 15 Arc-Second Global Relief Model**, NOAA NCEI, 2022, DOI 10.25921/fd45-gt74, https://www.ncei.noaa.gov/products/etopo-global-relief-model.
- **Udgave:** Ice Surface. Bibliotek: `https://www.ngdc.noaa.gov/mgg/global/relief/ETOPO2022/data/15s/15s_surface_elev_gtif/`. Filmønster: `ETOPO_2022_v1_15s_N60E015_surface.tif`.
- **Seks 15°-fliser:** N60E000 (32 MB), N60E015 (35 MB), N60E030 (33 MB), N75E000 (21 MB), N75E015 (27 MB), N75E030 (19 MB). I alt ca. 167 MB.
  - At navnet angiver nordvesthjørnet (N60 = lat 45–60) er en slutning, ikke kontrolleret. Kontrollér udstrækningen i den første fil.
- **Licens:** står ikke på siden. NOAA-data er normalt frie (amerikansk statsværk), men det er ikke verificeret.
- **Alternativ:** Copernicus DEM GLO-90 (90 m, AWS Open Data `copernicus-dem-90m`, ESA-vilkår). Mere detaljeret, men kræver mange 1°-fliser.
