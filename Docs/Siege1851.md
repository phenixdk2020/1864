# De historiske fæstningsværker og belejring (backlog 13 og 14)

## Værkerne
| Værk | Historisk dato | Front | Skanser |
|---|---|---|---|
| Dannevirkestillingen | marts 1861 | Syd | 8 (4 store), fra Hollingsted til Slien |
| Dybbølstillingen | juni 1861 | Vest | 7 (4 store), fra Vemmingbund til Alssund |
| Fredericias volde | maj 1862 | Vest-sydvest | 3 store |

- **Datoen** flyttes op til ±2 år × afvigelsen. Med chancen afvigelsen × 0,3 bliver værket aldrig foreslået.
- **Krigsministeriet på AUTO** bygger værket, når kassen kan betale det og stadig holde reserven.
- **Ellers** kommer værket som et forslag i Statsrådet, og spilleren trykker UDFØR.
- **Skanserne** placeres, hvor terrænet tillader det. Positionerne er omtrentlige.

## Belejring
1. **Valg af belejring:** hvis planens første mål har skanser, og korpset tror, det har 0,6–1,3 gange forsvarets styrke, vælger det at belejre i stedet for at storme.
2. **Korpset graver sig ned:** det marcherer frem til 9 km fra byen og graver sig ned. Der sker intet slag med skanserne eller de belejrede (inden for 10 km), men en undsætning udefra kan angribe.
3. **Hver dag under belejringen:**
   - skanserne mister 6 skud pr. kanon og 0,3 dags proviant;
   - hver 10. dag falder forsvaret ét trin;
   - de belejrede mister 0,2 % af mandskabet (halvdelen er sårede).
4. **Storm:** korpset stormer, når en skanse er skudt i stykker (forsvar 1), når det er blevet stærkt nok (1,3 gange forsvaret), eller efter 35 dage. Så kommer slagpanelet.

Gemning: `siege|korps|by|belejrer|startdag` i War-linjerne.

Test: `-CampaignWorks=0,1,2 -CampaignFortsComplete`.
