# Rekognoscering, tåge og fjendens AI (backlog 1 og 2)

## Hvad danskerne ved
- **I fred** er alle korps synlige. Attachéer og aviser fortæller, hvor de står.
- **I krig** ses et korps kun, når det er set eller meldt.

### Set af tropper
Et korps ses, hvis det er inden for synsvidde af en dansk enhed eller skanse. Synsvidden bestemmer også, hvor godt styrken kan tælles.

| Hvem ser | Synsvidde | Styrkens usikkerhed |
|---|---|---|
| Kavaleri og ridende artilleri | 20 km | ±5 % |
| Andre enheder | 8 km | ±25 % |
| Skanser | 10 km | ±25 % |

### Meldt af en by
- En dansk by inden for 12 km af korpset sender en melding. Styrken angives med ±40 % usikkerhed.
- Meldingen tager 1,5 dag med kurer. Med en station tager den 0,5 dag, og med felttelegrafen (forskning) 6 timer.

### På kortet
- Et korps, der ikke er set lige nu, tegnes falmet, der hvor det sidst blev meldt.
- Styrken vises som "ca. N?", og tidspunktet som "meldt kl. dato".

## Hvad fjenden ved og vælger
- **Fjendens viden:**
  - Danske enheder inden for 15 km af et korps kender fjenden præcist.
  - Andre enheder kender han kun af rygter (±30 %).
  - Skanserne kender han altid.
- **Mål:** korpset vurderer planens mål og alle danske byer inden for 120 km.
  - Værdi = 1 + indbyggere/5.000, plus 4 for planens første mål og 1,5 for de øvrige.
  - Point = værdi × min(styrkeforhold, 3) / (1 + afstand/40).
  - Byer, hvor styrkeforholdet er under 1,3, er udelukket.
- **Omgåelse:** hvis det bedste mål ikke er planens mål, omgår korpset. Beslutningen står i Statsrådets liste med begrundelse.
- **Venten:** hvis intet mål er muligt, venter korpset på forstærkninger.
- **Forstærkninger:** hver 30. krigsdag får Preussen 3.000 mand og Østrig 1.500, dog højst op til 1,5 × korpsets første styrke. En blokade halverer forstærkningerne.
- **Over vand:** byer over vand, som flåden spærrer for, springes over, så længe flåden behersker farvandet.

Gemning: `intel|korps|x|y|dag|anslåede mand|startstyrke` i War-linjerne.

## Audit 2026-10-08
- I 1825 afrundes direkte observationer til 10 mand og bymeldinger til 100; i 1851 fortsat til 100 og 1.000. Synsvidder, usikkerhed og leveringstider er uændrede; stationer og felttelegraf følger de eksisterende anlægs- og forskningsregler.
- War gemmer desuden `intel-runtime|korpsindeks|id|set nu|melding x|melding y|meldingsdag|ankomstdag|meldte mand|hvile indtil|næste overvejelse|venten noteret|både klar dag|spærrede byindeks` samt `intel-clock|sidste krigsdag`. Ældre saves uden disse linjer accepteres; meldinger undervejs kan ikke genskabes fra dem.
- Startstyrken gendannes som gemt, også efter forstærkning. Kortets forskydning af overlappende fjendemærker bruger kun kendte positioner.
