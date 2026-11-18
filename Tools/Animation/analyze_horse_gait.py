"""Analyse af heste-gangklip: blender -b -P Tools/Animation/analyze_horse_gait.py -- fil1.fbx fil2.fbx ...
Skriver pr. hov frame-for-frame (fremad-position y/x, z) og et sammendrag: stance-hov-hastighed (lineaer fit af hoven mens den er nede),
skridtlaengde, loop-sutur (forskel sidste->foerste frame mod gennemsnitlig frame-til-frame-forskel), baekkenhoejde, hals/hoved-udsving."""
import bpy, math, os, sys
import numpy as np
FPS = 30
files = sys.argv[sys.argv.index('--') + 1:]
hooves = ['front_hoof.L', 'front_hoof.R', 'hind_hoof.L', 'hind_hoof.R']


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=path)
    r = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    act = r.animation_data.action
    try:
        r.animation_data.action_slot = act.slots[0]
    except Exception:
        pass
    s, e = map(int, act.frame_range)
    return r, s, e


for path in files:
    r, s, e = load(path)
    n = e - s + 1
    H = {h: [] for h in hooves}
    pel, head, neck, allpos = [], [], [], []
    pitch = {b: [] for b in ('neck_01', 'neck_02', 'head', 'pelvis', 'spine_02')}
    yaw = {b: [] for b in ('neck_02', 'head', 'spine_02')}
    for fr in range(s, e + 1):
        bpy.context.scene.frame_set(fr)
        W = r.matrix_world
        for h in hooves:
            H[h].append(np.array(W @ r.pose.bones[h].head) * 100)
        pel.append(np.array(W @ r.pose.bones['pelvis'].head) * 100)
        head.append(np.array(W @ r.pose.bones['head'].tail) * 100)
        neck.append(np.array(W @ r.pose.bones['neck_01'].head) * 100)
        for b in pitch:
            pbn = r.pose.bones[b]
            dv = np.array(r.matrix_world.to_3x3() @ (pbn.tail - pbn.head)); dv /= np.linalg.norm(dv)
            pitch[b].append(math.degrees(math.atan2(dv[2], math.hypot(dv[0], dv[1]))))
        for b in yaw:
            pbn = r.pose.bones[b]
            dv = np.array(r.matrix_world.to_3x3() @ (pbn.tail - pbn.head))
            yaw[b].append(math.degrees(math.atan2(dv[1], -dv[0])))
        allpos.append(np.concatenate([np.array(W @ pb.head) * 100 for pb in r.pose.bones]))
    allpos = np.array(allpos)
    # sutur: sidste->foerste mod typisk frame-til-frame
    d = np.linalg.norm(np.diff(allpos, axis=0), axis=1)
    seam = np.linalg.norm(allpos[0] - allpos[-1])
    print('FILE', os.path.basename(path), 'frames', n, 'len', round((n - 1) / FPS, 3), 'cycle', round(n / FPS, 3))
    print('  seam (sidste->foerste) %.1f  gennemsnit frame-til-frame %.1f  max %.1f  foerste==sidste: %s' % (seam, d.mean(), d.max(), 'DUBLET' if seam < 0.05 * d.mean() else 'nej'))
    # hovenes bevaegelsesretning: bredeste udsving i xy
    allh = np.array([H[h] for h in hooves])  # 4,n,3
    xy = allh[:, :, :2].reshape(-1, 2)
    c = xy - xy.mean(0)
    u, sv, vt = np.linalg.svd(c, full_matrices=False)
    fwd = vt[0]
    print('  fremad-akse', np.round(fwd, 2))
    sp_all, stride_all = [], []
    for h in hooves:
        P = np.array(H[h])
        z = P[:, 2]
        f = P[:, :2] @ fwd
        zmin = z.min()
        down = z < zmin + 1.5
        # runs (cyklisk)
        idx = np.where(down)[0]
        runs, cur = [], []
        for i in range(n):
            if down[i]:
                cur.append(i)
            elif cur:
                runs.append(cur); cur = []
        if cur:
            if runs and runs[0][0] == 0:
                runs[0] = cur + runs[0]  # cyklisk samling (indekser uden wrap-korrektion; bruges kun til laengde)
            else:
                runs.append(cur)
        sp = []
        for rr in runs:
            if len(rr) < 3: continue
            rr2 = sorted(rr) if rr[0] <= rr[-1] else rr
            seg = [i for i in rr]
            # wrap: hvis run indeholder baade lave og hoeje indekser, brug unwrap
            fs = []
            prev = None
            for i in seg:
                v = f[i]
                fs.append(v)
            t = np.array(seg, float)
            if t.max() - t.min() > n * 0.6:   # wrap
                t = np.where(t < n / 2, t + n, t)
            A = np.vstack([t / FPS, np.ones_like(t)]).T
            sl, ic = np.linalg.lstsq(A, np.array(fs), rcond=None)[0]
            res = np.max(np.abs(A @ np.array([sl, ic]) - np.array(fs)))
            sp.append((abs(sl), len(seg), res))
        stance_frac = down.sum() / n
        print('  %-14s hoevehoejde z %.1f-%.1f  stance-frac %.2f  stance-hastighed(lin fit) %s  glid(max afvigelse) %s' % (h, z.min(), z.max(), stance_frac,
              [round(a, 1) for a, _, _ in sp], [round(b, 2) for _, _, b in sp]))
        sp_all += [a for a, _, _ in sp]
        # skridtlaengde: fremad-udsving af hoven over cyklussen
        stride_all.append(f.max() - f.min())
    pel = np.array(pel); head = np.array(head); neck = np.array(neck)
    # hals/hoved: udsving af hoved-spids relativt til nakke (lodret og vandret)
    rel = head - neck
    print('  baekken z %.1f-%.1f (udsving %.1f)  ' % (pel[:, 2].min(), pel[:, 2].max(), np.ptp(pel[:, 2])),
          'hoved-spids rel. nakke: z-udsving %.1f, fremad-udsving %.1f; hoved-spids abs z %.1f-%.1f (udsving %.1f)' % (np.ptp(rel[:, 2]), np.ptp(rel[:, :2] @ fwd), head[:, 2].min(), head[:, 2].max(), np.ptp(head[:, 2])))
    print('  knogle-haeldning (pitch, grader) udsving over cyklus: ' + ', '.join('%s %.1f' % (b, np.ptp(v)) for b, v in pitch.items()))
    print('  yaw (sidelaens drejning, grader) udsving: ' + ', '.join('%s %.1f' % (b, np.ptp(v)) for b, v in yaw.items()))
    print('  hovenes fremad-udsving (skridtlaengde, cm): ', [round(x, 1) for x in stride_all], ' median stance-hastighed %.1f cm/s' % (np.median(sp_all) if sp_all else 0))
