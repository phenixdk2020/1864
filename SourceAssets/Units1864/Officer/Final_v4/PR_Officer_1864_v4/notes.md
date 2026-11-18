# PR officer v4 — sabelgreb, 2026-10-30

Fire krummede fingre og tommelfinger erstatter den sammenvoksede højre vante. Manchetten er tilpasset håndleddet i bindeposen. Håndgeometrien følger `mixamorig:RightHand`. To læderremme forbinder venstre gehæng med skeden og følger `mixamorig:Hips`. Samme faste socket anvendes i alle tre poser; ingen offsets pr. pose. Rekvisittens geometri og origin er uændret.

Poserne er rettet med toleddet IK og faste knoglelængder. 30 fps. March er en 1-sekunds in-place testcyklus, rodbevægelse 0 cm og hastighed 0 cm/s. Ikke en produktionsgang med sikret fodkontakt. Ordre og Hvil er holdte poser. Der er 29930 trekanter. FBX-genindlæsning bekræfter 41 knogler med uændrede navne, forældre og bindematricer; normaliserede vægte, højst fire pr. vertex.

## Faste Unreal-sockets

Location er X/Y/Z i cm; rotation er Pitch/Yaw/Roll i grader. Værdierne gælder de eksisterende separate Props_v4-FBX'er, som bruger cm efter import. Socket Scale skal også anvendes; den kompenserer riggens eksisterende skala. Rekvisittens relative transform under socket sættes til identitet.

- **Sabel** — Unreal-knogle `RightHand` (Blender: `mixamorig:RightHand`). Location: `(8.859873, -10.981565, 4.255536)`. Rotation P/Y/R: `(-38.611339, -44.486661, -148.198385)`. Scale: `(1.106196, 1.106196, 1.106196)`.
- **Skede** — Unreal-knogle `Hips` (Blender: `mixamorig:Hips`). Location: `(33.243320, -9.561045, -19.432434)`. Rotation P/Y/R: `(-20.730171, 84.857239, -90.000003)`. Scale: `(1.106196, 1.106196, 1.106196)`.

FBX eksporteres med -Z frem/Y op og bevarede knogleakser. Konverteringen er afledt af UE 5.8 `FFbxDataConverter::ConvertPos`/`ConvertRotToQuat` og `FFbxImporter::MakeName`, som fjerner namespace. Forudsætter standard Convert Scene/Convert Scene Unit uden ekstra importrotation eller ændret rodskala. Ingen Unreal-import er udført; værdierne er matematisk konverteret og kræver kontrol ved den senere import. Blender-matricer og quaternioner findes i verification_v4.json.

## Geometrisk kontrol

Grebets og håndfladens centre er målt med separate ellipsefit til rekvisittens vertices og den evaluerede inderside af fingrene i gribezonen. Metode, sampleantal og målte centre findes pr. pose i verification_v4.json. Den næsten nul socket-anchor-fejl er en separat fastgørelseskontrol og erstatter ikke håndmålingen.

Lineær skinning er evalueret på alle 31 marchframes og på frame 1 og 31 i de holdte poser. Hele sablen inklusive fæste, bøjle og knop er kontrolleret mod hånd, håndled og krop. Bladet rapporteres også separat. Kontrollen bruger BVH-trekantintersektion og flervejs ray-paritet på rekvisitvertices. Kildemesh indeholder åbne områder, så overfladeintersektioner rapporteres særskilt. Tallene nedenfor er maksimum over de målte frames, i cm.

| Pose | Greb/håndflade | Blad ind i krop | Hele sabel ind i krop/hånd | Skede ind i krop |
|---|---:|---:|---:|---:|
| March_Skulder | 0.10825 | 0.00000 | 0.00000 | 0.00000 |
| Ordre | 0.10825 | 0.00000 | 0.00000 | 0.00000 |
| Hvil | 0.10824 | 0.00000 | 0.00000 | 0.00000 |

March: bladets ryg ligger ved højre skulder med målt mindste overfladeafstand 0.05446 cm. Ordre: fremad/opad. Hvil: spidsens nederste geometri ligger 0.29999 cm over jordplanet ved højre fod. Skeden hænger med mundingen frem/op og spidsen bagud/ned.

Preview: `preview_v4.png` viser tre poser. `sabre_closeup_v4.png` viser hver højrehånd med modsat side i et indsat billede samt skeden ved venstre hofte.

Resterende begrænsninger: Håndens modellering og hudmateriale er stiliseret; ikke anatomisk færdig til nærfilm. Unreal-import og runtime er ikke testet. Nationalt eksercits er ikke historisk verificeret. Kildens uniforms-/ansigtstekstur er bevaret. Ingen build, spilstart, .bat eller commit.
