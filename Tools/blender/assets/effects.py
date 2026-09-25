"""Effects: chunky particle meshes, volumetric light beams, water surfaces with foam, and floor zone markings.

Visual replacements only - the particle simulation (FX/FTChunkyParticles), beam aiming, flood levels and zone
volumes stay as they are in the code:
  * Particles. UFTChunkyParticles instances a 100 cm unit shape (FTVis::GetMesh) scaled by the particle size and,
    with Stretch, lengthened along +Z = the velocity. These meshes live in the same +-50 unit box, +Z leading, with
    one material slot (Configure() keeps setting material and colour); vertex colours are near-white shading.
  * Beams. GlowCone(), the searchlights and the projector use the unit Cone (apex +Z, base radius 50 at z -50) in
    the glow material, scaled per use. The beam meshes keep that unit space and build the volume from nested open
    cones whose brightness (vertex alpha of the glow slot) falls off towards the rim and the far end, light rays
    and floating dust motes.
  * Water. SM_FX_WaterSurface replaces SM_FT_WaterGrid (tank water, flood plane) in its unit space with a finer,
    gently rippled grid and shoreline shading for M_FT_Water; SM_FX_TankFoam lies on the tank water line.
  * Zones. Real-size floor markings for the marked AFTZone volumes of FTBuildStudioMap (placements = zone floor).
"""
import math
import random

import bmesh
from mathutils import Vector, noise

from ftb import palette as C
from ftb.core import bm_extrude, bm_lathe, bm_loft, bm_sphere, bm_tube, deform, xf
from ftb.registry import ASSETS, P, asset

from assets.characters import catmull, recolor, text_on

FXP, FXB, FXW, FXZ = "FX/Particles", "FX/Beams", "FX/Water", "FX/Zones"
UNIT = {"min": [-50.0, -50.0, -50.0], "max": [50.0, 50.0, 50.0], "parts": 1}
WHITE = (0.97, 0.97, 0.97)
FOAM = (0.95, 0.98, 1.0)
SALMON = tuple(C.lin_to_srgb(x) for x in (1.0, 0.35, 0.25))     # AFTZone MarkColor of the lunge mark (linear in code)
MINT = tuple(C.lin_to_srgb(x) for x in (0.3, 1.0, 0.6))         # safe-zone MarkColor


def fill_box(a, axes="xyz", half=48.0):
    """Stretch the finished mesh along `axes` so it spans the unit box like the code shape it replaces (same on-screen size)."""
    lo, hi = a.bounds()
    c = (lo + hi) / 2
    k = [half * 2 / max(hi[i] - lo[i], 1e-6) if "xyz"[i] in axes else 1.0 for i in range(3)]
    a.v = [Vector(((v.x - c.x) * k[0], (v.y - c.y) * k[1], (v.z - c.z) * k[2])) for v in a.v]


def unit_asset(name, folder, desc, replaces, integration, view=(1, 0.7, 0.5), preview=None, fill="xyz"):
    def deco(fn):
        def build(a):
            fn(a)
            if fill:
                fill_box(a, fill)
        asset(name, folder, desc=desc, replaces=replaces, pivot="unit-shape centre (+-50 cm box, +Z leading), like FTVis::GetMesh",
              integration=integration, view=view, tags=["fx", "unit"])(build)
        spec = ASSETS[-1]
        spec.expect = lambda: UNIT
        if preview:
            spec.preview_xf = preview
        return fn
    return deco


def clean(a, bm, col, matrix=None, **kw):
    """Effect part: no wear, no baked contact shadow."""
    kw.setdefault("wear", False)
    kw.setdefault("ao", False)
    return a.add(bm, matrix, col, **kw)


def blob(a, at, r, col=WHITE, squash=1.0, segs=14, rings=10, **kw):
    return clean(a, bm_sphere(r, r, r * squash, segs, rings), col, xf(at), **kw)


def particle_note(shape, users):
    return ("Runtime particles: after UFTChunkyParticles::Configure() call SetStaticMesh(this mesh) on the component "
            "(Configure keeps setting material and colour). Replaces the %s unit shape of: %s." % (shape, users))


# ============================================================================== particles

@unit_asset("SM_FX_RainStreak", FXP,
            "Rain streak: a falling drop with a round leading head and a long thinning tail, plus a glint - stretches nicely along the velocity.",
            ["UFTChunkyParticles Cube shape of the rain machine (Production/FTStations.cpp) and AFTAmbientRain (World/FTStudioObjects.cpp)"],
            particle_note("Cube", "rain machine, ambient street rain"), view=(1, 0.3, 0.1))
def fx_rain(a):
    prof = [(0.0, -50), (2.5, -40), (5.5, -22), (9.5, -4), (14.0, 14), (17.5, 26), (18.5, 33), (16.5, 40), (11.5, 45), (5.0, 48.5), (0.0, 49.5)]
    clean(a, bm_lathe(prof, 10), WHITE)
    blob(a, (8, -7, 36), 4.5, col=(1, 1, 1), segs=8, rings=5)


@unit_asset("SM_FX_Droplet", FXP,
            "Water droplet: a chunky teardrop with two tiny satellite drops, so every splash, wake and valve spray reads as water.",
            ["UFTChunkyParticles Sphere shape: shark splash and wake (Production/FTShark.cpp), boat wake (FTStations.cpp), valve spray (World/FTFloodController.cpp), set-piece splash (Props/FTSetPieces.cpp)"],
            particle_note("Sphere", "splashes, wakes and the flood valve spray"))
def fx_droplet(a):
    prof = [(0.0, -30), (14, -27.5), (24, -19), (28.5, -7), (26, 5), (19.5, 17), (11, 29), (4, 39), (0.0, 44)]
    clean(a, bm_lathe(prof, 14), WHITE)
    blob(a, (10, -12, -4), 5, col=(1, 1, 1), segs=8, rings=5)
    blob(a, (30, -26, -38), 8, segs=10, rings=7)
    blob(a, (-34, 22, 26), 6, segs=10, rings=7)


@unit_asset("SM_FX_Spark", FXP,
            "Spark: a hot core with a long four-point star flare - reads as a crackling electric spark or a firework ember.",
            ["UFTChunkyParticles Sphere shape of the breaker sparks (Production/FTStations.cpp) and the firework / sparkler shop items (Career/FTShopItems.cpp)"],
            particle_note("Sphere", "breaker sparks, firework and sparkler items"))
def fx_spark(a):
    clean(a, bm_lathe([(0, -50), (9, -6), (9, 6), (0, 50)], 4), WHITE, glow=10)
    for rot in ((90, 0, 0), (0, 0, 90)):
        clean(a, bm_lathe([(0, -32), (7, -4), (7, 4), (0, 32)], 4), WHITE, xf((0, 0, 0), rot), glow=8)
    blob(a, (0, 0, 0), 13, col=(1, 1, 1), segs=12, rings=8, glow=14)


@unit_asset("SM_FX_SmokePuff", FXP,
            "Smoke puff: a cauliflower cluster of soft round lobes, lighter on top and shaded underneath.",
            ["UFTChunkyParticles Ball shape of the smoke machine (Production/FTStations.cpp) and the smoke shop items (Career/FTShopItems.cpp)"],
            particle_note("Ball", "smoke machine, smoke bomb items"))
def fx_smoke(a):
    lobes = [((0, 0, -6), 30), ((18, 10, 8), 22), ((-17, 13, 5), 21), ((6, -19, 10), 21), ((-6, -6, 25), 19), ((15, -4, -18), 18),
             ((-15, -12, -16), 17), ((2, 18, -14), 17)]
    for at, r in lobes:
        k = 0.8 + 0.2 * (at[2] + 30) / 55.0
        blob(a, at, r, col=C.shade(WHITE, k), segs=18, rings=12)


@unit_asset("SM_FX_FoamBlob", FXP,
            "Foam blob: a squashed dollop of suds with bubbles of different sizes on top and a flat underside to rest on the floor.",
            ["UFTChunkyParticles Ball shape of the foam cannon (Production/FTStations.cpp)"], particle_note("Ball", "foam cannon"), fill="xy")
def fx_foam(a):
    rng = random.Random(7)
    b = bm_sphere(38, 38, 26, 20, 12)
    clean(a, deform(b, lambda c: Vector((c.x, c.y, max(c.z, -18.0)))), FOAM)
    for k in range(11):
        t = k / 11.0 * math.tau + rng.uniform(-0.2, 0.2)
        rr = rng.uniform(0.2, 0.85)
        x, y = 34 * rr * math.cos(t), 34 * rr * math.sin(t)
        z = 26 * math.sqrt(max(0.0, 1 - rr * rr)) - 4
        blob(a, (x, y, min(z, 34.0)), rng.uniform(7, 14), col=C.shade(FOAM, rng.uniform(0.92, 1.0)), segs=12, rings=8)
    for k in range(6):
        t = rng.uniform(0, math.tau)
        blob(a, (40 * math.cos(t), 40 * math.sin(t), -14), rng.uniform(3, 6), col=(0.9, 0.97, 1.0), segs=8, rings=5)


@unit_asset("SM_FX_WindStreak", FXP,
            "Wind streak: a thin curved swoosh with tapered ends and a fainter companion line.",
            ["UFTChunkyParticles Cube shape of the wind machine (Production/FTStations.cpp)"], particle_note("Cube", "wind machine"),
            view=(0.3, 1, 0.2), fill="xz")
def fx_wind(a):
    for off, w, amp in ((0.0, 9.0, 14.0), (22.0, 4.5, 9.0)):
        pts = [(off + amp * math.sin(math.pi * (z + 50) / 100.0), 0.0, z) for z in range(-50, 51, 10)]
        radii = [max(0.8, w * math.sin(math.pi * k / 10.0)) for k in range(11)]
        b = bm_tube(pts, 1.0, 8, True, radii)
        clean(a, deform(b, lambda c: Vector((c.x, c.y * 0.35, c.z))), WHITE)


@unit_asset("SM_FX_Confetti", FXP,
            "Confetti: a curled, slightly twisted paper strip (the particle colour tints it).",
            ["UFTChunkyParticles Cube shape of the confetti burst (Career/FTShopItems.cpp)"], particle_note("Cube", "confetti and party items"))
def fx_confetti(a):
    secs = []
    for k in range(11):
        z = -45 + 9 * k
        tw = math.radians(-25 + 5 * k)
        cy = 22 * math.sin(math.pi * k / 10.0)
        sec = []
        for (u, v) in ((-30, -2), (30, -2), (30, 2), (-30, 2)):
            sec.append((u * math.cos(tw) - v * math.sin(tw), cy + u * math.sin(tw) + v * math.cos(tw), z))
        secs.append(sec)
    clean(a, bm_loft(secs, caps=True), WHITE)


@unit_asset("SM_FX_Bubble", FXP,
            "Soap bubble: a round shell with a curved highlight and a small glint.",
            ["UFTChunkyParticles Ball shape of the bubble shop item (Career/FTShopItems.cpp)"], particle_note("Ball", "bubble machine item"))
def fx_bubble(a):
    blob(a, (0, 0, 0), 46, segs=24, rings=16)
    arc = [(46.8 * math.cos(math.radians(t)) * 0.72, 46.8 * math.sin(math.radians(t)) * 0.72, 33) for t in range(100, 181, 10)]
    clean(a, bm_tube(arc, 2.2, 6, True), (1, 1, 1), glow=0.6)
    blob(a, (24, 14, 30), 4, col=(1, 1, 1), segs=8, rings=5, glow=0.8)


# ============================================================================== beams

def cone_r(z):
    """Radius of the unit cone at height z (apex +50, base radius 50 at -50)."""
    return 50.0 * (50.0 - z) / 100.0


BEAM_MAT = "M_FT_BeamGlow"


def beam(a, seed, rays, motes, bands=0, strength=1.0):
    """Brightness is the vertex alpha of the M_FT_BeamGlow slot (additive, two-sided in Unreal)."""
    rng = random.Random(seed)
    seg = 10
    for rf, g0 in ((1.0, 0.07), (0.74, 0.09), (0.46, 0.13), (0.2, 0.2)):
        for k in range(seg):
            z0, z1 = 50 - 100.0 * k / seg, 50 - 100.0 * (k + 1) / seg
            t = (k + 0.5) / seg
            g = g0 * strength * (1.0 - 0.72 * t) * (1.0 + 0.25 * (1 - t) ** 3)
            pts = [(0, 0, z0), (0, 0, (z0 + z1) / 2), (0, 0, z1)]
            radii = [max(0.3, cone_r(z) * rf) for z in (z0, (z0 + z1) / 2, z1)]
            clean(a, bm_tube(pts, 1.0, 28, False, radii), WHITE, mat=BEAM_MAT, rough=min(1.0, g))
    for k in range(rays):
        th = rng.uniform(0, math.tau)
        rr = rng.uniform(0.25, 0.92)
        end = Vector((50 * rr * math.cos(th), 50 * rr * math.sin(th), -50))
        start = Vector((0, 0, 47))
        pts = [start.lerp(end, t) for t in (0, 0.5, 1)]
        clean(a, bm_tube(pts, 1.0, 5, True, [0.2, rng.uniform(0.6, 1.0), rng.uniform(1.2, 2.2)]), WHITE, mat=BEAM_MAT,
              rough=rng.uniform(0.12, 0.25) * strength)
    for k in range(motes):
        z = rng.uniform(-46, 38)
        r = cone_r(z) * math.sqrt(rng.uniform(0, 0.8))
        th = rng.uniform(0, math.tau)
        blob(a, (r * math.cos(th), r * math.sin(th), z), rng.uniform(0.5, 1.3), segs=6, rings=4, mat=BEAM_MAT,
             rough=min(1.0, rng.uniform(0.45, 0.8) * strength))
    for k in range(bands):
        z = -40 + 80.0 * (k + 0.5) / bands + rng.uniform(-4, 4)
        ring = [(cone_r(z) * 0.97 * math.cos(math.radians(t)), cone_r(z) * 0.97 * math.sin(math.radians(t)), z) for t in range(0, 360, 12)]
        clean(a, bm_tube(ring, 0.5, 5, False, None, True), WHITE, mat=BEAM_MAT, rough=0.1 * strength)


BEAM_PREVIEW = ((90, 0, 0), [1.5, 1.5, 6.0])


@unit_asset("SM_FX_LightBeam", FXB,
            "Volumetric light beam: nested soft cones that fade towards the rim and the far end, a bright core, faint light rays and floating dust motes.",
            ["GlowCone() cones (Production/FTStations.cpp): AFTLighthouse BeamCone, AFTStageLight Cone",
             "AFTCityShell SearchBeam0/1 (World/FTCity.cpp)"],
            "SetStaticMesh on BeamCone / Cone / SearchBeam%d after they are made (unit Cone space, apex +Z); keep the code scale and "
            "paint colour. Material M_FT_BeamGlow: two-sided additive, emissive = paint colour x vertex colour x vertex alpha "
            "(or leave FTVis::Glow() on it).",
            view=(0.4, 1, 0.3), preview=BEAM_PREVIEW, fill="")
def fx_light_beam(a):
    beam(a, 11, rays=10, motes=70)


@unit_asset("SM_FX_ProjectorBeam", FXB,
            "Projector beam: the light-beam volume with more flickering film rays, drifting dust and faint banding, like a cinema throw through a dusty room.",
            ["AFTProjector Beam (World/FTStudioObjects.cpp): test screening and Grand Cinema premiere"],
            "SetStaticMesh on AFTProjector::Beam in the constructor and after each FTVis::ApplyShape(Beam, Cone, ...) in the aiming code "
            "(unit Cone space, apex +Z); SetGlow/scale stay code-driven. Material M_FT_BeamGlow as for SM_FX_LightBeam.",
            view=(0.4, 1, 0.3), preview=BEAM_PREVIEW, fill="")
def fx_projector_beam(a):
    beam(a, 23, rays=26, motes=140, bands=4, strength=0.9)


# ============================================================================== water

WATER_N = 48


@asset("SM_FX_WaterSurface", FXW,
       desc="Water surface: a finer, gently rippled grid with a lighter shoreline band and soft caustic mottling in the vertex colours (M_FT_Water still drives colour and waves).",
       replaces=["SM_FT_WaterGrid (Editor/FTContentCommandlet.cpp) used by AFTStudioShell TankWater and AFTFloodController WaterPlane"],
       pivot="unit plane centre (100 x 100 cm, z = 0), like SM_FT_WaterGrid",
       integration="Point FTVis::GetMesh(EFTShape::WaterGrid) at this mesh (or SetStaticMesh on TankWater / WaterPlane); material and scale stay.",
       placements=lambda: [P((1800.0, 100.0, -50.0), scale=(9.2, 19.2, 1.0), note="AFTStudioShell TankWater (code scale 920 x 1920)")],
       view=(1, 0.7, 0.9), tags=["fx", "unit"])
def fx_water_surface(a):
    b = bmesh.new()
    n = WATER_N
    vs = [[b.verts.new((-50 + 100.0 * i / n, -50 + 100.0 * j / n, 0.25 * noise.noise(Vector((i * 0.35, j * 0.35, 0.5)))))
           for j in range(n + 1)] for i in range(n + 1)]
    for i in range(n):
        for j in range(n):
            b.faces.new((vs[i][j], vs[i + 1][j], vs[i + 1][j + 1], vs[i][j + 1]))
    bmesh.ops.recalc_face_normals(b, faces=b.faces)
    start = len(a.f)
    clean(a, b, WHITE, mat="M_FT_Water", rough=0.1)

    def shade(c):
        edge = min(50 - abs(c.x), 50 - abs(c.y))
        shore = max(0.0, 1.0 - edge / 5.0)
        caustic = 0.06 * noise.noise(Vector((c.x * 0.19, c.y * 0.19, 3.0))) + 0.04 * noise.noise(Vector((c.x * 0.5, c.y * 0.5, 7.0)))
        k = 0.84 + 0.16 * shore + caustic
        return (min(1.0, k * 0.98), min(1.0, k), min(1.0, k))
    recolor(a, start, shade)


TANK = {"center": (1800.0, 100.0, -50.0), "half": (460.0, 960.0)}


def _rect_perimeter(hx, hy, step):
    """Points (x, y, inward normal) every `step` cm around a rectangle."""
    out = []
    for (x0, y0, x1, y1, n) in ((-hx, -hy, hx, -hy, (0, 1)), (hx, -hy, hx, hy, (-1, 0)), (hx, hy, -hx, hy, (0, -1)), (-hx, hy, -hx, -hy, (1, 0))):
        L = math.hypot(x1 - x0, y1 - y0)
        for k in range(int(L // step)):
            t = k * step / L
            out.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, n))
    return out


@asset("SM_FX_TankFoam", FXW,
       desc="Foam line of the shark tank: a soft suds band hugging the tank walls, clumps of foam blobs, corner build-ups and loose bubbles drifting on the water.",
       replaces=["(new) foam on the AFTStudioShell TankWater line (World/FTStudioShell.cpp)"],
       placements=lambda: [P(TANK["center"], note="TankWater water line (StudioShell at the origin)")],
       pivot="tank water centre on the water line", integration="Static mesh actor at the placement, no collision, no shadow.",
       view=(1, 0.8, 0.9), tags=["fx"])
def fx_tank_foam(a):
    rng = random.Random(5)
    hx, hy = TANK["half"]
    ins = 5.0
    loop = [(-hx + ins, -hy + ins, 0.6), (hx - ins, -hy + ins, 0.6), (hx - ins, hy - ins, 0.6), (-hx + ins, hy - ins, 0.6)]
    path = []
    for i in range(4):
        p0, p1 = Vector(loop[i]), Vector(loop[(i + 1) % 4])
        path += [p0.lerp(p1, t / 12.0) for t in range(12)]
    b = bm_tube(path, 1.0, 8, False, [6.0] * len(path), True)
    clean(a, deform(b, lambda c: Vector((c.x, c.y, 0.6 + (c.z - 0.6) * 0.3))), FOAM)
    for (x, y, n) in _rect_perimeter(hx, hy, 26.0):
        d = rng.uniform(6, 34) * (1 if rng.random() < 0.8 else 2.2)
        r = rng.uniform(5, 13)
        blob(a, (x + n[0] * d, y + n[1] * d, 0.8), r, col=C.shade(FOAM, rng.uniform(0.9, 1.0)), squash=0.32, segs=9, rings=5)
        if rng.random() < 0.35:
            blob(a, (x + n[0] * (d + r), y + n[1] * (d + r), 1.2), rng.uniform(2, 4), col=(0.88, 0.96, 1.0), squash=0.8, segs=6, rings=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            for k in range(7):
                blob(a, (sx * (hx - rng.uniform(12, 55)), sy * (hy - rng.uniform(12, 55)), 1.4), rng.uniform(9, 17), col=FOAM, squash=0.4,
                     segs=10, rings=6)
    for k in range(60):
        x, y = rng.uniform(-hx + 60, hx - 60), rng.uniform(-hy + 60, hy - 60)
        blob(a, (x, y, 0.8), rng.uniform(2, 6), col=C.shade(FOAM, 0.95), squash=0.35, segs=7, rings=4)


# ============================================================================== zone marks

def zone_floor(center, extent):
    """AFTZone::OnConstruction puts the marks at -Extent.Z + 0.6 below the volume centre."""
    return (center[0], center[1], center[2] - extent[2] + 0.6)


def flat_strip(a, pts, width, col, glow, height=0.7):
    """A flat painted strip along pts on the floor (z = 0 is the floor mark height)."""
    b = bm_tube(pts, 1.0, 8, True, [width / 2] * len(pts))
    clean(a, deform(b, lambda c: Vector((c.x, c.y, c.z * height / width))), col, glow=glow)


def flat_text(a, s, size, at, yaw, col, glow):
    """Stencil letters lying on the floor; `yaw` turns the reading direction (0 = reads along +Y, top towards +X)."""
    ca, sa = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))

    def surf(u, v):
        x, y = v, u
        return (Vector((at[0] + x * ca - y * sa, at[1] + x * sa + y * ca, at[2])), Vector((0, 0, 1)))
    start = len(a.f)
    text_on(a, s, size, surf, depth=0.5, col=col)
    for fi in range(start, len(a.f)):
        a.fmat[fi] = "M_FT_VertexGlow"
        a.frough[fi] = math.sqrt(glow / 20.0)


def zone_asset(name, zone, center, extent, desc, expect_half):
    def deco(fn):
        floor = zone_floor(center, extent)
        asset(name, FXZ, desc=desc, replaces=["AFTZone %s mark (World/FTZone.cpp, placed by Editor/FTMapBuilder.cpp)" % zone],
              placements=lambda: [P(floor, note="%s floor mark" % zone)],
              pivot="zone centre on the floor-mark height (-Extent.Z + 0.6)",
              integration="Static mesh actor at the placement (no collision, no shadow); set Mark = None on %s in FTMapBuilder so the code strips "
                          "are not drawn on top (the label TextRender stays)." % zone,
              view=(0.6, 0.45, 1.0), tags=["fx", "zone"])(fn)
        spec = ASSETS[-1]
        spec.expect = lambda: {"min": [-expect_half[0], -expect_half[1], -0.5], "max": [expect_half[0], expect_half[1], 0.5], "parts": 1}
        return fn
    return deco


@zone_asset("SM_Zone_LungeMark", "Zone_LungeMark", (1180, 250, -60), (110, 150, 90),
            "Shark-lunge floor mark: a dashed glowing oval, an inner guide ring, four chevrons pointing in, and a shark-fin icon over wavy water lines in the middle.",
            (110, 150))
def zone_lunge(a):
    rx, ry = 104.0, 144.0
    dashes = 30
    for k in range(dashes):
        a0 = 360.0 * k / dashes
        pts = [(rx * math.cos(math.radians(t)), ry * math.sin(math.radians(t)), 0.0) for t in (a0, a0 + 4, a0 + 8)]
        flat_strip(a, pts, 10.0, SALMON, 0.6)
    ring = [(rx * 0.8 * math.cos(math.radians(t)), ry * 0.8 * math.sin(math.radians(t)), 0.0) for t in range(0, 361, 8)]
    flat_strip(a, ring, 3.0, SALMON, 0.45)
    for deg in (0, 90, 180, 270):
        c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
        tip = Vector((rx * 0.62 * c, ry * 0.62 * s, 0))
        back = Vector((rx * 0.72 * c, ry * 0.72 * s, 0))
        side = Vector((-s, c, 0)) * 10
        flat_strip(a, [back + side, tip, back - side], 4.0, SALMON, 0.6)
    fin = [(26, -18), (22, -4), (14, 8), (2, 18), (-10, 22), (-12, 14), (-6, 2), (-2, -18)]
    clean(a, bm_extrude([(y + 8, x) for (x, y) in fin], 0.8), SALMON, glow=0.7)
    for k in range(2):
        wave = [(-8 - 7 * k, y, 0.0) for y in range(-26, 27, 4)]
        wave = [(x + 2.5 * math.sin(y * 0.35), y, z) for (x, y, z) in wave]
        flat_strip(a, catmull(wave, 2), 2.6, SALMON, 0.5)


def safe_zone(a, hx, hy, seed):
    rng = random.Random(seed)
    w = 10.0
    for (x0, y0, x1, y1) in ((-hx, -hy, hx, -hy), (hx, -hy, hx, hy), (hx, hy, -hx, hy), (-hx, hy, -hx, -hy)):
        L = math.hypot(x1 - x0, y1 - y0)
        d = Vector(((x1 - x0) / L, (y1 - y0) / L, 0))
        n = Vector((-d.y, d.x, 0))
        s = -w / 2
        while s < L + w / 2 - 20:
            piece = min(rng.uniform(80, 140), L + w / 2 - s)
            p0 = Vector((x0, y0, 0)) + d * s + n * rng.uniform(-1.0, 1.0)
            p1 = Vector((x0, y0, 0)) + d * (s + piece) + n * rng.uniform(-1.0, 1.0)
            yaw = math.degrees(math.atan2((p1 - p0).y, (p1 - p0).x))
            clean(a, bmesh_box(piece, w, 0.7), MINT, xf((p0 + p1) / 2, (0, yaw, 0)), glow=0.6)
            s += piece - 6.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            for k in range(4):
                o = 16 + 13 * k
                p0 = (sx * (hx - o), sy * (hy - 6), 0.0)
                p1 = (sx * (hx - 6), sy * (hy - o), 0.0)
                flat_strip(a, [p0, p1], 4.0, MINT, 0.45)
    for (at, yaw, length) in (((hx - 42, 0, 0), 0, hy), ((-hx + 42, 0, 0), 180, hy), ((0, hy - 42, 0), 90, hx), ((0, -hy + 42, 0), -90, hx)):
        flat_text(a, "SAFE", 34 if length > 150 else 28, at, yaw, MINT, 0.5)


def bmesh_box(sx, sy, sz):
    b = bmesh.new()
    bmesh.ops.create_cube(b, size=1.0)
    for v in b.verts:
        v.co = Vector((v.co.x * sx, v.co.y * sy, v.co.z * sz))
    return b


@zone_asset("SM_Zone_Safe_Harpoon", "Zone_Safe_Harpoon", (2200, 1700, -60), (260, 320, 150),
            "Safe-zone outline by the harpoon: hand-laid glowing mint tape in overlapping pieces, hatched corners and SAFE stencils facing out.", (265, 325))
def zone_safe_harpoon(a):
    safe_zone(a, 260, 320, 31)


@zone_asset("SM_Zone_Safe_Shelf", "Zone_Safe_Shelf", (1450, 1820, -60), (200, 200, 150),
            "Safe-zone outline by the shelf (same tape language as SM_Zone_Safe_Harpoon).", (205, 205))
def zone_safe_shelf(a):
    safe_zone(a, 200, 200, 32)


@zone_asset("SM_Zone_Safe_SharkStation", "Zone_Safe_SharkStation", (1420, -1180, -60), (200, 220, 150),
            "Safe-zone outline by the shark station (same tape language as SM_Zone_Safe_Harpoon).", (205, 225))
def zone_safe_station(a):
    safe_zone(a, 200, 220, 33)


FIT_NOTES = {
    "SM_FX_WindStreak": "A flat ribbon by design: it fills the unit box along X and the stretched Z axis, not along Y.",
    "SM_FX_FoamBlob": "Flat underside so the resting blob sits on the floor; lower than the unit ball it replaces.",
}
for _spec in ASSETS:
    if _spec.name in FIT_NOTES:
        _spec.fit_note = FIT_NOTES[_spec.name]
