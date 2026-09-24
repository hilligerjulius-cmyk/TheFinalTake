"""Chunky low-poly modelling kit for The Final Take (Blender 4.2, bpy).

Everything is authored in UNREAL space and units (cm, X forward, Y right, Z up, FRotator
pitch/yaw/roll in degrees) so the numbers from the C++ builders can be used verbatim.
`Asset.build()` converts once to Blender space: (x, y, z)ue -> (x, -y, z)bl with the face winding
flipped. Exported with the FBX settings in export.py, Unreal gets the vertices back 1:1.

Look: soft bevels + face-area weighted normals (rounded toy look with crisp flat faces), a tiny
hand-made wobble, vertex colours from the studio palette (sRGB, alpha = roughness) with a gentle
per-part contact shadow, and at most three material slots so every mesh instances cheaply.
"""
import math
import random
import zlib

import bpy  # must come first when running as the pip 'bpy' module
import bmesh
from mathutils import Matrix, Vector, noise

from . import palette as P

MAT_OPAQUE = "M_FT_Vertex"
MAT_GLOW = "M_FT_VertexGlow"
MAT_GLASS = "M_FT_VertexGlass"
SLOT_ORDER = [MAT_OPAQUE, MAT_GLOW, MAT_GLASS]
MAT_ALIASES = {"opaque": MAT_OPAQUE, "glow": MAT_GLOW, "glass": MAT_GLASS}


# ---------------------------------------------------------------------------------------- math

def ue_rot(pitch=0.0, yaw=0.0, roll=0.0):
    """FRotationMatrix as a column-vector 3x3 matrix acting on Unreal-space vectors."""
    d = math.pi / 180.0
    sp, cp = math.sin(pitch * d), math.cos(pitch * d)
    sy, cy = math.sin(yaw * d), math.cos(yaw * d)
    sr, cr = math.sin(roll * d), math.cos(roll * d)
    return Matrix((
        (cp * cy, sr * sp * cy - cr * sy, -(cr * sp * cy + sr * sy)),
        (cp * sy, sr * sp * sy + cr * cy, cy * sr - cr * sp * sy),
        (sp, -sr * cp, cr * cp),
    ))


def xf(at=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    """Unreal-style transform (scale, then FRotator, then translate) as a 4x4 matrix."""
    s = Matrix.Diagonal((scale[0], scale[1], scale[2], 1.0))
    r = ue_rot(*rot).to_4x4()
    return Matrix.Translation(Vector(at)) @ r @ s


def v3(p):
    return Vector((float(p[0]), float(p[1]), float(p[2])))


def lerp(a, b, t):
    return a + (b - a) * t


# ---------------------------------------------------------------------------------- primitives
# Each returns a fresh bmesh in local space (centred like the matching SM_FT_* shape).

def _bevel_edges(bm, edges, width, segs, profile=0.5):
    if width <= 0 or not edges:
        return
    bmesh.ops.bevel(bm, geom=list(edges), offset=width, offset_type='OFFSET', segments=segs,
                    profile=profile, affect='EDGES', clamp_overlap=True)


def bm_box(sx, sy, sz, bevel=0.0, segs=2):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co.x *= sx
        v.co.y *= sy
        v.co.z *= sz
    if bevel > 0:
        _bevel_edges(bm, bm.edges, min(bevel, 0.49 * min(sx, sy, sz)), segs)
    return bm


def bm_cyl(r, h, sides=12, bevel=0.0, segs=2, r_top=None, ry=None, cap=True):
    """Cylinder/frustum along Z centred at 0; ry makes it elliptical."""
    bm = bmesh.new()
    rt = r if r_top is None else r_top
    bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=sides, radius1=r, radius2=max(rt, 0.0001), depth=h)
    if ry is not None and r > 0:
        for v in bm.verts:
            v.co.y *= ry / r
    if bevel > 0 and cap:
        rim = [e for e in bm.edges if abs(e.verts[0].co.z - e.verts[1].co.z) < 1e-4 and abs(abs(e.verts[0].co.z) - h / 2) < 1e-4]
        if rt < 0.001:
            rim = [e for e in rim if e.verts[0].co.z < 0]
        _bevel_edges(bm, rim, min(bevel, 0.45 * h, 0.45 * min(r, rt if rt > 0.001 else r)), segs)
    return bm


def bm_sphere(rx, ry=None, rz=None, segs=12, rings=8):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=rings, radius=1.0)
    ry = rx if ry is None else ry
    rz = rx if rz is None else rz
    for v in bm.verts:
        v.co.x *= rx
        v.co.y *= ry
        v.co.z *= rz
    return bm


def bm_ico(r, subdiv=1):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=r)
    return bm


def bm_torus(R, r, major=16, minor=8, rz=None):
    rz = r if rz is None else rz
    bm = bmesh.new()
    grid = []
    for i in range(major):
        u = 2 * math.pi * i / major
        ring = []
        for j in range(minor):
            w = 2 * math.pi * j / minor
            ring.append(bm.verts.new(((R + r * math.cos(w)) * math.cos(u), (R + r * math.cos(w)) * math.sin(u), rz * math.sin(w))))
        grid.append(ring)
    for i in range(major):
        for j in range(minor):
            a, b = grid[i][j], grid[(i + 1) % major][j]
            c, d = grid[(i + 1) % major][(j + 1) % minor], grid[i][(j + 1) % minor]
            bm.faces.new((a, b, c, d))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def bm_lathe(profile, sides=12, arc=360.0, smooth_caps=True):
    """Revolve [(radius, z), ...] (bottom to top) around Z. Radius 0 closes a pole."""
    bm = bmesh.new()
    rings = []
    full = abs(arc - 360.0) < 1e-3
    n = sides if full else sides + 1
    for (rad, z) in profile:
        if rad <= 1e-4:
            rings.append([bm.verts.new((0, 0, z))])
            continue
        ring = []
        for s in range(n):
            a = math.radians(arc) * s / sides
            ring.append(bm.verts.new((rad * math.cos(a), rad * math.sin(a), z)))
        rings.append(ring)
    for k in range(len(rings) - 1):
        r0, r1 = rings[k], rings[k + 1]
        for s in range(sides):
            s1 = (s + 1) % n if full else s + 1
            if len(r0) == 1 and len(r1) == 1:
                continue
            if len(r0) == 1:
                bm.faces.new((r0[0], r1[s], r1[s1]))
            elif len(r1) == 1:
                bm.faces.new((r0[s], r0[s1], r1[0]))
            else:
                bm.faces.new((r0[s], r0[s1], r1[s1], r1[s]))
    if full:
        for ring in (rings[0], rings[-1]):
            if len(ring) > 2:
                bm.faces.new(ring)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def bm_prism(profile_yz, depth):
    """Extrude a closed YZ polygon along X (centred), like SM_FT_Prism/Ramp."""
    bm = bmesh.new()
    back = [bm.verts.new((-depth / 2, y, z)) for (y, z) in profile_yz]
    front = [bm.verts.new((depth / 2, y, z)) for (y, z) in profile_yz]
    bm.faces.new(back)
    bm.faces.new(list(reversed(front)))
    n = len(profile_yz)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((back[i], back[j], front[j], front[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def bm_extrude(poly_xy, depth, bevel=0.0, segs=2):
    """Closed XY polygon extruded along +Z from 0..depth (concave OK)."""
    bm = bmesh.new()
    bot = [bm.verts.new((x, y, 0.0)) for (x, y) in poly_xy]
    top = [bm.verts.new((x, y, depth)) for (x, y) in poly_xy]
    n = len(poly_xy)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((bot[i], bot[j], top[j], top[i]))
    bm.faces.new(bot)
    bm.faces.new(top)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bevel > 0:
        rim = [e for e in bm.edges if len(e.link_faces) == 2 and abs(e.verts[0].co.z - e.verts[1].co.z) < 1e-5]
        _bevel_edges(bm, rim, min(bevel, depth * 0.45), segs)
    return bm


def _frames(pts):
    """Parallel-transport frames along a polyline."""
    tans = []
    for i in range(len(pts)):
        a = pts[max(i - 1, 0)]
        b = pts[min(i + 1, len(pts) - 1)]
        t = (b - a)
        tans.append(t.normalized() if t.length > 1e-9 else Vector((0, 0, 1)))
    up = Vector((0, 0, 1)) if abs(tans[0].z) < 0.9 else Vector((1, 0, 0))
    n = tans[0].cross(up).normalized()
    frames = []
    for i, t in enumerate(tans):
        if i > 0:
            n = n - t * n.dot(t)
            if n.length < 1e-6:
                n = t.orthogonal()
            n.normalize()
        b = t.cross(n).normalized()
        frames.append((n, b))
    return frames


def bm_tube(points, radius, sides=8, caps=True, radii=None, closed=False):
    pts = [v3(p) for p in points]
    bm = bmesh.new()
    frames = _frames(pts + ([pts[0]] if closed else []))[:len(pts)]
    rings = []
    for i, p in enumerate(pts):
        rr = radii[i] if radii else radius
        n, b = frames[i]
        rings.append([bm.verts.new(p + (n * math.cos(2 * math.pi * s / sides) + b * math.sin(2 * math.pi * s / sides)) * rr) for s in range(sides)])
    count = len(rings) if closed else len(rings) - 1
    for k in range(count):
        r0, r1 = rings[k], rings[(k + 1) % len(rings)]
        for s in range(sides):
            bm.faces.new((r0[s], r0[(s + 1) % sides], r1[(s + 1) % sides], r1[s]))
    if caps and not closed:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def bm_loft(sections, caps=True, closed_loop=True):
    """Skin a list of equally sized 3D loops (each a list of points)."""
    bm = bmesh.new()
    rings = [[bm.verts.new(v3(p)) for p in sec] for sec in sections]
    m = len(rings[0])
    for k in range(len(rings) - 1):
        r0, r1 = rings[k], rings[k + 1]
        for s in range(m if closed_loop else m - 1):
            bm.faces.new((r0[s], r0[(s + 1) % m], r1[(s + 1) % m], r1[s]))
    if caps and closed_loop:
        bm.faces.new(rings[0])
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def bm_leaf(length, width, droop=0.3, fold=0.18, thick=1.2, segs=7, tip=0.85, curl=0.0):
    """Closed, slightly folded leaf/frond growing along +X from the origin (droops towards -Z)."""
    tops, bots = [], []
    for i in range(segs + 1):
        t = i / segs
        w = width * 0.5 * max(0.12 * (1 - t) + 0.02, math.sin(math.pi * t ** 0.7) ** 0.8)
        x = length * t
        z = -droop * length * t * t
        mid = fold * w
        c = curl * t * t * width
        tops.append([(x, -w, z + c), (x, 0.0, z + mid), (x, w, z + c)])
        bots.append([(x, -w * 0.96, z + c - thick * 0.5), (x, 0.0, z + mid - thick), (x, w * 0.96, z + c - thick * 0.5)])
    bm = bmesh.new()
    top_v = [[bm.verts.new(p) for p in row] for row in tops]
    bot_v = [[bm.verts.new(p) for p in row] for row in bots]
    for i in range(segs):
        for j in range(2):
            bm.faces.new((top_v[i][j], top_v[i + 1][j], top_v[i + 1][j + 1], top_v[i][j + 1]))
            bm.faces.new((bot_v[i][j + 1], bot_v[i + 1][j + 1], bot_v[i + 1][j], bot_v[i][j]))
        for j in (0, 2):
            bm.faces.new((top_v[i][j], bot_v[i][j], bot_v[i + 1][j], top_v[i + 1][j]))
    for row_t, row_b in ((top_v[0], bot_v[0]), (top_v[-1], bot_v[-1])):
        bm.faces.new((row_t[0], row_t[1], row_t[2], row_b[2], row_b[1], row_b[0]))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


GLASS_TINT = (0.62, 0.86, 0.97)


def bm_shoreline(sx, sy, sz, rows=24, cols=7):
    """SM_FT_Shoreline (FTContentCommandlet.cpp BuildShoreline) scaled by size/100, as a closed skirt."""
    def pt(u, y):
        wave = 4 * math.sin(y * 0.13) + 2 * math.sin(y * 0.31)
        x = -50 + 100 * u + wave * (1 - u)
        z = min(0.0, (x - 20) * (110.0 / 120.0) + 2 * math.sin(y * 0.18) * (1 - u))
        return (x * sx / 100.0, y * sy / 100.0, z * sz / 100.0)
    bm = bmesh.new()
    grid = [[bm.verts.new(pt(c / cols, -50 + 100 * r / rows)) for c in range(cols + 1)] for r in range(rows + 1)]
    for r in range(rows):
        for c in range(cols):
            bm.faces.new((grid[r][c], grid[r][c + 1], grid[r + 1][c + 1], grid[r + 1][c]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        if f.normal.z < 0:
            f.normal_flip()
    return bm


def superellipse(w, h, n=12, power=3.0, offset=(0, 0), start=0.0):
    """Rounded-rectangle loop in the plane (u, v); power 2 = ellipse, higher = boxier."""
    out = []
    for i in range(n):
        a = start + 2 * math.pi * i / n
        c, s = math.cos(a), math.sin(a)
        u = w / 2 * math.copysign(abs(c) ** (2 / power), c)
        v = h / 2 * math.copysign(abs(s) ** (2 / power), s)
        out.append((u + offset[0], v + offset[1]))
    return out


def bm_text(text, size, depth, font_path=None, align='CENTER', bevel=0.0, resolution=3, spacing=1.0):
    """Extruded 3D lettering in the XY plane (reads along +X, faces +Z), base on z=0."""
    curve = bpy.data.curves.new("ft_text", 'FONT')
    curve.body = text
    curve.size = size
    curve.extrude = depth / 2
    curve.bevel_depth = bevel
    curve.bevel_resolution = 1 if bevel > 0 else 0
    curve.resolution_u = resolution
    curve.align_x = align
    curve.align_y = 'CENTER'
    curve.space_character = spacing
    if font_path:
        curve.font = load_font(font_path)
    obj = bpy.data.objects.new("ft_text", curve)
    bpy.context.scene.collection.objects.link(obj)
    dg = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(dg))
    bpy.data.objects.remove(obj)
    bpy.data.curves.remove(curve)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.001)
    for v in bm.verts:
        v.co.z += depth / 2
    return bm


_FONTS = {}


def load_font(path):
    if path not in _FONTS:
        _FONTS[path] = bpy.data.fonts.load(path, check_existing=True)
    return _FONTS[path]


# ------------------------------------------------------------------------------- deformers

def deform(bm, fn):
    for v in bm.verts:
        v.co = fn(v.co.copy())
    return bm


def wobble(bm, amount, scale=0.05, seed=0):
    """Coherent hand-made irregularity (amount in cm)."""
    if amount <= 0:
        return bm
    off = Vector((seed * 13.1, seed * 7.7, seed * 3.3))
    for v in bm.verts:
        p = v.co * scale + off
        v.co += Vector((noise.noise(p), noise.noise(p + Vector((31.4, 0, 0))), noise.noise(p + Vector((0, 17.2, 0))))) * amount
    return bm


def taper(bm, top_scale_x, top_scale_y=None, axis_z=True):
    """Scale XY linearly with height (1 at the bottom, top_scale at the top)."""
    top_scale_y = top_scale_x if top_scale_y is None else top_scale_y
    zs = [v.co.z for v in bm.verts]
    z0, z1 = min(zs), max(zs)
    for v in bm.verts:
        t = (v.co.z - z0) / max(z1 - z0, 1e-6)
        v.co.x *= lerp(1.0, top_scale_x, t)
        v.co.y *= lerp(1.0, top_scale_y, t)
    return bm


def bend_z(bm, amount):
    """Bow a part sideways (+X) along its height: parabola, amount cm at the top."""
    zs = [v.co.z for v in bm.verts]
    z0, z1 = min(zs), max(zs)
    for v in bm.verts:
        t = (v.co.z - z0) / max(z1 - z0, 1e-6)
        v.co.x += amount * t * t
    return bm


def bulge(bm, amount, axis=2):
    """Pillow the faces perpendicular to `axis` (cushions, sacks)."""
    ext = [max(abs(v.co[i]) for v in bm.verts) or 1.0 for i in range(3)]
    for v in bm.verts:
        u = [v.co[i] / ext[i] for i in range(3)]
        f = 1.0
        for i in range(3):
            if i != axis:
                f *= max(0.0, 1.0 - u[i] * u[i])
        v.co[axis] += math.copysign(amount * f, v.co[axis]) if abs(v.co[axis]) > 1e-6 else 0.0
    return bm


# ----------------------------------------------------------------------------------- asset

class Asset:
    """Accumulates parts (Unreal space) and turns them into one Blender mesh object."""

    def __init__(self, name, folder, desc="", replaces=None, pivot="", sharp_angle=50.0, wobble=0.0, vehicle=False):
        self.name = name
        self.folder = folder
        self.desc = desc
        self.replaces = replaces or []
        self.pivot = pivot
        self.sharp_angle = sharp_angle
        self.default_wobble = wobble
        self.vehicle = vehicle
        self.v = []
        self.vshade = []
        self.f = []
        self.fcol = []
        self.frough = []
        self.fmat = []
        self.fflat = []
        self.sockets = {}
        self.meta = {}
        self.fpart = []   # part index per face (for the coplanar-overlap check)
        self._part = 0
        self._seed = zlib.crc32(name.encode("utf-8")) % 997   # stable across runs (str hash() is salted)
        self.rng = random.Random(self._seed)

    # -- core ---------------------------------------------------------------------------
    def add(self, bm, matrix=None, col=P.GREY, rough=1.0, mat="opaque", flat=False, ao=True, jitter=None, var=0.035, glow=None):
        """glow: emissive strength on the code's scale (0.3 tape ... 20 bulbs); implies the glow slot.
        Stored as vertex alpha = sqrt(glow / 20) in the glow slot (alpha = roughness everywhere else)."""
        matrix = matrix or Matrix.Identity(4)
        if glow is not None and glow > 0:
            mat = "glow"
        if MAT_ALIASES.get(mat, mat) == MAT_GLOW:
            rough = math.sqrt(min(max(glow if glow else 4.0, 0.0), 20.0) / 20.0)
        jitter = self.default_wobble if jitter is None else jitter
        if jitter:
            self._seed += 1
            wobble(bm, jitter, seed=self._seed)
        bm.verts.ensure_lookup_table()
        bm.verts.index_update()
        base = len(self.v)
        pts = [matrix @ v.co for v in bm.verts]
        # De-coplanarise: push each part out along its normals by a tiny, growing amount so faces of different
        # parts never lie exactly in one plane (that renders black / z-fights). Later parts (details) win.
        eps = 0.02 + 0.003 * min(self._part, 50)
        bm.normal_update()
        nmat = matrix.to_3x3().inverted_safe().transposed()
        for k, v in enumerate(bm.verts):
            n = nmat @ v.normal
            if n.length > 1e-8:
                pts[k] = pts[k] + n.normalized() * eps
        zs = [p.z for p in pts] or [0.0]
        z0, z1 = min(zs), max(zs)
        glow = MAT_ALIASES.get(mat, mat) == MAT_GLOW
        for p in pts:
            self.v.append(p)
            if glow or not ao:
                s = 1.0
            else:
                t = (p.z - z0) / max(z1 - z0, 1e-6)
                s = 0.8 + 0.2 * min(1.0, t * 3.0) if (z1 - z0) > 4 else 1.0
                s *= 1.0 + var * noise.noise(p * 0.07 + Vector((self._seed, 0, 0)))
            self.vshade.append(s)
        flip = matrix.to_3x3().determinant() < 0
        m = MAT_ALIASES.get(mat, mat)
        for face in bm.faces:
            idx = [base + l.vert.index for l in face.loops]
            if flip:
                idx.reverse()
            self.f.append(tuple(idx))
            self.fcol.append(tuple(col))
            self.frough.append(rough)
            self.fmat.append(m)
            self.fflat.append(flat)
            self.fpart.append(self._part)
        self._part += 1
        bm.free()
        return self

    # -- convenience primitives (Unreal space: at = centre, rot = (pitch, yaw, roll)) ------
    def _auto_bevel(self, dims, bevel):
        if bevel is None:
            return max(0.25, min(6.0, 0.14 * min(dims)))
        return bevel

    def box(self, size, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, bevel=None, segs=None, tap=None, **kw):
        sx, sy, sz = size
        if segs is None:
            segs = 1 if min(size) < 14 else 2
        bm = bm_box(sx, sy, sz, self._auto_bevel(size, bevel), segs)
        if tap:
            taper(bm, *tap) if isinstance(tap, (tuple, list)) else taper(bm, tap)
        return self.add(bm, xf(at, rot), col, **kw)

    def cyl(self, r, h, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, sides=12, bevel=None, segs=None, r_top=None, ry=None, cap=True, **kw):
        b = self._auto_bevel((2 * r, h), bevel) if cap else 0
        if segs is None:
            segs = 1 if min(2 * r, h) < 14 else 2
        bm = bm_cyl(r, h, sides, b, segs, r_top, ry, cap)
        return self.add(bm, xf(at, rot), col, **kw)

    def cone(self, r, h, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, sides=12, r_top=0.0, bevel=0.0, **kw):
        bm = bm_cyl(r, h, sides, bevel, 2, r_top)
        return self.add(bm, xf(at, rot), col, **kw)

    def sphere(self, r, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, segs=12, rings=8, ry=None, rz=None, **kw):
        return self.add(bm_sphere(r, ry, rz, segs, rings), xf(at, rot), col, **kw)

    def torus(self, R, r, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, major=16, minor=8, rz=None, **kw):
        return self.add(bm_torus(R, r, major, minor, rz), xf(at, rot), col, **kw)

    def lathe(self, profile, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, sides=12, **kw):
        return self.add(bm_lathe(profile, sides), xf(at, rot), col, **kw)

    def tube(self, points, r, col=P.GREY, sides=8, caps=True, radii=None, closed=False, at=(0, 0, 0), rot=(0, 0, 0), **kw):
        return self.add(bm_tube(points, r, sides, caps, radii, closed), xf(at, rot), col, **kw)

    def prism(self, size, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, right=False, **kw):
        """SM_FT_Prism / SM_FT_Ramp equivalent: triangle in YZ, extruded along X."""
        sx, sy, sz = size
        prof = [(-sy / 2, -sz / 2), (sy / 2, -sz / 2), ((-sy / 2) if right else 0.0, sz / 2)]
        return self.add(bm_prism(prof, sx), xf(at, rot), col, **kw)

    def poly(self, profile_yz, depth, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, bevel=0.0, **kw):
        """Arbitrary YZ silhouette extruded along X (depth centred)."""
        bm = bm_extrude([(p[0], p[1]) for p in profile_yz], depth, bevel)
        deform(bm, lambda c: Vector((c.z - depth / 2, c.x, c.y)))
        return self.add(bm, xf(at, rot), col, **kw)

    def slab(self, poly_xy, depth, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, bevel=0.0, **kw):
        """Arbitrary XY footprint extruded up by depth from z=0."""
        return self.add(bm_extrude(poly_xy, depth, bevel), xf(at, rot), col, **kw)

    def loft(self, sections, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREY, caps=True, **kw):
        return self.add(bm_loft(sections, caps), xf(at, rot), col, **kw)

    def text(self, s, size, depth=2.0, at=(0, 0, 0), rot=(0, 0, 0), col=P.CREAM, font=None, align='CENTER', bevel=0.0, spacing=1.0, **kw):
        """Letters standing up, facing +X (readable from the front of a sign that faces +X)."""
        bm = bm_text(s, size, depth, font or FONT_SIGN, align, bevel, spacing=spacing)
        # made in XY facing +Z; stand it up facing +X like a yaw-0 UTextRenderComponent: a viewer on
        # the +X side reads it left to right, i.e. along Unreal -Y (the remap mirrors, so re-wind)
        deform(bm, lambda c: Vector((c.z, -c.x, c.y)))
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
        kw.setdefault("ao", False)
        kw.setdefault("jitter", 0.0)
        return self.add(bm, xf(at, rot), col, **kw)

    def leaf(self, length, width, at=(0, 0, 0), rot=(0, 0, 0), col=P.GREEN, droop=0.3, fold=0.18, thick=1.2, segs=7, curl=0.0, **kw):
        """Leaf/frond along local +X from `at`; rot pitch lifts it (positive = up)."""
        kw.setdefault("jitter", 0.0)
        return self.add(bm_leaf(length, width, droop, fold, thick, segs, curl=curl), xf(at, rot), col, **kw)

    def records(self, recs, pivot=(0, 0, 0), bevel=1.0, skip=None, recolor=None, **kw):
        """Upgrade code primitives (layout records, Unreal world space) into kit parts around `pivot`.
        Keeps size/position/colour exactly; adds bevels, rounder curves and the glow slot."""
        from .palette import lin_to_srgb
        px, py, pz = pivot
        shapes = ["Cube", "Cube", "Box", "Box", "Cylinder", "Cylinder", "Sphere", "Ball", "Cone", "Prism",
                  "Ramp", "Torus", "Capsule", "Glass", "Cube", "Shoreline"]
        for r in recs:
            if r["kind"] != "prim" or r["group"] == 14 or (skip and skip(r)):
                continue
            shape = shapes[r["group"]]
            sx, sy, sz = r["size"]
            at = (r["center"][0] - px, r["center"][1] - py, r["center"][2] - pz)
            rot = tuple(r["rot"])
            col = tuple(lin_to_srgb(c) for c in r["color"][:3])
            if recolor:
                col = recolor(r, col)
            e = r["emissive"]
            extra = dict(kw)
            if e >= 0.9:
                extra["glow"] = e
            elif e > 0.05:
                extra["glow"] = e
            rough = 1.0 - 0.7 * r.get("gloss", 0.0)
            extra.setdefault("rough", rough)
            m = min(sx, sy, sz)
            if shape in ("Cube", "Glass"):
                if shape == "Glass":
                    extra["mat"] = "glass"
                    extra.pop("glow", None)
                    col = GLASS_TINT
                b = min(bevel, 0.12 * m) if m > 2.5 else 0.0
                self.add(bm_box(sx, sy, sz, b, 1), xf(at, rot), col, **extra)
            elif shape == "Box":
                self.add(bm_box(sx, sy, sz, min(6.0, max(0.3, 0.12 * m)), 1 if m < 14 else 2), xf(at, rot), col, **extra)
            elif shape == "Cylinder":
                sides = 8 if max(sx, sy) < 12 else (12 if max(sx, sy) < 60 else 20)
                bm = bm_cyl(sx / 2, sz, sides, min(2.0, 0.1 * min(sx, sz)) if min(sx, sz) > 3 else 0.0, 1, None, sy / 2)
                self.add(bm, xf(at, rot), col, **extra)
            elif shape == "Cone":
                sides = 10 if max(sx, sy) < 60 else 16
                bm = bm_cyl(sx / 2, sz, sides, 0.0, 1, 0.0, sy / 2)
                self.add(bm, xf(at, rot), col, **extra)
            elif shape in ("Sphere", "Ball"):
                segs = 8 if max(sx, sy, sz) < 20 else (12 if shape == "Sphere" else 16)
                self.add(bm_sphere(sx / 2, sy / 2, sz / 2, segs, max(4, segs // 2 + 1)), xf(at, rot), col, **extra)
            elif shape in ("Prism", "Ramp"):
                prof = [(-sy / 2, -sz / 2), (sy / 2, -sz / 2), ((-sy / 2) if shape == "Ramp" else 0.0, sz / 2)]
                self.add(bm_prism(prof, sx), xf(at, rot), col, **extra)
            elif shape == "Torus":
                self.add(bm_torus(0.35 * sx, 0.15 * sx, 20, 8, 0.15 * sz), xf(at, rot, (1, sy / sx, 1)), col, **extra)
            elif shape == "Capsule":
                rr = 0.25 * sx
                prof = [(rr * math.cos(t), sz * (-0.5 + 0.25 * (1 + math.sin(t)))) for t in [-math.pi / 2 + k * math.pi / 6 for k in range(4)]]
                prof += [(rr * math.cos(t), sz * (0.25 + 0.25 * math.sin(t))) for t in [k * math.pi / 6 for k in range(4)]]
                self.add(bm_lathe(prof, 8), xf(at, rot, (1, sy / sx, 1)), col, **extra)
            elif shape == "Shoreline":
                self.add(bm_shoreline(sx, sy, sz), xf(at, rot), col, **extra)
        return self

    def socket(self, name, at, rot=(0, 0, 0)):
        self.sockets[name] = {"location": list(at), "rotation": list(rot)}

    # -- stats -----------------------------------------------------------------------------
    def coplanar_overlaps(self, min_area=4.0):
        """Faces of different parts lying in the same plane, facing the same way and overlapping (z-fighting /
        black shading). Returns [(overlap cm^2, part a, part b, centre)] sorted by area."""
        groups = {}
        for fi, face in enumerate(self.f):
            pts = [self.v[i] for i in face]
            if len(pts) < 3:
                continue
            n = (pts[1] - pts[0]).cross(pts[2] - pts[0])
            k = 2
            while n.length < 1e-6 and k + 1 < len(pts):
                n = (pts[k] - pts[0]).cross(pts[k + 1] - pts[0])
                k += 1
            if n.length < 1e-6:
                continue
            n.normalize()
            d = n.dot(pts[0])
            key = (round(n.x, 2), round(n.y, 2), round(n.z, 2), round(d * 4))
            groups.setdefault(key, []).append((fi, n, pts))
        out = []
        for key, faces in groups.items():
            parts = {self.fpart[fi] for fi, _, _ in faces}
            if len(parts) < 2:
                continue
            n = faces[0][1]
            u = n.orthogonal().normalized()
            w = n.cross(u)
            rects = []
            for fi, _, pts in faces:
                us = [p.dot(u) for p in pts]
                ws = [p.dot(w) for p in pts]
                rects.append((self.fpart[fi], min(us), max(us), min(ws), max(ws), pts[0]))
            by_part = {}
            for r in rects:
                by_part.setdefault(r[0], []).append(r)
            keys = sorted(by_part)
            for i in range(len(keys)):
                for j in range(i + 1, len(keys)):
                    area = 0.0
                    where = None
                    for a in by_part[keys[i]]:
                        for b in by_part[keys[j]]:
                            ou = min(a[2], b[2]) - max(a[1], b[1])
                            ow = min(a[4], b[4]) - max(a[3], b[3])
                            if ou > 0.2 and ow > 0.2:
                                area += ou * ow
                                where = a[5]
                    if area >= min_area:
                        out.append((round(area, 1), keys[i], keys[j], tuple(round(x, 1) for x in where)))
        return sorted(out, reverse=True)

    def tri_count(self):
        return sum(len(f) - 2 for f in self.f)

    def bounds(self):
        if not self.v:
            return (Vector((0, 0, 0)), Vector((0, 0, 0)))
        xs = [p.x for p in self.v]
        ys = [p.y for p in self.v]
        zs = [p.z for p in self.v]
        return Vector((min(xs), min(ys), min(zs))), Vector((max(xs), max(ys), max(zs)))

    # -- build -----------------------------------------------------------------------------
    def build(self, collection=None):
        """Create the Blender object (Blender space) at the origin."""
        verts = [(p.x / 1.0, -p.y / 1.0, p.z / 1.0) for p in self.v]
        faces = [tuple(reversed(f)) for f in self.f]
        mesh = bpy.data.meshes.new(self.name)
        mesh.from_pydata(verts, [], faces)
        mesh.validate(clean_customdata=False)
        used = []
        for m in SLOT_ORDER + sorted(set(self.fmat) - set(SLOT_ORDER)):
            if m in self.fmat:
                used.append(m)
        for m in used:
            mesh.materials.append(get_material(m))
        mesh.polygons.foreach_set("material_index", [used.index(m) for m in self.fmat])
        # face attributes that survive triangulation
        flat_attr = mesh.attributes.new("ft_flat", 'INT', 'FACE')
        flat_attr.data.foreach_set("value", [1 if x else 0 for x in self.fflat])
        col = mesh.color_attributes.new("Col", 'BYTE_COLOR', 'CORNER')
        uv = mesh.uv_layers.new(name="UVMap")
        loop_vi = [0] * len(mesh.loops)
        mesh.loops.foreach_get("vertex_index", loop_vi)
        cols = []
        uvs = []
        for pi, poly in enumerate(mesh.polygons):
            c = self.fcol[pi]
            rough = self.frough[pi]
            glow = self.fmat[pi] == MAT_GLOW
            n = poly.normal
            ax = max(range(3), key=lambda i: abs(n[i]))
            for li in poly.loop_indices:
                vi = loop_vi[li]
                s = self.vshade[vi]
                if glow:
                    cols.extend((c[0], c[1], c[2], rough))
                else:
                    cols.extend((min(1.0, c[0] * s), min(1.0, c[1] * s), min(1.0, c[2] * s), rough))
                co = verts[vi]
                if ax == 2:
                    uvs.extend((co[0] / 100.0, co[1] / 100.0))
                elif ax == 0:
                    uvs.extend((co[1] / 100.0, co[2] / 100.0))
                else:
                    uvs.extend((co[0] / 100.0, co[2] / 100.0))
        col.data.foreach_set("color_srgb", cols)
        uv.data.foreach_set("uv", uvs)
        mesh.color_attributes.active_color = col
        mesh.color_attributes.render_color_index = 0

        bm = bmesh.new()
        bm.from_mesh(mesh)
        # no global weld: parts stay separate shells (welding coincident faces of two parts breaks normals)
        bmesh.ops.triangulate(bm, faces=bm.faces, quad_method='BEAUTY', ngon_method='BEAUTY')
        flat_layer = bm.faces.layers.int.get("ft_flat")
        lim = math.radians(self.sharp_angle)
        for e in bm.edges:
            lf = e.link_faces
            sharp = len(lf) != 2
            if not sharp:
                if flat_layer and (lf[0][flat_layer] or lf[1][flat_layer]):
                    sharp = lf[0].normal.angle(lf[1].normal, 0.0) > math.radians(1.0)
                else:
                    sharp = lf[0].normal.angle(lf[1].normal, 0.0) > lim or lf[0].material_index != lf[1].material_index
            e.smooth = not sharp
        for fce in bm.faces:
            fce.smooth = True
        bm.to_mesh(mesh)
        bm.free()
        mesh.attributes.remove(mesh.attributes["ft_flat"])

        obj = bpy.data.objects.new(self.name, mesh)
        (collection or bpy.context.scene.collection).objects.link(obj)
        wn = obj.modifiers.new("WeightedNormal", 'WEIGHTED_NORMAL')
        wn.mode = 'FACE_AREA'
        wn.weight = 50
        wn.keep_sharp = True
        dg = bpy.context.evaluated_depsgraph_get()
        baked = bpy.data.meshes.new_from_object(obj.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
        obj.modifiers.clear()
        old = obj.data
        obj.data = baked
        baked.name = self.name
        bpy.data.meshes.remove(old)
        obj["ft_folder"] = self.folder
        return obj


# --------------------------------------------------------------------------------- materials

def get_material(name):
    m = bpy.data.materials.get(name)
    if m:
        return m
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    vc = nt.nodes.new("ShaderNodeVertexColor")
    vc.layer_name = "Col"
    nt.links.new(vc.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(vc.outputs["Alpha"], bsdf.inputs["Roughness"])
    if name == MAT_GLOW or name.endswith("Lights"):
        # emissive = rgb * 20 * alpha^2 (Unreal: same formula); preview scaled down for Cycles
        nt.links.new(vc.outputs["Color"], bsdf.inputs["Emission Color"])
        sq = nt.nodes.new("ShaderNodeMath")
        sq.operation = 'POWER'
        sq.inputs[1].default_value = 2.0
        nt.links.new(vc.outputs["Alpha"], sq.inputs[0])
        mul = nt.nodes.new("ShaderNodeMath")
        mul.operation = 'MULTIPLY'
        mul.inputs[1].default_value = 20.0 * 0.35
        nt.links.new(sq.outputs[0], mul.inputs[0])
        nt.links.new(mul.outputs[0], bsdf.inputs["Emission Strength"])
        bsdf.inputs["Roughness"].default_value = 0.5
        for l in list(nt.links):
            if l.to_socket == bsdf.inputs["Roughness"]:
                nt.links.remove(l)
    if name == MAT_GLASS or name.endswith("Glass"):
        bsdf.inputs["Alpha"].default_value = 0.35
        m.blend_method = 'BLEND' if hasattr(m, "blend_method") else m.blend_method
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m


import os as _os
_FONT_DIR = _os.path.join(_os.path.dirname(_os.path.dirname(_os.path.abspath(__file__))), "fonts")
FONT_SIGN = _os.path.join(_FONT_DIR, "LilitaOne-Regular.ttf")
FONT_BLOCK = _os.path.join(_FONT_DIR, "Bungee-Regular.ttf")
