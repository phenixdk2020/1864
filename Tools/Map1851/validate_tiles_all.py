"""Run the offline map validation suite and save a current evidence report."""
import datetime
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import subprocess
import sys
from build_tiles import ROOT,read,write,digest


def validate(root=ROOT):
    root=Path(root)
    jobs=['Tools/Map1851/'+n+'.py' for n in ('validate_tiles','validate_dem_tiles','validate_tile_edges','validate_tile_painter','validate_neighbours')]
    jobs+=['Tools/Countries/'+n+'.py' for n in ('validate_level_c','validate_officer_careers','validate_sweden_norway','validate_prussia')]
    results=[];env=dict(os.environ,PYTHONIOENCODING='utf-8')
    manifest_sha=digest(root/'Data/Campaign1851/MapTiles.json')
    assert not read(root/'Data/Campaign1851/MapTiles.json').get('painterV2InProgress'),'finish repaint before validation'
    def run(job):
        print('Kontrollerer '+job,flush=True)
        return job,subprocess.run([sys.executable,str(root/job)],cwd=root,env=env,capture_output=True,text=True,encoding='utf-8')
    with ThreadPoolExecutor(max_workers=2) as pool:
        for job,result in pool.map(run,jobs):
            print(result.stdout,flush=True)
            if result.stderr:print(result.stderr,flush=True)
            results.append(dict(file=job,exitCode=result.returncode,output=result.stdout,stderr=result.stderr))
            if result.returncode:raise AssertionError(job)
    assert digest(root/'Data/Campaign1851/MapTiles.json')==manifest_sha,'manifest changed during validation'
    manifest=read(root/'Data/Campaign1851/MapTiles.json')
    pngs=list((root/'Reference/Campaign1851/MapTiles').glob('*.png'))
    tile_result=json.loads(results[0]['output'].strip().splitlines()[-1])
    report=dict(date=str(datetime.date.today()),status='OK',manifestSHA256=digest(root/'Data/Campaign1851/MapTiles.json'),tileCount=len(manifest['tiles']),pngCount=len(pngs),pngBytes=sum(p.stat().st_size for p in pngs),gridSize=[2951,3932],gridCellKm=.71,legacyPixelsCompared=tile_result['legacyPixelsCompared'],monarchyPixelsCompared=tile_result['monarchyPixelsCompared'],unchangedInteriorSeaPixels=tile_result['unchangedInteriorSeaPixels'],originalForeignLandPixels=tile_result['originalForeignLandPixels'],repaintedOriginalForeignPixels=tile_result['repaintedOriginalForeignPixels'],landTone=tile_result['landTone'],passedValidators=[r['file'] for r in results],results=results)
    write(root/'Data/Campaign1851/MapTiles_Validation.json',report)
    print('ALLE 9 VALIDATORER OK; '+str(report['pngBytes'])+' PNG bytes',flush=True)
    return report

if __name__=='__main__':validate()
