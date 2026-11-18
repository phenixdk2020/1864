# SE officer 1864 — billedreference

Genereret 30. oktober 2026 med imagegen til TASK A. Fiktiv officer, visuel rekonstruktion, ikke en museumsverificeret uniformsopmåling.

## Filer

- SE_Officer_1864_Front.png — front, A-pose.
- SE_Officer_1864_Side.png — venstre profil, ansigt mod venstre.
- SE_Officer_1864_Back.png — ryg, A-pose.
- SE_Officer_1864_Normal.png — grå front med enkel skygning; **ikke et normal map**.

1024 × 1536 PNG, hel figur og lys grå baggrund. Ingen sabel eller skede; sort sværdbælte med to tomme ophæng på venstre hofte. Fronten er reference for de øvrige billeder. Visningerne er separat genererede og ikke geometrisk kalibrerede; kontroller silhuet, fingre, profilarmenes overlap, baglommer og beslag ved modellering.

## Historiske forbehold

Svensk linjeinfanteriofficer, fortolkning af m/1860. Toradet mørkeblå frakke, guldgalon og blåmelerede bukser med gul passepoil. Kendt afvigelse: generatorens høje blå hovedbeklædning er stoflignende, mens SFHM beskriver sortlakeret læder til m/1860; form, plade og pompon er også fortolkede. Skuldertegn er ikke verificeret til en bestemt grad eller et regiment. Profilen ændrer armens projektion og manchetternes galon i forhold til fronten.

Ansigt, alder, præcise farvenuancer, stof, kropsmål, remmenes længde, krogform og rygkonstruktion er kunstneriske skøn. Fraværet af sabel/skede er et bevidst krav til modulær modellering. Sættet må ikke bruges uændret som generel uniform for alle regimenter eller andre årstal.

## Kilder

- [Christian Braunstein / Statens försvarshistoriska museer: Svenska arméns uniformer, del 2, s. 98–107](https://sfhm.se/wp-content/uploads/2025/04/SFHM_skriftserie_Svenska_armens_uniformer_del_2_infanteriet.pdf). Museumsudgivelse, hovedgrundlag for m/1860, farver og hovedbeklædning. Den bløde m/1865-hue er senere end målåret.

## Prompter (ordret)

### Front

Use case: historical-scene. Create ONE 1024x1536 portrait full-body FRONT orthographic 3D modelling reference. Swedish line infantry officer circa 1864, uniform m/1860 (not guards). Dark navy DOUBLE-breasted mid-thigh tunic, brass buttons, dark blue standing collar and cuffs with paired elongated GOLD lace buttonholes, modest dark blue shoulder straps with small gold officer detail; blue-grey melange trousers with narrow yellow piping, black boots under trousers. Period tall stiff dark blue m/1860 headgear with black visor, brass Swedish three-crown badge, small dark pompon, no later m/1865 soft cap. Fictitious 35 year old man with brown moustache. Match supplied Danish reference's realistic render style, proportions and composition, but use Swedish uniform. Fixed anatomical A-pose, straight arms 45 degrees below horizontal, palms down, feet slightly apart. Black sword waistbelt OUTSIDE coat, TWO EMPTY leather hanger straps with small brass hooks on anatomical LEFT hip. Absolutely NO SABRE, NO SCABBARD, NO hilt or weapons, no backpack, no crossbelt, no sash, no text. Whole headgear to soles visible, generous margins, plain light grey background, soft even studio illumination, no perspective.

### Side

Use attached FRONT as exact character and clothing reference. Rotate SAME fixed A-pose exactly 90 degrees to strict LEFT SIDE orthographic profile, face and toes pointing image LEFT. Arms remain in body's frontal plane, overlapping in profile, not reaching forward. Same identity, headgear, clothing, proportions and vertical scale. Show two EMPTY hanger straps on visible anatomical LEFT hip. 1024x1536 portrait, ONE whole figure headgear through soles with generous margins. Strict orthographic camera, plain light grey background, even soft studio light, realistic 3D modelling reference matching Danish example. Anatomical A-pose arms straight 45 degrees below horizontal palms down feet apart. Black sword belt outside coat with TWO EMPTY suspension straps and hooks on anatomical LEFT hip. NO SABRE, NO SCABBARD, no hilt, no weapons, no backpack, no crossbelts, no text.

### Back

Use attached FRONT as exact character and clothing reference. Rotate SAME fixed A-pose exactly 180 degrees, strict orthographic BACK view. Same headgear, clothing, identity, proportions, colour and vertical scale. Plausible restrained back seams and coat-tail pocket flaps. Two EMPTY hanger straps ONLY on anatomical LEFT hip, image LEFT in rear view. 1024x1536 portrait, ONE whole figure headgear through soles with generous margins. Strict orthographic camera, plain light grey background, even soft studio light, realistic 3D modelling reference matching Danish example. Anatomical A-pose arms straight 45 degrees below horizontal palms down feet apart. Black sword belt outside coat with TWO EMPTY suspension straps and hooks on anatomical LEFT hip. NO SABRE, NO SCABBARD, no hilt, no weapons, no backpack, no crossbelts, no text.

### Normal

Transform attached FRONT into matte neutral medium-grey FRONT modelling reference. Preserve exact silhouette, pose, headgear, face, garment seams, buttons, belt, two empty hanger straps and geometry. Entire character grey, broad simple flat shading, no cloth texture or colour patterns or shiny highlights. This is a flat shaded modelling aid, NOT a tangent-space normal map. 1024x1536 portrait, ONE whole figure headgear through soles with generous margins. Strict orthographic camera, plain light grey background, even soft studio light, realistic 3D modelling reference matching Danish example. Anatomical A-pose arms straight 45 degrees below horizontal palms down feet apart. Black sword belt outside coat with TWO EMPTY suspension straps and hooks on anatomical LEFT hip. NO SABRE, NO SCABBARD, no hilt, no weapons, no backpack, no crossbelts, no text.

## Afgrænsning

Kun nye filer under Reference/Units/MultiView/. Det danske sæt er bevaret. Ingen lokal GPU-app eller Hunyuan startet; ingen build, Unreal, .bat eller commit kørt.

