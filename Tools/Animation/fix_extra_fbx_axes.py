"""Re-export only manifest extras through Blender; originals are read/hash checked only.
Blender 5.2: blender -b -P Tools/Animation/fix_extra_fbx_axes.py
Backups are immutable: a second run always reads the first backup.
"""
import sys, json, hashlib, shutil
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_shoulder_arms_set import load_sample, export_frames
from audit_combat_fbx import audit
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'SourceAssets/InfantryCombat'
BACKUP = ROOT/'Saved/Codex/axes_fix/before'
BACKUP.mkdir(parents=True, exist_ok=True)
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
originals = json.loads((SOURCE/'manifest.json').read_text())['clips']
protected = [SOURCE/c['file'] for c in originals] + [ROOT/'Content/Units/Human/CombatAnimations'/('A_Combat_'+c['name']+'.uasset') for c in originals]
hashes = {str(p.relative_to(ROOT)): sha(p) for p in protected}
(ROOT/'Saved/Codex/axes_fix/original_hashes_before.json').write_text(json.dumps(hashes,indent=2))
extras = []
for manifest in ('manifest_extra.json','manifest_cav.json'):
    extras += json.loads((SOURCE/manifest).read_text())['clips']
assert not ({c['file'] for c in extras} & {c['file'] for c in originals})
ref = audit(ROOT/'Saved/Codex/shoulder_arms/old_clips/A_Combat_WalkShoulder.fbx','v3_reference')
report = []
for clip in extras:
    path = SOURCE/clip['file']; backup = BACKUP/path.name
    if not backup.exists(): shutil.copy2(path, backup)
    before = audit(backup, 'before_axes_'+clip['name'])
    if before['global']['UpAxis'] != [2]:
        raise RuntimeError('Unexpected source convention: '+clip['name'])
    arm, frames = load_sample(backup)
    assert len(frames) == clip['frames'], (clip['name'],len(frames))
    export_frames(arm, clip['name'], frames, path)
    after = audit(path, 'after_axes_'+clip['name'])
    checks = {'GlobalSettings':after['global']==ref['global'],
              'Armature':after['models']['Armature']==ref['models']['Armature'],
              'hierarchy': sorted(after['hierarchy'])==sorted(ref['hierarchy']),
              'frame_count':len(frames)==clip['frames']}
    report.append({'name':'A_Combat_'+clip['name'],'before_sha256':sha(backup),'after_sha256':sha(path),
                   'before':{k:before[k] for k in ('global','hips_rotation_curves')},
                   'checks':checks,'hips_first':[(c['axis'],c['values'][0]) for c in after['hips_rotation_curves']]})
    if not all(checks.values()):
        (ROOT/'Saved/Codex/axes_fix/verification.json').write_text(json.dumps(report,indent=2))
        raise RuntimeError(str(checks))
assert hashes == {str(p.relative_to(ROOT)):sha(p) for p in protected}
(ROOT/'Saved/Codex/axes_fix/original_hashes_after.json').write_text(json.dumps(hashes,indent=2))
(ROOT/'Saved/Codex/axes_fix/verification.json').write_text(json.dumps(report,indent=2))
print('EXTRA_AXES_VERIFIED',len(report))
