"""v9 per-frame check of the shoulder-arms rifle (reference pose: elbow ~90 deg, rifle resting ON the right shoulder, leaning back).
blender -b -P measure_shoulder_rifle_v9.py -- <fbx> <a_fwd> <b_up> <c_side> <grip> [L=1.4] [frame step]
Rule: Barrel = normalize(RightShoulder(bone head) + Fwd*a + Up*b + Side*c - HandCentre); Fwd = up x (RightShoulder - LeftShoulder).
Per frame: signed distance (m, <0 = inside the mesh) from every rifle surface point (SM_Musket scaled to L, densified) to the deformed body mesh
(find_nearest on a BVH). Points within 0.12 m of the hand centre are the grip and are ignored.
  shoulder_clear = min over rifle points within 0.15 m of the RightArm joint (the shoulder top / deltoid contact)
  head / back / other = min over the remaining rifle points, classified by where the nearest body point lies
      head = nearest point above joint+0.10 m (head, neck, collar, shako); back = nearest point behind the back plane (backpack, blanket roll)
Also: lean (side view, + = muzzle back), front angle (+ = muzzle towards the soldier's right), elbow interior angle, butt-to-hand distances."""
import bpy, sys, json, os
import numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index('--') + 1:]
FBX = a[0]; CA, CB, CC, GRIP = map(float, a[1:5]); L = float(a[5]) if len(a) > 5 else 1.4; STEP = int(a[6]) if len(a) > 6 else 1
GLB = r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\SM_Musket.glb'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
bodies = [o for o in bpy.data.objects if o.type == 'MESH']
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=GLB)
rifle = next(o for o in bpy.data.objects if o not in before and o.type == 'MESH')
bpy.context.view_layer.update()
pts = [rifle.matrix_world @ v.co for v in rifle.data.vertices]
lo = Vector((min(p[i] for p in pts) for i in range(3))); hi = Vector((max(p[i] for p in pts) for i in range(3)))
ext = hi - lo; axis = max(range(3), key=lambda i: ext[i]); ctr = (lo + hi) / 2
perm = {0: Matrix.Identity(4), 1: Matrix(((0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))), 2: Matrix(((0, 0, 1, 0), (0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 0, 1)))}[axis]
Mr = Matrix.Scale(L / ext[axis], 4) @ perm @ Matrix.Translation(-ctr) @ rifle.matrix_world
RV0 = np.array([(Mr @ v.co)[:] for v in rifle.data.vertices])
_pts = [RV0]
for ed in rifle.data.edges:
    p0, p1 = RV0[ed.vertices[0]], RV0[ed.vertices[1]]; k = int(np.linalg.norm(p1 - p0) / 0.02)
    if k > 0: _pts.append(p0 + (p1 - p0) * (np.arange(1, k)[:, None] / k))
RV = np.vstack([q for q in _pts if len(q)]); RV = np.hstack([RV, np.ones((len(RV), 1))])
start = round(rig.animation_data.action.frame_range[0]); end = round(rig.animation_data.action.frame_range[1])
H = lambda n: rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head
up = Vector((0, 0, 1)); out = []; scan = {}
for f in range(start, end + 1, STEP):
    bpy.context.scene.frame_set(f); bpy.context.view_layer.update(); dg = bpy.context.evaluated_depsgraph_get()
    allv = []; allt = []; off = 0
    for m in bodies:
        e = m.evaluated_get(dg); me = e.to_mesh(); n = len(me.vertices)
        co = np.empty(n * 3); me.vertices.foreach_get('co', co); co = co.reshape(-1, 3)
        mw = np.array(m.matrix_world); allv.append(co @ mw[:3, :3].T + mw[:3, 3])
        me.calc_loop_triangles(); tri = np.empty(len(me.loop_triangles) * 3, dtype=np.int32); me.loop_triangles.foreach_get('vertices', tri)
        allt.append(tri.reshape(-1, 3) + off); off += n; e.to_mesh_clear()
    bvh = BVHTree.FromPolygons([tuple(v) for v in np.vstack(allv)], [tuple(t) for t in np.vstack(allt)])
    def eval_rule(CA, CB, CC):
        side = (H('RightShoulder') - H('LeftShoulder')).normalized(); fw = up.cross(side).normalized()
        sh = H('RightArm'); rs = H('RightShoulder'); mid = (H('LeftArm') + sh) / 2
        pb = rig.pose.bones['mixamorig:RightHand']; hand = rig.matrix_world @ ((pb.head + pb.tail) / 2)
        barrel = (rs + fw * CA + up * CB + side * CC - hand).normalized()
        z = (side - barrel * side.dot(barrel)).normalized(); y = z.cross(barrel)
        m = Matrix((barrel, y, z)).transposed().to_4x4(); m.translation = hand - barrel * ((GRIP - 0.5) * L)
        W = (np.array(m) @ RV.T).T[:, :3]
        AV = np.vstack(allv)
        dd = AV - np.array(mid); packm = ((dd @ np.array(fw)) < -0.06) & (AV[:, 2] > sh.z - 0.25) & (AV[:, 2] < sh.z + 0.40)   # backpack, blanket roll, straps (behind the back plane)
        PK = AV[packm][::2]
        RW = W[(W[:, 2] > sh.z - 0.1)][::5]
        pack_clear = 9.0
        for i0 in range(0, len(RW), 60):
            pack_clear = min(pack_clear, float(np.sqrt(((RW[i0:i0 + 60, None, :] - PK[None, :, :]) ** 2).sum(2)).min()))
        res = {'sh': 9.0, 'head': 9.0, 'back': 9.0, 'other': 9.0}; worst = {}
        for w in W[::2]:
            q = Vector(w)
            if (q - hand).length < 0.20: continue
            loc, nor, idx, dist = bvh.find_nearest(q)
            if loc is None: continue
            sd = dist if (q - loc).dot(nor) >= 0 else -dist
            if (q - sh).length < 0.15: key = 'sh'
            elif loc.z > sh.z + 0.10: key = 'head'
            elif (loc - mid).dot(fw) < -0.08: key = 'back'
            else: key = 'other'
            if sd < res[key]: res[key] = sd; worst[key] = tuple(round(v, 2) for v in ((q - sh).dot(fw), (q - sh).z, (q - sh).dot(side)))
        el, wr = H('RightForeArm'), H('RightHand')
        a1, b1 = (sh - el).normalized(), (wr - el).normalized()
        ra = rig.matrix_world @ rig.pose.bones['mixamorig:RightHand'].tail
        butt_c = m.translation + barrel * (-0.5 * L)
        hh, ht = rig.matrix_world @ pb.head, rig.matrix_world @ pb.tail; seg = ht - hh
        tt = max(0.0, min(1.0, (butt_c - hh).dot(seg) / seg.dot(seg)))
        r = dict(frame=f - start, sh_clear=round(res['sh'], 3), head_clear=round(res['head'], 3), back_clear=round(res['back'], 3), pack_clear=round(pack_clear, 3), other_clear=round(res['other'], 3),
                 lean_side=round(float(np.degrees(np.arctan2(-barrel.dot(fw), barrel.dot(up)))), 1), front_deg=round(float(np.degrees(np.arctan2(barrel.dot(side), barrel.dot(up)))), 1),
                 elbow=round(float(np.degrees(np.arccos(max(-1, min(1, a1.dot(b1)))))), 1), butt_hand_centre=round((butt_c - hand).length, 3),
                 butt_hand_bone=round((butt_c - (hh + seg * tt)).length, 3), worst=worst, hand_rel=tuple(round(v, 3) for v in ((hand - sh).dot(fw), (hand - sh).z, (hand - sh).dot(side))))
        return r
    if os.environ.get('SCAN'):
        cands = [tuple(float(x) for x in c.split(',')) for c in os.environ['SCAN'].split(';')]
        for c in cands:
            rr = eval_rule(*c)
            scan.setdefault(c, []).append(rr)
    else:
        out.append(eval_rule(CA, CB, CC))

if os.environ.get('SCAN'):
    for c, rs_ in scan.items():
        print('SCAN', c, 'sh', [r['sh_clear'] for r in rs_], 'head', min(r['head_clear'] for r in rs_), 'back', min(r['pack_clear'] for r in rs_), 'other', min(r['other_clear'] for r in rs_), 'lean', [r['lean_side'] for r in rs_], 'worst', rs_[0]['worst'], 'front', [r['front_deg'] for r in rs_])
else:
    print('MEASURE ' + FBX.replace(chr(92), '/').split('/')[-1])
    for r in out: print('  ', r)
