#!/bin/bash
# renders + sheets for shoulder v5 (Blender headless + PIL). Rule: Barrel = Up*U + Side*S + Forward*F, grip fraction G from the butt.
B="/c/Program Files/Blender Foundation/Blender 5.2/blender.exe"; U=${U:-0.176}; S=${S:--0.09}; F=${F:-0.115}; G=${G:-0.03}
D=C:/Projekter/1864-Campaign/Saved/Codex/shoulder_v8
for c in WalkShoulder ShoulderUp ShoulderDown; do
  "$B" -b -P C:/Projekter/1864-Campaign/Tools/Animation/preview_shoulder_v8.py -- "$D/A_Combat_$c.fbx" "$D/r_$c" $U $S $F $G 0,10,24,29 2>&1 | grep -E "V8_PREVIEW_DONE|Error"
done
python - <<PY
from PIL import Image
D='C:/Projekter/1864-Campaign/Saved/Codex/shoulder_v8/'
for c in ('WalkShoulder','ShoulderUp','ShoulderDown'):
    sheet=Image.new('RGB',(420*6,560*2))
    for r,(fa,fb) in enumerate(((0,10),(24,29))):
        for k,fr in enumerate((fa,fb)):
            for j,v in enumerate(('front','side','q34')):
                sheet.paste(Image.open(f'{D}r_{c}_{v}_{fr:02d}.png').convert('RGB'),((k*3+j)*420,r*560))
    sheet.save(f'{D}{c}_v8_sheet.png')
PY
