"""Per-frame contact check of the shoulder-arms rifle against the DEFORMED body mesh (the rifle mesh itself, SM_Musket scaled to L, is placed
exactly like preview_shoulder_v5.py / the C++ rule).
blender -b -P measure_shoulder_rifle.py -- <fbx> vec <up> <side> <fwd> <grip> [L=1.4]       Barrel = Up*u + Side*s + Fwd*f
blender -b -P measure_shoulder_rifle.py -- <fbx> pt  <fwd> <up> <side> <grip> [L=1.4]       Barrel = normalize(RightShoulder(bone head) + Fwd*a + Up*b + Side*c - Hand)
Forward = up x (RightShoulder - LeftShoulder) per frame. For every rifle vertex in the contact band (shoulder height -0.10 .. +0.04 m) the gap is
its distance in front of the body surface behind it (ray cast along the body forward axis against the deformed body mesh; negative = inside).
clearance = min gap over those vertices (0 = touching, <0 = inside/behind the shoulder). Also reported: lateral of the barrel at the shoulder
(from the spine midline), min forward offset of every barrel point from the chest band up vs the shoulder joint, muzzle height over head top."""
import bpy, sys, json, os
import numpy as np
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index('--') + 1:]
FBX, MODE = a[0], a[1]; C = list(map(float, a[2:6])); GRIP = float(a[5]); L = float(a[6]) if len(a) > 6 else 1.4
GLB = r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\SM_Musket.glb'
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
bodies = [o for o in bpy.data.objects if o.type == 'MESH']
armmask = []
for o in bodies:
    names = {g.index: g.name for g in o.vertex_groups}
    mk = []
    for v in o.data.vertices:
        best = max(v.groups, key=lambda g: g.weight).group if len(v.groups) else None
        n = names.get(best, '')
        mk.append(n.startswith('mixamorig:Right') and any(t in n for t in ('Arm', 'Hand')))   # right arm / forearm / hand / fingers
    armmask.append(np.array(mk))
armmask = np.concatenate(armmask)
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=GLB)
rifle = next(o for o in bpy.data.objects if o not in before and o.type == 'MESH')
bpy.context.view_layer.update()
pts = [rifle.matrix_world @ v.co for v in rifle.data.vertices]
lo = Vector((min(p[i] for p in pts) for i in range(3))); hi = Vector((max(p[i] for p in pts) for i in range(3)))
ext = hi - lo; axis = max(range(3), key=lambda i: ext[i]); ctr = (lo + hi) / 2
perm = {0: Matrix.Identity(4), 1: Matrix(((0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))), 2: Matrix(((0, 0, 1, 0), (0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 0, 1)))}[axis]
Mr = Matrix.Scale(L / ext[axis], 4) @ perm @ Matrix.Translation(-ctr) @ rifle.matrix_world
RV0 = np.array([(Mr @ v.co)[:] for v in rifle.data.vertices])           # rifle vertices in holder space (butt at -X, muzzle at +X)
_pts = [RV0]                                                            # densify: points along every edge every <= 1.5 cm (the rifle mesh is low-poly)
for ed in rifle.data.edges:
    p0, p1 = RV0[ed.vertices[0]], RV0[ed.vertices[1]]; k = int(np.linalg.norm(p1 - p0) / 0.015)
    if k > 0: _pts.append(p0 + (p1 - p0) * (np.arange(1, k)[:, None] / k))
RV = np.vstack([q for q in _pts if len(q)])
RV = np.hstack([RV, np.ones((len(RV), 1))])
start = round(rig.animation_data.action.frame_range[0]); end = round(rig.animation_data.action.frame_range[1])
H = lambda n: rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head
up = Vector((0, 0, 1)); out = []; bad = []
for f in range(start, end + 1):
    bpy.context.scene.frame_set(f); bpy.context.view_layer.update(); dg = bpy.context.evaluated_depsgraph_get()
    V = []; allv = []; allt = []; off = 0
    for m in bodies:
        e = m.evaluated_get(dg); me = e.to_mesh(); n = len(me.vertices)
        co = np.empty(n * 3); me.vertices.foreach_get('co', co); co = co.reshape(-1, 3)
        mw = np.array(m.matrix_world); wv = co @ mw[:3, :3].T + mw[:3, 3]; V.append(wv)
        me.calc_loop_triangles(); tri = np.empty(len(me.loop_triangles) * 3, dtype=np.int32); me.loop_triangles.foreach_get('vertices', tri)
        allv.append(wv); allt.append(tri.reshape(-1, 3) + off); off += n; e.to_mesh_clear()
    V = np.vstack(V)
    bvh = BVHTree.FromPolygons([tuple(v) for v in np.vstack(allv)], [tuple(t) for t in np.vstack(allt)])
    side = (H('RightShoulder') - H('LeftShoulder')).normalized(); fw = up.cross(side).normalized()
    sh = H('RightArm'); mid = (H('LeftArm') + H('RightArm')) / 2
    pb = rig.pose.bones['mixamorig:RightHand']; hand = rig.matrix_world @ ((pb.head + pb.tail) / 2)
    if MODE == 'vec': barrel = (up * C[0] + side * C[1] + fw * C[2]).normalized()
    else: barrel = (H('RightShoulder') + fw * C[0] + up * C[1] + side * C[2] - hand).normalized()   # anchored on the RightShoulder (clavicle) bone head
    z = (side - barrel * side.dot(barrel)).normalized(); y = z.cross(barrel)
    m = Matrix((barrel, y, z)).transposed().to_4x4()
    m.translation = hand - barrel * ((GRIP - 0.5) * L)
    W = (np.array(m) @ RV.T).T[:, :3]                                    # rifle vertices in world
    d = V - np.array(mid); lat = d @ np.array(side); fwd = d @ np.array(fw); zz = V[:, 2]
    rd = W - np.array(mid); rl = rd @ np.array(side); rf = rd @ np.array(fw); rz = W[:, 2]
    band = (rz > sh.z - 0.10) & (rz < sh.z + 0.04)
    def gap_of(pt):
        o = Vector(pt)
        h = bvh.ray_cast(o, fw, 0.4)
        if h[0] is not None and h[1].dot(fw) > 0: return -h[3]      # inside the body (the surface in front of it faces forward)
        h = bvh.ray_cast(o, -fw, 0.6)
        return h[3] if h[0] is not None else None
    gaps = [g for g in (gap_of(W[i]) for i in np.where(band)[0]) if g is not None]
    clr = min(gaps) if gaps else None
    inside = [g for g in (gap_of(W[i]) for i in np.where((rz > sh.z + 0.04) & (rz < sh.z + 0.45))[0][::3]) if g is not None]   # above the shoulder: neck/head
    min_all = min(inside) if inside else None
    if os.environ.get('DBG') and min_all is not None and min_all < 0:
        idx = [i for i in np.where((rz > sh.z + 0.04) & (rz < sh.z + 0.45))[0][::3] if gap_of(W[i]) is not None and gap_of(W[i]) < 0]
        i = idx[0]; q = Vector(W[i]); print('DBG frame', f - start, 'vertex rel RightArm fwd/up/lat-from-mid', round((q - sh).dot(fw), 3), round(q.z - sh.z, 3), round((q - mid).dot(side), 3), 'n', len(idx), 'hit', bvh.ray_cast(q, fw, 0.4)[3])
    # barrel centre line
    cl = [hand + barrel * (L * (k / 28 - GRIP)) for k in range(29)]
    offs = [(p - sh).dot(fw) for p in cl if p.z >= sh.z - 0.15]
    shp = hand + barrel * ((sh.z - hand.z) / barrel.z)                   # centre line at shoulder-joint height
    hb = pb; hh = rig.matrix_world @ hb.head; ht = rig.matrix_world @ hb.tail
    butt_c = m.translation + barrel * (-0.5 * L)                          # butt plate centre (rifle end at -X of the holder)
    seg = ht - hh; tt = max(0.0, min(1.0, (butt_c - hh).dot(seg) / seg.dot(seg))); bd = (butt_c - (hh + seg * tt)).length
    NA = V[~armmask]                                                      # body vertices that are NOT the right arm/hand (thigh, hip, pouch, scabbard, torso...)
    hc = np.array(hand)
    hand_clear = float(np.sqrt(((NA - hc) ** 2).sum(1)).min()) - 0.045    # hand sphere radius 4.5 cm
    low = W[(rz < sh.z - 0.30) & (np.linalg.norm(W - hc, axis=1) > 0.07)][::6]   # lower rifle: hip/thigh/pouch/scabbard zone (the chest contact above is judged by the shoulder clearance)
    NAs = NA[::2]
    rifle_low_clear = 9.0
    if len(low):
        for i0 in range(0, len(low), 40):
            dd = np.sqrt(((low[i0:i0 + 40, None, :] - NAs[None, :, :]) ** 2).sum(2)).min(1).min()
            rifle_low_clear = min(rifle_low_clear, float(dd))
    out_deg = float(np.degrees(np.arctan2(-barrel.dot(side), barrel.dot(up))))
    r = dict(frame=f - start, hand_clear=round(hand_clear, 3), rifle_low_clear=round(rifle_low_clear, 3), out_deg=round(out_deg, 1), butt_to_hand_bone=round(bd, 3), butt_to_hand_centre=round((butt_c - hand).length, 3), butt_below_hand=round((hand - butt_c).dot(barrel), 3), clearance=None if clr is None else round(float(clr), 3), min_gap_above=None if min_all is None else round(float(min_all), 3), n_contact_verts=len(gaps),
             lat_at_shoulder=round(float((shp - mid).dot(side)), 3), min_fwd_of_shoulder=round(min(offs), 3),
             muzzle_over_head=round(float(cl[-1].z - zz.max()), 3), hand_fwd=round((hand - sh).dot(fw), 3), tilt_back_deg=round(float(np.degrees(np.arcsin(-barrel.dot(fw)))), 1))
    out.append(r)
print('MEASURE ' + FBX.replace(chr(92), '/').split('/')[-1])
for r in out: print('  ', r)
