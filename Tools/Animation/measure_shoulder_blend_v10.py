"""v10 per-frame check of ShoulderUp / ShoulderDown with the phase-blended rifle rule.
blender -b -P measure_shoulder_blend_v10.py -- <fbx> <up|down> [smooth|linear] [L=1.4]
Phase u = i/(N-1) for Up, 1 - i/(N-1) for Down.  Rule A: barrel from the right-hand centre towards the left-hand centre, grip 22 %.
Rule B: barrel from the right-hand centre to Contact = RightShoulder + Fwd*0.117 + Up*0.05 + Side*0.10, grip 3 %.
Blend: dir = slerp(A, B, w(u)); grip = lerp(0.22, 0.03, w(u)); the grip point is the right-hand centre.  w = smoothstep(clamp((u-0.10)/0.65)) ('smooth') or u ('linear').
Per frame: left-hand centre to the blended barrel line (and its position along the rifle), signed distance of the rifle mesh to the deformed body (<0 = inside),
split into shoulder / head (head, neck, collar, shako) / backpack / other, and the hand positions relative to the right shoulder joint."""
import bpy, sys, json, math
import numpy as np
from mathutils import Vector, Matrix, Quaternion
from mathutils.bvhtree import BVHTree
a = sys.argv[sys.argv.index('--') + 1:]
FBX, MODE = a[0], a[1]
WM = a[2] if len(a) > 2 else 'smooth'
L = float(a[3]) if len(a) > 3 else 1.4
CA, CB, CC = 0.117, 0.05, 0.10
GA, GB = 0.22, 0.03
GLB = r'R:\Onedrive\Dokumenter\Unreal Projects\Game1864\Reference\Units\MultiView\3D figurer\SM_Musket.glb'


def S_(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3 - 2 * t)


def wgt(u):
    return u if WM == 'linear' else S_((u - 0.10) / 0.65)


bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
bodies = [o for o in bpy.data.objects if o.type == 'MESH']
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=GLB)
rifle = next(o for o in bpy.data.objects if o not in before and o.type == 'MESH')
bpy.context.view_layer.update()
pts = [rifle.matrix_world @ v.co for v in rifle.data.vertices]
lo = Vector((min(p[i] for p in pts) for i in range(3)))
hi = Vector((max(p[i] for p in pts) for i in range(3)))
ext = hi - lo
axis = max(range(3), key=lambda i: ext[i])
ctr = (lo + hi) / 2
perm = {0: Matrix.Identity(4), 1: Matrix(((0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))),
        2: Matrix(((0, 0, 1, 0), (0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 0, 1)))}[axis]
Mr = Matrix.Scale(L / ext[axis], 4) @ perm @ Matrix.Translation(-ctr) @ rifle.matrix_world
RV0 = np.array([(Mr @ v.co)[:] for v in rifle.data.vertices])
_p = [RV0]
for ed in rifle.data.edges:
    p0, p1 = RV0[ed.vertices[0]], RV0[ed.vertices[1]]
    k = int(np.linalg.norm(p1 - p0) / 0.02)
    if k > 0:
        _p.append(p0 + (p1 - p0) * (np.arange(1, k)[:, None] / k))
RV = np.vstack([q for q in _p if len(q)])
RV = np.hstack([RV, np.ones((len(RV), 1))])
lmask = []     # the LEFT hand + forearm are excluded from the body mesh: the left hand legitimately touches the rifle while gripping / letting go
for o in bodies:
    names = {g.index: g.name for g in o.vertex_groups}
    for v in o.data.vertices:
        best = max(v.groups, key=lambda g: g.weight).group if len(v.groups) else None
        n_ = names.get(best, '')
        lmask.append(n_.startswith('mixamorig:Left') and any(t in n_ for t in ('Hand', 'ForeArm')))
lmask = np.array(lmask)
start = round(rig.animation_data.action.frame_range[0])
end = round(rig.animation_data.action.frame_range[1])
NFR = end - start + 1
H = lambda n: rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head
HC = lambda s: rig.matrix_world @ ((rig.pose.bones['mixamorig:%sHand' % s].head + rig.pose.bones['mixamorig:%sHand' % s].tail) / 2)
up = Vector((0, 0, 1))
out = []
for f in range(start, end + 1):
    i = f - start
    u = i / (NFR - 1) if MODE == 'up' else 1 - i / (NFR - 1)
    bpy.context.scene.frame_set(f)
    bpy.context.view_layer.update()
    dg_ = bpy.context.evaluated_depsgraph_get()
    allv, allt, off = [], [], 0
    for m in bodies:
        e = m.evaluated_get(dg_)
        me = e.to_mesh()
        n = len(me.vertices)
        co = np.empty(n * 3)
        me.vertices.foreach_get('co', co)
        co = co.reshape(-1, 3)
        mw = np.array(m.matrix_world)
        allv.append(co @ mw[:3, :3].T + mw[:3, 3])
        me.calc_loop_triangles()
        tri = np.empty(len(me.loop_triangles) * 3, dtype=np.int32)
        me.loop_triangles.foreach_get('vertices', tri)
        allt.append(tri.reshape(-1, 3) + off)
        off += n
        e.to_mesh_clear()
    AV = np.vstack(allv)
    TT = np.vstack(allt)
    TT = TT[~lmask[TT].any(axis=1)]
    bvh = BVHTree.FromPolygons([tuple(v) for v in AV], [tuple(t) for t in TT])
    rs, ls = H('RightShoulder'), H('LeftShoulder')
    side = (rs - ls).normalized()
    fw = up.cross(side).normalized()
    sh = H('RightArm')
    mid = (H('LeftArm') + sh) / 2
    Rc, Lc = HC('Right'), HC('Left')
    dA = (Lc - Rc).normalized()
    dB = ((rs + fw * CA + up * CB + side * CC) - Rc).normalized()
    w = wgt(u)
    d = Quaternion().slerp(dA.rotation_difference(dB), w) @ dA
    g = GA + (GB - GA) * w
    z = (side - d * side.dot(d)).normalized()
    y = z.cross(d)
    m = Matrix((d, y, z)).transposed().to_4x4()
    m.translation = Rc - d * ((g - 0.5) * L)
    W = (np.array(m) @ RV.T).T[:, :3]
    tl = (Lc - Rc).dot(d)
    left_line = ((Lc - Rc) - d * tl).length
    gripping = left_line < 0.08 and 0.0 < tl < (1 - g) * L
    res = {'sh': 9.0, 'head': 9.0, 'back': 9.0, 'other': 9.0}
    worst = {}
    for wpt in W[::2]:
        q = Vector(wpt)
        if (q - Rc).length < 0.12 or (gripping and (q - Lc).length < 0.12):
            continue
        loc, nor, idx, dist = bvh.find_nearest(q)
        if loc is None:
            continue
        sd = dist if (dist > 0.12 or (q - loc).dot(nor) >= 0) else -dist      # the inside test is only meaningful close to the surface
        if (q - sh).length < 0.15:
            key = 'sh'
        elif loc.z > sh.z + 0.10:
            key = 'head'
        elif (loc - mid).dot(fw) < -0.08:
            key = 'back'
        else:
            key = 'other'
        if sd < res[key]:
            res[key] = sd
            worst[key] = tuple(round(v, 2) for v in ((q - sh).dot(fw), (q - sh).z, (q - sh).dot(side)))
    el, wr = H('RightForeArm'), H('RightHand')
    a1, b1 = (sh - el).normalized(), (wr - el).normalized()
    out.append(dict(frame=i, u=round(u, 3), w=round(w, 3), left_to_line=round(left_line, 3), left_t=round(tl, 3), gripping=bool(gripping),
                    sh=round(res['sh'], 3), head=round(res['head'], 3), back=round(res['back'], 3), other=round(res['other'], 3),
                    lean=round(math.degrees(math.atan2(-d.dot(fw), d.dot(up))), 1), front=round(math.degrees(math.atan2(d.dot(side), d.dot(up))), 1),
                    elbow=round(math.degrees(a1.angle(b1)), 1), grip=round(g, 3), worst=worst,
                    rh=tuple(round(v, 3) for v in ((H('RightHand') - sh).dot(fw), (H('RightHand') - sh).z, (H('RightHand') - sh).dot(side))),
                    lh=tuple(round(v, 3) for v in ((H('LeftHand') - sh).dot(fw), (H('LeftHand') - sh).z, (H('LeftHand') - sh).dot(side)))))
print('MEASURE ' + FBX.replace(chr(92), '/').split('/')[-1] + ' ' + MODE + ' ' + WM)
for r in out:
    print('  ', json.dumps(r))
