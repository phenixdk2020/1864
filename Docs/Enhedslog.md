# Enhedslog (2026-10-13)

Hver enhed fører en log over, hvad der sker med den, og hvad der udløste det. Åbn den med knappen **LOG** øverst til højre i enhedspanelet (nederst på skærmen); den viser den valgte enheds seneste 18 hændelser, nyeste nederst: `mm:ss [Kategori] Hvad - Hvorfor`.

## Hvad der logges

| Kategori | Hændelse | Hvorfor står der |
|---|---|---|
| Ordre | ordre modtaget (FLYT, ANGRIB, STOP ...) | hvem gav den (spilleren, officerens AI, arvet), afstand til målet, vejpunkter |
| Bevægelse | march påbegyndt, nået målet, marchen standset | afstand; standsning har sin årsag i linjerne omkring |
| Formation | fx `linje -> kolonne`, `kolonne -> linje`, `linje -> karré` | årsagen: marchordre over X m, fjenden Y m væk inden for udfoldningsgrænsen, kavaleritrussel, stop-og-ild, spillerens knap, marchen slut |
| Kavalerireaktion / Kavalerikandidat | karré, eller hvorfor der IKKE blev karré (fx "for lav fart", "angrebskorridoren peger et andet sted hen") | tærsklen eller reglen |
| Stop-og-ild | marchen suspenderet under ild, og hvornår den genoptages | ro-tid og regler |
| Ild | salve nr. N afgivet | skud, ammunition tilbage |
| Ammunition | ved under 20 % tilbage | |
| Tab | mænd ramt | antal, årsag, ny styrke |
| Moral | opløsning, rystet af nabo, samlet (eget, kommandocirkel, rally-kald) | moral/samhold mod grænser |
| Stilling | står / knæler / ligger | |
| Feltofficer, Ilddækning, Flankeplan, Reserve, Charge-kandidat ... | officerens beslutninger | reglen |

Rutinemæssige vurderinger ("Vurder overordnet mission", observationer) og gentagelser inden for 20 s (45 s for kavaleri) udelades. Loggen gemmer højst 120 poster pr. enhed.

## Andet

- Efter slaget skrives alle enheders logge i rapportfilen (`Saved/Reports/battle_*.txt`, afsnittet `=== ENHEDSLOG ===`).
- Testflag: `-Strategy1864ShowUnitLog` åbner loggen fra start. `-Strategy1864DebugDecisions` skriver desuden den lange beslutningslinje (`PROJECT1864-DECISION|...`) til konsollen.
- Hvis en forventet handling ikke står i loggen (fx karré mod kavaleri), så leder man efter den nærmeste "afvist"-linje under Kavalerikandidat eller Kavalerireaktion; mangler den også, er situationen aldrig blevet vurderet.
