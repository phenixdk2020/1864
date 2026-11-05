# Åbne opgaver (1864-Campaign)

## I gang
- Fejlrapporten 2026-11-01: kodefejlene er rettet 2026-11-03 (se historikken). Tilbage:
  - bygge og kontrollere København (kirken) og Odense i spillet;
  - garnisonsgrund i Rønne og Altona;
  - kun 1 reserveret grund i Odense.

## Næste
- **Tysk OCR:** tesseract deu eller lokal vision-model, så de østrigske reglementer og Preussen 1847 kan læses; danske reglementer søges manuelt på kb.dk.
- **4 mod 1 og kavaleri (Allan 2026-11-03):** "her vil jeg se samarbejde og at de holder 1 kompagni i reserve og bruger kavaleriet klogt. Og også de rigtige animationer på kavaleriet."
  - Status 2026-11-03: bygget og kørt. Testslag 4: reserve holdt (doktrin) og indsat (hul), husarer chokerede og samlede sig (39 s). Accept: RESERVE-NONE, CAV-FLANK, CAV-SQUARE, CAV-RALLY PASS; RESERVE-HOLD/-COMMIT FAIL i fixturen (kompagnierne får ingen ordre og ser ikke fjenden: "Ingen fjende i syne", order=0).
    - bataljonsplan med reserve efter doktrin (2026-11-03): bataljon med mindst 4 kompagnier holder altid reserve, indsættelse efter styrkeforhold (se Docs/TestFlags.md);
    - eskadronens mission (flankechok, karré, samling, forfølgelse);
    - flag, launcher `Start-Test-4-Bataljon-Rytteri.bat` og seks accept-ID'er (`RESERVE-*`, `CAV-*`), se Docs/TestFlags.md.
  - Tilbage:
    - (løst 2026-11-03: reserven udtages nu under marchen; alle fire RESERVE-/CAV-SQUARE-accepttest PASS)
    - (løst 2026-11-03: chok afbrydes også mod en karré under dannelse med god moral, når eskadronen er over 100 m væk. Test 4: afbrudt på 273 m, 80/80 ryttere ved 200 s. Uden samlingspunkt falder eskadronen tilbage, Copilot-fund, bygget men ikke testet)
    - (løst 2026-11-03: soldater på 13°-bakke inden for ±6 cm af jorden med egen cache til mændenes mål, `Battlefield_Bakke.json`); ydeevne ikke målt;
    - (kontrolleret: fanebæreren projiceres selv på terrænet)
    - (løst 2026-11-03: Codex-fundene er rettet og bygget: salvens ofreliste, en frigivet reserve inden for skudafstand og fjernelse af chokordren; accepttestene PASS)
    - Test 4 stopper ved sidste skærmbillede (420 s), før slaget er slut, og giver ingen rapport; forlæng skærmbillederne for at få slutresultatet.
    - hestens skridt er for korte (glidende fødder), sablen følger ikke hånden.
  - Udgangspunkt: Docs/Backlog.md, "Fund fra 4-mod-1-testene", punkt (1)–(3):
    - rytteri-tasking (implementeret, utestet);
    - trusselsvurdering og reserve (implementeret, utestet);
    - sidetrin (uændret).
  - Animationer: Docs/Manglende-Animationer.md afsnit 5, klip 32–41. Rigget hest og rytterklip importeret 2026-11-03.
- Officerer (2026-11-05: i slaget, importeret og testet med probe; se historikken):
  - mangler:
    - egne klip til at trække sablen og stikke den i skeden, med IK til fæstet. Soldaternes sabelklip når ikke fæstet (33–39 cm væk); nu skifter sablen plads med det samme;
    - hvil- og ordreklippet står cirka 8 cm for lavt. Det er løftet i koden og bør rettes i Blender;
    - dødsklippet hopper, når han falder under march;
    - han er ikke set i karré, og hans fald er ikke set (`Fate`);
    - nærbilleder af officeren kræver et kamera på ham, fordi skærmbilledet sigter mod kompagniets midte.
- Underofficer: ingen rolle endnu (idé i Backlog).
- **Kolonne til linje (Allan 2026-11-05):** "en enkelt mand gik igennem hele formationen og forsinkede dem". Formation til kolonne er højre om 90°, kolonne til formation er venstre om 90°; ingen soldat skifter plads.
  - Rettet og bygget 2026-11-05 (`PlanColumnDrill`): linjen får en reserverot bagerst. En mand, der støder sammen med en anden, flyttes højst 2 rotter eller til reserven. Test 4: manden flyttes nu 1 rot, før 62.
  - Tilbage: 2. kompagni venter stadig 10 s én gang ("gave up: still forming"); årsagen er ikke fundet. Hvorfor to mænd deler plads i kolonnen, er heller ikke fundet.
- **Hestene accelererer (Allan 2026-11-05):** rytteri og stab får `AccelerationCmPerSecondSquared` = 250 (0 til 9 m/s på cirka 3,5 s), bygget. Accepttestene RESERVE-* og CAV-* (6) består alle. Ikke set visuelt.
- **Nedbrud ved minimeret vindue** under testens skærmbilleder: rettet og bygget. Billedet springes over, når vinduet er minimeret (SWindow).
- **Sigtede skud (`-Strategy1864AimedFire`) 2026-11-05:** danskerne ramte 8,2 %, svenskerne 13,5 %. Danskerne samler ilden på få mænd (én fik 87 skud), og 58 skud blev spildt. Målfordelingen skal undersøges.
- **De 2 HQ-officerer til hest (Allan 2026-11-05):** "skal også kigges på - med animationer og det hele".
  - I dag: statisk hest (`SM_Horse_Static`) og en soldat, der sidder frosset (`StrategyHQVisualComponent`).
  - Plan: genbrug rytteriets riggede hest (`SK_Horse_1864`) med gangarter og rytterklippene. Rytteren er officersfiguren med sabel og skede.
- Natkøens øvrige opgaver blev ikke kørt:
  - Odense kompakt og hurtigere opstart;
  - huse på veje og vand;
  - København og A-byer.
