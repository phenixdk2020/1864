"""Lossless binary FBX structure audit and targeted curve repair. No Unreal dependency."""
import struct, zlib, json, hashlib, subprocess, copy, bisect
import sys
from pathlib import Path
from collections import Counter
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/Codex/bonetrack_audit'
OUT.mkdir(parents=True,exist_ok=True)
class Node:
    pass
def read(data):
    version=struct.unpack_from('<I',data,23)[0]
    assert version==7400
    def node(pos):
        end,count,size,nlen=struct.unpack_from('<IIIB',data,pos)
        if not end:return None,pos+13
        n=Node(); n.name=data[pos+13:pos+13+nlen]; pos+=13+nlen
        n.values=[]; n.raw=[]
        for _ in range(count):
            start=pos; typ=chr(data[pos]);pos+=1
            if typ in 'Y C I F D L'.split():
                fmt={'Y':'h','C':'?','I':'i','F':'f','D':'d','L':'q'}[typ]; value=struct.unpack_from('<'+fmt,data,pos)[0];pos+=struct.calcsize(fmt)
            elif typ in 'SR':
                size=struct.unpack_from('<I',data,pos)[0];pos+=4;value=data[pos:pos+size];pos+=size
            else:
                count,enc,size=struct.unpack_from('<III',data,pos);pos+=12;raw=data[pos:pos+size];pos+=size
                if enc:raw=zlib.decompress(raw)
                fmt={'f':'f','d':'d','l':'q','i':'i','b':'b','c':'b'}[typ];value=list(struct.unpack('<'+fmt*count,raw))
            n.values.append(value);n.raw.append(data[start:pos])
        n.children=[];n.sentinel=False
        while pos<end:
            c,pos=node(pos)
            if c is None:n.sentinel=True;break
            n.children.append(c)
        return n,end
    nodes=[];pos=27
    while True:
        n,pos=node(pos)
        if n is None:break
        nodes.append(n)
    return nodes,data[:27],data[pos:]
def encode(n,start):
    raw=b''.join(n.raw);pos=start+13+len(n.name)+len(raw);children=b''
    for c in n.children:
        b=encode(c,pos);children+=b;pos+=len(b)
    if n.sentinel:children+=bytes(13);pos+=13
    return struct.pack('<IIIB',pos,len(n.raw),len(raw),len(n.name))+n.name+raw+children
def write(doc):
    nodes,header,tail=doc;data=header
    for n in nodes:data+=encode(n,len(data))
    return data+bytes(13)+tail
def child(n,name):return next((x for x in n.children if x.name==name),None)
def top(doc,name):return next(x for x in doc[0] if x.name==name)
def clean(v):
    if isinstance(v,bytes):return v.decode('utf8','backslashreplace')
    if isinstance(v,list):return [clean(x) for x in v]
    return v
def tree(n):return dict(name=clean(n.name),values=clean(n.values),children=[tree(c) for c in n.children])
def tracks(doc):
    objects=top(doc,b'Objects').children;ids={n.values[0]:n for n in objects};cons=top(doc,b'Connections').children
    result={}
    for c in cons:
        v=c.values
        if len(v)>3 and v[2] in ids and ids[v[2]].name==b'Model' and ids[v[1]].name==b'AnimationCurveNode':
            name=ids[v[2]].values[1].split(b'\0')[0].decode()
            for e in cons:
                w=e.values
                if len(w)>3 and w[2]==v[1] and ids[w[1]].name==b'AnimationCurve':result[(name,v[3].decode(),w[3].decode())]=ids[w[1]]
    return result
def summary(doc):
    objects=top(doc,b'Objects').children;t=tracks(doc)
    samples={}
    for key,n in t.items():
        times=child(n,b'KeyTime').values[0];vals=child(n,b'KeyValueFloat').values[0]
        def sample(f):
            tick=times[0]+round(f*46186158000/30);i=max(0,min(len(times)-1,bisect.bisect_right(times,tick)-1))
            if i+1==len(times):return vals[i]
            a=(tick-times[i])/(times[i+1]-times[i]);return vals[i]*(1-a)+vals[i+1]*a
        samples['|'.join(key)]=[sample(f) for f in (0,10,24)]
    fields=[b'Lcl Translation',b'Lcl Rotation',b'Lcl Scaling',b'PreRotation',b'PostRotation',b'RotationOrder',b'InheritType',b'GeometricTranslation',b'GeometricRotation',b'GeometricScaling']
    defaults={b'Lcl Translation':[0,0,0],b'Lcl Rotation':[0,0,0],b'Lcl Scaling':[1,1,1],b'PreRotation':[0,0,0],b'PostRotation':[0,0,0],b'RotationOrder':[0],b'InheritType':[0],b'GeometricTranslation':[0,0,0],b'GeometricRotation':[0,0,0],b'GeometricScaling':[1,1,1]}
    effective=[]
    for n in objects:
        if n.name!=b'Model':continue
        pr={c.values[0]:c.values[4:] for c in child(n,b'Properties70').children}
        effective.append(dict(name=clean(n.values[1]),type=clean(n.values[2]),fields={clean(k):dict(value=pr.get(k,defaults[k]),explicit=k in pr) for k in fields}))
    return dict(counts=dict(Counter(n.name.decode() for n in objects)),global_settings=tree(top(doc,b'GlobalSettings')),models=[tree(n) for n in objects if n.name==b'Model'],effective_model_fields=effective,connections=tree(top(doc,b'Connections')),poses_geometry_deformers=[tree(n) for n in objects if n.name in (b'Pose',b'Geometry',b'Deformer')],samples_0_10_24=samples)
def digest(data):return hashlib.sha256(data).hexdigest()
def array(n,values,typ):
    fmt={'f':'f','l':'q'}[typ];raw=struct.pack('<'+fmt*len(values),*values)
    n.values=[values];n.raw=[typ.encode()+struct.pack('<III',len(values),0,len(raw))+raw]
def patch_curves(template,motion,keys):
    result=copy.deepcopy(template);dst=tracks(result);src=tracks(motion)
    assert set(dst)==set(src)
    for key in keys:dst[key].children=copy.deepcopy(src[key].children)
    return result
def compare(a,b,path=''):
    differences=[]
    if type(a)!=type(b):return [dict(path=path,HEAD=a,v3=b)]
    if isinstance(a,dict):
        for k in a.keys()|b.keys():differences+=compare(a.get(k),b.get(k),path+'/'+str(k))
    elif isinstance(a,list):
        if len(a)!=len(b):differences.append(dict(path=path+'/length',HEAD=len(a),v3=len(b)))
        for i,(x,y) in enumerate(zip(a,b)):differences+=compare(x,y,path+'/'+str(i))
    elif a!=b:differences.append(dict(path=path,HEAD=a,v3=b))
    return differences
def repair(old):
    source=ROOT/'SourceAssets/InfantryCombat'
    protected=[source/c['file'] for c in json.loads((source/'manifest.json').read_text())['clips']]
    protected += [ROOT/'Content/Units/Human/CombatAnimations'/('A_Combat_'+c['name']+'.uasset') for c in json.loads((source/'manifest.json').read_text())['clips']]
    before={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in protected}
    clips=[]
    for m in ['manifest_extra.json','manifest_cav.json']:clips+=json.loads((source/m).read_text())['clips']
    clips += [dict(name=n,file='A_Combat_'+n+'.fbx',frames=42) for n in ['WalkShoulder','ShoulderUp','ShoulderDown']]
    backup=OUT/'before';backup.mkdir(exist_ok=True)
    # Stage and verify every file before touching any source FBX.
    staged=OUT/'patched';staged.mkdir(exist_ok=True);report=[]
    for clip in clips:
        path=source/clip['file'];saved=backup/path.name
        if not saved.exists():saved.write_bytes(path.read_bytes())
        motion=read(saved.read_bytes());src=tracks(motion);dst=tracks(old)
        shoulder=clip['name'] in ['WalkShoulder','ShoulderUp','ShoulderDown']
        if shoulder:
            baseline=read(subprocess.check_output(['git','show','HEAD:SourceAssets/InfantryCombat/'+path.name],cwd=ROOT))
            # Preserve the individual transition's original non-right-arm motion.
            result=patch_curves(old,baseline,[k for k in dst if k[0]!='Armature'])
            keys=[k for k in dst if k[0].startswith('mixamorig:Right') and not any(x in k[0] for x in ['UpLeg','Leg','Foot','Toe'])]
            result=patch_curves(result,motion,keys)
            tr=tracks(result)
            for axis in ['d|X','d|Z']:
                curve=tr[('mixamorig:Hips','Lcl Translation',axis)];values=child(curve,b'KeyValueFloat').values[0]
                array(child(curve,b'KeyValueFloat'),[values[0]]*len(values),'f')
        else:
            keys=[k for k in dst if k[0]!='Armature']
            result=patch_curves(old,motion,keys)
        # Root is exactly HEAD, extended with its constant samples for the clip duration.
        rt=tracks(result);times=child(src[('mixamorig:Hips','Lcl Translation','d|X')],b'KeyTime').values[0]
        if not shoulder:
            for key,n in rt.items():
                if key[0]!='Armature':continue
                orig=dst[key];value=child(orig,b'KeyValueFloat').values[0][0]
                # Use source timing and key attribute counts, but HEAD root values.
                n.children=copy.deepcopy(src[key].children)
                array(child(n,b'KeyValueFloat'),[value]*len(times),'f')
            for n in top(result,b'Objects').children:
                if n.name==b'AnimationStack':
                    pr=child(n,b'Properties70')
                    for prop in pr.children:
                        if prop.values[0] in [b'LocalStart',b'ReferenceStart',b'LocalStop',b'ReferenceStop']:
                            val=times[0] if prop.values[0].endswith(b'Start') else times[-1]
                            prop.values[-1]=val;prop.raw[-1]=b'L'+struct.pack('<q',val)
        data=write(result);parsed=read(data)
        # Complete structure equality after excluding only AnimationCurve payload and duration.
        def structure(doc):
            d=copy.deepcopy(doc)
            for n in top(d,b'Objects').children:
                if n.name==b'AnimationCurve':n.children=[]
                if n.name==b'AnimationStack':n.children=[]
            return [tree(n) for n in d[0]]
        assert structure(parsed)==structure(old),clip['name']
        finaltracks=tracks(parsed)
        assert all(len(child(n,b'KeyTime').values[0])==clip['frames'] for n in finaltracks.values()),clip['name']
        assert len(times)==clip['frames']
        if not shoulder:
            assert all(tree(finaltracks[k])['children']==tree(src[k])['children'] for k in keys)
        else:
            base=tracks(baseline)
            allowed=set(keys)|{('mixamorig:Hips','Lcl Translation',a) for a in ['d|X','d|Z']}
            assert all(tree(finaltracks[k])['children']==tree(base[k])['children'] for k in base if k not in allowed)
            for axis in ['d|X','d|Z']:
                vals=child(finaltracks[('mixamorig:Hips','Lcl Translation',axis)],b'KeyValueFloat').values[0];assert len(set(vals))==1
        (staged/path.name).write_bytes(data)
        (OUT/(clip['name']+'.after.json')).write_text(json.dumps(summary(parsed),indent=2))
        report.append(dict(clip=path.stem,frames=clip['frames'],before_sha256=digest(saved.read_bytes()),after_sha256=digest(data),exact_HEAD_structure=True,non_root_motion_preserved=not shoulder,only_right_chain_and_hips_XZ_changed_from_own_HEAD=shoulder,method='HEAD WalkShoulder binary template; curve payload substitution only; duration adapted'))
    for clip in clips:(source/clip['file']).write_bytes((staged/clip['file']).read_bytes())
    after={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in protected};assert before==after
    (OUT/'protected_originals.json').write_text(json.dumps(dict(before=before,after=after,equal=True),indent=2))
    (OUT/'verification.json').write_text(json.dumps(report,indent=2))
    (OUT/'reimport_names.txt').write_text('\n'.join(c['clip'] for c in report)+'\n')
    print('VERIFIED_AND_PATCHED',len(report))
def main():
    source=ROOT/'SourceAssets/InfantryCombat';name='A_Combat_WalkShoulder.fbx'
    head=subprocess.check_output(['git','show','HEAD:SourceAssets/InfantryCombat/'+name],cwd=ROOT)
    (OUT/'HEAD_WalkShoulder.fbx').write_bytes(head)
    v3path=OUT/'before'/name
    old=read(head);v3=read((v3path if v3path.exists() else source/name).read_bytes())
    for label,doc in [('HEAD',old),('v3',v3)]:
        (OUT/(label+'.json')).write_text(json.dumps(summary(doc),indent=2))
        (OUT/(label+'.full_tree.json')).write_text(json.dumps([tree(n) for n in doc[0]],indent=2))
    (OUT/'HEAD_v3_summary_differences.json').write_text(json.dumps(compare(summary(old),summary(v3)),indent=2))
    assert write(old)==head,'Writer must reproduce HEAD byte for byte'
    print('HEAD_ROUNDTRIP_BYTE_IDENTICAL',digest(head))
    if '--apply' in sys.argv:repair(old)
if __name__=='__main__':main()
