from pathlib import Path
import re, subprocess
paths=[Path(p) for p in subprocess.check_output(['git','diff','--name-only'],text=True).splitlines() if p.endswith(('.cpp','.h'))]
paths.append(Path('Source/Strategy1864/Combat/StrategyBattleLedger.h'))
for p in paths:
 s=p.read_text(encoding='utf-8-sig')
 # Strip comments and strings before lexical delimiter checking.
 clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '',s)
 stack=[];pairs={')':'(',']':'[','}':'{'}
 for c in clean:
  if c in '([{':stack.append(c)
  elif c in pairs:
   assert stack and stack.pop()==pairs[c],(str(p),'unbalanced delimiter')
 assert not stack,(str(p),stack)
 for inc in re.findall(r'#include "([^"\n]+)"',s):
  if inc.startswith('../'):assert (p.parent/inc).exists(),(p,inc)
 assert 'OfficersKilled' not in s,(p,'officers killed')
scenario=Path('Source/Strategy1864/Tests/StrategyOOBTestScenario.cpp').read_text(encoding='utf-8')
assert not re.search(r'Fate\s*=\s*3|TEXT\("officersKilled"\)',scenario)
for cls,header,source,methods in [
 ('AStrategyUnit','Source/Strategy1864/Units/StrategyUnit.h','Source/Strategy1864/Units/StrategyUnit.cpp',['EnsureBattleLedger','RecordBattleLoss','ApplyStrengthLossWithCause','RecordBattleVolley','CaptureBattlePrisoners','RecordEquipmentAbandonment','RecordEquipmentCapture','RemainingReportEquipment']),
 ('AStrategyOOBTestScenario','Source/Strategy1864/Tests/StrategyOOBTestScenario.h','Source/Strategy1864/Tests/StrategyOOBTestScenario.cpp',['FinalizeAfterActionReport','TickBattleLedger']),
 ('AStrategyHUD','Source/Strategy1864/Player/StrategyHUD.h','Source/Strategy1864/Player/StrategyHUD.cpp',['DrawAfterActionReport']),
 ('ACampaign1851Map','Source/Game1864/Public/Campaign1851Map.h','Source/Game1864/Private/Campaign1851Career.cpp',['RecordBattleOfficerCareers'])]:
 h=Path(header).read_text(encoding='utf-8-sig');c=Path(source).read_text(encoding='utf-8-sig')
 for method in methods:
  assert re.search(r'\b'+method+r'\s*\(',h),(header,method)
  assert len(re.findall(r'\b'+cls+'::'+method+r'\s*\(',c))==1,(source,method)
battle=Path('Source/Game1864/Private/Campaign1851Battles.cpp').read_text(encoding='utf-8')
for key in ['startMen','killed','wounded','prisoners','highestMoraleLoss','heldField','capturedEquipment','lostEquipment','officerParticipants']:
 assert 'TEXT("'+key+'")' in scenario and 'TEXT("'+key+'")' in battle,key
assert 'S.Career = O.Career' in Path('Source/Game1864/Private/Campaign1851Officers.cpp').read_text(encoding='utf-8')
assert 'O.Career = S.Career' in Path('Source/Game1864/Private/Campaign1851Officers.cpp').read_text(encoding='utf-8')
assert 'P.Num() >= 12' in Path('Source/Game1864/Private/Campaign1851Army.cpp').read_text(encoding='utf-8')
assert 'P.Num() >= 8' in Path('Source/Game1864/Private/Campaign1851Army.cpp').read_text(encoding='utf-8')
print(f'Statisk kontrol bestået: {len(paths)} C++-filer; klammer, relative includes, nye definitioner, officersregel, rapportfelter og gem/indlæs.')
