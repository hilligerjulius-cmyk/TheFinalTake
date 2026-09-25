"""The crew base character (Characters/FTCharacter.cpp, AFTCharacter::BuildBody) as a modular Blender kit.

Art direction: one charming, slightly goofy base character with a readable silhouette - oversized soft head, compact
torso, slim and slightly long arms, simplified mitten gloves and chunky sneakers, a minimal face (eyes, brows, a small
mouth; emotion comes from eyes, head and posture) and a subtle movie-crew identity (work shirt with pocket and pencil,
crew ID badge, utility belt with tape measure and gaffer tape, walkie-talkie). Everything is soft and bevelled
(subdivided cages, dense lathes) but stays within a multiplayer budget.

Modular: every slot the game swaps is its own mesh, one per StaticMeshComponent the code already toggles and paints
(see SLOTS below): hair, headwear, eyewear, headphones, top, overall bib, sleeves, gloves, belt, walkie, trousers,
trouser hems, shoes, first-person arms. Costumes (cowboy, astronaut, monster, detective, pirate, knight, stunt ...)
sit over the same base on the same joints (Neck / Chest / Hips / Shoulder / Hip frames) and hide the slots they
cover, exactly like the four costumes in AFTCharacter::SetCostumePartsVisible.

Integration without touching gameplay code:
  * Unit space. The code builds each part from a 100 cm unit primitive scaled by Size/100 (FTVis::ApplyShape) and
    UpdateFace rescales eyes, pupils and the mouth every frame. These meshes are modelled at real size in the
    component's frame and then divided by Size/100 (and rotated back by the component rotation), so replacing the
    static mesh is all it takes - joints, expressions, blinking, visibility and painting keep working.
  * Crew colours. Parts FTVis::Paint colours per crew seat / costume carry a near-white vertex colour in the slot
    M_FT_CrewPaint (darker seams and panels); the material multiplies it with the painted colour (custom primitive
    data 0-2). Fixed-colour details (badge, zip, buttons, laces, soles, tape, lenses) use M_FT_Vertex / Glass / Glow.
"""
import math

import bmesh
import bpy
from mathutils import Vector

from ftb import palette as C
from ftb.core import FONT_BLOCK, bm_extrude, bm_lathe, bm_loft, bm_sphere, bm_text, bm_tube, deform, ue_rot, xf
from ftb.registry import ASSETS, asset

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "layout"))
import cppactor as CA  # noqa: E402

SRC = "Characters/FTCharacter.cpp"
PAINT = "M_FT_CrewPaint"
FF, FH, FB, FP = "Characters/Face", "Characters/HairHeadwear", "Characters/Body", "Characters/FirstPerson"

# paint-slot tones (multiplied with the crew colour by the material)
PW = (0.97, 0.97, 0.97)
PS = C.shade(PW, 0.8)        # stitching, seams
PD = C.shade(PW, 0.66)       # grooves, inner ear, gaps
PANEL = C.shade(PW, 0.86)    # two-tone panels (yoke, patches, rubberised palm)
LIPS = C.hex_rgb(0x4A1E2C)
TONGUE = C.hex_rgb(0xE8747A)
SOLE = C.CREAM_DARK

# modular slots: slot -> (code components, what swaps it)
SLOTS = {
    "Base face": ("Head (+Nose), EyeL/R, PupilL/R, BrowL/R, Mouth, EarL/R",
                  "always on (ears hidden under hoods); UpdateFace animates eyes, pupils, brows and mouth"),
    "Hair": ("HairTop, HairBack, HairBun", "crew look HairStyle; hidden under caps, hats, hoods and helmets"),
    "Headwear": ("CapCrown, CapBrim, CapBadge; AccHead anchor", "crew look bCap; replaced by costume headwear or a shop hat on AccHead"),
    "Eyewear": ("GlassL/R; AccFace anchor", "crew look bGlasses; replaced by shop sunglasses on AccFace"),
    "Headphones": ("PhoneL/R, PhoneBand", "crew look bPhones"),
    "Top": ("Torso (+pocket, pencil, crew badge), Collar, ArmL/R sleeves, Bib", "painted Shirt / Sleeve / Bib; costumes recolour or cover it (RCCoat)"),
    "Utility": ("Belt (+buckle, tape measure, gaffer tape), Walkie (+antenna, screen)", "hidden by costumes that cover the waist"),
    "Gloves": ("HandL/R, FPHandL/R, FPCuffL/R", "painted Glove / crew accent"),
    "Trousers": ("Pelvis, LegL/R, CuffL/R", "painted Pants / Legs; shorts-style overlays sit on Hips (LGShorts)"),
    "Shoes": ("ShoeL/R (+soles, laces)", "painted Shoe"),
    "Costume layer": ("LG*, SH*, RC*, FK* parts on Neck / Chest / Hips / Shoulder", "one mesh per costume piece over the base silhouette"),
}

_COMPS = None


def comps():
    global _COMPS
    if _COMPS is None:
        _actor, cs, _sk = CA.run("AFTCharacter")
        _COMPS = {c.name: c for c in cs}
    return _COMPS


def _rigid(c):
    """Actor-space frame of component c without its own scale (the mesh pivot frame)."""
    m = CA.mat_xf(c.loc.t(), c.rot.t())
    return CA.mat_mul(c.parent.matrix(), m) if isinstance(c.parent, CA.Comp) else m


def expect_for(main, merge=()):
    cs = comps()
    inv = CA.mat_inv_rigid_scaled(_rigid(cs[main]))
    lo, hi = [1e9] * 3, [-1e9] * 3
    names = [main] + list(merge)
    for n in names:
        c = cs[n]
        l, h = CA.shape_bounds(c.shape, CA.mat_mul(inv, c.matrix()))
        lo = [min(lo[i], l[i]) for i in range(3)]
        hi = [max(hi[i], h[i]) for i in range(3)]
    return {"min": lo, "max": hi, "parts": len(names)}


def unitize(a, size, rot):
    """Real-size mesh in the parent orientation (relative to the component origin) -> unit space of the primitive."""
    rinv = ue_rot(*rot).transposed()
    pts = [rinv @ v for v in a.v]
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    a.meta["fit_bounds"] = [[round(x, 2) for x in lo], [round(x, 2) for x in hi]]
    a.meta["unit_space"] = {"size": [round(x, 3) for x in size], "rotation": list(rot),
                            "real_size_cm": [round(hi[i] - lo[i], 1) for i in range(3)]}
    s = [x / 100.0 for x in size]
    a.v = [Vector((p.x / s[0], p.y / s[1], p.z / s[2])) for p in pts]


def part(name, comp, folder, desc, merge=(), also=(), view=(1, 0.55, 0.35), note=""):
    """Register a crew part. The builder models at real size in the component's PARENT orientation with the origin
    at the component location; the wrapper turns it into the component's unit space."""
    def deco(fn):
        c = comps()[comp]
        size, rot = c.size.t(), c.rot.t()
        comps_txt = ", ".join((comp,) + tuple(also))
        text = "SetStaticMesh on %s (keep the code's scale, rotation, visibility and FTVis::Paint)" % comps_txt
        if merge:
            text += "; hide %s (merged into this mesh)" % ", ".join(merge)
        text += (". " + note) if note else "."

        def build(a):
            orig = a.add

            def add(bm, matrix=None, col=C.GREY, **kw):
                kw.setdefault("wear", False)     # clean, polished toy finish: no grime / worn edges on the crew
                return orig(bm, matrix, col, **kw)
            a.add = add
            fn(a)
            unitize(a, size, rot)

        asset(name, folder, desc=desc,
              replaces=["AFTCharacter::BuildBody() (%s): %s%s" % (SRC, comps_txt, (" + " + ", ".join(merge)) if merge else "")],
              pivot="%s component origin (parent %s, location %s), unit space of the code primitive %s size %s" %
                    (comp, c.parent.name if isinstance(c.parent, CA.Comp) else "-", c.loc, c.shape, c.size),
              integration=text, view=view, tags=["crew", "unit:%s" % comp])(build)
        spec = ASSETS[-1]
        spec.expect = lambda: expect_for(comp, merge)
        spec.preview_xf = (rot, [x / 100.0 for x in size])
        spec.crew_comp = comp
        spec.crew_also = tuple(also)
        spec.crew_merge = tuple(merge)
        return fn
    return deco


def rel(comp, p):
    """Point in the frame of the component's parent -> relative to the component origin."""
    c = comps()[comp].loc
    return Vector((p[0] - c.x, p[1] - c.y, p[2] - c.z))


# ------------------------------------------------------------------------------ soft modelling helpers

def se(v, e):
    return math.copysign(abs(v) ** e, v)


def rball(rx, ry, rz, e=0.5, segs=24, rings=16):
    """Soft rounded box (superellipsoid-like): e = 1 is a sphere, smaller is boxier with round edges."""
    b = bm_sphere(1.0, 1.0, 1.0, segs, rings)
    return deform(b, lambda c: Vector((se(c.x, e) * rx, se(c.y, e) * ry, se(c.z, e) * rz)))


def cage(sx, sy, sz, cuts=2):
    """Subdivided box cage (full size sx, sy, sz) to deform and then smooth()."""
    b = bmesh.new()
    bmesh.ops.create_cube(b, size=1.0)
    if cuts:
        bmesh.ops.subdivide_edges(b, edges=list(b.edges), cuts=cuts, use_grid_fill=True)
    return deform(b, lambda c: Vector((c.x * sx, c.y * sy, c.z * sz)))


def smooth(b, levels=2):
    """Catmull-Clark subdivision of a cage (Blender's Subdivision Surface modifier)."""
    me = bpy.data.meshes.new("_ft_cage")
    b.to_mesh(me)
    b.free()
    ob = bpy.data.objects.new("_ft_cage", me)
    bpy.context.scene.collection.objects.link(ob)
    mod = ob.modifiers.new("sub", 'SUBSURF')
    mod.levels = mod.render_levels = levels
    dg = bpy.context.evaluated_depsgraph_get()
    out = bmesh.new()
    out.from_object(ob, dg)
    bpy.data.objects.remove(ob)
    bpy.data.meshes.remove(me)
    return out


def catmull(pts, n=6):
    """Smooth polyline through pts (Catmull-Rom), n samples per segment."""
    P = [Vector(p) for p in pts]
    P = [P[0] * 2 - P[1]] + P + [P[-1] * 2 - P[-2]]
    out = []
    for i in range(1, len(P) - 2):
        p0, p1, p2, p3 = P[i - 1], P[i], P[i + 1], P[i + 2]
        for k in range(n):
            t = k / n
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3))
    out.append(P[-2])
    return [tuple(p) for p in out]


def lerp_list(vals, t):
    """Value at t (0..1) of a list sampled evenly over 0..1."""
    x = max(0.0, min(1.0, t)) * (len(vals) - 1)
    i = min(int(x), len(vals) - 2)
    return vals[i] + (vals[i + 1] - vals[i]) * (x - i)


def limb(a, pts, radii, col, n=5, sides=20, **kw):
    """Soft tapered tube through pts (smoothed), radii sampled evenly along it."""
    path = catmull(pts, n)
    rr = [lerp_list(radii, k / (len(path) - 1)) for k in range(len(path))]
    a.add(bm_tube(path, 1.0, sides, True, rr), None, col, **kw)
    return path, rr


def recolor(a, start, fn):
    """Recolour the faces added since face index `start`: fn(centroid) -> colour or None."""
    for fi in range(start, len(a.f)):
        pts = [a.v[i] for i in a.f[fi]]
        col = fn(sum(pts, Vector()) / len(pts))
        if col is not None:
            a.fcol[fi] = tuple(col)


def text_on(a, s, size, surf, depth=0.5, col=C.CREAM, mat="opaque", font=FONT_BLOCK, spacing=1.0):
    """Lettering wrapped onto a surface: surf(u, v) -> (point, unit normal), u along the reading direction and v up;
    the letters stand out along the normal."""
    b = bm_text(s, size, depth, font, 'CENTER', spacing=spacing)

    def f(c):
        p, n = surf(c.x, c.y)
        return Vector(p) + Vector(n) * (c.z - depth * 0.35)
    deform(b, f)
    bmesh.ops.recalc_face_normals(b, faces=b.faces)
    a.add(b, None, col, mat=mat, rough=0.6)


# ============================================================================== head + face

HEAD_R = (23.0, 22.0, 22.0)
NECK_HEAD = 21.0   # Head component z in the Neck frame


def _cheek(zz):
    return 1.0 + 0.07 * math.exp(-((zz + 0.35) / 0.35) ** 2)


def _flat(xs):
    return 0.86 + (xs - 0.86) * 0.7 if xs > 0.86 else xs


def _head_shape(c):
    """Unit sphere -> the crew head: a soft toy head, fuller in the cheeks, with a gently flattened face plane."""
    return Vector((_flat(c.x) * HEAD_R[0], c.y * HEAD_R[1] * _cheek(c.z), c.z * HEAD_R[2]))


def head_x(y, z):
    """Front surface x of the head (Head frame) at lateral y and height z."""
    zz = z / HEAD_R[2]
    yy = y / (HEAD_R[1] * _cheek(zz))
    q = 1.0 - yy * yy - zz * zz
    return _flat(math.sqrt(q)) * HEAD_R[0] if q > 0 else 0.0


def neck_head_x(y, z_neck):
    return head_x(y, z_neck - NECK_HEAD)


def head_dir(yaw_deg, elev_deg, out=0.0):
    """Point on the head ellipsoid (Neck frame) in a direction from the head centre, pushed out by `out` cm."""
    yw, el = math.radians(yaw_deg), math.radians(elev_deg)
    d = Vector((math.cos(el) * math.cos(yw) * HEAD_R[0], math.cos(el) * math.sin(yw) * HEAD_R[1], math.sin(el) * HEAD_R[2]))
    return Vector((0, 0, NECK_HEAD)) + d + d.normalized() * out


@part("SM_Crew_Head", "Head", FF, merge=("Nose",),
      desc="Base head: an oversized soft toy head with full cheeks and a gently flattened face plane, a tiny rounded nose and a neck stub. Deliberately bare - eyes, brows and mouth are separate animated parts.",
      view=(1, 0.55, 0.2))
def crew_head(a):
    a.add(deform(bm_sphere(1.0, 1.0, 1.0, 36, 24), _head_shape), None, PW, mat=PAINT, rough=0.7)
    a.cyl(8.0, 12, at=(-2.5, 0, -24.5), col=C.shade(PW, 0.88), mat=PAINT, sides=20, bevel=2.5, rough=0.7)
    nz = 18.0 - NECK_HEAD
    nx = head_x(0, nz)
    a.sphere(1.5, at=(nx - 0.6, 0, nz), ry=2.2, rz=2.0, col=C.shade(PW, 0.95), mat=PAINT, segs=16, rings=10, rough=0.6)


@part("SM_Crew_Eye", "EyeL", FF, also=("EyeR",),
      desc="Eye: a clean glossy white oval with a thin ink outline where it meets the face - the main carrier of emotion (blink and panic keep scaling it).",
      view=(1, 0.3, 0.2))
def crew_eye(a):
    rx, ry, rz = 3.3, 4.0, 5.5
    a.sphere(rx, ry=ry, rz=rz, col=C.WHITE, segs=24, rings=16, rough=0.15)
    x0 = 1.0
    k = math.sqrt(1 - (x0 / rx) ** 2)
    ring = [(x0, ry * k * math.cos(math.radians(t)), rz * k * math.sin(math.radians(t))) for t in range(0, 360, 15)]
    a.tube(ring, 0.32, col=C.INK, sides=6, caps=False, closed=True, rough=0.4)


@part("SM_Crew_Pupil", "PupilL", FF, also=("PupilR",),
      desc="Pupil: a big glossy ink oval with two catch-lights (large and small) so the eyes read alive from across the stage.",
      view=(1, 0.3, 0.2))
def crew_pupil(a):
    a.sphere(1.0, at=(-0.2, 0, 0), ry=2.5, rz=3.5, col=C.INK, segs=20, rings=12, rough=0.1)
    a.sphere(0.45, at=(0.55, 0.85, 1.35), ry=0.75, rz=0.95, col=C.WHITE, segs=10, rings=6, glow=0.8)
    a.sphere(0.3, at=(0.6, -0.9, -1.4), ry=0.38, rz=0.45, col=C.WHITE, segs=8, rings=5, glow=0.5)


def _brow(a, comp):
    """Pill-shaped brow hugging the forehead."""
    o = comps()[comp].loc
    inner = 1 if o.y < 0 else -1          # direction towards the face centre
    pts, radii = [], []
    for k in range(9):
        t = -1 + 2 * k / 8
        y = 4.6 * t
        z = 0.7 * (1 - t * t) - 0.3 * t * inner      # arched, inner end a touch lower
        pts.append((neck_head_x(o.y + y, o.z + z) + 0.9 - o.x, y, z))
        radii.append(0.75 + 0.55 * (1 - abs(t)) ** 0.5)
    a.tube(pts, 1.0, col=PW, mat=PAINT, sides=10, radii=radii, rough=0.85)
    for p, r in ((pts[0], radii[0]), (pts[-1], radii[-1])):
        a.sphere(r, at=p, col=PW, mat=PAINT, segs=10, rings=6, rough=0.85)


@part("SM_Crew_BrowL", "BrowL", FF, desc="Left eyebrow: a soft pill that hugs the forehead (expressions roll and lift it; paint: hair colour).", view=(1, 0.2, 0.2))
def crew_brow_l(a):
    _brow(a, "BrowL")


@part("SM_Crew_BrowR", "BrowR", FF, desc="Right eyebrow (mirror of SM_Crew_BrowL).", view=(1, -0.2, 0.2))
def crew_brow_r(a):
    _brow(a, "BrowR")


@part("SM_Crew_Mouth", "Mouth", FF,
      desc="Mouth: a small dark smile bean following the face curve, with a tongue tucked in the bottom - a subtle smile at the neutral size, a round 'O' when panic stretches it.",
      view=(1, 0.2, 0.15))
def crew_mouth(a):
    o = comps()["Mouth"].loc
    pts, radii = [], []
    for k in range(11):
        t = -1 + 2 * k / 10
        y, z = 4.8 * t, 0.55 * t * t
        pts.append((neck_head_x(y, o.z + z) - o.x - 0.35, y, z))
        radii.append(0.55 + 0.75 * (1 - t * t) ** 0.5)
    a.tube(pts, 1.0, col=LIPS, sides=10, radii=radii, rough=0.5)
    a.sphere(1.0, at=(pts[5][0] + 0.35, 0, -0.55), ry=2.6, rz=0.6, col=TONGUE, segs=12, rings=6, rough=0.5)


def _ear(a, s):
    """s = -1 left ear (outer side -Y), +1 right."""
    a.sphere(3.0, ry=1.7, rz=4.6, col=PW, mat=PAINT, segs=18, rings=12, rough=0.7)
    a.sphere(1.7, at=(0.35, s * 0.95, 0.2), ry=0.8, rz=2.7, col=C.shade(PW, 0.8), mat=PAINT, segs=12, rings=8, rough=0.8)


@part("SM_Crew_EarL", "EarL", FF, desc="Left ear: a simple rounded ear with one soft inner dimple.", view=(0.2, -1, 0.2))
def crew_ear_l(a):
    _ear(a, -1)


@part("SM_Crew_EarR", "EarR", FF, desc="Right ear (mirror of SM_Crew_EarL).", view=(0.2, 1, 0.2))
def crew_ear_r(a):
    _ear(a, 1)


# ============================================================================== hair

def lock(a, root, tip, r0, bulge=2.0, center=(0, 0, 0), col=PW, sides=10):
    """One sculpted hair clump: a soft tapered lock from root to a rounded point, bowed away from `center`."""
    r, t = Vector(root), Vector(tip)
    mid = (r + t) * 0.5
    mid = mid + (mid - Vector(center)).normalized() * bulge
    limb(a, [tuple(r), tuple(mid), tuple(t)], [r0, r0 * 0.95, r0 * 0.72, r0 * 0.22], col, n=4, sides=sides, mat=PAINT, rough=0.85)


def hair_shell(a, origin, front_z, back_z, grow=(0.8, 0.9, 1.6), col=PW):
    """Hair volume hugging the skull (Neck frame), trimmed by a hairline that runs from front_z (forehead) to back_z."""
    o = Vector(origin)
    b = bm_sphere(1.0, 1.0, 1.0, 36, 22)

    def f(c):
        p = Vector((c.x * (HEAD_R[0] + grow[0]), c.y * (HEAD_R[1] * _cheek(c.z) + grow[1]), c.z * (HEAD_R[2] + grow[2]))) + Vector((0, 0, NECK_HEAD))
        line = back_z + (front_z - back_z) * (0.5 + 0.5 * c.x)
        p.z = max(p.z, line)
        return p - o
    a.add(deform(b, f), None, col, mat=PAINT, rough=0.85)


@part("SM_Crew_HairTop", "HairTop", FH,
      desc="Short hair: a sculpted cap of big soft clumps - a side-swept fringe that stops above the brows, chunky tufts over the crown, sideburns and a cowlick (paint: hair colour).",
      view=(1, 0.7, 0.8))
def crew_hair_top(a):
    o = Vector(comps()["HairTop"].loc.t())
    hc = Vector((0, 0, NECK_HEAD)) - o
    hair_shell(a, o, 36.5, 25.0)
    for k, yaw in enumerate((34, 14, -6, -26)):
        lock(a, head_dir(yaw * 0.6 + 8, 64, 0.6) - o, head_dir(yaw - 12, 40, 1.4) - o, 4.4 - 0.25 * abs(k - 1.5), 1.6, hc)
    for yaw in (-150, -118, -86, -55, 55, 86, 118, 150, 180):
        lock(a, head_dir(yaw * 0.3, 80, 0.4) - o, head_dir(yaw, 24, 1.2) - o, 5.0, 1.8, hc)
    for s in (-1, 1):
        lock(a, head_dir(s * 76, 34, 0.4) - o, head_dir(s * 70, 8, 0.8) - o, 2.8, 0.8, hc)
    top = head_dir(-25, 82, 0.3) - o
    lock(a, top, top + Vector((-3.0, -1.5, 7.0)), 2.2, 1.2, hc)


@part("SM_Crew_HairBack", "HairBack", FH,
      desc="Back hair: a full rounded nape volume with soft clumps falling down and flicking out at the neckline (paint: hair colour).",
      view=(-1, 0.6, 0.3))
def crew_hair_back(a):
    o = Vector(comps()["HairBack"].loc.t())
    hc = Vector((0, 0, NECK_HEAD)) - o
    b = bm_sphere(1.0, 1.0, 1.0, 36, 22)

    def f(c):
        p = Vector((c.x * 24.2 - 0.6, c.y * (HEAD_R[1] * _cheek(c.z) + 1.4), c.z * 23.2)) + Vector((0, 0, NECK_HEAD + 0.4))
        p.x = min(p.x, 5.0 - 0.25 * max(0.0, p.z - 20.0))      # stays behind the ears and the face
        p.z = max(p.z, 8.0 + 0.12 * p.x)
        return p - o
    a.add(deform(b, f), None, PW, mat=PAINT, rough=0.85)
    # broad clumps pressed into the nape, flicking out a little at the neckline
    for k in range(6):
        yaw = 128 + k * 20.8
        swing = 5 * (k < 2) - 5 * (k > 3)
        lock(a, head_dir(yaw, 26, 0.9) - o, head_dir(yaw + swing, -20, 2.4) - o, 5.2, 0.5, hc)
    for yaw in (150, 172, 194, 216):
        root = head_dir(yaw, -16, 1.8) - o
        lock(a, root, root + Vector((-2.2, 0.5 * math.sin(math.radians(yaw)), -2.6)), 2.4, 0.3, hc)


@part("SM_Crew_HairBun", "HairBun", FH, desc="Top-knot bun: wound strands with a coral scrunchie and two loose wisps (paint: hair colour).", view=(-1, 0.6, 0.5))
def crew_hair_bun(a):
    a.sphere(8.8, col=PW, mat=PAINT, segs=24, rings=16, rough=0.85)
    for k in range(3):
        a.torus(6.9 - k * 1.6, 1.5, at=(0, 0, -2.5 + k * 3.0), rot=(10 * (k % 2), k * 50, 0), col=C.shade(PW, 0.9 + 0.05 * (k % 2)),
                mat=PAINT, major=24, minor=8)
    a.torus(5.4, 1.7, at=(1.0, 0, -7.2), rot=(-18, 0, 0), col=C.CORAL, major=20, minor=8, rough=0.6)
    for s in (-1, 1):
        lock(a, (2.0, s * 4.6, -6.0), (5.0, s * 7.0, -13.5), 1.1, 0.4)


# ============================================================================== cap, glasses, headphones

CAP = {"zc": 30.0, "r": (24.0, 23.2, 17.2), "e": 0.8, "front": 35.6, "back": 27.0}


def cap_line(x):
    """Bottom rim height of the cap crown (Neck frame) at forward position x."""
    return CAP["back"] + (CAP["front"] - CAP["back"]) * (0.5 + 0.5 * max(-1.0, min(1.0, x / CAP["r"][0])))


def cap_pt(yaw, el, out=0.0):
    """Point on the crown surface (Neck frame) for a direction (unit-sphere yaw / elevation), pushed out along the normal."""
    rx, ry, rz = CAP["r"]
    e = CAP["e"]
    u = (math.cos(math.radians(el)) * math.cos(math.radians(yaw)), math.cos(math.radians(el)) * math.sin(math.radians(yaw)), math.sin(math.radians(el)))
    p = Vector((se(u[0], e) * rx, se(u[1], e) * ry, se(u[2], e) * rz + CAP["zc"]))
    g = Vector((abs(p.x / rx) ** (2 / e - 1) / rx * math.copysign(1, p.x), abs(p.y / ry) ** (2 / e - 1) / ry * math.copysign(1, p.y),
                abs((p.z - CAP["zc"]) / rz) ** (2 / e - 1) / rz * math.copysign(1, p.z - CAP["zc"])))
    return p + (g.normalized() * out if g.length > 1e-9 else Vector((0, 0, out)))


def cap_rim_el(yaw):
    """Elevation at which the crown surface crosses the rim line in direction yaw."""
    lo, hi = -80.0, 85.0
    for _ in range(40):
        m = (lo + hi) / 2
        p = cap_pt(yaw, m)
        if p.z < cap_line(p.x):
            lo = m
        else:
            hi = m
    return hi


@part("SM_Crew_CapCrown", "CapCrown", FH,
      desc="Crew cap crown: a rounded six-panel crown with stitched seams, embroidered eyelets, a covered top button, a sweatband rim and a snapback strap at the back (paint: cap colour).",
      view=(1, 0.8, 0.6))
def crew_cap_crown(a):
    o = Vector(comps()["CapCrown"].loc.t())
    rx, ry, rz = CAP["r"]
    b = rball(rx, ry, rz, CAP["e"], 36, 24)

    def f(c):
        p = c + Vector((0, 0, CAP["zc"]))
        p.z = max(p.z, cap_line(p.x))
        return p - o
    a.add(deform(b, f), None, PW, mat=PAINT, rough=0.8)
    rim = [cap_pt(y, cap_rim_el(y), 0.2) - o for y in range(0, 360, 10)]
    a.tube(rim, 1.2, col=PS, mat=PAINT, sides=8, caps=False, closed=True, rough=0.8)
    for k in range(6):
        yaw = 30 + 60 * k
        el0 = cap_rim_el(yaw)
        seam = [cap_pt(yaw, el0 + (86 - el0) * t, 0.12) - o for t in (0.04, 0.2, 0.4, 0.6, 0.8, 0.97)]
        a.tube(seam, 0.32, col=PS, mat=PAINT, sides=5, rough=0.8)
        if k in (1, 2, 3, 4):
            ey = yaw + 30
            p = cap_pt(ey, 52, 0.1) - o
            a.torus(0.85, 0.28, at=p, rot=(52 - 90, ey, 0), col=PD, mat=PAINT, major=10, minor=4)
    a.sphere(2.2, at=cap_pt(0, 90, 0.2) - o, rz=1.1, col=C.shade(PW, 0.92), mat=PAINT, segs=14, rings=8)
    # snapback: dark opening arch at the back with the strap across it
    back = cap_pt(180, cap_rim_el(180) + 12, -0.6) - o
    a.sphere(1, at=back, rot=(0, 0, 0), ry=6.5, rz=4.2, col=C.INK, segs=16, rings=8, rough=0.9)
    strap = cap_pt(180, cap_rim_el(180) + 4, 0.2) - o
    a.box((1.2, 12.5, 2.4), at=strap, col=C.shade(PW, 0.82), mat=PAINT, bevel=0.5)
    for y in (-3.6, -1.2, 1.2, 3.6):
        a.cyl(0.55, 0.6, at=strap + Vector((-0.7, y, 0)), rot=(-90, 0, 0), col=C.GREY, sides=10, bevel=0.15)




def _brim_top(y):
    """Top surface height of the peak (Neck frame, before the pitch): curved down towards the sides."""
    return 36.4 - 0.009 * y * y


def _brim_back_x(y):
    return 13.0 + 7.6 * math.sqrt(max(0.0, 1 - (y / 20.5) ** 2))


def _brim_front_x(y, half=16.0):
    return 19.5 + 13.5 * math.sqrt(max(0.0, 1 - (y / (half + 0.5)) ** 2))


@part("SM_Crew_CapBrim", "CapBrim", FH, desc="Curved cap peak with a rounded edge and four rows of stitching; its back edge tucks into the crown (paint: brim colour).", view=(1, 0.5, 0.9))
def crew_cap_brim(a):
    o = Vector(comps()["CapBrim"].loc.t())
    rot = ue_rot(*comps()["CapBrim"].rot.t())
    thick = 1.8
    ys = [16.0 * math.sin(math.radians(t)) for t in range(-90, 91, 10)]
    outline = [(_brim_front_x(y), y) for y in ys] + [(_brim_back_x(y), y) for y in reversed(ys[1:-1])]

    def place(x, y, z):
        """Neck-frame point on the peak -> tilted like the code's brim, relative to the brim origin."""
        return rot @ (Vector((x, y, z)) - o)
    b = bm_extrude(outline, thick, 0.8, 3)
    deform(b, lambda c: place(c.x, c.y, _brim_top(c.y) - thick + c.z))
    a.add(b, None, PW, mat=PAINT, rough=0.8)
    for f in (0.42, 0.6, 0.76, 0.9):
        row = []
        for t in range(-80, 81, 10):
            y = 14.8 * math.sin(math.radians(t))
            x = _brim_back_x(y) + (_brim_front_x(y) - _brim_back_x(y)) * f
            row.append(place(x, y, _brim_top(y) + 0.08))
        a.tube(row, 0.2, col=PS, mat=PAINT, sides=4, rough=0.8)


@part("SM_Crew_CapBadge", "CapBadge", FH, desc="Embroidered clapperboard patch on the cap front: cream slate, striped ink clapper, coral stitched border.", view=(1, 0.3, 0.2))
def crew_cap_badge(a):
    o = Vector(comps()["CapBadge"].loc.t())
    tilt = 32.0
    R = ue_rot(tilt, 0, 0)
    base = cap_pt(0, 38, 0.3) - o

    def at(y, z, out=0.0):
        """Point on the patch plane, which follows the crown slope."""
        return base + R @ Vector((out, y, z))
    a.box((0.8, 8.0, 4.6), at=at(0, -0.9), rot=(tilt, 0, 0), col=C.CREAM, bevel=0.35)
    a.box((0.9, 8.2, 1.7), at=at(0, 2.3, 0.1), rot=(tilt, 0, -5), col=C.INK, bevel=0.3)
    for k in range(4):
        a.box((0.3, 1.0, 1.8), at=at(-3.0 + k * 2.0, 2.3, 0.5), rot=(tilt, 0, 30), col=C.WHITE, bevel=0, jitter=0)
    for k in range(2):
        a.box((0.2, 5.6, 0.35), at=at(-0.6, -0.4 - 1.5 * k, 0.45), rot=(tilt, 0, 0), col=C.shade(C.CREAM, 0.55), bevel=0, jitter=0)
    edge = [at(4.3 * math.cos(math.radians(t)), -0.2 + 3.3 * math.sin(math.radians(t)), 0.3) for t in range(0, 360, 20)]
    a.tube(edge, 0.25, col=C.CORAL, sides=4, caps=False, closed=True)


def _glass(a, comp):
    """One half of the round crew glasses, relative to the lens centre (Neck-frame orientation)."""
    o = Vector(comps()[comp].loc.t())
    s = -1 if o.y < 0 else 1           # outer side
    R = 6.2
    ring = [(0, R * math.cos(math.radians(t)), R * math.sin(math.radians(t))) for t in range(0, 360, 12)]
    a.tube(ring, 0.95, col=PW, mat=PAINT, sides=10, caps=False, closed=True, rough=0.35)
    a.cyl(R - 0.3, 0.35, rot=(-90, 0, 0), col=C.GLASS, sides=30, bevel=0, mat="glass")
    a.box((0.2, 2.8, 0.45), at=(0.35, -1.6, 2.6), col=C.WHITE, glow=0.6, bevel=0, jitter=0)
    # half bridge towards the nose
    a.tube([(0.2, -s * (R + 0.2), 1.0), (1.3, -s * (R + 1.2), 1.9), (1.0, -s * abs(o.y), 1.4)], 0.62, col=PW, mat=PAINT, sides=8)
    # hinge block and a temple that clears the cheek and hooks over the ear
    a.box((1.8, 1.6, 2.2), at=(-0.3, s * (R + 0.6), 1.0), col=C.shade(PW, 0.86), mat=PAINT, bevel=0.5)
    zz = (o.z + 1.2 - NECK_HEAD) / HEAD_R[2]
    pts = []
    for xn in (22.0, 17.0, 11.0, 5.0, 0.5):
        yh = HEAD_R[1] * _cheek(zz) * math.sqrt(max(0.0, 1 - (xn / HEAD_R[0]) ** 2 - zz * zz))
        pts.append((xn - o.x, s * max(R + 0.9, yh + 1.4 - abs(o.y)), 1.2))
    ear = comps()["EarL" if s < 0 else "EarR"].loc
    pts.append((ear.x - 2.5 - o.x, s * (abs(ear.y) + 1.2 - abs(o.y)), ear.z + 4.2 - o.z))
    pts.append((ear.x - 4.2 - o.x, s * (abs(ear.y) + 1.0 - abs(o.y)), ear.z + 1.0 - o.z))
    a.tube(catmull(pts, 3), 0.55, col=PW, mat=PAINT, sides=8)


@part("SM_Crew_GlassL", "GlassL", FH, desc="Round crew glasses, left half: chunky round frame, tinted lens with a glint, half bridge, hinge and a temple hooked over the ear (paint: frame colour).",
      view=(1, -0.6, 0.3))
def crew_glass_l(a):
    _glass(a, "GlassL")


@part("SM_Crew_GlassR", "GlassR", FH, desc="Round crew glasses, right half (mirror of SM_Crew_GlassL).", view=(1, 0.6, 0.3))
def crew_glass_r(a):
    _glass(a, "GlassR")


def _phone_cup(a, comp, cable):
    """Headphone cup around its centre (Neck-frame orientation); the cushion faces the head."""
    s = -1 if comps()[comp].loc.y < 0 else 1        # outer side
    a.add(rball(7.3, 2.5, 7.3, 0.72, 32, 14), None, C.CHARCOAL, rough=0.35)
    a.torus(5.6, 1.9, at=(0, -s * 2.4, 0), rot=(0, 0, 90), col=C.RUBBER, major=24, minor=10, rough=0.9)
    a.cyl(4.0, 0.5, at=(0, -s * 2.3, 0), rot=(0, 0, 90), col=C.INK, sides=20, bevel=0)
    a.cyl(5.0, 0.9, at=(0, s * 2.5, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=24, bevel=0.35)
    a.torus(5.2, 0.45, at=(0, s * 2.95, 0), rot=(0, 0, 90), col=C.CORAL, major=24, minor=5)
    a.box((1.4, 0.4, 2.6), at=(0, s * 3.05, 0.6), col=C.CREAM, bevel=0.15, jitter=0)
    for k in (-1, 1):
        a.tube(catmull([(k * 7.5, s * 0.4, 0), (k * 7.9, s * 0.8, 4.2), (k * 3.2, s * 1.4, 8.2)], 3), 0.65, col=C.GREY, sides=8, rough=0.35)
    a.cyl(0.95, 3.6, at=(0, s * 1.6, 8.8), col=C.GREY, sides=10, bevel=0.3)
    if cable:
        a.tube(catmull([(-3.6, s * 1.2, -6.0), (-6.4, s * 1.0, -11.0), (-5.0, s * 0.8, -17.0), (-2.0, s * 0.8, -20.0)], 4),
               0.5, col=C.RUBBER, sides=6)


@part("SM_Crew_PhoneCupL", "PhoneL", FH, desc="Left headphone cup: soft charcoal shell, fat cushion towards the head, coral trim ring, fork yoke and the cable to the walkie.", view=(0.4, -1, 0.3))
def crew_phone_l(a):
    _phone_cup(a, "PhoneL", True)


@part("SM_Crew_PhoneCupR", "PhoneR", FH, desc="Right headphone cup (mirror of SM_Crew_PhoneCupL, no cable).", view=(0.4, 1, 0.3))
def crew_phone_r(a):
    _phone_cup(a, "PhoneR", False)


@part("SM_Crew_PhoneBand", "PhoneBand", FH, desc="Headphone band arching over the head (and over a cap): steel band, padded top cushion with stitching, chrome sliders at both ends.", view=(1, 0.2, 0.3))
def crew_phone_band(a):
    o = Vector(comps()["PhoneBand"].loc.t())
    ph = comps()["PhoneR"].loc
    half, z0, top = 25.0, ph.z + 8.0 - o.z, 4.3
    arc = [(0.0, half * math.cos(math.radians(t)), z0 + (top - z0) * math.sin(math.radians(t))) for t in range(0, 181, 10)]
    a.tube(arc, 1.1, col=C.GREY_DARK, sides=10, rough=0.4)
    pad = [(p[0], p[1] * 1.04, p[2] + 1.1) for p in arc if abs(p[1]) <= 13.5]
    a.tube(pad, 1.0, col=C.CHARCOAL, sides=12, rough=0.9, radii=[1.2] + [2.0] * (len(pad) - 2) + [1.2])
    a.tube([(p[0], p[1] * 1.05, p[2] + 3.1) for p in pad[1:-1]], 0.18, col=C.GREY_DARK, sides=4)
    for s in (-1, 1):
        a.box((2.6, 2.4, 6.5), at=(0, s * (half - 0.4), z0 + 1.5), col=C.CHROME, bevel=0.6, rough=0.3)
        for k in range(3):
            a.box((2.8, 2.6, 0.35), at=(0, s * (half - 0.4), z0 - 0.4 + k * 1.9), col=C.GREY_DARK, bevel=0, jitter=0)


# ============================================================================== torso, collar, bib

TORSO_KEYS = [(0.70, -22.5), (0.80, -19.0), (0.90, -12.0), (0.97, -3.0), (1.0, 5.0), (0.99, 11.0), (0.95, 16.5),
              (0.83, 20.3), (0.56, 22.4), (0.22, 23.1)]
TRX, TRY = 16.0, 23.0
YOKE_Z = 13.5          # two-tone shoulder yoke above this height (Torso frame)


def torso_r(z):
    k = TORSO_KEYS
    if z <= k[0][1]:
        return k[0][0]
    for (r0, z0), (r1, z1) in zip(k, k[1:]):
        if z <= z1:
            return r0 + (r1 - r0) * (z - z0) / (z1 - z0)
    return k[-1][0]


def torso_pt(y, z, out=0.0, back=False):
    """Point on the shirt surface (Torso frame) at lateral y / height z, front or back, pushed out along the normal."""
    r = torso_r(z)
    rx, ry = TRX * r, TRY * r
    x = rx * math.sqrt(max(0.0, 1 - (y / ry) ** 2)) * (-1 if back else 1)
    n = Vector((x / rx ** 2, y / ry ** 2, 0)).normalized()
    return Vector((x, y, z)) + n * out, n


def yaw_of(n):
    return math.degrees(math.atan2(n.y, n.x))


def sweep(a, path, profile, col, **kw):
    """Closed sweep: a 2D profile [(out, up)] carried around a closed horizontal path (outward = away from the path centre)."""
    c = sum((Vector(p) for p in path), Vector()) / len(path)
    rings = []
    for p in path + [path[0]]:
        p = Vector(p)
        n = Vector((p.x - c.x, p.y - c.y, 0)).normalized()
        rings.append([p + n * u + Vector((0, 0, w)) for (u, w) in profile])
    b = bm_loft(rings, caps=False)
    bmesh.ops.remove_doubles(b, verts=b.verts, dist=1e-4)
    a.add(b, None, col, **kw)


def rrect(w, h, n=5):
    """Rounded-rectangle loop (w x h) for sweeps, counter-clockwise."""
    r = min(w, h) * 0.45
    pts = []
    for cx, cy, a0 in ((w / 2 - r, h / 2 - r, 0), (-w / 2 + r, h / 2 - r, 90), (-w / 2 + r, -h / 2 + r, 180), (w / 2 - r, -h / 2 + r, 270)):
        for k in range(n):
            t = math.radians(a0 + 90 * k / (n - 1))
            pts.append((cx + r * math.cos(t), cy + r * math.sin(t)))
    return pts


def on_torso(a, bm, y, z, out, col, spin=0.0, back=False, **kw):
    """Place a part built facing +X onto the shirt surface."""
    p, n = torso_pt(y, z, out, back)
    a.add(bm, xf(p, (0, yaw_of(n), spin)), col, **kw)
    return p, n


@part("SM_Crew_Torso", "Torso", FB,
      merge=("CrewPocket", "CrewPocketFlap", "CrewPencil", "CrewBadge", "CrewBadgeStripe"),
      desc="Crew work shirt: a compact rounded torso with a two-tone shoulder yoke and piping, zip placket, side seams, chest pocket with flap and pencil, a clip-on crew ID badge (clapperboard icon, coral stripe) and a small CREW print on the back (paint: shirt colour).",
      view=(1, 0.6, 0.3))
def crew_torso(a):
    prof = [(p[0], p[1]) for p in catmull([(r, z, 0) for r, z in TORSO_KEYS], 4)]
    prof = sorted([q for q in prof if abs(q[1] - YOKE_Z) > 0.3] + [(torso_r(YOKE_Z), YOKE_Z)], key=lambda q: q[1]) + [(0.0, 23.2)]
    start = len(a.f)
    b = bm_lathe(prof, 40)
    a.add(deform(b, lambda c: Vector((c.x * TRX, c.y * TRY, c.z))), None, PW, mat=PAINT, rough=0.8)
    recolor(a, start, lambda c: PANEL if c.z > YOKE_Z else None)
    r = torso_r(YOKE_Z)
    a.tube([(TRX * r * math.cos(math.radians(t)) * 1.012, TRY * r * math.sin(math.radians(t)) * 1.012, YOKE_Z) for t in range(0, 360, 8)],
           0.38, col=PS, mat=PAINT, sides=6, caps=False, closed=True)
    # side seams and zip placket
    for s in (-1, 1):
        a.tube([(0.0, s * (TRY * torso_r(z) + 0.1), z) for z in (-21, -12, -3, 5, 11, 16)], 0.3, col=PS, mat=PAINT, sides=5)
    zip_pts = [torso_pt(0, z, 0.25)[0] for z in (20.5, 17, 13, 9.5, 7.0)]
    a.tube(zip_pts, 0.7, col=C.shade(PW, 0.74), mat=PAINT, sides=8)
    for z in (19.0, 16.5, 14.0, 11.5, 9.0):
        a.box((0.5, 1.6, 0.35), at=torso_pt(0, z, 0.9)[0], col=C.GREY, bevel=0.1, jitter=0)
    pull, n = torso_pt(0, 18.6, 1.3)
    a.box((0.5, 1.1, 2.4), at=pull + Vector((0, 0, -1.0)), rot=(0, yaw_of(n), 0), col=C.CHROME, bevel=0.25, rough=0.3)
    # chest pocket (fixed accent colour like the code), flap, stitching and a pencil
    pocket = C.TEAL_DARK
    py, pz = -10.5, 4.0
    on_torso(a, rball(0.9, 5.6, 6.0, 0.3, 20, 14), py, pz, 0.55, pocket, rough=0.8)
    p, n = torso_pt(py, pz, 1.35)
    t = Vector((-n.y, n.x, 0))
    stitch = [p + t * 5.0 * math.cos(math.radians(k)) + Vector((0, 0, 5.3 * math.sin(math.radians(k)))) for k in range(-90, 271, 20)]
    a.tube(stitch, 0.16, col=C.shade(pocket, 0.7), sides=4)
    on_torso(a, rball(1.0, 6.0, 2.0, 0.35, 20, 10), py, pz + 5.2, 1.3, C.CREAM_DARK, rough=0.8)
    fb, n = torso_pt(py, pz + 4.4, 2.3)
    a.cyl(0.55, 0.4, at=fb, rot=(-90, yaw_of(n), 0), col=C.GREY, sides=10, bevel=0.1)
    pb, n = torso_pt(py - 3.0, pz + 3.0, 1.2)
    top = pb + Vector((0.2, -0.6, 10.5))
    a.tube([pb, top], 0.95, col=C.YELLOW, sides=6, rough=0.6)
    a.cyl(1.0, 1.4, at=top + Vector((0.02, -0.06, 0.7)), col=C.GREY, sides=10, bevel=0.2)
    a.cyl(0.95, 1.2, at=top + Vector((0.04, -0.1, 1.9)), col=C.hex_rgb(0xF08A9A), sides=10, bevel=0.4)
    # crew ID badge on the right chest: card, coral stripe, clapper icon, text lines, metal clip
    by, bz = 10.5, 11.5
    p, n = torso_pt(by, bz, 0.7)
    yw = yaw_of(n)
    R = ue_rot(0, yw, 0)

    def card(u, v, out=0.0):
        return p + R @ Vector((out, u, v))
    a.add(rball(0.35, 4.6, 3.2, 0.25, 20, 10), xf(p, (0, yw, 0)), C.CREAM, rough=0.4)
    a.add(rball(0.15, 4.5, 0.9, 0.25, 16, 6), xf(card(0, 2.0, 0.35), (0, yw, 0)), C.CORAL, rough=0.5)
    a.box((0.3, 2.6, 2.0), at=card(-2.6, -0.6, 0.35), rot=(0, yw, 0), col=C.INK, bevel=0.2, jitter=0)
    for k in range(3):
        a.box((0.25, 0.55, 0.7), at=card(-3.4 + k * 0.85, 0.25, 0.5), rot=(0, yw, 35), col=C.WHITE, bevel=0, jitter=0)
    for k, w in enumerate((4.0, 3.2, 3.6)):
        a.box((0.2, w, 0.4), at=card(0.2 + w / 2 - 1.4, 0.5 - k * 1.0, 0.36), rot=(0, yw, 0), col=C.shade(C.CREAM, 0.55), bevel=0, jitter=0)
    a.box((0.9, 1.6, 1.6), at=card(0, 3.8, 0.1), rot=(0, yw, 0), col=C.GREY, bevel=0.3, rough=0.35)
    # CREW print on the back, under the yoke
    text_on(a, "CREW", 6.0, lambda u, v: torso_pt(u, 6.0 + v, 0.0, back=True), depth=0.6, col=C.CREAM, spacing=1.05)


@part("SM_Crew_Collar", "Collar", FB, desc="Ribbed crew-neck collar ring that dips slightly at the front (paint: cream, or the costume shirt colour).", view=(1, 0.5, 0.8))
def crew_collar(a):
    ring = []
    for t in range(0, 360, 8):
        c, s = math.cos(math.radians(t)), math.sin(math.radians(t))
        ring.append((10.8 * c - 0.6, 12.8 * s, -1.4 * max(0.0, c) ** 2))
    a.tube(ring, 2.1, col=PW, mat=PAINT, sides=12, caps=False, closed=True, rough=0.85)
    for dz in (1.1, -1.1):
        rib = [Vector((x, y, z + dz)) + Vector((x + 0.6, y, 0)).normalized() * 1.75 for (x, y, z) in ring]
        a.tube(rib, 0.35, col=PS, mat=PAINT, sides=5, caps=False, closed=True)


@part("SM_Crew_Bib", "Bib", FB, desc="Overall bib: a soft panel that follows the chest, with a stitched patch pocket, two metal buttons and straps running up under the collar (paint: bib colour).", view=(1, 0.5, 0.3))
def crew_bib(a):
    o = rel("Torso", comps()["Bib"].loc.t())       # bib origin in the Torso frame

    def surf(y, z, out):
        return torso_pt(y, z, out)[0] - o
    b = cage(1.0, 23.0, 25.0, cuts=4)

    def f(c):
        z = o.z + c.z
        w = 1.0 - 0.18 * max(0.0, (c.z + 4.0) / 16.5)          # narrower towards the top
        return surf(c.y * w, z, 1.9 + c.x)
    a.add(smooth(deform(b, f), 1), None, PW, mat=PAINT, rough=0.8)
    edge = []
    for k in range(0, 360, 12):
        t = math.radians(k)
        yy, zz = 9.8 * se(math.cos(t), 0.4), 11.2 * se(math.sin(t), 0.4)
        w = 1.0 - 0.18 * max(0.0, (zz + 4.0) / 16.5)
        edge.append(surf(yy * w, o.z + zz, 2.5))
    a.tube(edge, 0.2, col=PS, mat=PAINT, sides=4, caps=False, closed=True)
    pocket = [surf(4.2 * se(math.cos(math.radians(k)), 0.35), o.z - 1.5 + 3.6 * se(math.sin(math.radians(k)), 0.35), 2.6) for k in range(0, 360, 15)]
    a.add(bm_loft([pocket, [p + Vector((0.5, 0, 0)) for p in pocket]], caps=True), None, PANEL, mat=PAINT, rough=0.8)
    for s in (-1, 1):
        btn = surf(s * 8.2, o.z + 10.5, 2.8)
        a.cyl(1.25, 0.8, at=btn, rot=(-90, s * 20, 0), col=C.CHROME, sides=16, bevel=0.3, rough=0.3)
        strap = [surf(s * 8.2, o.z + 10.0, 2.2), surf(s * 8.8, o.z + 15.0, 1.4), surf(s * 9.2, 20.2, 1.0) + Vector((-1.0, 0, 0.4))]
        a.tube(strap, 1.3, col=PW, mat=PAINT, sides=8)


# ============================================================================== utility belt, walkie

BELT_R = (14.9, 18.7)


def belt_pt(deg, out=0.0, z=0.0):
    """Point on the belt's outer face (Belt frame) at angle deg (0 = front, -90 = left hip) and its outward normal."""
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    rx, ry = BELT_R
    n = Vector((c / rx, s / ry, 0)).normalized()
    return Vector((rx * c, ry * s, z)) + n * (0.75 + out), n


@part("SM_Crew_Belt", "Belt", FB, merge=("CrewBeltBuckle",),
      desc="Utility belt: a rounded strap with a cream buckle and punched tail, a yellow tape measure on the left hip, a roll of gaffer tape on a carabiner and a canvas pouch at the back.",
      view=(1, 0.8, 0.35))
def crew_belt(a):
    rx, ry = BELT_R
    path = [(rx * math.cos(math.radians(t)), ry * math.sin(math.radians(t)), 0.0) for t in range(0, 360, 8)]
    sweep(a, path, rrect(1.5, 5.0, 4), C.CHARCOAL, rough=0.55)
    # tail tab with punched holes left of the buckle
    p, n = belt_pt(-17, 0.25)
    a.add(rball(0.45, 3.6, 2.2, 0.35, 16, 8), xf(p, (0, yaw_of(n), 0)), C.shade(C.CHARCOAL, 1.15), rough=0.55)
    for k, d in enumerate((-12.5, -17.0, -21.5)):
        q, m = belt_pt(d, 0.72 if k else 0.3)
        a.cyl(0.42, 0.3, at=q, rot=(-90, yaw_of(m), 0), col=C.INK, sides=8, bevel=0)
    # buckle (the code's CrewBeltBuckle colour)
    bx = rx + 1.3
    frame = [(bx, u, w) for (u, w) in rrect(8.4, 5.6, 6)]
    a.add(rball(0.35, 3.9, 2.5, 0.35, 16, 8), xf((bx - 0.45, 0, 0)), C.shade(C.CREAM_DARK, 0.8), rough=0.4)
    a.tube(frame, 0.6, col=C.CREAM_DARK, sides=8, caps=False, closed=True, rough=0.35)
    a.box((0.45, 0.6, 4.8), at=(bx + 0.2, -1.2, 0), col=C.CREAM_DARK, bevel=0.15, rough=0.35)
    # tape measure
    p, n = belt_pt(-58, 2.3)
    yw = yaw_of(n)
    R = ue_rot(0, yw, 0)
    a.add(rball(1.9, 3.6, 3.6, 0.55, 24, 14), xf(p, (0, yw, 0)), C.YELLOW, rough=0.45)
    a.cyl(2.5, 0.5, at=p + R @ Vector((1.8, 0, 0.1)), rot=(-90, yw, 0), col=C.CHARCOAL, sides=20, bevel=0.2)
    a.cyl(0.8, 0.5, at=p + R @ Vector((2.1, 0, 0.1)), rot=(-90, yw, 0), col=C.GREY, sides=12, bevel=0.15)
    a.box((1.2, 2.4, 0.35), at=p + R @ Vector((0.3, 2.9, -3.3)), rot=(0, yw, 0), col=C.hex_rgb(0xE8E2C8), bevel=0.1, jitter=0)
    a.box((1.2, 0.5, 1.0), at=p + R @ Vector((0.3, 4.0, -3.0)), rot=(0, yw, 0), col=C.GREY_DARK, bevel=0.15, jitter=0)
    a.box((0.9, 2.2, 1.0), at=p + R @ Vector((0.2, 0.0, 3.8)), rot=(0, yw, 0), col=C.CHARCOAL, bevel=0.3)
    a.box((0.5, 1.8, 5.0), at=p + R @ Vector((-2.1, 0.0, 1.0)), rot=(0, yw, 0), col=C.GREY_DARK, bevel=0.2)
    # gaffer tape roll on a carabiner
    p, n = belt_pt(-138, 0.4)
    yw = yaw_of(n)
    R = ue_rot(0, yw, 0)
    a.torus(1.7, 0.32, at=p + R @ Vector((0.6, 0, -2.4)), rot=(0, yw + 90, 90), col=C.GREY, major=16, minor=6, rough=0.3)
    roll = p + R @ Vector((1.9, 0, -7.2))
    a.torus(3.0, 1.3, at=roll, rot=(-90, yw, 0), col=C.hex_rgb(0x3A3F4E), major=24, minor=10, rz=1.25, rough=0.7)
    a.cyl(1.75, 2.3, at=roll, rot=(-90, yw, 0), col=C.CREAM_DARK, sides=20, bevel=0.2, rough=0.8)
    a.cyl(1.25, 2.4, at=roll, rot=(-90, yw, 0), col=C.shade(C.CREAM_DARK, 0.45), sides=16, bevel=0)
    a.box((0.6, 0.4, 2.8), at=roll + R @ Vector((0.4, 3.9, -1.8)), rot=(0, yw, 20), col=C.hex_rgb(0x3A3F4E), bevel=0.1, jitter=0)
    # canvas pouch at the back right
    p, n = belt_pt(150, 1.6, -0.6)
    yw = yaw_of(n)
    R = ue_rot(0, yw, 0)
    a.add(rball(1.6, 3.4, 3.8, 0.45, 20, 12), xf(p, (0, yw, 0)), C.GREY_DARK, rough=0.85)
    a.add(rball(0.7, 3.6, 1.7, 0.4, 16, 8), xf(p + R @ Vector((1.2, 0, 2.6)), (0, yw, 0)), C.shade(C.GREY_DARK, 1.15), rough=0.85)
    a.cyl(0.5, 0.4, at=p + R @ Vector((1.95, 0, 2.2)), rot=(-90, yw, 0), col=C.GREY, sides=10, bevel=0.1)


@part("SM_Crew_Walkie", "Walkie", FB, merge=("CrewRadioAntenna", "CrewRadioScreen"),
      desc="Walkie-talkie on the right hip: soft charcoal body, glowing teal screen in a bezel, speaker slots, push-to-talk button, channel knob, red status LED, rubber antenna and belt clip.",
      view=(1, 1, 0.3))
def crew_walkie(a):
    c = Vector((0.0, -0.6, 0.0))
    a.add(rball(2.9, 2.4, 5.9, 0.45, 24, 16), xf(c), C.CHARCOAL, rough=0.45)
    a.box((0.3, 3.8, 3.6), at=c + Vector((2.75, 0, 2.2)), col=C.INK, bevel=0.3, jitter=0)
    a.box((0.3, 3.0, 2.8), at=c + Vector((2.95, 0, 2.2)), col=C.TEAL_LIGHT, glow=1.6, bevel=0.1, jitter=0)
    a.box((0.2, 1.6, 0.3), at=c + Vector((3.12, -0.3, 2.9)), col=C.shade(C.TEAL_LIGHT, 0.6), glow=0.8, bevel=0, jitter=0)
    for k in range(3):
        a.box((0.35, 3.0, 0.35), at=c + Vector((2.8, 0, -1.2 - k * 1.0)), col=C.INK, bevel=0.1, jitter=0)
    a.add(rball(0.9, 0.55, 1.4, 0.5, 12, 8), xf(c + Vector((0.2, 2.35, 0.8))), C.GREY, rough=0.5)
    a.cyl(0.8, 1.3, at=c + Vector((1.0, 0.9, 6.2)), col=C.GREY_DARK, sides=14, bevel=0.25)
    a.sphere(0.35, at=c + Vector((1.9, -0.6, 5.9)), col=C.RED, glow=2.0, segs=8, rings=5)
    limb(a, [c + Vector((-0.9, 0.3, 5.0)), c + Vector((-0.9, 0.3, 11.0)), c + Vector((-0.9, 0.3, 17.0))], [0.8, 0.6, 0.45], C.RUBBER, n=3, sides=10, rough=0.7)
    a.sphere(0.6, at=c + Vector((-0.9, 0.3, 17.2)), col=C.RUBBER, segs=10, rings=6)
    a.box((3.4, 0.35, 6.0), at=c + Vector((0, -2.55, 1.2)), col=C.GREY, bevel=0.15, rough=0.4)


# ============================================================================== trousers + shoes

PEL = (14.2, 17.0, 8.4, 0.5)


def pel_pt(y, z, out=0.0, back=False):
    """Point on the trouser seat (Pelvis frame) and its normal."""
    rx, ry, rz, e = PEL
    q = 1.0 - abs(y / ry) ** (2 / e) - abs(z / rz) ** (2 / e)
    x = rx * max(q, 0.0) ** (e / 2) * (-1 if back else 1)
    k = 2 / e - 1
    n = Vector((math.copysign(abs(x / rx) ** k / rx, x), math.copysign(abs(y / ry) ** k / ry, y), math.copysign(abs(z / rz) ** k / rz, z)))
    n = n.normalized() if n.length > 1e-9 else Vector((1 if not back else -1, 0, 0))
    return Vector((x, y, z)) + n * out, n


@part("SM_Crew_Pelvis", "Pelvis", FB, desc="Trouser seat: a soft rounded block with fly seam, back patch pockets and belt loops (paint: trouser colour).", view=(1, 0.8, 0.3))
def crew_pelvis(a):
    rx, ry, rz, e = PEL
    a.add(rball(rx, ry, rz, e, 32, 20), None, PW, mat=PAINT, rough=0.8)
    fly = [pel_pt(1.4, z, 0.1)[0] for z in (6.0, 3.0, 0.0, -2.5)] + [pel_pt(0.5, -4.2, 0.1)[0], pel_pt(0.0, -5.0, 0.1)[0]]
    a.tube(fly, 0.28, col=PS, mat=PAINT, sides=5)
    for s in (-1, 1):
        p, n = pel_pt(s * 6.8, 0.2, 0.25, back=True)
        a.add(rball(0.45, 3.8, 3.4, 0.3, 16, 10), xf(p, (0, yaw_of(n), 0)), PANEL, mat=PAINT, rough=0.8)
    for deg in (28, -28, 90, -90, 180):
        y = ry * 0.98 * math.sin(math.radians(deg))
        p, n = pel_pt(y, 5.4, 0.35, back=abs(deg) > 90)
        a.add(rball(0.45, 0.9, 2.4, 0.4, 10, 8), xf(p, (0, yaw_of(n), 0)), PANEL, mat=PAINT, rough=0.8)


def _leg(a, s):
    """s = -1 left leg (outer side -Y), +1 right. Leg frame: hip joint at z +21."""
    path, rr = limb(a, [(0, 0, 20.5), (0.3, 0, 4.0), (0.4, 0, -12.5)], [7.0, 7.1, 6.9, 6.6], PW, n=6, sides=28, mat=PAINT, rough=0.8)
    a.add(rball(0.7, 4.2, 4.6, 0.35, 18, 12), xf((7.25, 0, -1.0)), PANEL, mat=PAINT, rough=0.8)
    ring = [(7.95, 3.6 * se(math.cos(math.radians(k)), 0.4), -1.0 + 4.0 * se(math.sin(math.radians(k)), 0.4)) for k in range(0, 360, 20)]
    a.tube(ring, 0.18, col=PS, mat=PAINT, sides=4, caps=False, closed=True)
    a.add(rball(3.6, 0.8, 4.2, 0.35, 16, 12), xf((0.2, s * 7.4, 6.2)), PANEL, mat=PAINT, rough=0.8)
    a.add(rball(3.9, 0.9, 1.3, 0.4, 16, 8), xf((0.2, s * 8.0, 10.3)), PW, mat=PAINT, rough=0.8)
    a.cyl(0.5, 0.4, at=(0.2, s * 8.95, 10.0), rot=(0, 0, 90), col=C.GREY, sides=10, bevel=0.1)
    seam = [(0.35 * (1 - t), s * (lerp_list([7.0, 7.1, 6.9, 6.6], t) + 0.04), 20.0 - 32.5 * t) for t in (0.08, 0.3, 0.55, 0.8, 1.0)]
    a.tube(seam, 0.25, col=PS, mat=PAINT, sides=5)


@part("SM_Crew_LegL", "LegL", FB, desc="Left trouser leg: roomy work trousers with a stitched knee patch, a cargo pocket with snap flap on the outer thigh and an outside seam (paint: trouser colour).", view=(1, -0.8, 0.2))
def crew_leg_l(a):
    _leg(a, -1)


@part("SM_Crew_LegR", "LegR", FB, desc="Right trouser leg (mirror of SM_Crew_LegL).", view=(1, 0.8, 0.2))
def crew_leg_r(a):
    _leg(a, 1)


@part("SM_Crew_Cuff", "CuffL", FB, also=("CuffR",), desc="Rolled-up trouser hem: two soft rolls over the ankle (paint: trouser colour, a shade darker).", view=(1, 0.6, 0.4))
def crew_cuff(a):
    a.cyl(6.8, 6.4, col=PW, mat=PAINT, sides=24, bevel=1.0)
    a.torus(7.2, 2.0, at=(0, 0, 1.4), col=PW, mat=PAINT, major=32, minor=10, rough=0.8)
    a.torus(7.1, 1.8, at=(0, 0, -1.7), col=C.shade(PW, 0.93), mat=PAINT, major=32, minor=10, rough=0.8)


def SHOE_TOP(t):
    """Upper height over the shoe length (t = -1 heel .. 1 toe)."""
    return 4.8 - 0.8 * t if t < 0 else 4.8 - 3.2 * t * t


@part("SM_Crew_Shoe", "ShoeL", FB, also=("ShoeR",),
      merge=("CrewSole0", "CrewLace0_0", "CrewLace0_1", "CrewLace0_2"),
      note="Also hide CrewSole1 and CrewLace1_0..2 on the right foot.",
      desc="Chunky sneaker / work shoe: soft rounded upper with an upturned toe, cream rubber sole with a midsole line and toe bumper, padded tongue, cream laces with a little bow, eyelets and a heel pull tab (paint: shoe colour).",
      view=(1, 0.7, 0.45))
def crew_shoe(a):
    sole = SOLE
    a.add(rball(13.0, 7.7, 1.7, 0.3, 32, 12), xf((0, 0, -3.9)), sole, rough=0.7)
    line = [(13.05 * se(math.cos(math.radians(k)), 0.3), 7.75 * se(math.sin(math.radians(k)), 0.3), -3.7) for k in range(0, 360, 6)]
    a.tube(line, 0.28, col=C.shade(sole, 0.7), sides=5, caps=False, closed=True)
    half = 4.2
    b = cage(23.4, 14.8, 2 * half, cuts=1)

    def up(c):
        t = max(-1.0, min(1.0, c.x / 11.2))
        top = SHOE_TOP(t)
        z = -2.6 + (c.z + half) / (2 * half) * (top + 2.6)
        return Vector((c.x - 0.4, c.y * (1 - 0.22 * abs(t) ** 3), z))
    a.add(smooth(deform(b, up), 2), None, PW, mat=PAINT, rough=0.7)
    a.add(rball(4.0, 6.2, 2.5, 0.75, 20, 12), xf((8.9, 0, -1.4)), sole, rough=0.7)
    a.add(rball(2.9, 3.3, 0.8, 0.6, 16, 8), xf((-3.2, 0, 5.7), (25, 0, 0)), PANEL, mat=PAINT, rough=0.8)
    a.add(rball(1.0, 1.8, 1.9, 0.5, 12, 8), xf((-11.6, 0, 4.3)), PANEL, mat=PAINT, rough=0.8)

    def top_z(x):
        return SHOE_TOP(max(-1.0, min(1.0, (x + 0.4) / 11.2))) - 0.55
    for x in (-2.0, 2.0, 6.0):
        z = top_z(x) + 0.3
        a.tube([(x, -3.1, z - 0.2), (x + 0.2, 0, z + 0.15), (x, 3.1, z - 0.2)], 0.5, col=C.CREAM, sides=8, rough=0.6)
        for s in (-1, 1):
            a.sphere(0.4, at=(x - 0.9, s * 3.4, z - 0.55), col=C.INK, segs=8, rings=5)
    zb = top_z(-4.6) + 0.5
    for s in (-1, 1):
        a.torus(1.3, 0.42, at=(-4.8, s * 1.7, zb + 0.2), rot=(0, 0, s * 22), col=C.CREAM, major=14, minor=6, rough=0.6)
        a.tube([(-4.6, s * 0.4, zb), (-5.6, s * 1.4, zb - 0.6), (-6.2, s * 2.6, zb - 1.6)], 0.38, col=C.CREAM, sides=6)
    a.sphere(0.7, at=(-4.6, 0, zb + 0.1), col=C.CREAM, segs=10, rings=6)
    for v in a.v:
        v.z += 0.022 * max(0.0, v.x - 5.0) ** 2           # toe spring: the whole front curls up a little


# ============================================================================== arms + gloves

ARM_PATH = [(0, 0, 13.0), (-0.2, 0, 4.0), (-0.6, 0, -5.0), (0.4, 0, -14.5), (1.7, 0, -23.6)]
ARM_R = [6.1, 5.8, 5.2, 4.8, 4.5]


@part("SM_Crew_Arm", "ArmL", FB, also=("ArmR",),
      desc="Sleeve: a slim, slightly long arm with a relaxed bend at the elbow, soft shoulder cap, back seam and a rolled cuff at the wrist (paint: sleeve colour).", view=(1, -0.7, 0.2))
def crew_arm(a):
    path, rr = limb(a, ARM_PATH, ARM_R, PW, n=5, sides=24, mat=PAINT, rough=0.8)
    a.sphere(6.3, at=(0, 0, 13.0), col=PW, mat=PAINT, segs=24, rings=14, rough=0.8)
    a.torus(4.6, 1.1, at=(1.65, 0, -22.8), rot=(7.7, 0, 0), col=PANEL, mat=PAINT, major=24, minor=8, rough=0.8)
    a.tube([(p[0] - r - 0.05, p[1], p[2]) for p, r in list(zip(path, rr))[2:-3]], 0.25, col=PS, mat=PAINT, sides=5)
    for dz in (-3.0, -5.0):
        a.tube([(4.9, -2.6, dz + 0.5), (5.6, 0, dz), (4.9, 2.6, dz + 0.5)], 0.22, col=PS, mat=PAINT, sides=4)


def _hand(a, s_in):
    """Mitten work glove. s_in = direction towards the body (+1 for the left hand)."""
    a.cone(5.2, 3.2, at=(1.7, 0, -1.3), rot=(7.7, 0, 0), col=PANEL, mat=PAINT, sides=24, r_top=5.8, bevel=0.6)
    a.box((4.2, 0.8, 2.0), at=(1.7, -s_in * 5.7, -1.3), col=C.shade(PW, 0.8), mat=PAINT, bevel=0.35)
    b = cage(11.0, 8.0, 13.0, cuts=2)

    def f(c):
        t = -c.z / 6.5
        x = c.x * (1 + 0.06 * t - 0.12 * max(0.0, -t))
        y = c.y * (1 - 0.12 * max(0.0, -t)) + s_in * 1.6 * max(0.0, t) ** 2
        return Vector((x + 2.2, y, c.z - 9.0))
    a.add(smooth(deform(b, f), 2), None, PW, mat=PAINT, rough=0.75)
    th = [(6.0, s_in * 1.6, -5.0), (8.0, s_in * 2.8, -8.2), (8.3, s_in * 3.2, -10.8)]
    limb(a, th, [2.5, 2.4, 2.1], PW, n=4, sides=14, mat=PAINT, rough=0.75)
    a.sphere(2.05, at=th[-1], col=PW, mat=PAINT, segs=14, rings=8, rough=0.75)
    a.add(rball(3.8, 0.8, 4.4, 0.5, 16, 10), xf((2.0, s_in * 3.8, -10.0)), PANEL, mat=PAINT, rough=0.9)
    for x in (0.4, 3.6):
        a.tube([(x, -s_in * 3.55, -9.5), (x, -s_in * 3.3, -12.5), (x, -s_in * 2.4, -14.6)], 0.28, col=PD, mat=PAINT, sides=5)


@part("SM_Crew_HandL", "HandL", FB, desc="Left glove: a chunky mitten work glove with a separate thumb, rubberised palm pad, finger grooves on the back and a flared cuff with a strap (paint: glove colour).", view=(1, -1, 0.1))
def crew_hand_l(a):
    _hand(a, 1)


@part("SM_Crew_HandR", "HandR", FB, desc="Right glove (mirror of SM_Crew_HandL).", view=(1, 1, 0.1))
def crew_hand_r(a):
    _hand(a, -1)


# ============================================================================== first-person arms

@part("SM_Crew_FPSleeve", "FPSleeveL", FP, also=("FPSleeveR",), desc="First-person forearm sleeve with a rolled cuff and a seam along the top (paint: sleeve colour).", view=(0.3, -1, 0.6))
def crew_fp_sleeve(a):
    limb(a, [(-22, 0, 0), (-5, 0, 0), (12, 0, 0), (16.2, 0, 0)], [5.8, 5.6, 5.2, 5.0], PW, n=4, sides=24, mat=PAINT, rough=0.8)
    a.torus(5.0, 1.1, at=(15.6, 0, 0), rot=(-90, 0, 0), col=PANEL, mat=PAINT, major=24, minor=8, rough=0.8)
    a.tube([(-18, 0, 5.72), (-5, 0, 5.62), (8, 0, 5.3), (13.5, 0, 5.12)], 0.24, col=PS, mat=PAINT, sides=5)


@part("SM_Crew_FPCuff", "FPCuffL", FP, also=("FPCuffR",), desc="First-person glove cuff: a flared, ribbed cuff (paint: the crew accent colour).", view=(0.3, -1, 0.6))
def crew_fp_cuff(a):
    a.cone(5.2, 4.4, rot=(-90, 0, 0), col=PW, mat=PAINT, sides=24, r_top=5.8, bevel=0.6, rough=0.8)
    for x in (-1.3, 1.3):
        a.torus(5.45 + 0.13 * x, 0.28, at=(x, 0, 0), rot=(-90, 0, 0), col=PS, mat=PAINT, major=24, minor=5)


def _fp_hand(a, s_in):
    """First-person mitten, palm down, fingers along +X; s_in = towards the screen centre (+1 for the left hand)."""
    b = cage(15.5, 11.2, 8.4, cuts=2)

    def f(c):
        t = c.x / 7.75
        y = c.y * (1 - 0.12 * max(0.0, -t))
        z = c.z * (1 - 0.1 * max(0.0, -t)) - 1.5 * max(0.0, t) ** 2
        return Vector((c.x + 0.8, y, z))
    a.add(smooth(deform(b, f), 2), None, PW, mat=PAINT, rough=0.75)
    th = [(-3.5, s_in * 3.6, -1.2), (0.5, s_in * 6.2, -2.0), (4.0, s_in * 7.0, -2.4)]
    limb(a, th, [2.9, 2.7, 2.3], PW, n=4, sides=14, mat=PAINT, rough=0.75)
    a.sphere(2.25, at=th[-1], col=PW, mat=PAINT, segs=14, rings=8, rough=0.75)
    a.add(rball(3.6, 4.0, 0.55, 0.5, 16, 8), xf((0.5, 0, 3.75)), C.CREAM_DARK, rough=0.8)
    for y in (-2.6, 0.0, 2.6):
        a.tube([(x, y, 3.55 - 1.5 * max(0.0, (x - 0.8) / 7.75) ** 2) for x in (4.2, 6.2, 8.0)], 0.28, col=PD, mat=PAINT, sides=5)


@part("SM_Crew_FPHandL", "FPHandL", FP, merge=("FPThumbL", "FPKnucklePad0", "FPFingerSeam0_0", "FPFingerSeam0_1", "FPFingerSeam0_2"),
      desc="First-person left glove: a chunky mitten with the thumb, sewn knuckle pad and finger grooves in one mesh (paint: glove colour).", view=(-0.3, -0.4, 1))
def crew_fp_hand_l(a):
    _fp_hand(a, 1)


@part("SM_Crew_FPHandR", "FPHandR", FP, merge=("FPThumbR", "FPKnucklePad1", "FPFingerSeam1_0", "FPFingerSeam1_1", "FPFingerSeam1_2"),
      desc="First-person right glove (mirror of SM_Crew_FPHandL).", view=(-0.3, 0.4, 1))
def crew_fp_hand_r(a):
    _fp_hand(a, -1)


FIT_NOTES = {
    "SM_Crew_GlassL": "Temple arm runs back over the ear; the code torus is only the lens ring.",
    "SM_Crew_GlassR": "Temple arm runs back over the ear; the code torus is only the lens ring.",
    "SM_Crew_PhoneCupL": "Cable hangs down from the cup; the code cylinder is only the cup.",
    "SM_Crew_PhoneBand": "Band arches from cup to cup over the head and the cap; the code box is only the top strip.",
    "SM_Crew_Bib": "Straps run up under the collar; the code box is only the front panel.",
}
for _spec in ASSETS:
    if _spec.name in FIT_NOTES:
        _spec.fit_note = FIT_NOTES[_spec.name]
