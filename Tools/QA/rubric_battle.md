# Tjekliste til video-QA af 3D-slagene

Bruges af alle tre bedømmere (Ollama, Codex, Copilot) via `Tools/QA/battle_video_qa.py`. Hvert punkt har et id, som fundene skal bruge i feltet `kind`. Grundlaget er eksercerreglementet 1863 (straks opslutning, løbende sammenrykning mod midten eller fanen, omordning i pauser) og de fejltyper, der er rettet i slagene 2026-10-20..23 (se `Docs/IMPLEMENTATION-AND-FIX-HISTORY.md`).

Billederne er enten **nøglebilleder** (ét pr. sekund) eller **bevægelsesstriber** (6 billeder med 0,2 s mellemrum, læses venstre→højre, øverst→nederst). Bevægelsesfejl (`foot_slide`, `turn`, `step_up`) kan kun ses på striber.

| id | Hvad der er forkert | Hvordan det ser ud |
|---|---|---|
| `overlap` | To mænd oven i eller gennem hinanden | Kroppe smelter sammen, to hoveder i samme figur, en mand går igennem en anden. Mindste naboafstand under 40 cm i målingerne. |
| `rifle_hand` | Geværet sidder ikke i hånden | Geværet svæver ved siden af hånden, stikker gennem kroppen eller sidder forkert på skulderen under march eller sideskridt. |
| `flag_hand` | Fanen sidder ikke i fanebærerens hånd | Stangen svæver, hånden griber i luften, den faldne fane ligger et andet sted end der, hvor manden bukker sig. |
| `bearer` | Fanebæreren står forkert | I kolonne skal han gå ca. 1,8 m foran forreste række, ikke inde i den og ikke langt væk. I linje står han ved siden af højre fløj, på linje med forreste række og én rotte ude (projektets valg 2026-10-20, ikke en fejl). Han må ikke være over to rækker fra nærmeste mand. I karré står han i midten med officeren ved siden af (Allan 2026-11-05); her er afstanden til mændene stor, og det er rigtigt, ikke en fejl. Står han ved et hjørne eller i karréens side, er det en fejl. |
| `ground` | Figurer svæver eller står i jorden | Fødder over terrænet, ben skåret af ved jorden, døde der ligger i luften. |
| `ranks` | Uordnede rækker og huller | Ujævne rækker, store huller der ikke lukkes, en "dyb blok" i stedet for to rækker. |
| `column` | Kolonnen har forkert form | Rækkerne er ikke parallelle, bagerste række hænger efter, kolonnen bliver bredere eller smallere end sit antal rotter. |
| `step_up` | Opslutning efter tab virker ikke | Når en mand falder i forreste række, skal manden bag ham træde frem straks (reglementet). Hul bliver stående, eller to mænd træder frem. |
| `closing` | Sammenrykning mod midten/fanen virker ikke | Huller i linjen lukkes ikke inden for nogle sekunder, eller mændene trækker sammen i en klump. |
| `death_pose` | Døde mænd ligger forkert | Faldet passer ikke til bevægelsen (march-død mens han går med gevær på skulderen), lig der glider, står op eller ligger oven i hinanden. |
| `volley` | Salver er ikke samtidige | Rækken skyder spredt, når der er givet salve; mænd sigter i forskellige retninger; skud uden røg. |
| `reload` | Ladning ser forkert ud | Ladestokken mangler eller går gennem kroppen; mænd lader mens de går; ladning tager urealistisk kort tid. |
| `smoke` | Krudtrøg forkert | Ingen røg ved skud, røg der hopper eller forsvinder brat, røg der dækker alt uden at drive. |
| `foot_slide` | Fødderne glider | Gangen passer ikke til farten; figuren glider fremad med stive ben eller går på stedet. |
| `turn` | Drejning ser forkert ud | Mænd drejer på stedet som en snurretop, kolonnen drejer som en stiv plade, bagerste mænd skærer hjørnet. |
| `other` | Andet urealistisk | Alt andet, der bryder illusionen (forkert uniformsfarve, figurer der blinker, kamera inde i en figur). |

## Alvor

- **3**: ses straks af spilleren og ødelægger billedet (fx `overlap`, svævende gevær tæt på kameraet).
- **2**: ses ved nærmere eftersyn (fx ujævn række, fanebærer lidt for tæt).
- **1**: småting.

## Regler for bedømmerne

- Rapportér kun det, der faktisk kan ses på billedet. Hvert fund skal have billedets navn og et sted i billedet (fx "forreste række, venstre fløj").
- Skriv beskrivelsen på dansk.
- Sæt `confidence` ærligt mellem 0 og 1. Er du i tvivl, så lav sikkerheden lav i stedet for at udelade fundet.
- Ingen fund er et gyldigt svar.
