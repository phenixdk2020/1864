# Spilvalg og estimatmetode

Spillets eksisterende PR-rækker og by-/hærrækker er bevaret som modelgrundlag. `reserve`, `growth`, `weights`, `caution`, `relation`, `tradeValue`, garnisonsmænd og kanoner er spilvalg, ikke kildeobservationer. `growth` beholder spillets skala; der udledes ingen historisk vækstrate herfra. Reserve i Nations er en spilreserve og kan ikke summeres med Landwehr/Kriegsreserve i forskningsnoterne. `tradeValue` er en traktatbonus og ikke målt bilateral årsomsætning. Relation i 1851 følger landeskabelonens forslag.

Generaler: hver af de ni egenskaber sættes neutralt til 5, erfaring til 50. Regimenters erfaring og seks færdigheder er neutrale 50-skøn. Tallene er egne spilskøn, ingen historiske karakterbøger. Sandsynligheder, forudsætninger, blokeringer og eventeffekter er spilvalg; historiske overskrifter gør ikke virkningerne historiske. Events før scenariestart er historik og må ikke udløses igen.

Halvårsbefolkning beregnes som round((ultimo året før + ultimo scenarieåret)/2). Det er et scenarieskøn med afrundingspræcision til person, ikke en præcis optælling den pågældende dag. HGIS' egne skøn og kildekoder bevares i observationslisten. Nationale tal, provinser og Bezirke interpoleres hver for sig; ingen skjult residualkorrektion. Årstal, ID-numre, skemaversioner, filstørrelser og kontroloptællinger er tekniske identifikatorer, ikke historiske målinger.

Spillets 2.900 jernbane-km i 1851 bevares eksplicit som spil. Sølvpariteten 37/28 er en beregnet normalisering, ikke en dokumenteret markedskurs eller købekraftskurs. AI-controller og onMap=false følger den eksisterende nabomodel; niveau B er stadig målet.
