# Dansk infanteriofficer 1864 — reference til 3D

Genereret 29. oktober 2026 med imagegen. Fiktiv kompagniofficer til fods, tænkt som kaptajn. Dette er en visuel rekonstruktion, ikke en dokumenteret uniformsopmåling.

## Filer og brug

- DK_Officer_1864_Front.png: front, A-pose.
- DK_Officer_1864_Side.png: venstre profil, ansigt mod billedets venstre side.
- DK_Officer_1864_Back.png: ryg, A-pose.
- DK_Officer_1864_Normal.png: grå front med enkel skygning uden stoftekstur; reservevariant, **ikke et normal map** og ikke egnet som normaltekstur på en mesh.

Alle billeder er 1024 × 1536 PNG med lys grå baggrund og hel figur. Fronten var billedreference for de tre øvrige billeder. Identitet, lys og proportioner er tilstræbt ens; billederne er genereret hver for sig og er ikke geometrisk kalibrerede kamerarenderinger. Profilens overlappende arme/hænder bør kontrolleres ved modellering.

Ingen sabel, skede, gevær eller rygsæk er tegnet. Gehænget er tomt. I spillet tilføjes SourceAssets/Units1864/Saber.fbx og Scabbard.fbx separat; disse filer er ikke ændret.

## Kilder og historiske forbehold

1. [Preben Kannik: Den danske hærs uniformering i 1864, Chakoten 1964/2](https://www.chakoten.dk/wp-content/uploads/2018/01/Danske-h%C3%A6rs-uniformering-i-1864.pdf), især afsnittene Infanteriet og Officerer: grundlag for mørkeblå toradet frakke, røde spejl/kanter, hvide metalknapper, lyseblå bukser, officersfelthuens sølvbesætning og sortlakeret gehæng med to remme og hvide beslag. Feltgradstegn er valgt frem for gallaepauletter.
2. [Nationalmuseets uniformsekspert om våbenfrakke M/1848](https://lex.dk/v%C3%A5benfrakke): understøtter våbenfrakkens indførelse i 1848 og infanteriets røde spejl/hvide knapper.
3. [Hærens gradstegn, digitaliseret skrift, s. 9](https://slaegtsbibliotek.dk/2025/954107.pdf): kaptajn beskrives med dobbelt skulderguldsnor og tre rosetter. Det er anvendt som fortolkning; figurens små skulderdetaljer er ikke en verificeret gengivelse af et bestemt bevaret eksemplar.
4. [Nationalmuseets eksempel på epauletter M/1842 og historik](https://lex.dk/epaulet): epauletter og feltgradstegn må ikke forveksles med senere skulderdistinktioner.
5. Lokal farvereference, visuelt undersøgt: SourceAssets/Units1864/DK_Infantry_1864_Texture_0.png. Farverne er tilpasset spillets mørke marineblå og dæmpede lyseblå; promptens hexværdier er visuelle skøn, ikke målte historiske farver.

**Skøn og kendte afvigelser:** Ansigt, alder, kropsmål, stofstruktur, præcise snit, spænder, krogform og remmenes længde er kunstneriske skøn. Kaptajnens guldsnore/rosetter kræver historisk efterkontrol. Generatoren har forenklet kravespejlene til en næsten gennemgående rød krave med færre synlige knapper/knaphulslinjer end specificeret; den viser syv synlige brystknapper pr. række frem for promptens otte. Rygklapper og deres knapplacering er også fortolkede. Disse detaljer bør korrigeres på den endelige model ved krav om museumsnøjagtighed.

**Hovedbeklædning:** Brugerens kepi er fortolket som en lav, kepilignende officersfelthue med skygge. Det er ikke en høj chakot M/1858 eller en verificeret senere kepimodel. Kappens fravær gør uniformens konstruktion synlig, men er ikke repræsentativt for alle forhold under vinterfelttoget.

**1848–51:** Våbenfrakkens grundtype kan bruges som udgangspunkt efter indførelsen i 1848. Hele sættet er ikke verificeret til Treårskrigen; særligt felthue, distinktioner og detaljer skal tilpasses det konkrete år og regiment. Sættet må ikke mærkes som universelt korrekt for begge krige.

## Prompter (ordret, engelsk)

### Front

Create ONE full-body FRONT orthographic character reference image, 1024x1536 portrait, for Hunyuan3D. Danish line infantry company officer on foot, 1864, imagined man aged 35 with short brown hair and neat brown moustache. Neutral face. Anatomical A pose: straight arms extended to sides 45 degrees BELOW horizontal, palms facing ground, fingers straight relaxed, feet slightly apart. Entire cap to boot soles visible, generous margins, centered, no text. Uniform: mid-thigh dark navy blue double-breasted wool waffenrock, two rows of eight small silver-white metal buttons, scarlet standing-collar patches with two tiny white buttons and blind buttonhole lines each side; narrow red piping on cuffs and coat edges. Small period FIELD shoulder distinctions of doubled gold cord with three small rosettes for captain (interpretive reconstruction), NOT large fringed parade epaulettes, no modern rank slides. Light muted blue-grey long trousers, black ankle boots under trousers. Low dark blue kepi-like Danish officer FIELD cap with black leather visor, narrow SILVER cord round lower edge and silver embroidered red-white cockade, no tall shako, no plume. Clearly visible black patent leather sword waistbelt OUTSIDE tunic with silver-white fittings and TWO EMPTY narrow suspension straps on wearer's LEFT hip ending in empty small metal hooks, straps hanging freely at upper thigh. Absolutely NO SABRE, NO SCABBARD, NO sword handle, no weapon of any kind, no rifle, no backpack, no sash, no cross belts. Colours based on the inspected in-game texture: near-black navy cloth around #101b2d, trouser muted blue-grey around #6289a5, subdued red accents. Physically plausible cloth, clean realistic 3D model reference render, uniform soft frontal studio lighting with minimal shadows, plain solid light grey #dedede background, orthographic camera at body center, no foreshortening, no perspective distortion. Only one figure, no props on ground.

### Side

Use the attached FRONT image as the exact character and uniform reference. Render the SAME man in the SAME fixed anatomical A-pose, rotated exactly 90 degrees to show his LEFT SIDE in strict orthographic profile, face and toes pointing image LEFT. Arms remain 45 degrees down from horizontal in the body's frontal plane, so they overlap in profile; do NOT reposition arms forward. Same uniform, cap, shoulder cords, colour, proportions, empty two sword suspension straps on anatomical LEFT hip, same soft studio lighting and light grey background. Full body with cap and soles visible, same vertical scale and margins as reference, portrait 1024x1536. No sabre, no scabbard, no weapon, no backpack, no text, no perspective, no three-quarter angle.

### Back

Use attached front image as exact character identity, uniform and geometry reference. Render SAME Danish infantry company officer 1864 from directly BEHIND, strict orthographic REAR view rotated 180 degrees. Same fixed anatomical A-pose, straight arms 45 degrees below horizontal and palms toward floor, feet apart, same proportions and framing, full body cap through boot soles. Same dark navy mid-thigh tunic with subtle red piping, small gold cord field shoulder distinctions, muted light blue-grey trousers and black boots, low blue cap with silver lower band. Back tunic has two straight vertical pocket flaps, red piped, with two small white metal buttons each, upper ones at waist. Black patent leather sword waistbelt clearly outside tunic. Two empty hanging sword suspension straps ONLY on anatomical LEFT hip (image LEFT in rear view), small silver hooks, no connecting sword or scabbard. Same soft frontal studio lighting, plain light grey background. Portrait exactly 1024x1536. NO sabre, NO scabbard, NO weapons, NO backpack, NO text, NO perspective, NO three-quarter angle.

### Normal / reservevariant

Transform attached FRONT character reference into a FLAT-SHADED FRONT geometry reference, exactly 1024x1536 portrait. Preserve precise same man, silhouette, A-pose, fingers, cap, uniform construction, buttons, shoulder cords, belt and two EMPTY hanger straps, same front camera, full body, same scale and plain light grey background. Use matte neutral medium grey material on entire character, no fabric textures, no colour patterns, no specular highlights, broad flat shading to make surface planes, seams and folds legible. Minimal uniform studio illumination, no dramatic shadow. This is a flat-shaded modelling aid, NOT a tangent-space normal texture. Absolutely no sword, no sabre, no scabbard, no weapon, no backpack, no text. Do not change shape or outfit.

## Afgrænsning

Danmark er afsluttet som selvstændigt referencesæt. Sverige, Norge og Preussen er ikke påbegyndt. Ingen kildekode eller projektdokumentation er ændret; ingen build, spil/editor, .bat eller commit er kørt.

