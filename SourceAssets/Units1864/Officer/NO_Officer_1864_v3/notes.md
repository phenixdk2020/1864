# NO-officer v3

- Trekanter: 18000. UV bevaret; original 2048 × 2048 base color kopieret uændret.
- 41 knogler: identiske navne, hierarki og lokale bindematricer som DK_Infantry_1864.fbx (kontrolleret i script).
- Automatisk bone-heat skinning på midlertidigt tilpasset skelet; 8764 vertices korrigeret (hoved/hat, bælte, vantehænder, ben/fødder og eventuelle uvægtede punkter). Geometrien bevarer sin oprindelige A-pose og bindes med disse vægte til infanteriets uændrede skelet; proportioner afviger lidt fra infanteriet.
- Højde 1,80 m inkl. hovedbeklædning, fødder ved Z=0. Z op i Blender, fremad -Y ligesom infanteriet. FBX -Z/Y, samme eksportakse som eksisterende infanteriklips; anvend eksisterende infanteri-importkonvention i senere Unreal-trin.
- preview_v3.png: A-pose til venstre, eksisterende A_Combat_Walk.fbx, frame 12, til højre. Arme, ben og hoved skal vurderes visuelt; ingen Unreal-validering.
- Kendte kildedefekter: sammenvoksede fingre, bløde ansigtsdetaljer, fladt bælte/gehæng og afrundede støvler. Ingen fingeranimation; skelet er bevaret uden ekstra knogler. Frakkeskøder kan strække eller klippe i store skridt; ingen cloth-simulering.
- Ingen klip ændret eller leveret; 30 fps-testframe alene, ingen ny ganghastighed eller root motion.

## Afsluttende visuel og FBX-kontrol

Genindlæst FBX: 17999 trekanter, 41 knogler, identiske navne/forældre, maksimal lokal bindematrixafvigelse 8.7738037109375E-05. Ingen uvægtede vertices. Teksturens SHA-256 matcher v2. Højde 1.7998391875298694 m; laveste punkt 0.0001606455771252513 m (FBX-afrunding).

Krøllede manchetter og skarpe folder ved underarme/albuer; hænderne rører hinanden i testposen. Riggen er strukturelt kompatibel og testet med eksisterende marchframe, men ikke en fejlfri produktionsgodkendt skinning. Kun armgeometrien er tilpasset til infanteriets bindepose; torso/hoved/ben bevarer grundformen. Tidligere prøveeksporter er bevaret i ../_scripts/*pass_v3/ uden overskrivning af originale assets.

