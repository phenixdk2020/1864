"""Retter halen paa den riggede hest (SK_Horse_1864) og eksporterer FBX med de samme indstillinger som export_horse_mesh_b1.py.
blender -b <Horse_1_rigged_v3.blend> -P fix_horse_tail.py [-- --no-export]
Blend-filen aendres IKKE (der gemmes ikke). Skelet, knoglenavne, skala, akser og al anden geometri er uroert.
Hvad der goeres paa Horse_1_Mesh.001 i hukommelsen:
 1. Det gamle haalemesh (ca. 7.300 flader: en tyk, aaben skal af graa haar, vaegtet paa tail_01-03 og delvist paa laar/skinneben/baekken) fjernes.
    Flader paa bagbenenes bagside, der blot er fejlvaegtet paa tail_02/03, bevares og faar vaegte fra naermeste ben-/baekken-vertex.
 2. En ny, lukket, tilspidset hale (tube, haarlokke-bule) bygges langs haleknoglerne, vaegtet blodt over tail_01-03 (og baekkenet foroven).
 3. Halens UV'er flyttes til en moerk, ensfarvet plet i den eksisterende tekstur (ingen teksturaendring).
Foer overskrivning gemmes den gamle FBX som Horse_1_rigged_v3.prev.fbx (kun hvis den ikke findes)."""
import bpy, bmesh, sys, shutil, math
import numpy as np
from pathlib import Path
from mathutils import Vector as V
from mathutils.kdtree import KDTree

DARK_UV = (0.0742, 0.4262)   # texel (150..154, 870..876): ensfarvet moerk gra-brun (lineaer 0.07/0.05/0.05), ligger paa bagbenets skinneben
TAIL_BONES = ('tail_01', 'tail_02', 'tail_03')

def apply_fix(mesh_obj):
    me = mesh_obj.data
    names = [g.name for g in mesh_obj.vertex_groups]
    gi = {n: i for i, n in enumerate(names)}
    bm = bmesh.new(); bm.from_mesh(me)
    dl = bm.verts.layers.deform.active
    uvl = bm.loops.layers.uv.active
    def dom(v):
        return names[max(v[dl].items(), key=lambda kv: kv[1])[0]] if v[dl] else None
    # texture for the colour test (non-brown = hair)
    img = next(n for n in mesh_obj.material_slots[0].material.node_tree.nodes if n.type == 'TEX_IMAGE').image
    W, H = img.size
    px = np.array(img.pixels[:]).reshape(H, W, 4)[:, :, :3]
    def face_sat(f):
        uv = sum((l[uvl].uv for l in f.loops), V((0, 0))) / len(f.loops)
        c = px[min(H - 1, int(uv.y * H)), min(W - 1, int(uv.x * W))]
        return float((c.max() - c.min()) / (c.max() + 1e-6)), float(c.mean())
    tf = [f for f in bm.faces if all(dom(v) in TAIL_BONES for v in f.verts)]
    S = set(tf); seen = set(); comps = []
    for f in tf:
        if f in seen: continue
        st = [f]; seen.add(f); c = []
        while st:
            a = st.pop(); c.append(a)
            for e in a.edges:
                for g in e.link_faces:
                    if g in S and g not in seen: seen.add(g); st.append(g)
        comps.append(c)
    D = set()
    for c in comps:   # haar-oeer ligger midt i halezonen; fejlvaegtede bagben-bagsider ligger ved |y| > 0.115
        if np.mean([abs(sum(v.co.y for v in f.verts) / 3) for f in c]) < 0.115: D.update(c)
    for f in bm.faces:
        if f in D: continue
        cx = sum((v.co for v in f.verts), V()) / len(f.verts)
        if abs(cx.y) < 0.115 and cx.z < 1.14 and cx.x > 0.74:
            if cx.x > 0.80 and cx.z < 0.98: D.add(f); continue
            sat, lum = face_sat(f)
            if sat < 0.30 and cx.z < 1.14: D.add(f)   # graa/hvide/moerke haar-flader (kroppen er braun)
    for f in bm.faces:   # graa/hvide haar-lapper (lyse, ikke braune, ikke sokker) bag laaret/haserne, fx loese baand/staenger ved bagbenet
        if f in D: continue
        cx = sum((v.co for v in f.verts), V()) / len(f.verts)
        if cx.x > 0.66 and 0.36 < cx.z < 1.2 and abs(cx.y) < 0.3:
            sat, lum = face_sat(f)
            if sat < 0.30 and lum > 0.18: D.add(f)
    print('FIX removed faces', len(D))
    keep_tail = [f for f in tf if f not in D]
    bmesh.ops.delete(bm, geom=list(D), context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    # smaa loese oeer (<= 30 flader): haarrester midt mellem haserne og moerke splinter ved bagbenene fjernes (brune oeer er benets egen overflade og bevares)
    seen = set(); small = []
    for f in bm.faces:
        if f in seen: continue
        st = [f]; seen.add(f); c = []
        while st:
            a = st.pop(); c.append(a)
            for e in a.edges:
                for g in e.link_faces:
                    if g not in seen: seen.add(g); st.append(g)
        if len(c) <= 30:
            m_ = sum((sum((v.co for v in q.verts), V()) / len(q.verts) for q in c), V()) / len(c)
            sl = [face_sat(q) for q in c]
            sat_m = sum(x[0] for x in sl) / len(sl); lum_m = sum(x[1] for x in sl) / len(sl)
            mid = abs(m_.y) < 0.08 and 0.3 < m_.z < 0.85 and m_.x > 0.74          # lapper/baand midt mellem haserne (hvor halen sad)
            flake = len(c) <= 5 and lum_m < 0.15 and sat_m < 0.30 and 0.2 < m_.z < 0.9 and m_.x > 0.66 and abs(m_.y) < 0.25   # moerke splinter
            if mid or flake: small.extend(c)
    print('FIX removed small islands faces', len(small))
    bmesh.ops.delete(bm, geom=small, context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.verts.ensure_lookup_table()
    # reweight remaining tail-dominant vertices (bagbenets bagside) from nearest non-tail-dominant vertex
    tail_idx = [v.index for v in bm.verts if dom(v) in TAIL_BONES]
    other = [v for v in bm.verts if dom(v) not in TAIL_BONES and v[dl]]
    kd = KDTree(len(other))
    for i, v in enumerate(other): kd.insert(v.co, i)
    kd.balance()
    for vi in tail_idx:
        v = bm.verts[vi]
        _, i, _ = kd.find(v.co)
        src = other[i]
        for k in list(v[dl].keys()): del v[dl][k]
        for k, w in src[dl].items(): v[dl][k] = w
    print('FIX reweighted verts', len(tail_idx))
    # ---- ny hale ----
    path = [(0.780, 1.15), (0.800, 1.10), (0.835, 1.00), (0.862, 0.88), (0.878, 0.70), (0.886, 0.50), (0.890, 0.32), (0.893, 0.20), (0.894, 0.145)]
    rad = [(0.045, 0.045), (0.055, 0.055), (0.075, 0.065), (0.088, 0.068), (0.092, 0.070), (0.088, 0.066), (0.074, 0.056), (0.052, 0.042), (0.036, 0.030)]
    NR, NS = 56, 40
    zs = np.array([p[1] for p in path]); order = np.argsort(zs)
    def interp(arr, z):
        return float(np.interp(z, zs[order], np.array(arr)[order]))
    px_ = [p[0] for p in path]
    def smooth(t):
        t = min(1.0, max(0.0, t)); return t * t * (3 - 2 * t)
    ring_verts = []; first = len(bm.verts)
    for r in range(NR):
        t = r / (NR - 1)
        z = 1.15 + (0.145 - 1.15) * t
        cx = interp(px_, z); ry = interp([q[0] for q in rad], z); rx = interp([q[1] for q in rad], z)
        ring = []
        for s in range(NS):
            th = 2 * math.pi * s / NS
            lock = 1.0 + (0.09 * math.sin(5 * th + 1.5 * z) + 0.06 * math.sin(8 * th - 1.0 * z + 1.0)) * smooth(t * 6)   # haarlokke-bule
            x = cx + rx * lock * math.cos(th) + (0.010 * math.sin(3 * th + 5 * z) if t > 0.1 else 0.0) + 0.008 * math.sin(z * 8.0) * smooth(t * 5)
            y = ry * lock * math.sin(th)
            v = bm.verts.new((x, y, z)); ring.append(v)
            v[dl][gi['pelvis']] = 0.0; del v[dl][gi['pelvis']]
            a = smooth((z - 0.743) / 0.2)                  # 0 under tail_02-leddet, 1 over
            b = smooth((z - 0.396) / 0.2)                  # 0 under tail_03-leddet, 1 over
            pel = 0.5 * smooth((z - 0.98) / 0.16)
            w1, w2, w3 = a, (1 - a) * b, (1 - a) * (1 - b)
            ws = {'tail_01': w1 * (1 - pel), 'tail_02': w2 * (1 - pel), 'tail_03': w3 * (1 - pel), 'pelvis': pel}
            for n, w in ws.items():
                if w > 1e-4: v[dl][gi[n]] = w
        ring_verts.append(ring)
    tip = bm.verts.new((path[-1][0], 0.0, 0.125)); tip[dl][gi['tail_03']] = 1.0
    ring_verts.append([tip] * NS)
    newfaces = []
    top = bm.verts.new((path[0][0], 0.0, 1.15))   # laag oeverst (inde i krydset)
    top[dl][gi['tail_01']] = 0.5; top[dl][gi['pelvis']] = 0.5
    for s_ in range(NS):
        try: newfaces.append(bm.faces.new([ring_verts[0][(s_ + 1) % NS], ring_verts[0][s_], top]))
        except ValueError: pass
    for r in range(NR):
        for s in range(NS):
            a, b = ring_verts[r][s], ring_verts[r][(s + 1) % NS]
            c, d = ring_verts[r + 1][(s + 1) % NS], ring_verts[r + 1][s]
            quad = [a, b, c, d] if r < NR - 1 else [a, b, tip]
            try:
                f = bm.faces.new(quad); newfaces.append(f)
            except ValueError:
                pass
    # normaler udad (ring omdreje saa de peger ud)
    bmesh.ops.recalc_face_normals(bm, faces=newfaces)
    # UV: moerk plet
    for f in newfaces:
        f.smooth = True
        for l in f.loops:
            v = l.vert
            ang = math.atan2(v.co.y, v.co.x - 0.85)
            l[uvl].uv = (DARK_UV[0] + 0.0009 * math.sin(ang), DARK_UV[1] + 0.0013 * (1.15 - v.co.z))
    n_new_loops = sum(len(f.loops) for f in newfaces)
    # bmesh bevarer meshets custom normals paa de gamle loops (verificeret: 0 aendrede af 73.238 sammenlignede loops);
    # halens nye loops har ingen custom normal og skygges glat (flader er glatte).
    bm.to_mesh(me); bm.free()
    me.update()
    return n_new_loops

def export_fbx(rig, mesh, project):
    out_dir = project / 'SourceAssets/Horse'
    fbx = out_dir / 'Horse_1_rigged_v3.fbx'
    prev = out_dir / 'Horse_1_rigged_v3.prev.fbx'
    if fbx.exists() and not prev.exists():
        shutil.copy2(fbx, prev)
    for o in bpy.data.objects: o.select_set(False)
    rig.select_set(True); mesh.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
        bake_anim=False, axis_forward='-Z', axis_up='Y', apply_unit_scale=True, use_armature_deform_only=False)
    print('HORSE_FBX', fbx, fbx.stat().st_size)

if __name__ == '__main__':
    project = Path(__file__).resolve().parents[2]
    rig = bpy.data.objects['Horse_1_Armature_v2.001']
    mesh = bpy.data.objects['Horse_1_Mesh.001']
    me = mesh.data
    n_loops_before = len(me.loops)
    print('custom normals before', me.has_custom_normals)
    n_new = apply_fix(mesh)
    print('custom normals after', me.has_custom_normals, 'loops', n_loops_before, '->', len(me.loops), 'new', n_new)
    if '--no-export' not in sys.argv:
        export_fbx(rig, mesh, project)
