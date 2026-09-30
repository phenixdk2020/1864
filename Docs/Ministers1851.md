# Ministrene og AI for alle poster

## Ressorterne (8)
| Ressort | AI'en tager sig af |
|---|---|
| Indenrigs | Skoler, rådhuse, handel og industri i byerne |
| Offentlige arbejder | Chausséer og jernbaner |
| Krigsministeriet | Øvelser, officerer, ledige poster, forskning, de historiske værker og doktriner |
| Transport | Troppetog |
| Intendanturen | Depoter og trænkolonner |
| **Udenrigs** (ny) | Gesandter, handelstraktater, alliance, garantier og fredstilbud |
| **Marinen** (ny) | Skibsbygning og blokade |
| **Finanserne** (ny) | Statslån og afdrag |

- Hver ressort kan stå på **MANUEL**, **RÅDGIVER** eller **AUTO**.
- Knapperne *alle: MANUEL / RÅDGIVER / AUTO* i Statsrådet sætter alle ressorter på én gang.
- Beslutningerne og rådene står i listen til højre med begrundelse og ministerens navn.

## Ministrene
- Hver ressort har en minister med et navn fra tiden. Enkelte navne er spillets valg.
- Ministeren hører til en strømning: helstaten, Ejderpolitikken eller skandinavismen.
- Ministeren har tre evner (1–10), slået med to terninger ud fra seed og navn:

| Evne | Virkning |
|---|---|
| Dygtig | Ressortens budget × (0,7 + 0,06 × dygtighed), og bedre valg |
| Sparsom | Budget × (1,25 − 0,05 × sparsomhed). Finansministeren afdrager tidligere |
| Forsigtig | Udenrigs: alliance kun ved forsigtighed ≤ 6. Marinen: kræver større overlegenhed før blokade. Finanserne: låner kun billigt |

- **Regeringsskifte:** en ny regering beholder ministre fra sin egen strømning og udskifter resten.
- **NY:** knappen udskifter ministeren med en anden fra regeringens strømning.

## Hvad AUTO gør
- **Udenrigs:**
  - Slutter fred på det bedste vilkår, fjenden tager imod, når krigsstillingen er under −0,4 efter 90 dage, eller når krigen har varet 240 dage.
  - Søger garantier, alliance og handelstraktater.
  - Sender gesandter, hvor forholdet er for lavt.
- **Marinen:**
  - Blokerer i krig, når flåden er klart overlegen, og hæver blokaden, når den ikke er.
  - Bygger den bedste skibsklasse pr. rigsdaler, til flåden er 1,5 gange fjendens.
- **Finanserne:**
  - Låner, når kassen er under reserven, og renten er lav nok.
  - Afdrager, når der er rigeligt i kassen.
- **Krigsministeriet:**
  - Skifter fra bajonet til ildkamp eller spredt orden mod Preussen.
  - Vælger fæstningsdoktrinen, når mindst 6 skanser står færdige.

## Gemning
`min|ressort|navn|strømning|dygtig|sparsom|forsigtig|siden` i Politics-linjerne.
