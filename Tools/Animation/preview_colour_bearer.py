"""Measure + render a colour-bearer clip (FBX with mesh) with a 2.8 m pole through the right-hand centre.
blender -b -P preview_colour_bearer.py -- <clip fbx> <out prefix> <frames e.g. 0,8,15,22> [res_x res_y ortho z_center]
Pole: leans LEAN deg back (top behind) in the side view, foot FOOT m below the hand; length 2.8 m. Writes <prefix>_metrics.json and <prefix>_<view>_<frame>.png"""
import bpy, json, math, os, sys
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index('--') + 1:]
FBX, PREFIX = os.path.abspath(a[0]), os.path.abspath(a[1])
FRAMES = [int(x) for x in a[2].split(',')]
RX, RY = (int(a[3]), int(a[4])) if len(a) > 4 else (560, 840)
ORTHO = float(a[5]) if len(a) > 5 else 3.8
ZC = float(a[6]) if len(a) > 6 else 1.6
LEAN = float(os.environ.get('LEAN', '12'))
FOOT = float(os.environ.get('FOOT', '0.65'))
PLEN = 2.8
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=FBX)
rig = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = next(o for o in bpy.data.objects if o.type == 'MESH')
act = rig.animation_data.action
start = round(act.frame_range[0])
nfr = round(act.frame_range[1]) - start + 1
sc = bpy.context.scene
up = Vector((0, 0, 1))


def H(n):
    return rig.matrix_world @ rig.pose.bones['mixamorig:' + n].head


def hand_centre():
    pb = rig.pose.bones['mixamorig:RightHand']
    return rig.matrix_world @ ((pb.head + pb.tail) / 2)


def seg_dist(p0, p1, q0, q1, n=60):
    best = 9
    for i in range(n + 1):
        p = p0.lerp(p1, i / n)
        d = q1 - q0
        t = max(0, min(1, (p - q0).dot(d) / d.dot(d)))
        best = min(best, (p - (q0 + d * t)).length)
    return best


sc.frame_set(start)
bpy.context.view_layer.update()
right0 = H('RightShoulder') - H('LeftShoulder')
right0.z = 0
right0.normalize()
fwd0 = up.cross(right0).normalized()
dg = bpy.context.evaluated_depsgraph_get()
ground = 9
for k in range(nfr):
    sc.frame_set(start + k)
    bpy.context.view_layer.update()
    ev = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    ground = min(ground, min((ev.matrix_world @ v.co).z for v in me.vertices))
    ev.to_mesh_clear()
print('GROUND', ground)
rows = []
d_pole = (-fwd0 * math.sin(math.radians(LEAN)) + up * math.cos(math.radians(LEAN))).normalized()   # foot -> top
for k in range(nfr):
    sc.frame_set(start + k)
    bpy.context.view_layer.update()
    right = H('RightShoulder') - H('LeftShoulder')
    right.z = 0
    right.normalize()
    fwd = up.cross(right).normalized()
    sh, el, wr = H('RightArm'), H('RightForeArm'), H('RightHand')
    hc = hand_centre()
    rel = hc - sh
    ang = math.degrees(math.acos(max(-1, min(1, (sh - el).normalized().dot((wr - el).normalized())))))
    foot = hc - d_pole * (FOOT / math.cos(math.radians(LEAN)))
    top = foot + d_pole * PLEN
    hd, ht = H('Head'), H('HeadTop_End')
    ax = (ht - hd).normalized()
    # head capsule (bare head r 9 cm from Head bone head to top) and shako (r 11 cm, from head top - 4 cm to +19 cm)
    c_head = seg_dist(foot, top, hd, ht) - 0.09
    c_shako = seg_dist(foot, top, ht - ax * 0.04, ht + ax * 0.19) - 0.11
    ev = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = ev.to_mesh()
    zn = H('Neck').z
    dmin = 9
    dd = top - foot
    for v in me.vertices:
        pw = ev.matrix_world @ v.co
        if pw.z < zn - 0.02:
            continue
        t = max(0, min(1, (pw - foot).dot(dd) / dd.dot(dd)))
        dmin = min(dmin, (pw - (foot + dd * t)).length)
    ev.to_mesh_clear()
    rows.append({'frame': k, 'clear_mesh_head_cm': dmin * 100, 'hand_fwd': rel.dot(fwd), 'hand_down': -rel.z, 'hand_out': rel.dot(right), 'elbow_deg': ang,
                 'elbow_rel_sh': [(el - sh).dot(fwd), (el - sh).z, (el - sh).dot(right)], 'hand_h_ground': hc.z - ground,
                 'wrist_h_ground': wr.z - ground, 'shoulder_h_ground': sh.z - ground, 'pole_foot_h': foot.z - ground,
                 'clear_head_cm': c_head * 100, 'clear_shako_cm': c_shako * 100})
mm = lambda key: (min(r[key] for r in rows), max(r[key] for r in rows))
summ = {k: mm(k) for k in ('hand_fwd', 'hand_down', 'hand_out', 'elbow_deg', 'hand_h_ground', 'pole_foot_h', 'clear_head_cm', 'clear_shako_cm', 'clear_mesh_head_cm')}
open(PREFIX + '_metrics.json', 'w').write(json.dumps({'ground': ground, 'summary': summ, 'frames': rows}, indent=1))
print('SUMMARY', json.dumps(summ))

# pole + render
bpy.ops.mesh.primitive_cylinder_add(radius=0.016, depth=PLEN, vertices=12)
pole = bpy.context.active_object
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.04)
ball = bpy.context.active_object
bpy.ops.mesh.primitive_plane_add(size=1)
flag = bpy.context.active_object
for o, col in ((pole, (0.35, 0.2, 0.08, 1)), (ball, (0.9, 0.7, 0.1, 1)), (flag, (0.8, 0.05, 0.05, 1))):
    m = bpy.data.materials.new('m')
    m.diffuse_color = col
    o.data.materials.append(m)
sc.render.engine = 'BLENDER_WORKBENCH'
sc.render.resolution_x, sc.render.resolution_y = RX, RY
sc.display.shading.light = 'STUDIO'
sc.display.shading.color_type = 'MATERIAL'
sc.world = bpy.data.worlds.new('w')
sc.world.color = (0.45, 0.55, 0.7)
cam = bpy.data.objects.new('c', bpy.data.cameras.new('c'))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.type = 'ORTHO'
cam.data.ortho_scale = ORTHO
cam.data.clip_end = 100
orig = (H('LeftFoot') + H('RightFoot')) / 2
orig.z = ground
views = {'front': fwd0, 'side': right0, 'q34': (fwd0 + right0).normalized()}
for i in FRAMES:
    sc.frame_set(start + i)
    bpy.context.view_layer.update()
    hc = hand_centre()
    foot = hc - d_pole * (FOOT / math.cos(math.radians(LEAN)))
    top = foot + d_pole * PLEN
    zax = d_pole
    xax = right0
    yax = zax.cross(xax).normalized()
    mrot = Matrix((xax, yax, zax)).transposed().to_4x4()
    mrot.translation = (foot + top) / 2
    pole.matrix_world = mrot
    ball.location = top
    # flag: 1.0 m x 0.8 m in the pole/side plane (fly pointing forward-down is pole-axis cross right); sits near the top, flying backwards
    fl = Matrix((-fwd0 * 1.0 * 0 + right0 * 0, right0 * 0, zax)).transposed().to_4x4()
    # plane lies in the pole + back-horizontal plane
    back = -fwd0
    fx = back * 1.0
    fz = zax * 0.8
    fy = fx.cross(fz).normalized() * 0.01
    fm = Matrix((fx, fy, fz)).transposed().to_4x4()
    fm.translation = top - zax * 0.45 + back * 0.5
    flag.matrix_world = fm
    for vn, d in views.items():
        pos = orig + d * 8 + up * ZC
        cam.location = pos
        cam.rotation_euler = ((orig + up * ZC) - pos).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = f'{PREFIX}_{vn}_{i:02d}.png'
        bpy.ops.render.render(write_still=True)
print('COLOUR_PREVIEW_DONE')
