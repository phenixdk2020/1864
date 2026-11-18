from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path('SourceAssets/Units1864/Officer/Final_v4')
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',24)
for nat in ['DK','SE','NO','PR']:
 p=root/f'{nat}_Officer_1864_v4';out=p/'arms_v4.png';assert not out.exists()
 imgs=[Image.open(p/f'arms_{k}_v4.png').convert('RGB') for k in 'abc'];sheet=Image.new('RGB',(2400,900),(50,55,62));draw=ImageDraw.Draw(sheet)
 for i,(im,label) in enumerate(zip(imgs,['a: March / sabel ved skulder','b: Fremadrettet ordre','c: Sabel ved h\u00f8jre fod'])):
  sheet.paste(im,(800*i,50));draw.text((800*i+20,12),label,font=font,fill='white')
 sheet.save(out)
