# Handelsvarer, statslån, avis, statistik og opslagsværk (backlog 10, 12, 16, 17, 18)

## Udførslen
| Vare | Udførsel pr. indbygger på landet og år (1851-pris) |
|---|---|
| Korn | 0,6 rd. |
| Kvæg | 0,3 rd. |
| Smør | 0,15 rd. |

- **Prisindeks:** en langsom markedsbølge over 29 måneder plus lidt støj.
- **Kornet** følger årets høst, fra august til juli (0,85–1,15 af mængden). Prisen stiger, når høsten er dårlig.
- **Krimkrigen** (oktober 1853–marts 1856): korn × 1,5, øvrige varer × 1,15.
- **Krisen 1857** (november 1857–1858): alt × 0,7.
- **Krig:** kvæg × 0,5, fordi Hamborg lukker, og det øvrige × 0,8. Uden herredømmet til søs halveres det hele igen.
- **Besatte amter** udfører intet.
- **Told:** 2 % af udførslen går i kassen hver måned.
- **Stemningen** ændres hver måned med (kornindeks − 1) × 1,5.

## Statslån
- **Renten for et nyt lån** er 4 %, + 2 % pr. års indtægter i gæld, + 1,5 % i krig og + 1 %, hvis stemningen er under 30. Den er højst 9 %.
- **Lånerammen** er højst 3 års indtægter (skatter, told, Øresund og handel).
- **I STATSKASSEN:**
  - LÅN 100.000 (Hamborg) og LÅN 250.000 (London).
  - AFDRAG 100.000.
  - Renten trækkes hver måned.

## AVISEN (knappen ved siden af SKANSER)
- **AVISEN:** alle nyheder efter dato, med den nyeste som overskrift. De sidste 120 gemmes.
- **MARKEDET:** prisindeks, udførsel og told pr. vare, samt statsgælden.
- **STATISTIK:** hver måned noteres befolkning, kasse, hær, jernbane-km, spænding, stemning og kornpris. Det vises som grafer.
- **OPSLAG:** 12 afsnit om spillets regler.

## Gemning (v26)
`Economy`-linjer:
- `debt|gæld|rente`
- `h|dag|befolkning|kasse|hær|bane|spænding|stemning|korn|gæld`
- `prog|værk|tilstand|dag`
- `end|0/1`
- `n|dag|nyhed`

Test: `-CampaignOpenWindow=gazette -CampaignGazetteTab=0..3`.
