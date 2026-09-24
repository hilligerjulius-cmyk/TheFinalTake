"""Reusable dressing used all over the studio and the city (instanced many times)."""
import math

from ftb import layout as L
from ftb import palette as C
from ftb.core import FONT_BLOCK
from ftb.registry import P, asset, kit

STUDIO_SHELL = "FTStudioShell.cpp"


def both(kitname, fn, where=None):
    """Placements + per-placement coverage from a helper called in the studio AND the city."""
    ps, cs = [], []
    for area in ("studio", "city"):
        p, c = kit(area, kitname, fn, where=where)
        ps += p()
        cs += c()
    return (lambda: ps), (lambda: cs)


def recs_where(area, fn):
    return [r for r in L.records(area) if r["kind"] == "prim" and fn(r)]


def near(a, b, tol=0.6):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


# ============================================================================== potted plant

_plant_p, _plant_c = both("Plant", lambda a, c: P(a[0], (0, 0, 0), (a[1],) * 3, "Plant(Loc, Scale=%.2f)" % a[1]))


@asset("SM_Prop_PottedPlant", "Shared/Props",
       desc="Terracotta pot with a rolled rim, soil, pebbles and seven broad, individually bent leaves on stems.",
       replaces=["AFTStudioShell::Plant() - pot cylinder, rim torus, soil disc, 7 leaf balls + 7 stem capsules (FTStudioShell.cpp:294-309)"],
       placements=_plant_p, covers=_plant_c, per_placement=True,
       pivot="floor contact, centre of the pot; uniform scale = Plant() Scale")
def potted_plant(a):
    a.lathe([(0, 0), (19.5, 0), (21.5, 3), (24.5, 44), (25.5, 46)], col=C.TERRACOTTA, sides=16, rough=0.75)
    a.torus(24.5, 3.6, at=(0, 0, 47.5), col=C.CREAM_DARK, major=18, minor=8, rz=3.2)
    a.torus(22.5, 1.4, at=(0, 0, 17), col=C.shade(C.TERRACOTTA, 0.8), major=18, minor=6)
    a.cyl(22.5, 3, at=(0, 0, 47.2), col=C.WOOD_DARK, sides=16, bevel=0.8)
    rng = a.rng
    for i in range(6):
        ang = rng.uniform(0, 2 * math.pi)
        r = rng.uniform(6, 17)
        a.sphere(rng.uniform(1.6, 2.6), at=(r * math.cos(ang), r * math.sin(ang), 48.8), col=C.shade(C.CREAM_DARK, rng.uniform(0.7, 1.0)), segs=6, rings=4, rz=1.2)
    for i in range(7):
        yaw = i * 137.5
        lift = 46 + (i % 3) * 11
        root_z = 70 + (i % 3) * 13
        d = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)))
        root = (d[0] * 4, d[1] * 4, root_z)
        a.tube([(d[0] * 1.5, d[1] * 1.5, 48), (d[0] * 3, d[1] * 3, 55 + (i % 3) * 5), root], 1.6, col=C.GREEN_DARK, sides=6)
        a.leaf(58 - (i % 3) * 5, 23 - (i % 2) * 3, at=root, rot=(lift, yaw, 0), col=C.GREEN if i % 2 else C.TEAL_DARK,
               droop=0.5, fold=0.22, thick=1.4, segs=8, rough=0.6)
    # young centre shoot
    a.leaf(46, 15, at=(0, 0, 66), rot=(78, 40, 0), col=C.LEAF_LIGHT, droop=0.35, thick=1.2, rough=0.6)
    a.leaf(40, 13, at=(0, 0, 64), rot=(74, 220, 0), col=C.GREEN, droop=0.35, thick=1.2, rough=0.6)


# ============================================================================== wooden crate

_crate_p, _crate_c = both("Crate", lambda a, c: P(a[0], (0, a[2], 0), (a[1] / 100.0,) * 3, "Crate(Size=%g)" % a[1]))


@asset("SM_Prop_WoodCrate", "Shared/Props",
       desc="Plank crate with a dark banding strap, cream corner battens, stencilled lid and carved hand holds.",
       replaces=["AFTStudioShell::Crate() - box, mid band, 4 battens, 2 hand holds (FTStudioShell.cpp:311-326)"],
       placements=_crate_p, covers=_crate_c, per_placement=True,
       pivot="floor contact, centre; modelled at Size 100, uniform scale = Size/100")
def wood_crate(a):
    S = 100.0
    a.box((S - 4, S - 4, S - 3), at=(0, 0, S / 2 - 0.5), col=C.WOOD, bevel=0, rough=0.85)
    # planks on the four sides (slight irregularity), grooves read as dark gaps
    for side in range(4):
        yaw = side * 90
        rot = math.radians(yaw)
        n = (math.cos(rot), math.sin(rot))
        for k in range(4):
            z = 8 + k * 23.5
            off = a.rng.uniform(-0.6, 0.6)
            a.box((3, S - 10, 20.5), at=(n[0] * (S / 2 - 1.2), n[1] * (S / 2 - 1.2), z + 10 + off * 0.3), rot=(0, yaw, a.rng.uniform(-0.8, 0.8)),
                  col=C.shade(C.WOOD, a.rng.uniform(0.9, 1.08)), bevel=1.2, segs=1, rough=0.85)
    # lid planks
    for k in range(4):
        a.box((S - 10, 21, 3), at=(0, -35 + k * 23.3, S - 0.5), col=C.shade(C.WOOD, a.rng.uniform(0.92, 1.08)), bevel=1.1, segs=1, rough=0.85)
    # corner posts + top/bottom frame
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((9, 9, S), at=(sx * (S / 2 - 3.5), sy * (S / 2 - 3.5), S / 2), col=C.WOOD_DARK, bevel=2, segs=1, rough=0.8)
    for z in (4, S - 4):
        for yaw in (0, 90):
            for s in (-1, 1):
                o = (s * (S / 2 - 2.5), 0) if yaw == 0 else (0, s * (S / 2 - 2.5))
                a.box((6, S - 6, 8) if yaw == 0 else (S - 6, 6, 8), at=(o[0], o[1], z), col=C.WOOD_DARK, bevel=1.8, segs=1, rough=0.8)
    # code: dark mid band all round
    a.box((S * 1.02, S * 1.02, 12), at=(0, 0, S / 2), col=C.WOOD_DARK, bevel=2.5, segs=1, rough=0.8)
    # code: cream battens on the +/-X faces, hand holds above the band
    for sx in (-1, 1):
        for e in (-0.36, 0.36):
            a.box((5.5, 12, S * 0.9), at=(sx * S * 0.51, e * S, S / 2), col=C.CREAM_DARK, bevel=1.5, rough=0.8)
            for z in (14, S - 14):
                a.cyl(1.4, 1.5, at=(sx * S * 0.54, e * S, z), rot=(90, 0, 0), col=C.GREY_DARK, sides=6, bevel=0)
        a.box((3, S * 0.28, S * 0.1), at=(sx * S * 0.525, 0, S * 0.7), col=C.INK, bevel=1.5)
        a.box((2, S * 0.32, 3), at=(sx * S * 0.53, 0, S * 0.7 + S * 0.065), col=C.WOOD_DARK, bevel=0.8)
    a.text("FRAGILE", 11, 0.8, at=(0, 0, S + 1.0), rot=(90, 90, 0), col=C.CORAL_DARK, font=FONT_BLOCK)


# ============================================================================== flight cases

def _case(a, X, Y, Z):
    a.box((X - 5, Y - 5, Z - 3), at=(0, 0, Z / 2), col=C.CHARCOAL, bevel=3, rough=0.75)
    # aluminium extrusions: mid seam, lid seam, verticals, ball corners
    for z in (Z * 0.5, Z - 3.5):
        a.box((X + 1, Y + 1, 5 if z == Z * 0.5 else 6), at=(0, 0, z), col=C.GREY, bevel=1.3, rough=0.3)
    a.box((X + 1, Y + 1, 5), at=(0, 0, 3), col=C.GREY, bevel=1.3, rough=0.3)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((7.5, 7.5, Z - 4), at=(sx * (X / 2 - 3.2), sy * (Y / 2 - 3.2), Z / 2), col=C.GREY, bevel=2, rough=0.3)
            for z in (6.5, Z - 6.5):
                a.sphere(7.2, at=(sx * (X / 2 - 3), sy * (Y / 2 - 3), z), col=C.GREY_DARK, segs=8, rings=5, rough=0.35)
    # front (-Y): twist latches, centre label plate, chunky handle; back gets the hinges
    for sx in (-1, 1):
        a.box((14, 3, 18), at=(sx * X * 0.28, -Y / 2 - 1.2, Z * 0.56), col=C.CREAM_DARK, bevel=1.2, rough=0.3)
        a.box((7, 3, 8), at=(sx * X * 0.28, -Y / 2 - 3.2, Z * 0.54), col=C.CHARCOAL, bevel=1.2, rough=0.5)
        a.cyl(2.2, 2, at=(sx * X * 0.28, -Y / 2 - 4.5, Z * 0.54), rot=(0, 0, 90), col=C.CHROME, sides=8, rough=0.2)
        a.box((12, 4, 5), at=(sx * X * 0.3, Y / 2 + 1.5, Z * 0.52), col=C.GREY, bevel=1.2, rough=0.3)
    a.box((X * 0.34, 2, Z * 0.2), at=(0, -Y / 2 - 1, Z * 0.26), col=C.TEAL, bevel=0.7, rough=0.6)
    a.box((X * 0.24, 2, 2.5), at=(0, -Y / 2 - 1.7, Z * 0.3), col=C.CREAM, bevel=0.4)
    a.box((X * 0.18, 2, 2.5), at=(-X * 0.03, -Y / 2 - 1.7, Z * 0.23), col=C.CREAM, bevel=0.4)
    for sx in (-1, 1):
        a.box((7, 4.5, 6), at=(sx * 13, -Y / 2 - 2, Z * 0.8), col=C.GREY_DARK, bevel=1.2, rough=0.4)
    a.tube([(-13, -Y / 2 - 3.5, Z * 0.8), (-11, -Y / 2 - 9, Z * 0.84), (11, -Y / 2 - 9, Z * 0.84), (13, -Y / 2 - 3.5, Z * 0.8)], 2.0, col=C.RUBBER, sides=8, rough=0.7)
    # side handles
    for sx in (-1, 1):
        a.box((3, 22, 8), at=(sx * (X / 2 + 1.5), 0, Z * 0.78), col=C.GREY_DARK, bevel=1.2, rough=0.4)
    # stencil + sticker on top
    a.box((X * 0.3, Y * 0.3, 0.6), at=(X * 0.18, Y * 0.12, Z + 0.1), rot=(0, 8, 0), col=C.YELLOW, bevel=0.2, rough=0.8)
    a.text("FT", 9, 0.5, at=(X * 0.18, Y * 0.12, Z + 0.45), rot=(90, 98, 0), col=C.INK, font=FONT_BLOCK)


def _case_pick(size):
    return "tall" if size[2] >= 65 else "flat"


_CASE_DIMS = {"tall": (100.0, 70.0, 80.0), "flat": (85.0, 60.0, 52.0)}


def _case_kit(kind):
    dims = _CASE_DIMS[kind]
    return both("FlightCase", lambda a, c: P(a[0], (0, a[2], 0), tuple(a[1][i] / dims[i] for i in range(3)), "FlightCase(Size=%s)" % a[1]),
                where=lambda args: _case_pick(args[1]) == kind)


_ct_p, _ct_c = _case_kit("tall")
_cf_p, _cf_c = _case_kit("flat")


@asset("SM_Prop_FlightCase_Tall", "Shared/Props",
       desc="Road case 100x70x80: charcoal panels, aluminium extrusions, ball corners, twist latches, rubber handle, stencil sticker.",
       replaces=["AFTStudioShell::FlightCase() for sizes >= 65 cm tall (FTStudioShell.cpp:328-350)"],
       placements=_ct_p, covers=_ct_c, per_placement=True,
       pivot="floor contact, centre; front (latches) faces -Y like the code; scale = Size/(100,70,80)", view=(0.7, -1, 0.6))
def flight_case_tall(a):
    _case(a, *_CASE_DIMS["tall"])


@asset("SM_Prop_FlightCase_Flat", "Shared/Props",
       desc="Low road case 85x60x52 for stacking on top of the tall ones.",
       replaces=["AFTStudioShell::FlightCase() for sizes < 65 cm tall (FTStudioShell.cpp:328-350)"],
       placements=_cf_p, covers=_cf_c, per_placement=True,
       pivot="floor contact, centre; front faces -Y; scale = Size/(85,60,52)", view=(0.7, -1, 0.6))
def flight_case_flat(a):
    _case(a, *_CASE_DIMS["flat"])


# ============================================================================== traffic cone

_cone_p, _cone_c = both("Cone", lambda a, c: P(a[0], (0, 0, 0), (1, 1, 1), "Cone()"))


@asset("SM_Prop_TrafficCone", "Shared/Props",
       desc="Chunky traffic cone with a square rubber foot and two reflective bands.",
       replaces=["AFTStudioShell::Cone() - base box, cone, white band (FTStudioShell.cpp:352-357)"],
       placements=_cone_p, covers=_cone_c, per_placement=True)
def traffic_cone(a):
    a.box((46, 46, 5), at=(0, 0, 2.5), col=C.ORANGE, bevel=1.8, rough=0.7)
    a.box((34, 34, 2), at=(0, 0, 5.5), col=C.shade(C.ORANGE, 0.8), bevel=0.8, rough=0.7)
    a.lathe([(17.5, 4.5), (15.5, 20), (11, 38), (7.2, 54), (4.2, 66), (2.8, 69), (0, 69.8)], col=C.ORANGE, sides=14, rough=0.55)
    a.cyl(12.6, 9, at=(0, 0, 39.5), r_top=10.4, col=C.WHITE, sides=14, bevel=0.6, rough=0.4)
    a.cyl(8.2, 6, at=(0, 0, 55), r_top=6.9, col=C.WHITE, sides=14, bevel=0.5, rough=0.4)


# ============================================================================== cable coil

def _coil_placements():
    out = []
    # studio street: two stacked tori (FTStudioShell.cpp:510-511)
    out.append(P((-2060, -1060, 0), (0, 20, 0), (1, 1, 1.35), "street coil"))
    # practical-effects corner: 4 tori at z -114 (FTStudioShell.cpp:815-818)
    for i in range(4):
        out.append(P((700 + i * 30, 1600, -120), (0, i * 40, 0), (1, 1, 0.8), "effects corner"))
    return out


@asset("SM_Prop_CableCoil", "Shared/Props",
       desc="Coiled stage cable: three loose loops, a trailing end and a chunky connector.",
       replaces=["Ink torus pairs: street clutter (FTStudioShell.cpp:510-511) and the 4 coils in the effects corner (FTStudioShell.cpp:815-818)"],
       placements=_coil_placements,
       covers=lambda: recs_where("studio", lambda r: r["group"] == L.GROUP["TorusDeco"] and r["color"][0] < 0.01 and r["size"][0] >= 70),
       pivot="floor contact, centre of the coil")
def cable_coil(a):
    for k, (R, z, dx) in enumerate(((33, 5, 0), (31.5, 12, 1.5), (30, 19, -1))):
        a.torus(R, 4.2, at=(dx, dx * 0.5, z), rot=(a.rng.uniform(-4, 4), k * 30, a.rng.uniform(-4, 4)), col=C.RUBBER, major=22, minor=7, rough=0.6)
    a.tube([(30, -8, 4), (42, -18, 3), (52, -14, 2.5), (60, -20, 2.5)], 4, col=C.RUBBER, sides=8, rough=0.6)
    a.box((12, 9, 9), at=(64, -22, 4.5), rot=(0, -30, 0), col=C.YELLOW, bevel=2, rough=0.5)
    a.cyl(3, 5, at=(70, -25, 4.5), rot=(-90, -30, 0), col=C.GREY_DARK, sides=8)


# ============================================================================== film posters

_POSTER_LINES = {}


def _poster_kit(film):
    return both("Poster", lambda a, c: P(a[0], (0, a[1], 0), (a[3],) * 3, "Poster(Film=%d, Scale=%.2f)" % (a[2], a[3])), where=lambda args: args[2] == film)


def _poster_frame(a, bg, trim):
    a.box((8, 170, 250), col=C.CHARCOAL, bevel=3, rough=0.6)
    a.box((3, 156, 236), at=(3.5, 0, 0), col=trim, bevel=1.2, rough=0.4)
    a.box((2, 146, 226), at=(5.0, 0, 0), col=bg, bevel=0.6, rough=0.8)
    a.box((1.5, 146, 36), at=(6.2, 0, 95), col=C.shade(bg, 0.55), bevel=0.4, rough=0.8)  # title band (TextRender sits here)
    for sy in (-1, 1):
        for sz in (-1, 1):
            a.sphere(3.2, at=(5.5, sy * 76, sz * 116), col=C.BRASS, segs=8, rings=5, rough=0.3)


_pj_p, _pj_c = _poster_kit(0)
_pm_p, _pm_c = _poster_kit(1)
_pc_p, _pc_c = _poster_kit(2)


@asset("SM_Deco_Poster_JawsOfTheStudio", "Shared/Posters",
       desc="Framed one-sheet: raised shark fin cutting through layered relief waves; dark band on top for the title text.",
       replaces=["AFTStudioShell::Poster(Film 0) frame, backing, sea and fin (FTStudioShell.cpp:263-292)"],
       placements=_pj_p, covers=_pj_c, per_placement=True, view=(1, 0.5, 0.25),
       pivot="centre of the frame, art faces +X; uniform scale = Poster() Scale")
def poster_jaws(a):
    _poster_frame(a, C.BLUE, C.BRASS)
    for k, (z, col) in enumerate(((-88, C.DEEP_BLUE), (-66, C.shade(C.BLUE, 0.8)), (-46, C.shade(C.DEEP_BLUE, 1.25)))):
        pts = [(-73 + i * 146 / 12, z + 6 * math.sin(i * 1.2 + k)) for i in range(13)]
        a.poly([(-73, -113)] + pts + [(73, -113)], 2 + k, at=(6.5 + k, 0, 0), col=col, rough=0.8)
    a.poly([(-30, -44), (18, -44), (4, 40), (-6, 24)], 5, at=(9.5, 0, 0), col=C.NAVY, bevel=1.0, rough=0.6)
    a.poly([(-24, -44), (-10, -44), (-4, 14)], 2, at=(12.5, 0, 0), col=C.NAVY_LIGHT, rough=0.6)
    a.sphere(16, at=(7, 44, 48), col=C.YELLOW, rz=16, ry=16, rough=0.8, segs=12, rings=6)
    for i in range(3):
        a.box((1.5, 28, 3), at=(7.5, -40 + i * 14, 30 + i * 9), rot=(0, 0, -8), col=C.WHITE, bevel=0.6)


@asset("SM_Deco_Poster_MoonfallMotel", "Shared/Posters",
       desc="Framed one-sheet: purple night, a big grey moon, a glowing motel arrow sign on a pole.",
       replaces=["AFTStudioShell::Poster(Film 1) (FTStudioShell.cpp:263-292)"],
       placements=_pm_p, covers=_pm_c, per_placement=True, view=(1, 0.5, 0.25),
       pivot="centre of the frame, art faces +X; uniform scale = Poster() Scale")
def poster_moonfall(a):
    _poster_frame(a, C.PURPLE, C.CYAN)
    a.cyl(34, 3, at=(7, 12, 36), rot=(90, 0, 0), col=C.GREY, sides=20, bevel=1.0, rough=0.8)
    for (y, z, r) in ((0, 44, 7), (24, 28, 5), (8, 22, 4)):
        a.cyl(r, 2, at=(8.7, y, z), rot=(90, 0, 0), col=C.shade(C.GREY, 0.8), sides=10, bevel=0.5)
    a.box((3, 6, 120), at=(7.5, -40, -50), col=C.CHARCOAL, bevel=1)
    a.poly([(-46, -30), (4, -30), (22, -10), (4, 10), (-46, 10)], 3, at=(9, 0, 0), rot=(0, 0, 0), col=C.CYAN, bevel=0.8, mat="glow")
    a.text("MOTEL", 11, 1.2, at=(11, -20, -10), col=C.NAVY)
    a.poly([(-73, -113), (73, -113), (73, -80), (20, -70), (-30, -84), (-73, -74)], 2, at=(6.5, 0, 0), col=C.shade(C.PURPLE, 0.6))
    for (y, z) in ((-60, 70), (-20, 84), (50, -40), (60, 80), (-55, 20)):
        a.sphere(2.2, at=(6.8, y, z), col=C.CREAM, segs=6, rings=4, mat="glow")


@asset("SM_Deco_Poster_CastleOnFire", "Shared/Posters",
       desc="Framed one-sheet: cream castle towers with battlements and a yellow flame crown.",
       replaces=["AFTStudioShell::Poster(Film 2) (FTStudioShell.cpp:263-292)"],
       placements=_pc_p, covers=_pc_c, per_placement=True, view=(1, 0.5, 0.25),
       pivot="centre of the frame, art faces +X; uniform scale = Poster() Scale")
def poster_castle(a):
    _poster_frame(a, C.CORAL, C.YELLOW)
    for i in range(4):
        y = -45 + i * 30
        h = 90 + (i % 2) * 14
        z0 = -20 + (i % 2) * 14 - 45
        a.box((3, 22, h), at=(7, y, z0 + h / 2 - 45 + 45), col=C.CREAM, bevel=1)
        for m in (-1, 1):
            a.box((3, 6, 7), at=(7, y + m * 7, z0 + h + 3.5), col=C.CREAM, bevel=0.8)
        a.box((1.5, 7, 12), at=(8.8, y, z0 + h * 0.6), col=C.NAVY, bevel=0.6)
    a.poly([(-30, 30), (-18, 58), (-8, 42), (0, 78), (8, 42), (18, 58), (30, 30)], 3, at=(8, 0, 0), col=C.YELLOW, bevel=0.8, mat="glow")
    a.poly([(-16, 32), (-6, 50), (0, 38), (6, 50), (16, 32)], 2, at=(10.5, 0, 0), col=C.ORANGE, bevel=0.5, mat="glow")
    a.poly([(-73, -113), (73, -113), (73, -95), (-73, -95)], 2, at=(6.5, 0, 0), col=C.CORAL_DARK)


# ============================================================================== rope posts + ropes

def _is_post(r):
    return r["group"] in (L.GROUP["CylSolid"], L.GROUP["CylDeco"]) and near(r["size"], (10, 10, 92))


def _post_placements():
    out = []
    for area in ("studio", "city"):
        for r in recs_where(area, _is_post):
            c = r["center"]
            out.append(P((c[0], c[1], c[2] - 46), note="%s:%d" % (area, r["line"])))
    return out


def _post_covers():
    out = []
    for area in ("studio", "city"):
        posts = recs_where(area, _is_post)
        balls = recs_where(area, lambda r: r["group"] == L.GROUP["SphereDeco"] and r["size"][0] in (15, 16) and r["color"][0] > 0.6 and r["color"][2] < 0.12)
        for p in posts:
            ball = [b for b in balls if abs(b["center"][0] - p["center"][0]) < 1 and abs(b["center"][1] - p["center"][1]) < 1]
            out.append([p] + ball[:1])
    return out


@asset("SM_Prop_BrassStanchion", "Shared/Props",
       desc="Brass rope stanchion: weighted domed foot, fluted pole, collar and ball top with a rope hook ring.",
       replaces=["Brass post cylinder + ball: studio entrance (FTStudioShell.cpp:571-578), lobby (FTStudioShell.cpp:682-689), cinema plaza (FTCity.cpp:356-363)"],
       placements=_post_placements, covers=_post_covers, per_placement=True,
       pivot="floor contact, centre (placed 4 cm up on the sidewalks, as in the code)")
def brass_stanchion(a):
    a.lathe([(0, 0), (15, 0), (15, 2.5), (12.5, 6), (6, 9), (3.2, 12), (0, 12.5)], col=C.BRASS, sides=16, rough=0.3)
    a.cyl(3.0, 76, at=(0, 0, 50), col=C.BRASS, sides=10, bevel=0, rough=0.3)
    for z in (22, 84):
        a.torus(3.4, 1.4, at=(0, 0, z), col=C.shade(C.BRASS, 0.85), major=12, minor=6, rough=0.3)
    a.lathe([(3.0, 86), (5.5, 88), (5.5, 90), (3.4, 92), (0, 92.5)], col=C.BRASS, sides=12, rough=0.3)
    a.sphere(8, at=(0, 0, 99), col=C.BRASS, segs=14, rings=9, rough=0.25)
    a.torus(3.6, 1.1, at=(0, 7.2, 91), rot=(0, 0, 90), col=C.shade(C.BRASS, 0.8), major=10, minor=5, rough=0.3)
    a.torus(3.6, 1.1, at=(0, -7.2, 91), rot=(0, 0, 90), col=C.shade(C.BRASS, 0.8), major=10, minor=5, rough=0.3)


def _rope_records(area):
    return recs_where(area, lambda r: r["group"] == L.GROUP["CapsuleDeco"] and r["color"][0] > 0.5 and r["color"][1] < 0.05 and r["size"][2] >= 150)


def _rope_placements():
    out = []
    for area in ("studio", "city"):
        for r in _rope_records(area):
            c = r["center"]
            n = int(round(r["size"][2] / 150.0))
            for k in range(n):
                x = c[0] - r["size"][2] / 2 + 75 + k * 150
                out.append(P((x, c[1], 95), (0, 90, 0), note="%s:%d" % (area, r["line"])))
    return out


@asset("SM_Prop_VelvetRope", "Shared/Props",
       desc="150 cm velvet rope sagging between two stanchions, brass snap hooks on both ends.",
       replaces=["Carpet-red capsules between the posts: entrance (FTStudioShell.cpp:579-580), plaza (FTCity.cpp:364-367; one 600 cm capsule = 4 spans)"],
       placements=_rope_placements,
       covers=lambda: _rope_records("studio") + _rope_records("city"),
       pivot="rope mid-span at hook height (95 cm above the plaza/street floor = stanchion ring); spans local Y -75..75", view=(1, 0.4, 0.3))
def velvet_rope(a):
    pts = []
    for i in range(13):
        t = i / 12
        y = -68 + 136 * t
        z = -16 * math.sin(math.pi * t)
        pts.append((0, y, z))
    a.tube(pts, 3.0, col=C.CARPET, sides=8, rough=0.9)
    for s in (-1, 1):
        a.cyl(3.4, 6, at=(0, s * 69, 0), rot=(0, 0, 90), col=C.BRASS, sides=8, bevel=0.8, rough=0.3)
        a.tube([(0, s * 72, 0), (0, s * 75, 2.5), (0, s * 75.5, 5)], 1.0, col=C.BRASS, sides=6, rough=0.3)


# ============================================================================== street lamp

def _lamp_placements():
    out = []
    # studio street lamps: pole at (-2200, Y), head towards -X (FTStudioShell.cpp:515-522)
    for y in (-1500, 500, 2100):
        out.append(P((-2200, y, 0), (0, 180, 0), note="studio street"))
    for c in L.kit_calls("city", "StreetLamp"):
        base, arm = c["call"]["args"][0], c["call"]["args"][1]
        out.append(P(base, (0, 90 if arm > 0 else -90, 0), note="city StreetLamp(ArmDir=%g)" % arm))
    return out


def _lamp_covers():
    studio = L.select("studio", lines=(516, 520))
    per = [[r for r in studio if abs(r["center"][1] - y) < 1] for y in (-1500, 500, 2100)]
    return per + [c["records"] for c in L.kit_calls("city", "StreetLamp")]


@asset("SM_Prop_StreetLamp", "Shared/Street",
       desc="Art-deco street lamp: fluted cast base, tapered navy pole with collars, swan-neck arm, lantern head with a glowing lens.",
       replaces=["AFTCityShell::StreetLamp() (FTCity.cpp:106-117)", "studio street lamps (FTStudioShell.cpp:515-522)"],
       placements=_lamp_placements, covers=_lamp_covers, per_placement=True,
       pivot="floor contact at the pole; the arm points along local +X (yaw 90/-90 in the city, 180 on the studio street)",
       integration="Keep the code's PointLight at head position (local X 85, Z 360).", view=(0.4, 1, 0.35))
def street_lamp(a):
    a.lathe([(0, 0), (17, 0), (17, 4), (14, 8), (11, 16), (9, 24), (7.5, 30), (0, 31)], col=C.NAVY_LIGHT, sides=12, rough=0.5)
    for k in range(8):
        ang = k * 45
        a.box((3, 3, 20), at=(math.cos(math.radians(ang)) * 12.5, math.sin(math.radians(ang)) * 12.5, 14), rot=(0, ang, 0), col=C.NAVY, bevel=1.0, rough=0.5)
    a.cyl(7, 390, at=(0, 0, 225), r_top=5.2, col=C.NAVY, sides=12, bevel=0, rough=0.45)
    for z in (60, 300, 404):
        a.torus(7.2 if z < 300 else 6, 2.2, at=(0, 0, z), col=C.BRASS, major=14, minor=6, rough=0.3)
    a.sphere(8, at=(0, 0, 424), col=C.NAVY, segs=12, rings=7, rough=0.45)
    a.cone(3, 10, at=(0, 0, 435), col=C.BRASS, sides=8, rough=0.3)
    # swan neck arm
    arm = [(0, 0, 404)] + [(8 + 80 * t, 0, 404 + 26 * math.sin(math.pi * t * 0.9)) for t in (0.1, 0.25, 0.45, 0.65, 0.85)] + [(86, 0, 426)]
    a.tube(arm, 3.2, col=C.NAVY, sides=8, rough=0.45)
    a.tube([(20, 0, 404), (45, 0, 412), (60, 0, 420)], 1.6, col=C.NAVY, sides=6, rough=0.45)
    # lantern head
    a.cyl(5, 8, at=(86, 0, 424), col=C.NAVY, sides=8)
    a.cone(28, 14, at=(86, 0, 414), r_top=8, col=C.CHARCOAL, sides=8, rough=0.5)
    a.cyl(19, 16, at=(86, 0, 399), r_top=24, col=C.CHARCOAL, sides=8, bevel=1.0, rough=0.5, cap=True)
    a.cyl(17, 10, at=(86, 0, 392), r_top=18.5, col=C.AMBER, sides=8, bevel=0.5, mat="glow")
    a.cyl(12, 3, at=(86, 0, 386), col=C.YELLOW, sides=8, mat="glow")


# ============================================================================== palm tree

_palm_p, _palm_c = both("Palm", lambda a, c: P(a[0], (0, 0, 0), (a[1] / 300.0,) * 3, "Palm(Height=%g)" % a[1]))


@asset("SM_Prop_PalmTree", "Shared/Nature",
       desc="Leaning palm: ringed segmented trunk, crown of nine drooping fronds with a folded midrib, coconut cluster.",
       replaces=["AFTStudioShell::Palm() - 5 trunk cylinders, 12 leaf balls, 6 stems, 3 coconuts (FTStudioShell.cpp:359-381)"],
       placements=_palm_p, covers=_palm_c, per_placement=True,
       pivot="trunk base on the ground; leans towards +X like the code; modelled at Height 300, uniform scale = Height/300")
def palm_tree(a):
    H = 300.0
    segs = 7
    for i in range(segs):
        t0, t1 = i / segs, (i + 1) / segs
        x0, x1 = 30 * t0 ** 1.3, 30 * t1 ** 1.3
        r0, r1 = 14 - 5.5 * t0, 14 - 5.5 * t1
        a.cyl(r0, H / segs + 3, at=((x0 + x1) / 2, 0, H * (t0 + t1) / 2), rot=(-6 - 4 * t0, 0, 0), r_top=r1 * 0.92, col=C.WOOD if i % 2 else C.WOOD_DARK, sides=10, bevel=2.0, rough=0.9)
        a.torus(r1 * 0.95, 1.8, at=(x1, 0, H * t1 - 2), rot=(-8, 0, 0), col=C.shade(C.WOOD_DARK, 0.85), major=10, minor=5, rough=0.9)
    top = (30, 0, H)
    a.sphere(12, at=top, col=C.GREEN_DARK, segs=10, rings=6, rz=9)
    for k in range(9):
        yaw = k * 40 + a.rng.uniform(-8, 8)
        long = k % 3 != 2
        a.leaf(125 if long else 95, 42 if long else 34, at=(top[0], top[1], top[2] + 4), rot=(24 if long else 40, yaw, 0),
               col=C.GREEN if k % 2 else C.TEAL_DARK, droop=0.55 if long else 0.45, fold=0.3, thick=2.2, segs=9, rough=0.65)
    for k in range(3):
        ang = math.radians(k * 120 + 20)
        a.sphere(12, at=(top[0] + 12 * math.cos(ang), 12 * math.sin(ang), top[2] - 13), col=C.WOOD, ry=11, rz=14, segs=10, rings=7, rough=0.8)


# ============================================================================== rocks

def _rock_recs():
    tank = recs_where("studio", lambda r: r["color"][0] < 0.4 and r["color"][0] > 0.25 and r["group"] in (L.GROUP["SphereDeco"], L.GROUP["BoxSolid"]) and 1500 < r["center"][0] < 2300 and r["center"][1] < 700)
    # the prop rock between the warehouse crates (FTStudioShell.cpp:1024, BoxDeco in rock colour)
    return tank + L.select("studio", lines=[(1024, 1024)])


def _rock_placements():
    out = []
    for r in _rock_recs():
        s = r["size"]
        z = r["center"][2] - s[2] * 0.5
        out.append(P((r["center"][0], r["center"][1], z), r["rot"], (s[0] / 100.0, s[1] / 100.0, s[2] / 100.0), "tank rock line %d" % r["line"]))
    return out


@asset("SM_Prop_Boulder", "Shared/Nature",
       desc="Chunky faceted boulder (1 m reference) with a flattened top facet and a darker underside.",
       replaces=["Rocks on the island and in the tank (FTStudioShell.cpp:875-878) and the prop rock in the warehouse (FTStudioShell.cpp:1024)"],
       placements=_rock_placements, covers=lambda: [[r] for r in _rock_recs()], per_placement=True,
       pivot="bottom centre; 100 cm reference, placements scale it to the code sizes")
def boulder(a):
    from mathutils import Vector
    from ftb.core import bm_ico, deform, wobble, xf
    bm = bm_ico(50, 3)
    wobble(bm, 9, scale=0.03, seed=4)
    deform(bm, lambda c: Vector((c.x * 1.02, c.y * 0.98, c.z * 0.8)))
    deform(bm, lambda c: Vector((c.x, c.y, max(c.z * 1.2, -46.0) + 46.0)))
    deform(bm, lambda c: Vector((c.x, c.y, c.z if c.z < 88 else 88 + (c.z - 88) * 0.4)))
    a.add(bm, col=C.ROCK_A, rough=0.9, flat=True, jitter=0)
    for (x, y, r, col) in ((34, 22, 15, C.ROCK_B), (-30, -30, 11, C.ROCK_A)):
        b = bm_ico(r, 1)
        wobble(b, r * 0.25, scale=0.08, seed=int(r))
        deform(b, lambda c: Vector((c.x, c.y, max(c.z, -r * 0.5) + r * 0.5)))
        a.add(b, xf((x, y, 0)), col=col, rough=0.9, flat=True, jitter=0)


# ============================================================================== floor arrows

def _arrow_calls(color_pred):
    out = []
    for area in ("studio", "city"):
        for c in L.kit_calls(area, "Arrow"):
            if color_pred(c["call"]["args"][2]):
                out.append(c)
    return out


def _arrow_pl(calls):
    out = []
    for c in calls:
        frm, to, colr, count = c["call"]["args"]
        dx, dy = to[0] - frm[0], to[1] - frm[1]
        yaw = math.degrees(math.atan2(dy, dx))
        for i in range(count):
            t = i / (count - 1) if count > 1 else 0
            out.append(P((frm[0] + dx * t, frm[1] + dy * t, frm[2] + (to[2] - frm[2]) * t), (0, yaw, 0), note="Arrow %s" % c["call"]["line"]))
    return out


def _arrow_asset(name, col, pred, desc):
    calls = lambda: _arrow_calls(pred)

    @asset(name, "Shared/Decals", desc=desc,
           replaces=["AFTStudioShell::Arrow() painted floor arrows (FTStudioShell.cpp:233-244), one mesh per arrow"],
           placements=lambda: _arrow_pl(calls()),
           covers=lambda: [r for c in calls() for r in c["records"]],
           pivot="the code's arrow point P (shaft behind, head in front along +X); 1.2 cm thick, sits on the floor plane",
           view=(0.3, 0.3, 1.0))
    def build(a):
        a.slab([(-70, -13), (6, -13), (6, -30), (47, 0), (6, 30), (6, 13), (-70, 13)], 1.2, at=(0, 0, -0.5), col=col, bevel=0.5, glow=0.35, ao=False)
        a.slab([(-64, -8), (0, -8), (0, -20), (34, 0), (0, 20), (0, 8), (-64, 8)], 0.4, at=(0, 0, 0.7), col=C.shade(col, 1.12), glow=0.35, ao=False)
    return build


def _col_is(lin, target):
    t = [C.srgb_to_lin(x) for x in target]
    return all(abs(lin[i] - t[i]) < 0.02 for i in range(3))


_arrow_asset("SM_Deco_FloorArrow_Coral", C.CORAL, lambda c: _col_is(c, C.CORAL), "Painted coral wayfinding arrow (street -> lobby -> office, boulevard).")
_arrow_asset("SM_Deco_FloorArrow_Yellow", C.YELLOW, lambda c: _col_is(c, C.YELLOW), "Painted yellow wayfinding arrow (lobby -> Stage 4, effects corner).")
_arrow_asset("SM_Deco_FloorArrow_Cyan", C.CYAN, lambda c: _col_is(c, C.CYAN), "Painted cyan arrow (camera deck).")
_arrow_asset("SM_Deco_FloorArrow_Magenta", C.MAGENTA, lambda c: _col_is(c, C.MAGENTA), "Painted magenta arrow (wardrobe / stairs routes).")
