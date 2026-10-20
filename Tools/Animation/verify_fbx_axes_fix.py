"""Raw binary FBX checks plus shoulder preservation and transition checks; no Unreal."""
import sys, json, hashlib, math
from pathlib import Path
from io_scene_fbx import parse_fbx
from mathutils import Euler
sys.path.insert(0,str(Path(__file__).resolve().parent))
from audit_combat_fbx import audit, child
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'SourceAssets/InfantryCombat'
OUT = ROOT/'Saved/Codex/shoulder_arms/v3'
OLD = ROOT/'Saved/Codex/shoulder_arms/old_clips'

def curves(path):
    root,_ = parse_fbx.parse(str(path))
    objects = child(root,b'Objects'); connections = child(root,b'Connections')
    byid = {o.props[0]:o for o in objects.elems if o.props}
    models = {k:o.props[1].split(b'\x00')[0].decode() for k,o in byid.items() if o.id == b'Model'}
    nodes = {c.props[1]:(models[c.props[2]],c.props[3].decode()) for c in connections.elems
             if len(c.props)>3 and c.props[2] in models}
    return {(*nodes[c.props[2]],c.props[3].decode()):list(child(byid[c.props[1]],b'KeyValueFloat').props[0])
            for c in connections.elems if len(c.props)>3 and c.props[2] in nodes and byid[c.props[1]].id==b'AnimationCurve'}

def delta(a,b,keys):
    maximum=0.0
    for k in keys:
        assert len(a[k])==len(b[k]),k
        for x,y in zip(a[k],b[k]):
            d=abs(x-y)
            if k[1]=='Lcl Rotation': d=abs((x-y+180)%360-180)
            maximum=max(maximum,d)
    return maximum

ref = audit(OLD/'A_Combat_WalkShoulder.fbx','v3_ref_final')
report = {'shoulders':{},'extras':{},'originals_unchanged':False}
for name in ('WalkShoulder','ShoulderUp','ShoulderDown'):
    path=SOURCE/('A_Combat_'+name+'.fbx')
    s=audit(path,'v3_'+name)
    a=curves(OLD/path.name); b=curves(path)
    unchanged=[k for k in a if k[0] not in ('Armature','mixamorig:RightArm','mixamorig:RightForeArm')
               and not (k[0]=='mixamorig:Hips' and k[1]=='Lcl Translation')]
    retained=delta(a,b,unchanged)
    hip={axis:b[('mixamorig:Hips','Lcl Translation','d|'+axis)] for axis in 'XYZ'}
    checks={'GlobalSettings':s['global']==ref['global'], 'Armature':s['models']['Armature']==ref['models']['Armature'],
            'hierarchy':sorted(s['hierarchy'])==sorted(ref['hierarchy']),
            'untouched_tracks_within_0.001':retained<.001,
            'hips_xz_constant_0.0001_cm':all(max(hip[t])-min(hip[t])<.0001 for t in 'XZ'),
            'hips_y_bob_preserved_0.0001_cm':delta(a,b,[('mixamorig:Hips','Lcl Translation','d|Y')])<.0001}
    report['shoulders'][name]={'checks':checks,'max_untouched_delta':retained,
        'hips_ranges_cm':{axis:[min(v),max(v)] for axis,v in hip.items()},
        'hips_first_rotation':[(c['axis'],c['values'][0]) for c in s['hips_rotation_curves']]}
    assert all(checks.values()),report['shoulders'][name]
for manifest in ('manifest_extra.json','manifest_cav.json'):
    for clip in json.loads((SOURCE/manifest).read_text())['clips']:
        path=SOURCE/clip['file']; before=ROOT/'Saved/Codex/axes_fix/before'/path.name
        a,b=curves(before),curves(path)
        keys=[k for k in a if k[0]!='Armature']
        d=delta(a,b,[k for k in keys if k[1]!='Lcl Rotation'])
        angle=0.0
        for bone in {k[0] for k in keys if k[1]=='Lcl Rotation'}:
            ka=[(bone,'Lcl Rotation','d|'+axis) for axis in 'XYZ']
            for i in range(len(a[ka[0]])):
                qa=Euler(tuple(math.radians(a[k][i]) for k in ka),'XYZ').to_quaternion()
                qb=Euler(tuple(math.radians(b[k][i]) for k in ka),'XYZ').to_quaternion()
                # abs(dot) treats q and -q as the same rotation.
                dot=min(1.0,abs(qa.dot(qb)))
                angle=max(angle,math.degrees(2*math.acos(dot)))
        report['extras'][clip['name']]={'max_translation_cm_or_scale_delta':d,
            'max_rotation_degrees':angle,'same_samples':all(len(a[k])==len(b[k]) for k in keys)}
        assert d<.001 and angle<.1,(clip['name'],d,angle)
# Compare real exported right-arm endpoints with the protected Walk and shoulder.
walk=curves(SOURCE/'A_Combat_Walk.fbx')
shoulder=curves(SOURCE/'A_Combat_WalkShoulder.fbx')
transitions={}
for name,ends in [('ShoulderUp',[(walk,0,0),(shoulder,-1,-1)]),
                  ('ShoulderDown',[(shoulder,0,0),(walk,-1,-1)])]:
    c=curves(SOURCE/('A_Combat_'+name+'.fbx'))
    endpoints=[]; adjacent=0.0
    for other,ci,oi in ends:
        errors=[]
        for bone in ('RightArm','RightForeArm','RightHand'):
            kk=[('mixamorig:'+bone,'Lcl Rotation','d|'+a) for a in 'XYZ']
            q0=Euler(tuple(math.radians(c[k][ci]) for k in kk),'XYZ').to_quaternion()
            q1=Euler(tuple(math.radians(other[k][oi]) for k in kk),'XYZ').to_quaternion()
            errors.append(math.degrees(2*math.acos(min(1,abs(q0.dot(q1))))))
        endpoints.append(max(errors))
    for bone in ('RightArm','RightForeArm','RightHand'):
        kk=[('mixamorig:'+bone,'Lcl Rotation','d|'+a) for a in 'XYZ']
        qs=[Euler(tuple(math.radians(c[k][i]) for k in kk),'XYZ').to_quaternion() for i in range(len(c[kk[0]]))]
        adjacent=max(adjacent,max(math.degrees(2*math.acos(min(1,abs(q0.dot(q1))))) for q0,q1 in zip(qs,qs[1:])))
    assert max(endpoints)<.1,(name,endpoints)
    assert adjacent<15,(name,adjacent)
    transitions[name]={'right_arm_endpoint_error_degrees':endpoints,'max_adjacent_frame_rotation_degrees':adjacent}
report['transitions']=transitions
hashes=json.loads((ROOT/'Saved/Codex/axes_fix/original_hashes_before.json').read_text())
assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in hashes.items())
report['originals_unchanged']=True
(OUT/'verification.json').write_text(json.dumps(report,indent=2))
print('V3_RAW_VERIFIED')
