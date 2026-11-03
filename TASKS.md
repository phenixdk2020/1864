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
- Officerer:
  - Codex-natkøen 30/10 nåede ikke at rette sablen i hånden, skulderen og fødderne færdigt (Final_v4);
  - import i Unreal mangler.
- Natkøens øvrige opgaver blev ikke kørt:
  - Odense kompakt og hurtigere opstart;
  - huse på veje og vand;
  - København og A-byer.
