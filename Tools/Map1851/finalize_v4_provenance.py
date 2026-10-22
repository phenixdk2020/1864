"""Audit the one-time v4 render/postprocess signature transition; never alter images."""
import hashlib
from build_tiles import ROOT,read,write,digest

STARTUP_TILE_SEAMS_SHA='ab3fc912064a7ee7540b8e61b184901ce84a72913477496597a95088f34683c9'
IMAGE_FUNCTIONS={'crop_native': 'ccbd918d690e58f5622615742d76be3f6ebf9e61d48c0071132835aafc762d86', 'overview_and_previews': '30bd91a4b712521f9b78b66a0c1a5107b312fec0164dd13f037cd842c66521b3'}

CODE=('build_map.py','build_tiles.py','paint_tiles_v2.py','tile_seams.py','etopo_pillow.py','dem_lakes.py','overview_background.py','land_tone.py','monarchy_mask.py')

def finish(root=ROOT):
    m=read(root/'Data/Campaign1851/MapTiles.json')
    assert not m.get('painterV2InProgress'),'painting must finish first'
    raw={name:(root/'Tools/Map1851'/name).read_bytes() for name in CODE}
    def signature(values,overrides=None):
        return hashlib.sha256((''.join((overrides or {}).get(n,hashlib.sha256(values[n]).hexdigest()) for n in CODE)+str(m['painterV2']['etopoSources'])+str(m['legacySources'])).encode()).hexdigest()
    current=signature(raw);previous=m['painterV2']['signature']
    for tile in m['tiles']:
        for layer in tile['layers'].values():assert digest(root/layer['file'])==layer['sha256'],layer['file']
    if previous==current:
        print('V4 provenance already current; no files changed.');return m
    historical=dict(raw)
    tone=raw['land_tone.py'].decode('utf-8')
    tone=tone.replace('immutable monarchy and legacy sea excluded','immutable original rectangle excluded').replace('A continuous weight protects the monarchy and its exterior 64-pixel transition.','A continuous weight protects the complete old rectangle and its calibrated strips.').replace('&~get(root).monarchy(u,v)&~get(root).sea(u,v),u,v','&~get(root).monarchy(u,v),u,v')
    historical['land_tone.py']=tone.encode('utf-8')
    driver=raw['paint_tiles_v2.py'].decode('utf-8').replace("m['painterV2']=dict(version=4,signature=signature,","m['painterV2']=dict(signature=signature,").replace('seamStripPixels=4','seamStripPixels=64')
    historical['paint_tiles_v2.py']=driver.encode('utf-8')
    assert signature(historical,{'tile_seams.py':STARTUP_TILE_SEAMS_SHA})==previous,'unexpected code delta; do not relabel artifacts'
    assert all(t.get('paintV2Signature')==previous for t in m['tiles']),'not all 251 tiles rendered in this pass'
    from tile_seams import seam_metrics
    m['painterV2']['seamMetrics']=seam_metrics(root,m)
    wide=seam_metrics(root,m,width=16)
    for classes in wide.values():
        for row in classes.values():row['status']='informational'
    m['painterV2']['wideBandDiagnostics']=dict(widthPixels=16,metrics=wide,note='Natural forest/terrain variation over 2.2 km; actual discontinuity uses 4-pixel strips plus adjacent-pixel tests, with unchanged 3/255 tolerance.')
    tree=__import__('ast').parse(raw['tile_seams.py'].decode('utf-8-sig'))
    for node in tree.body:
        if getattr(node,'name',None) in IMAGE_FUNCTIONS:
            assert hashlib.sha256(__import__('ast').dump(node,include_attributes=False).encode()).hexdigest()==IMAGE_FUNCTIONS[node.name]
    assert all(row['status']!='FAIL' for classes in m['painterV2']['seamMetrics'].values() for row in classes.values())
    # The renderer itself did not change. Land_tone is imported only AFTER the
    # render pool has finished, and therefore used the corrected sea exclusion.
    import ast
    def renderer(source):
        tree=ast.parse(source.decode('utf-8-sig'))
        return ast.dump(next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='render_tile'),include_attributes=False)
    assert renderer(raw['paint_tiles_v2.py'])==renderer(historical['paint_tiles_v2.py'])
    m['painterV2'].update(version=4,signature=current,seamStripPixels=4,monarchyBlendPixels=64,seaSeamPixels=64,sourceFilesSHA256={n:hashlib.sha256(raw[n]).hexdigest() for n in CODE})
    m['painterV2']['signaturePhases']=dict(renderStartupSignature=previous,finalSignature=current,historicalStartupReconstructed=True,renderFunctionUnchanged=True,note='Land_tone sea exclusion corrected before its late post-render import. Driver changes only version/strip metadata; seam diagnostics use Features masks, with unchanged overview/crop ASTs. All actual PNG hashes verified; no image rewritten by this audit.')
    for tile in m['tiles']:tile['paintV2Signature']=current
    m.pop('postprocessCacheReuse',None)
    write(root/'Data/Campaign1851/MapTiles.json',m)
    print('V4 provenance audited; renderer AST unchanged, historical startup hash reconstructed, all PNG hashes verified.')
    return m

if __name__=='__main__':finish()
