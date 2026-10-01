"""Builds the battlefield's foliage and props in Blender (PROJECT 1864) and exports one FBX each to
SourceAssets/Battle1864/Meshes: grass clumps (cards), a Norway spruce, a Scots pine, a broadleaf tree, a bush (all
cards on a trunk, normals pointing out from the crown so the cards shade like a volume) and a post-and-rail fence.
Vertex colour R is the wind weight (0 at the root, 1 at the tips), G an ambient occlusion term.
Material slots: Bark, Spruce, Pine, Leaves, Grass, Wood.
Run: blender -b --python Tools/Battle/make_battle_meshes.py"""
import bpy, bmesh, math, os, random
from mathutils import Vector, Matrix

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'SourceAssets', 'Battle1864', 'Meshes')
os.makedirs(OUT, exist_ok=True)


class Builder:
    def __init__(self, slots):
        self.slots = slots
        self.verts, self.faces, self.uvs, self.cols, self.mats, self.normals = [], [], [], [], [], []

    def quad(self, corners, uvs, slot, cols, normals=None):
        i = len(self.verts)
        self.verts += [Vector(c) for c in corners]
        self.faces.append((i, i + 1, i + 2, i + 3))
        self.uvs.append(uvs)
        self.cols.append(cols)
        self.mats.append(self.slots.index(slot))
        self.normals.append(normals)

    def card(self, origin, along, up, length, width, slot, wind0, wind1, centre=None, ao=1.0, uv=((0, 0), (1, 0), (1, 1), (0, 1))):
        """A card from origin along 'along' (length) and +/- 'up' (half width each side); U along, V across."""
        a, u = Vector(along).normalized(), Vector(up).normalized()
        o = Vector(origin)
        c = [o - u * width * 0.5, o + a * length - u * width * 0.5, o + a * length + u * width * 0.5, o + u * width * 0.5]
        cols = [(wind0, ao, 0, 1), (wind1, ao, 0, 1), (wind1, ao, 0, 1), (wind0, ao, 0, 1)]
        normals = None
        if centre is not None:
            normals = [((p - Vector(centre)).normalized() + Vector((0, 0, 0.35))).normalized() for p in c]
        self.quad(c, list(uv), slot, cols, normals)

    def cylinder(self, base, top, r0, r1, sides, slot, v_scale, wind0, wind1, ao0=0.6, ao1=1.0):
        axis = Vector(top) - Vector(base)
        z = axis.normalized()
        x = z.orthogonal().normalized()
        y = z.cross(x)
        length = axis.length
        for s in range(sides):
            a0, a1 = 2 * math.pi * s / sides, 2 * math.pi * (s + 1) / sides
            d0, d1 = x * math.cos(a0) + y * math.sin(a0), x * math.cos(a1) + y * math.sin(a1)
            c = [Vector(base) + d0 * r0, Vector(base) + d1 * r0, Vector(top) + d1 * r1, Vector(top) + d0 * r1]
            uv = [(s / sides, 0), ((s + 1) / sides, 0), ((s + 1) / sides, length * v_scale), (s / sides, length * v_scale)]
            cols = [(wind0, ao0, 0, 1), (wind0, ao0, 0, 1), (wind1, ao1, 0, 1), (wind1, ao1, 0, 1)]
            self.quad(c, uv, slot, cols, [d0, d1, d1, d0])

    def export(self, name):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        me = bpy.data.meshes.new(name)
        me.from_pydata([tuple(v) for v in self.verts], [], self.faces)
        me.update()
        for s in self.slots:
            me.materials.append(bpy.data.materials.get(s) or bpy.data.materials.new(s))
        uvl = me.uv_layers.new(name='UVMap')
        col = me.color_attributes.new(name='Col', type='BYTE_COLOR', domain='CORNER')
        loop = 0
        custom = []
        for f, poly in enumerate(me.polygons):
            poly.material_index = self.mats[f]
            for k, li in enumerate(poly.loop_indices):
                uvl.data[li].uv = self.uvs[f][k]
                col.data[li].color = self.cols[f][k]
                n = self.normals[f]
                custom.append(tuple(n[k]) if n else tuple(poly.normal))
        me.normals_split_custom_set(custom)
        ob = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(ob)
        path = os.path.join(OUT, name + '.fbx')
        bpy.ops.export_scene.fbx(filepath=path, object_types={'MESH'}, mesh_smooth_type='OFF', use_mesh_modifiers=False,
                                 colors_type='LINEAR', axis_forward='-Y', axis_up='Z')
        print('EXPORTED', name, len(self.faces), 'quads')


def grass(name, height, width, cards, seed):
    random.seed(seed)
    b = Builder(['Grass'])
    for k in range(cards):
        a = math.pi * k / cards + random.uniform(-0.2, 0.2)
        d = Vector((math.cos(a), math.sin(a), 0))
        off = Vector((random.uniform(-0.08, 0.08), random.uniform(-0.08, 0.08), 0))
        lean = Vector((random.uniform(-0.12, 0.12), random.uniform(-0.12, 0.12), 0))
        h = height * random.uniform(0.85, 1.1)
        # Two segments: the blade card bends a little.
        p0, p1, p2 = off, off + Vector((0, 0, h * 0.5)) + lean * 0.3, off + Vector((0, 0, h)) + lean
        w = width * 0.5
        n = d.cross(Vector((0, 0, 1)))
        for (q0, q1, v0, v1) in ((p0, p1, 0.0, 0.5), (p1, p2, 0.5, 1.0)):
            c = [q0 - d * w, q0 + d * w, q1 + d * w, q1 - d * w]
            b.quad(c, [(0, v0), (1, v0), (1, v1), (0, v1)], 'Grass',
                   [(v0, 0.55 + 0.45 * v0, 0, 1)] * 2 + [(v1, 0.55 + 0.45 * v1, 0, 1)] * 2,
                   [(n + Vector((0, 0, 1.2))).normalized()] * 4)
    b.export(name)


def spruce(name, seed, height=16.0):
    random.seed(seed)
    b = Builder(['Bark', 'Spruce'])
    b.cylinder((0, 0, -0.3), (0, 0, height), 0.26, 0.03, 8, 'Bark', 0.6, 0.0, 0.25)
    z = 1.6
    whorl = 0
    while z < height - 0.4:
        t = (z - 1.6) / (height - 2.0)
        reach = 0.4 + 3.2 * (1.0 - t) ** 1.1
        count = 6 if t < 0.7 else 4
        for k in range(count):
            a = 2 * math.pi * (k + 0.5 * (whorl % 2)) / count + random.uniform(-0.25, 0.25)
            out = Vector((math.cos(a), math.sin(a), 0))
            droop = random.uniform(-0.45, -0.2) if t < 0.8 else random.uniform(-0.1, 0.25)
            along = (out + Vector((0, 0, droop))).normalized()
            side = out.cross(Vector((0, 0, 1))).normalized()
            L = reach * random.uniform(0.85, 1.1)
            centre = (0, 0, z - 0.4)
            wind = 0.3 + 0.7 * min(1.0, L / 3.0)
            # Two cards per branch: one flat (seen from above), one tilted (seen from the side).
            b.card((0, 0, z), along, side, L, L * 0.75, 'Spruce', 0.15 * t, wind, centre, 0.55 + 0.45 * t)
            tilt = (side * 0.5 + Vector((0, 0, 1))).normalized()
            b.card((0, 0, z + 0.05), along, tilt, L * 0.95, L * 0.6, 'Spruce', 0.15 * t, wind, centre, 0.5 + 0.5 * t,
                   uv=((0, 1), (1, 1), (1, 0), (0, 0)))
        z += random.uniform(0.38, 0.5)
        whorl += 1
    # The leader at the top.
    b.card((0, 0, height - 0.8), (0, 0, 1), (1, 0, 0), 1.2, 0.6, 'Spruce', 0.5, 0.8, (0, 0, height - 1.5))
    b.card((0, 0, height - 0.8), (0, 0, 1), (0, 1, 0), 1.2, 0.6, 'Spruce', 0.5, 0.8, (0, 0, height - 1.5))
    b.export(name)


def pine(name, seed, height=17.0):
    random.seed(seed)
    b = Builder(['Bark', 'Pine'])
    # A tall, slightly crooked trunk; the crown in the top third.
    lean = Vector((random.uniform(-0.6, 0.6), random.uniform(-0.6, 0.6), 0))
    pts = [Vector((0, 0, -0.3)), lean * 0.3 + Vector((0, 0, height * 0.45)), lean + Vector((0, 0, height * 0.8)), lean * 1.2 + Vector((0, 0, height))]
    radii = [0.3, 0.22, 0.12, 0.03]
    for i in range(3):
        b.cylinder(pts[i], pts[i + 1], radii[i], radii[i + 1], 8, 'Bark', 0.6, 0.15 * i, 0.15 * (i + 1), 0.4 + 0.2 * i, 0.6 + 0.2 * i)
    crown_c = lean + Vector((0, 0, height * 0.8))
    for k in range(20):
        z = height * random.uniform(0.58, 0.97)
        base = lean * (z / height) + Vector((0, 0, z))
        a = random.uniform(0, 2 * math.pi)
        out = Vector((math.cos(a), math.sin(a), random.uniform(0.1, 0.5))).normalized()
        L = random.uniform(1.8, 3.6) * (1.15 - (z / height - 0.58))
        tip = base + out * L
        b.cylinder(base, tip, 0.08, 0.02, 4, 'Bark', 1.0, 0.2, 0.6, 0.6, 0.8)
        # Tufts along the branch and at its end: crossed cards.
        for f in (0.45, 0.8, 1.05):
            p = base + out * L * f
            for j in range(2):
                ang = a + j * math.pi / 2 + random.uniform(-0.3, 0.3)
                d = Vector((math.cos(ang), math.sin(ang), 0))
                sz = random.uniform(2.0, 2.9) * (0.75 + 0.25 * f)
                b.card(p - d * sz * 0.5 - Vector((0, 0, sz * 0.3)), d, (0, 0, 1), sz, sz * 0.75, 'Pine', 0.5, 0.9, crown_c, 0.6 + 0.4 * f)
            sz = random.uniform(2.0, 2.8)
            b.card(p - out * sz * 0.5, out, out.cross(Vector((0, 0, 1))), sz, sz, 'Pine', 0.5, 0.9, crown_c, 0.9)
    b.export(name)


def broadleaf(name, seed, height=13.0, clusters=110, slot='Leaves'):
    random.seed(seed)
    b = Builder(['Bark', slot])
    trunk_top = Vector((0, 0, height * 0.35))
    b.cylinder((0, 0, -0.3), trunk_top, 0.42, 0.3, 8, 'Bark', 0.6, 0.0, 0.1, 0.35, 0.6)
    crown_c = Vector((0, 0, height * 0.62))
    rx, rz = height * 0.38, height * 0.33
    for k in range(5):
        a = 2 * math.pi * k / 5 + random.uniform(-0.3, 0.3)
        tip = crown_c + Vector((math.cos(a) * rx * 0.6, math.sin(a) * rx * 0.6, random.uniform(-0.5, 2.5)))
        b.cylinder(trunk_top, tip, 0.22, 0.06, 6, 'Bark', 0.8, 0.1, 0.5, 0.5, 0.8)
    for k in range(clusters):
        # A point in the crown's ellipsoid, biased to the surface.
        while True:
            p = Vector((random.uniform(-1, 1), random.uniform(-1, 1), random.uniform(-1, 1)))
            if 0.35 < p.length <= 1.0:
                break
        pos = crown_c + Vector((p.x * rx, p.y * rx, p.z * rz))
        sz = random.uniform(2.0, 3.0) * height / 13.0
        a = random.uniform(0, 2 * math.pi)
        d = Vector((math.cos(a), math.sin(a), random.uniform(-0.3, 0.3))).normalized()
        up = (d.cross(Vector((0, 0, 1))).normalized() * random.uniform(-0.5, 0.5) + Vector((0, 0, 1))).normalized()
        ao = 0.45 + 0.55 * max(0.0, min(1.0, (p.length - 0.35) / 0.65 * 0.6 + (p.z + 1) * 0.2))
        b.card(pos - d * sz * 0.5 - up * 0.0, d, up, sz, sz, slot, 0.5, 0.9, crown_c, ao, uv=((0, 0), (1, 0), (1, 1), (0, 1)))
    b.export(name)


def bush(name, seed):
    random.seed(seed)
    b = Builder(['Leaves'])
    c = Vector((0, 0, 0.5))
    for k in range(26):
        p = Vector((random.uniform(-1, 1), random.uniform(-1, 1), random.uniform(-0.2, 1)))
        pos = c + Vector((p.x * 1.0, p.y * 1.0, p.z * 0.7))
        sz = random.uniform(1.0, 1.4)
        a = random.uniform(0, 2 * math.pi)
        d = Vector((math.cos(a), math.sin(a), 0))
        b.card(pos - d * sz * 0.5 - Vector((0, 0, sz * 0.4)), d, (0, 0, 1), sz, sz, 'Leaves', 0.2, 0.6, c, 0.6 + 0.4 * random.random())
    b.export(name)


def fence(name, seed, length=3.0):
    """Post and rail: two posts, two split rails, a little crooked. U along the wood grain."""
    random.seed(seed)
    b = Builder(['Wood'])

    def beam(p0, p1, w, h):
        axis = (Vector(p1) - Vector(p0))
        L = axis.length
        z = axis.normalized()
        x = z.orthogonal().normalized()
        if abs(z.z) < 0.9:
            x = z.cross(Vector((0, 0, 1))).normalized()
        y = z.cross(x).normalized()
        corners = [x * w + y * h, -x * w + y * h, -x * w - y * h, x * w - y * h]
        for i in range(4):
            c0, c1 = corners[i], corners[(i + 1) % 4]
            n = (c0 + c1).normalized()
            b.quad([Vector(p0) + c0, Vector(p0) + c1, Vector(p1) + c1, Vector(p1) + c0],
                   [(0, i * 0.25), (0, (i + 1) * 0.25), (L / 1.5, (i + 1) * 0.25), (L / 1.5, i * 0.25)], 'Wood',
                   [(0, 0.8, 0, 1)] * 4, [n] * 4)
        for (p, s) in ((p0, -1), (p1, 1)):
            b.quad([Vector(p) + c for c in (corners if s > 0 else corners[::-1])], [(0, 0), (0.05, 0), (0.05, 0.05), (0, 0.05)], 'Wood',
                   [(0, 0.8, 0, 1)] * 4, [z * s] * 4)

    for x in (0.0, length):
        lean = Vector((random.uniform(-0.04, 0.04), random.uniform(-0.04, 0.04), 0))
        beam((x, 0, -0.3), Vector((x, 0, 1.15)) + lean, 0.06, 0.06)
    for zr in (0.45, 0.9):
        beam((-0.15, 0.08, zr + random.uniform(-0.04, 0.04)), (length + 0.15, 0.08, zr + random.uniform(-0.04, 0.04)), 0.05, 0.035)
    b.export(name)


grass('SM_Grass_Clump_A', 0.45, 0.55, 3, 1)
grass('SM_Grass_Clump_B', 0.75, 0.6, 3, 2)
grass('SM_Grass_Clump_C', 0.28, 0.5, 2, 3)
spruce('SM_Spruce_A', 11, 16.0)
spruce('SM_Spruce_B', 12, 12.0)
pine('SM_Pine_A', 21, 17.0)
pine('SM_Pine_B', 22, 14.0)
broadleaf('SM_Broadleaf_A', 31, 13.0)
broadleaf('SM_Oak_A', 32, 11.0, 140)
bush('SM_Bush_A', 41)
fence('SM_Fence_Rail', 51)
