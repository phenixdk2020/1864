import json, sys
from pathlib import Path
from io_scene_fbx import parse_fbx
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/Codex/shoulder_arms/raw_audit'
OUT.mkdir(parents=True,exist_ok=True)
def val(v):
    if isinstance(v,bytes): return v.decode('utf8','backslashreplace')
    if hasattr(v,'tolist'): return v.tolist()
    return v
def tree(e): return {'id':val(e.id),'props':[val(v) for v in e.props],'children':[tree(c) for c in e.elems]}
def child(e,n): return next((c for c in e.elems if c.id==n),None)
def props(e):
    p=child(e,b'Properties70')
    return {val(c.props[0]):[val(v) for v in c.props[4:]] for c in p.elems} if p else {}
def audit(path,label):
    root,ver=parse_fbx.parse(str(path))
    (OUT/(label+'.tree.json')).write_text(json.dumps(tree(root),indent=2))
    objects=child(root,b'Objects'); connections=child(root,b'Connections')
    models={c.props[0]:c for c in objects.elems if c.id==b'Model'}
    names={k:val(v.props[1]).split('\x00')[0] for k,v in models.items()}
    summary={'file':str(path),'version':ver,'global':props(child(root,b'GlobalSettings')),'models':{names[k]:props(v) for k,v in models.items()},'hierarchy':[],'poses':[],'hips_rotation_curves':[]}
    for c in connections.elems:
        if c.props[0]==b'OO' and c.props[1] in models:
            summary['hierarchy'].append([names[c.props[1]],names.get(c.props[2],c.props[2])])
    for o in objects.elems:
        if o.id==b'Pose': summary['poses'].append(tree(o))
    # Follow animation curve -> curve node -> Hips Lcl Rotation connections.
    hips=next(k for k,n in names.items() if n.endswith('Hips'))
    nodes=[c.props[1] for c in connections.elems if len(c.props)>3 and c.props[2]==hips and c.props[3]==b'Lcl Rotation']
    byid={o.props[0]:o for o in objects.elems if o.props}
    for c in connections.elems:
        if len(c.props)>3 and c.props[2] in nodes:
            curve=byid[c.props[1]]
            summary['hips_rotation_curves'].append({'axis':val(c.props[3]),'times':val(child(curve,b'KeyTime').props[0]),'values':val(child(curve,b'KeyValueFloat').props[0])})
    (OUT/(label+'.summary.json')).write_text(json.dumps(summary,indent=2))
    main={n:p for n,p in summary['models'].items() if n.endswith('Hips') or 'Armature' in n or n=='Root' or n=='root'}
    print(label,json.dumps({'global':summary['global'],'roots_hips':main,'top':summary['hierarchy'][:4],'poses':len(summary['poses']),'hips_first':[(x['axis'],x['values'][0]) for x in summary['hips_rotation_curves']]}))
    return summary
if __name__ == '__main__':
    items=[(ROOT/'Saved/Codex/shoulder_arms/old_clips/A_Combat_WalkShoulder.fbx','old'),(ROOT/'SourceAssets/InfantryCombat/A_Combat_WalkShoulder.fbx','new'),(ROOT/'SourceAssets/InfantryCombat/A_Combat_Walk.fbx','walk')]
    if '--' in sys.argv and sys.argv[sys.argv.index('--')+1:]:
        items=[(ROOT/'SourceAssets/InfantryCombat'/('A_Combat_'+n+'.fbx'),'fixed_'+n) for n in ['WalkShoulder','ShoulderUp','ShoulderDown']]
    else:
        for m in ['manifest_extra.json','manifest_cav.json']:
            items += [(ROOT/'SourceAssets/InfantryCombat'/c['file'],c['name']) for c in json.loads((ROOT/'SourceAssets/InfantryCombat'/m).read_text())['clips']]
    for p,l in items: audit(p,l)
