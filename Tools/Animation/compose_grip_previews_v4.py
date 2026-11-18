"""Compose Blender scratch renders into the two authorised final contact sheets."""
from pathlib import Path
import tempfile,sys,json
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[2]
TMP=Path(tempfile.gettempdir())/'officer_grip_v4'
BASE=ROOT/'SourceAssets/Units1864/Officer/Final_v4'
apply='--apply' in sys.argv
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',28)
small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',21)
labels=['March_Skulder','Ordre','Hvil']
for nat in ['DK','SE','NO','PR']:
 dest=BASE/f'{nat}_Officer_1864_v4' if apply else TMP
 sheet=Image.new('RGB',(2400,965),(32,38,45));draw=ImageDraw.Draw(sheet)
 for i,label in enumerate(labels):
  sheet.paste(Image.open(TMP/f'{nat}_{label}_full.png').convert('RGB'),(i*800,65))
  draw.text((i*800+24,18),f'{nat} — {label.replace("_"," ")}',font=font,fill='white')
 sheet.save(dest/('preview_v4.png' if apply else f'{nat}_preview.png'))
 sheet=Image.new('RGB',(1700,1820),(32,38,45));draw=ImageDraw.Draw(sheet)
 for i,label in enumerate(labels+['hip']):
  x=(i%2)*850;y=(i//2)*910
  p=TMP/(f'{nat}_{label}_hand.png' if i<3 else f'{nat}_hip.png')
  sheet.paste(Image.open(p).convert('RGB'),(x,y+60))
  title=f'{nat} — '+(label.replace('_',' ') if i<3 else 'Skede / venstre hofte')
  draw.text((x+20,y+15),title,font=font,fill='white')
  if i<3:
   inset=Image.open(TMP/f'{nat}_{label}_palm.png').convert('RGB').resize((285,285),Image.Resampling.LANCZOS)
   sheet.paste(inset,(x+555,y+615));draw.rectangle((x+554,y+614,x+841,y+901),outline=(230,230,230),width=2)
   draw.text((x+565,y+620),'Modsat side',font=small,fill='white')
 sheet.save(dest/('sabre_closeup_v4.png' if apply else f'{nat}_closeup.png'))
 print('COMPOSED',nat,str(dest),flush=True)
