"""The Grand Cinema: plaza, building, foyer, mezzanine, hall, stage and curtains, marquee actor, seating."""
import math

from mathutils import Vector

from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_box, bulge, deform, xf
from ftb.registry import P, asset
from assets.studio_lobby import vent

F = "City/GrandCinema"
CIN = "AFTCityShell::BuildCinema()"
PLZ = "AFTCityShell::BuildPlaza()"
ACT = "World/FTCity.cpp"


def rec(*ranges, where=None):
    return lambda: L.select("city", lines=list(ranges), where=where)


def actor(name, comps):
    return "%s (%s): %s" % (name, ACT, comps)


# ============================================================================== plaza

@asset("SM_City_CinemaPlaza", F,
       desc="Cinema plaza: cream stone paving with joints, the red carpet runner with brass edges, two gold star medallions with brass rings, terracotta planters with round hedges.",
       replaces=[PLZ + " plaza slab, joints, carpet, brass lines, star emblems, planters, hedges"],
       placements=lambda: [P((-10800, -150, 0))], covers=rec((346, 353), (366, 379)),
       pivot="world-aligned, pivot (-10800, -150, 0)", integration="Rope posts/ropes are SM_Prop_BrassStanchion / SM_Prop_VelvetRope placements; Blockers stay.", view=(1, 0.6, 1))
def plaza(a):
    pv = (-10800, -150, 0)
    a.records(L.select("city", lines=[(346, 353), (366, 379)]), pivot=pv, bevel=1.5)
    for (x, y) in ((-10620, -800), (-10620, 500)):
        pts = []
        for k in range(10):
            ang = math.radians(90 + k * 36)
            rr = 80 if k % 2 == 0 else 34
            pts.append((math.cos(ang) * rr, math.sin(ang) * rr))
        a.slab(pts, 1.2, at=(x - pv[0], y - pv[1], 4.8), col=C.BRASS, bevel=0.4, glow=0.5, ao=False)
        a.cyl(30, 1.4, at=(x - pv[0], y - pv[1], 4.9), col=C.CARPET, sides=16, bevel=0.3, ao=False)
    for y in (-1480, 1180):
        a.box((810, 60, 8), at=(0, y - pv[1], 66), col=C.PLAZA_STONE, bevel=3)
        for x in range(-10550, -11150, -200):
            for k in range(3):
                a.sphere(8, at=(x - pv[0] + (k - 1) * 22, y - pv[1] + 14, 110 + (k % 2) * 6), col=C.CORAL if k != 1 else C.YELLOW, segs=6, rings=4)


# ============================================================================== building shell + facade

@asset("SM_Cinema_Shell", F,
       desc="Grand Cinema shell: dark indigo outer walls with the entrance opening, roof slab with a parapet cap and rooftop vents, interior floor slab.",
       replaces=[CIN + " floor slab, outer walls (incl. entrance header), roof"],
       placements=lambda: [P((-11200, -150, 0))], covers=rec((395, 402)),
       pivot="world-aligned, pivot (-11200, -150, 0) = middle of the facade at ground level; facade faces +X",
       integration="Wall collision stays with the code CubeSolids.", view=(1, 0.8, 0.6))
def cinema_shell(a):
    pv = (-11200, -150, 0)
    a.records(L.select("city", lines=[(395, 402)]), pivot=pv, bevel=2)
    a.box(( 3620, 30, 20), at=(-1800, 1100 - pv[1] - 20, 1590), col=C.shade(C.CINEMA_OUTER, 1.4), bevel=3)
    a.box((3620, 30, 20), at=(-1800, -1400 - pv[1] + 20, 1590), col=C.shade(C.CINEMA_OUTER, 1.4), bevel=3)
    for x in (-900, -2200, -3000):
        a.box((120, 120, 60), at=(x, 300, 1610), col=C.GREY, bevel=6)
        a.cyl(40, 8, at=(x, 300, 1644), col=C.GREY_DARK, sides=12)
    for x in range(-3400, -200, 400):
        for y in (1101 - pv[1], -1401 - pv[1]):
            a.box((24, 6, 1400), at=(x, y, 770), col=C.shade(C.CINEMA_OUTER, 1.25), bevel=2)


@asset("SM_Cinema_FacadeDressing", F,
       desc="Art-deco facade dressing: cream pilasters with glowing brass crowns, gold bands, brass entrance jambs and header, entrance glass, warm upper windows with frames and a sunburst above the doors.",
       replaces=[CIN + " facade pilasters + prism caps, gold bands, entrance frame, glass, upper windows"],
       placements=lambda: [P((-11200, -150, 0))], covers=rec((404, 425)),
       pivot="same as SM_Cinema_Shell (-11200, -150, 0)", view=(1, 0.5, 0.35))
def cinema_facade(a):
    pv = (-11200, -150, 0)
    recs = L.select("city", lines=[(404, 425)])
    a.records([r for r in recs if r["group"] != L.GROUP["PrismDeco"]], pivot=pv, bevel=2)
    for r in recs:
        if r["group"] == L.GROUP["PrismDeco"]:
            c = r["center"]
            for k in range(3):
                a.box((26, 70 - k * 20, 26), at=(c[0] - pv[0], c[1] - pv[1], 1552 + k * 26), col=C.BRASS, bevel=4, glow=0.4)
            a.sphere(10, at=(c[0] - pv[0], c[1] - pv[1], 1640), col=C.YELLOW, segs=10, rings=6, glow=2)
        if r["group"] == L.GROUP["BoxDeco"] and r["size"][2] > 1000:
            c = r["center"]
            for k in (-1, 1):
                a.box((6, 8, 1500), at=(c[0] - pv[0] + 14, c[1] - pv[1] + k * 22, 770), col=C.CREAM_DARK, bevel=1.5)
        if r["group"] == L.GROUP["CubeDeco"] and abs(r["size"][1] - 120) < 1:
            c = r["center"]
            a.box((8, 130, 10), at=(c[0] - pv[0] + 3, c[1] - pv[1], c[2] - 64), col=C.CREAM, bevel=2)
            a.box((6, 4, 120), at=(c[0] - pv[0] + 3, c[1] - pv[1], c[2]), col=C.shade(C.CINEMA_OUTER, 0.8), bevel=0.8)
    for k in range(9):
        ang = math.radians(-80 + k * 20)
        a.box((6, 10, 90), at=(8, 55 * math.sin(ang) * 2.2, 460 + 55 * math.cos(ang)), rot=(0, 0, -math.degrees(ang)), col=C.BRASS, bevel=2, glow=0.5)


# ============================================================================== foyer

@asset("SM_Cinema_Foyer", F,
       desc="Foyer: crimson carpet with brass inlay strips, plum wall panelling with brass pilaster strips, skirting and a crown frieze, vents.",
       replaces=[CIN + " foyer carpet, brass inlays, foyer wall panels + brass strips"],
       placements=lambda: [P((-11920, -150, 0))], covers=rec((427, 438)),
       pivot="world-aligned, pivot (-11920, -150, 0)", view=(0.6, 0.9, 1))
def foyer(a):
    pv = (-11920, -150, 0)
    a.records(L.select("city", lines=[(427, 438)]), pivot=pv, bevel=1)
    for y, s in ((1055, -1), (-1355, 1)):
        a.box((1350, 4, 16), at=(0, y - pv[1] + s * 4, 12), col=C.shade(C.FOYER_WALL, 0.7), bevel=1)
        a.box((1350, 5, 24), at=(0, y - pv[1] + s * 4, 1370), col=C.BRASS, bevel=1.5, rough=0.35)
        for x in range(-12550, -11300, 150):
            a.poly([(-30, 0), (30, 0), (0, 30)], 3, at=(x - pv[0], y - pv[1] + s * 6, 1330), rot=(0, 90 if s < 0 else -90, 0), col=C.BRASS, glow=0.3)
    vent(a, (-11500 - pv[0], 1053 - pv[1], 1100), rot=(0, -90, 0), w=90, h=40, col=C.BRASS)


def _chandeliers():
    return [(-11650, -150, 820), (-12250, 150, 820)]


@asset("SM_Cinema_Chandelier", F,
       desc="Art-deco chandelier: long brass rod with a ceiling rose, tiered brass rings, glass drops, an opal centre globe and eight candle bulbs.",
       replaces=[CIN + " chandelier rod, ring, globe and 8 bulbs (x2)"],
       placements=lambda: [P(c) for c in _chandeliers()],
       covers=lambda: [L.select("city", lines=[(441, 449)], where=lambda r, c=c: abs(r["center"][0] - c[0]) < 200 and abs(r["center"][1] - c[1]) < 200) for c in _chandeliers()],
       per_placement=True, pivot="chandelier ring centre (the code's C, Z 820); rod reaches up to the roof (Z 1540)", integration="Keep the code PointLight 80 cm below.", view=(1, 0.6, 0.2))
def chandelier(a):
    a.cyl(2.5, 720, at=(0, 0, 360), col=C.BRASS, sides=8, rough=0.3)
    a.cyl(18, 6, at=(0, 0, 716), col=C.BRASS, sides=14, bevel=2, rough=0.3)
    for (R, z, r) in ((80, 0, 5), (58, 22, 4), (36, 40, 3.5)):
        a.torus(R, r, at=(0, 0, z), col=C.BRASS, major=28, minor=6, rough=0.3)
        for k in range(8):
            ang = math.radians(k * 45)
            a.tube([(0, 0, z + 60), (R * math.cos(ang), R * math.sin(ang), z)], 1.2, col=C.BRASS, sides=5, rough=0.3)
    for k in range(16):
        ang = math.radians(k * 22.5)
        a.cone(3.5, 12, at=(80 * math.cos(ang), 80 * math.sin(ang), -12), rot=(180, 0, 0), col=C.CREAM, sides=6, glow=1.5)
    a.sphere(30, at=(0, 0, -30), col=C.CREAM, segs=16, rings=10, glow=8)
    for k in range(8):
        ang = math.radians(k * 45)
        p = (78 * math.cos(ang), 78 * math.sin(ang), 12)
        a.cyl(4, 12, at=(p[0], p[1], 8), col=C.WHITE, sides=8)
        a.sphere(6, at=p, col=C.WINDOW_WARM, segs=8, rings=6, glow=10)


@asset("SM_Cinema_BoxOffice", F,
       desc="Box office: coral counter with cream top and brass trim, glass screen with a speaking grille and ticket slot, ticket roll dispenser, brass bell and the navy BOX OFFICE board.",
       replaces=[CIN + " box office counter, top, glass, sign board"],
       placements=lambda: [P((-11650, 930, 0))], covers=rec((453, 456)),
       pivot="counter centre on the floor (-11650, 930, 0); customers stand on -Y", integration="'BOX OFFICE' stays a TextRender.", view=(0.4, -1, 0.5))
def box_office(a):
    pv = (-11650, 930, 0)
    a.records(L.select("city", lines=[(453, 456)]), pivot=pv, bevel=3)
    for k in range(8):
        a.box((4, 3, 90), at=(-200 + k * 57, -61, 55), col=C.CREAM, bevel=1)
    a.box((466, 4, 8), at=(0, 30, 280), col=C.BRASS, bevel=1.5, rough=0.35)
    for s in (-1, 1):
        a.box((8, 8, 170), at=(s * 232, 30, 200), col=C.BRASS, bevel=2, rough=0.35)
    for x in (-120, 120):
        a.cyl(12, 2, at=(x, 27, 160), rot=(0, 0, 90), col=C.BRASS, sides=14, bevel=0.5, rough=0.35)
        a.box((60, 10, 4), at=(x, 28, 122), col=C.BRASS, bevel=1)
    a.box((30, 26, 30), at=(-180, 0, 134), col=C.CHARCOAL, bevel=4)
    a.cyl(10, 24, at=(-180, 0, 152), rot=(90, 0, 0), col=C.CORAL, sides=12, bevel=2)
    a.sphere(6, at=(150, -40, 122), rz=5, col=C.BRASS, segs=10, rings=6, rough=0.2)
    for k in range(4):
        a.box((24, 12, 0.6), at=(40 + k * 3, -30, 119.5 + k * 0.6), rot=(0, k * 7, 0), col=C.YELLOW if k % 2 else C.CREAM, bevel=0.2)


@asset("SM_Cinema_Concessions", F,
       desc="Concessions stand: teal counter with cream top, a glass popcorn machine full of popcorn, red and blue soda cups with straws, candy jars and the POPCORN - CANDY - SODA board.",
       replaces=[CIN + " concessions counter, top, popcorn case + popcorn, soda cups, sign board"],
       placements=lambda: [P((-12260, 930, 0))], covers=rec((458, 467)),
       pivot="counter centre on the floor (-12260, 930, 0)", integration="Board text stays a TextRender.", view=(0.4, -1, 0.5))
def concessions(a):
    pv = (-12260, 930, 0)
    recs = L.select("city", lines=[(458, 467)])
    keep = [r for r in recs if not (r["group"] == L.GROUP["SphereDeco"] or r["group"] == L.GROUP["CylDeco"])]
    a.records(keep, pivot=pv, bevel=3)
    for k in range(9):
        a.box((50, 3, 20), at=(-230 + k * 58, -61, 60), col=C.shade(C.TEAL, 1.25), bevel=1.5)
    # popcorn machine (code glass at (-12400, 940, 175))
    x0 = -12400 - pv[0]
    a.box((84, 74, 10), at=(x0, 10, 124), col=C.RED, bevel=2)
    a.box((84, 74, 12), at=(x0, 10, 236), col=C.RED, bevel=3)
    a.box((60, 40, 14), at=(x0, 10, 250), col=C.YELLOW, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((4, 4, 110), at=(x0 + sx * 40, 10 + sy * 35, 180), col=C.RED, bevel=1)
    rng = a.rng
    for k in range(40):
        a.sphere(rng.uniform(3, 5), at=(x0 + rng.uniform(-34, 34), 10 + rng.uniform(-28, 28), 132 + rng.uniform(0, 26)), col=C.POPCORN, segs=6, rings=4, glow=0.6)
    # soda cups (code: red at -12150, blue at -12050; 40 wide, 60 tall at z 150)
    for (x, c) in ((-12150, C.RED), (-12050, C.BLUE)):
        a.cyl(17, 58, at=(x - pv[0], 910 - pv[1], 150), r_top=20, col=c, sides=14, bevel=2)
        a.cyl(21, 5, at=(x - pv[0], 910 - pv[1], 181), col=C.WHITE, sides=14, bevel=1.5)
        a.cyl(2, 36, at=(x - pv[0] + 6, 910 - pv[1], 198), rot=(8, 0, 0), col=C.WHITE, sides=6)
        a.box((22, 2, 12), at=(x - pv[0], 910 - pv[1] - 18, 150), col=C.WHITE, bevel=0.8)
    for k, c in enumerate((C.MAGENTA, C.CYAN, C.YELLOW)):
        a.cyl(10, 26, at=(-160 + k * 26 + 280, 30, 132), col=C.GLASS, sides=12, bevel=2, mat="glass")
        for j in range(5):
            a.sphere(3.5, at=(-160 + k * 26 + 280 + rng.uniform(-5, 5), 30 + rng.uniform(-5, 5), 124 + j * 4), col=c, segs=6, rings=4)


# ============================================================================== mezzanine + partition

@asset("SM_Cinema_Mezzanine", F,
       desc="Mezzanine to the projection booth: 21-step crimson stair with brass nosings along the south wall, booth floor with navy carpet, turned brass balusters and handrails, brass wall stripe.",
       replaces=[CIN + " mezzanine stairs, booth floor + carpet, baluster posts, rails, wall stripe"],
       placements=lambda: [P((-12000, -1000, 0))], covers=rec((474, 489)),
       pivot="world-aligned, pivot (-12000, -1000, 0)", integration="Collision and invisible Blockers stay; 'PROJECTION BOOTH ^' stays a TextRender.", view=(1, 0.9, 0.8))
def mezzanine(a):
    pv = (-12000, -1000, 0)
    recs = L.select("city", lines=[(474, 489)])
    posts = [r for r in recs if r["group"] == L.GROUP["CylDeco"]]
    a.records([r for r in recs if r not in posts], pivot=pv, bevel=1.2)
    for r in posts:
        c = r["center"]
        a.lathe([(3.5, 0), (5, 6), (2.6, 30), (4.2, 60), (2.6, 90), (4, 98), (0.1, 100)], at=(c[0] - pv[0], c[1] - pv[1], 424), col=C.BRASS, sides=8, rough=0.35)
    for k in range(21):
        x = -11400 - 30 * (k + 0.5) - pv[0]
        a.box((6, 222, 3), at=(x + 13, -1250 - pv[1], 4 + 20 * (k + 1) + 1), col=C.BRASS, bevel=1, rough=0.35)
    a.tube([(-11400 - pv[0], -1142 - pv[1], 110), (-12030 - pv[0], -1142 - pv[1], 110 + 420)], 3, col=C.BRASS, sides=8, rough=0.35)
    for k in range(6):
        x = -11420 - k * 120
        z = 4 + (-11400 - x) / 30 * 20
        a.cyl(2.5, 100, at=(x - pv[0], -1142 - pv[1], z + 55), col=C.BRASS, sides=8, rough=0.35)


@asset("SM_Cinema_Partition", F,
       desc="Foyer/hall partition: plum wall with the double-door opening, brass door jambs and header, the projection booth window with a brass frame, HALL 1 board with a bulb border.",
       replaces=[CIN + " partition wall pieces, booth glass, door trims, HALL 1 board"],
       placements=lambda: [P((-12620, 0, 0))], covers=rec((497, 510)),
       pivot="world-aligned, pivot (-12620, 0, 0) = door opening centre; foyer side +X", integration="'HALL 1 - WORLD PREMIERE' stays a TextRender.", view=(1, 0.5, 0.4))
def partition(a):
    pv = (-12620, 0, 0)
    a.records(L.select("city", lines=[(497, 510)]), pivot=pv, bevel=2)
    for (z, h) in ((520, 12), (720, 12)):
        a.box((14, 410, h), at=(24, -900, z), col=C.BRASS, bevel=2, rough=0.35)
    for y in (-1100, -700):
        a.box((14, 12, 210), at=(24, y, 620), col=C.BRASS, bevel=2, rough=0.35)
    for k in range(14):
        y = -270 + k * 540 / 13
        for z in (428, 512):
            a.sphere(4.5, at=(31, y, z), col=C.WINDOW_WARM, segs=6, rings=4, glow=6)


# ============================================================================== hall

@asset("SM_Cinema_Hall", F,
       desc="Hall 1: dark plum carpet, deep aubergine ceiling with coffers, hall walls with brass pilasters and fan crowns, sconce-lit side walls, amber aisle lights and the EXIT box.",
       replaces=[CIN + " hall carpet, ceiling, wall panels, pilasters + fan prisms, EXIT box, aisle lights"],
       placements=lambda: [P((-13700, -150, 0))], covers=rec((512, 522), (549, 555)),
       pivot="world-aligned, pivot (-13700, -150, 0)", integration="'EXIT' stays a TextRender.", view=(0.8, 0.9, 1))
def hall(a):
    pv = (-13700, -150, 0)
    a.records(L.select("city", lines=[(512, 522), (549, 555)]), pivot=pv, bevel=1)
    for x in range(-14600, -12700, 350):
        a.box(( 24, 2400, 30), at=(x - pv[0] + 175, 0, 1520), col=C.shade(C.HALL_CEIL, 1.5), bevel=3)
    for y in (-800, 0, 800):
        a.box((2100, 24, 30), at=(0, y, 1520), col=C.shade(C.HALL_CEIL, 1.5), bevel=3)
    for y, s in ((1055, -1), (-1355, 1)):
        a.box((2110, 5, 20), at=(0, y - pv[1] + s * 3, 12), col=C.shade(C.HALL_WALL, 0.7), bevel=1)
        for x in range(-14700, -12700, 350):
            b = bm_box(300, 8, 540, 6, 1)
            bulge(b, 3, axis=1)
            a.add(b, xf((x - pv[0] + 175 + 7, y - pv[1] + s * 6, 330)), C.shade(C.HALL_WALL, 1.25), rough=0.95)


@asset("SM_Cinema_Stage", F,
       desc="Wooden stage apron in front of the screen: plank top, brass lip with footlight cups, stair treads at both ends.",
       replaces=[CIN + " stage platform + brass trim"],
       placements=lambda: [P((-14545, -150, 0))], covers=rec((525, 526)),
       pivot="stage footprint centre on the floor (-14545, -150, 0); audience side +X", view=(1, 0.4, 0.5))
def stage(a):
    pv = (-14545, -150, 0)
    a.records(L.select("city", lines=[(525, 526)]), pivot=pv, bevel=2)
    for y in range(-1140, 1150, 40):
        a.box((426, 36, 1.2), at=(0, y, 64.6), col=C.shade(C.STAGE_WOOD, a.rng.uniform(0.9, 1.15)), bevel=0.3, segs=1, ao=False)
    for y in range(-1050, 1051, 150):
        a.cyl(7, 6, at=(221, y, 60), rot=(90, 0, 0), col=C.BRASS, sides=10, bevel=1, rough=0.35)
        a.sphere(4, at=(223, y, 60), col=C.WINDOW_WARM, segs=6, rings=4, glow=4)
    for y in (-1200, 900):
        for k in range(3):
            a.box((40, 120, 16 * (3 - k)), at=(235 + k * 40 - 20 + 20, y + 60, 8 * (3 - k)), col=C.STAGE_WOOD, bevel=2)


@asset("SM_Cinema_Curtains", F,
       desc="Stage curtains: heavy crimson side drapes with deep folds, gold tie-backs with tassels, a scalloped valance with a gold fringe.",
       replaces=[CIN + " side curtains + fold strips, valance, valance trim"],
       placements=lambda: [P((-14680, -150, 0))], covers=rec((528, 536)),
       pivot="world-aligned, pivot (-14680, -150, 0); curtains face +X", view=(1, 0.3, 0.35))
def curtains(a):
    pv = (-14680, -150, 0)
    for yc in (-1180, 880):
        y0 = yc - pv[1]
        for f in range(7):
            y = y0 - 105 + f * 35
            a.cyl(20, 1500, at=(0 + (f % 2) * 8, y, 760), r_top=18, col=C.CARPET if f % 2 == 0 else C.CURTAIN_FOLD, sides=8, bevel=0, rough=0.95)
        a.torus(22, 5, at=(26, y0, 520), rot=(90, 0, 0), col=C.BRASS, major=16, minor=6, rough=0.35)
        a.cone(8, 30, at=(40, y0, 480), col=C.YELLOW, sides=8, r_top=3)
        a.sphere(6, at=(40, y0, 498), col=C.BRASS, segs=8, rings=5)
    a.box((80, 2360, 150), at=(0, 0, 1440), col=C.CARPET, bevel=6, rough=0.95)
    for k in range(20):
        y = -1150 + k * 121
        a.cyl(60, 20, at=(42, y, 1370), rot=(0, 0, 90), col=C.CARPET if k % 2 == 0 else C.CURTAIN_FOLD, sides=12, bevel=2, rough=0.95)
    a.box(( 6, 2360, 12), at=(42, 0, 1368), col=C.BRASS, bevel=2, glow=0.5)
    for k in range(60):
        a.box((3, 2, 16), at=(44, -1170 + k * 40, 1300), col=C.YELLOW, bevel=0.4)


def _sconces():
    return L.select("city", lines=[(541, 541)])


@asset("SM_Cinema_Sconce", F,
       desc="Brass wall sconce: stepped back plate, fluted up-light cup and an opal bulb.",
       replaces=[CIN + " six hall sconces (cone + bulb)"],
       placements=lambda: [P((r["center"][0], r["center"][1], 420), (0, 90 if r["center"][1] < 0 else -90, 0)) for r in _sconces()],
       covers=lambda: [L.select("city", lines=[(541, 542)], where=lambda q, r=r: abs(q["center"][0] - r["center"][0]) < 1 and abs(q["center"][1] - r["center"][1]) < 1) for r in _sconces()],
       per_placement=True, pivot="cup centre on the wall line (Z 420); faces local +X into the hall", view=(1, 0.4, 0.3))
def sconce(a):
    a.box((6, 30, 60), at=(-14, 0, 0), col=C.BRASS, bevel=2, rough=0.35)
    a.lathe([(4, -25), (12, -18), (20, 0), (22, 25)], col=C.BRASS, sides=12, rough=0.35)
    a.sphere(11, at=(0, 0, 30), col=C.WINDOW_WARM, segs=10, rings=6, glow=9)


# ============================================================================== marquee actor

@asset("SM_Cinema_Marquee", F,
       desc="Grand Cinema marquee: navy canopy with a brass edge and a coffered underside, brass tie rods, coral marquee box with a cream letter board, brass top rail and stepped art-deco ends.",
       replaces=[actor("AFTCinemaMarquee", "Canopy, CanopyTrim, CanopyRod x2, Marquee, MarqueeBoard, MarqueeTop")],
       placements=lambda: [P((-11200, -150, 0))], pivot="actor root (facade base); marquee projects along +X",
       integration="Bulbs are separate (SM_Cinema_MarqueeBulb, chase-lit by the code); Line0-2 and the crown stay TextRenders.", view=(1, 0.6, 0.3))
def marquee(a):
    a.box((340, 960, 26), at=(170, 0, 440), col=C.NAVY, bevel=6)
    a.box((6, 964, 30), at=(342, 0, 440), col=C.BRASS, bevel=2, glow=0.5)
    for k in range(6):
        a.box((300, 8, 10), at=(170, -400 + k * 160, 422), col=C.shade(C.NAVY, 1.4), bevel=2)
    for y in (-440, 440):
        # code: 560 cm rod centred at (220, y, 700), pitch -38 -> ends at (47.5, 479) and (392.5, 921)
        a.tube([(47.5, y, 479.4), (392.5, y, 920.6)], 3, col=C.BRASS, sides=8, rough=0.3)
    a.box((150, 1160, 230), at=(190, 0, 580), col=C.CORAL, bevel=10)
    a.box((2, 1090, 180), at=(266, 0, 580), col=C.CREAM, glow=0.7)
    a.box((6, 1100, 6), at=(268, 0, 672), col=C.CORAL_DARK, bevel=1.5)
    a.box((6, 1100, 6), at=(268, 0, 488), col=C.CORAL_DARK, bevel=1.5)
    a.box((160, 1180, 14), at=(190, 0, 702), col=C.BRASS, bevel=4, glow=0.4)
    for s in (-1, 1):
        for k in range(3):
            a.box((150 - k * 30, 20, 60 - k * 12), at=(190, s * (590 + k * 16), 610 - k * 10), col=C.CREAM, bevel=4)
        a.sphere(12, at=(190, s * 640, 700), col=C.YELLOW, segs=10, rings=6, glow=4)


@asset("SM_Cinema_MarqueeBulb", F,
       desc="Marquee bulb in a chrome cup (the code drives the chase glow per component).",
       replaces=[actor("AFTCinemaMarquee", "BulbTop0-17, BulbLow0-17")],
       pivot="bulb centre (component location), cup behind it towards -X", view=(1, 0.5, 0.3))
def marquee_bulb(a):
    a.cyl(7.5, 6, at=(-4, 0, 0), rot=(90, 0, 0), col=C.CHROME, sides=10, bevel=1, rough=0.25)
    a.sphere(6.5, col=C.WINDOW_WARM, segs=10, rings=7, glow=6)


@asset("SM_Cinema_BladeSign", F,
       desc="Vertical GRAND blade sign: navy blade with a glowing brass border, stepped crown and bottom finial, bulb dots.",
       replaces=[actor("AFTCinemaMarquee", "Blade, BladeTrim")],
       placements=lambda: [P((-11200, -150, 0))], pivot="actor root; blade at local (22, 1030, 1000)",
       integration="Letter0-4 stay TextRenders (or use SM_Sign_City_GRAND_Blade).", view=(1, 0.5, 0.3))
def blade_sign(a):
    a.box((34, 190, 860), at=(22, 1030, 1000), col=C.NAVY, bevel=8)
    a.box((4, 200, 870), at=(40, 1030, 1000), col=C.BRASS, bevel=1.5, glow=0.6)
    a.box((5, 176, 846), at=(41, 1030, 1000), col=C.NAVY, bevel=1.5)
    for k in range(2):
        a.box((34 - k * 6, 190 - k * 60, 20), at=(22, 1030, 1440 + k * 20), col=C.BRASS if k % 2 else C.CORAL, bevel=5, glow=0.3)
    a.cone(36, 45, at=(22, 1030, 542), rot=(180, 0, 0), col=C.BRASS, sides=8)
    for k in range(20):
        a.sphere(4, at=(44, 1030 + (-88 if k % 2 == 0 else 88), 600 + (k // 2) * 84), col=C.WINDOW_WARM, segs=6, rings=4, glow=8)


@asset("SM_Cinema_PosterCase", F,
       desc="Illuminated poster case: charcoal box with a magenta light-box face, brass frame and corner studs.",
       replaces=[actor("AFTCinemaMarquee", "PosterBox, PosterGlow")],
       placements=lambda: [P((-11200, -150, 0))], pivot="actor root; case at local (20, -1030, 900)",
       integration="PosterTitle / PosterStars stay TextRenders.", view=(1, 0.5, 0.3))
def poster_case(a):
    a.box((30, 200, 520), at=(20, -1030, 900), col=C.CHARCOAL, bevel=5)
    a.box((2, 176, 490), at=(36, -1030, 900), col=C.MAGENTA, glow=0.8)
    for (y, z, w, h) in ((-1030, 1150, 190, 10), (-1030, 650, 190, 10)):
        a.box((6, w, h), at=(37, y, z), col=C.BRASS, bevel=2, rough=0.35)
    for y in (-1123, -937):
        a.box((6, 10, 510), at=(37, y, 900), col=C.BRASS, bevel=2, rough=0.35)


@asset("SM_Cinema_SearchlightBase", F,
       desc="Rooftop searchlight turret base with a slewing ring and cable.", replaces=[actor("AFTCinemaMarquee", "SearchBase0-1")],
       placements=lambda: [P((-11400, -1300, 1580)), P((-11400, 1000, 1580))], pivot="turret foot on the roof", view=(1, 0.5, 0.4))
def searchlight_base(a):
    a.cyl(45, 50, at=(0, 0, 25), r_top=38, col=C.GREY_DARK, sides=16, bevel=4)
    a.torus(40, 4, at=(0, 0, 52), col=C.GREY, major=18, minor=6, rough=0.4)
    for k in range(8):
        ang = math.radians(k * 45 + 22.5)
        a.box((6, 6, 5), at=(41 * math.cos(ang), 41 * math.sin(ang), 3), rot=(0, k * 45 + 22.5, 0), col=C.GREY, bevel=1)
    a.tube([(36, 0, 14), (46, 4, 6), (52, 6, 1)], 3, col=C.RUBBER, sides=6)


@asset("SM_Cinema_SearchlightHead", F,
       desc="Searchlight drum with cooling ribs, a glowing lens and a rear cap (rotated by the code).",
       replaces=[actor("AFTCinemaMarquee", "SearchHead: SearchDrum, SearchLens")],
       pivot="SearchHead component; the light points along +X", integration="SearchBeam cone + SpotLight stay.", view=(1, 0.5, 0.4))
def searchlight_head(a):
    a.cyl(40, 90, rot=(-90, 0, 0), col=C.GREY, sides=18, bevel=4, rough=0.4)
    for k in range(5):
        a.torus(40.5, 2, at=(-30 + k * 12, 0, 0), rot=(-90, 0, 0), col=C.shade(C.GREY, 0.8), major=20, minor=5)
    a.cyl(35, 4, at=(46, 0, 0), rot=(-90, 0, 0), col=C.CREAM, sides=18, glow=20)
    a.torus(38, 3, at=(46, 0, 0), rot=(-90, 0, 0), col=C.CHROME, major=20, minor=6, rough=0.25)
    for s in (-1, 1):
        a.cyl(8, 10, at=(0, s * 44, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=2)


@asset("SM_Cinema_ChartBoard", F,
       desc="Box-office chart board: brass frame with rounded corners, dark panel with ruled lines, a crown ornament and a star.",
       replaces=[actor("AFTBoxOfficeBoard", "Frame, Panel")],
       placements=lambda: [P((-12592, 700, 0))], pivot="actor root; board faces +X", integration="Header, Lines and Footer stay TextRenders.", view=(1, 0.4, 0.3))
def chart_board(a):
    a.box((12, 600, 340), at=(-6, 0, 300), col=C.BRASS, bevel=6, glow=0.3)
    a.box((4, 570, 312), at=(1, 0, 300), col=C.CHART_PANEL, bevel=1.5)
    for k in range(9):
        a.box((1, 540, 1), at=(3.4, 0, 408 - k * 32), col=C.shade(C.CHART_PANEL, 1.8), bevel=0)
    a.poly([(-60, 0), (60, 0), (40, 30), (20, 20), (0, 40), (-20, 20), (-40, 30)], 8, at=(-2, 0, 470), col=C.BRASS, bevel=2, glow=0.4)


# ============================================================================== seating + audience

@asset("SM_Cinema_SeatingTiers", F,
       desc="Stadium seating tiers for the hall: twelve carpeted steps in alternating plum tones, brass nosing on every step, riser kick plates with little amber step lights, and side cheeks.",
       replaces=[actor("AFTCinemaSeating", "Tiers ISM (12 riser blocks + 12 brass nosings, OnConstruction)")],
       pivot="AFTCinemaSeating actor root (row 0, seat 0 at the front-left); rows run along +X, seats along +Y",
       integration="Replaces the Tiers ISM visually; keep the ISM (it is solid) for collision and hide it, or give this mesh collision.",
       view=(-0.6, 0.9, 0.7))
def seating_tiers(a):
    rows, cols, row_sp, seat_sp, riser = 12, 20, 100.0, 72.0, 32.0
    width = (cols - 1) * seat_sp + 120.0
    mid_y = (cols - 1) * seat_sp * 0.5
    for r in range(rows):
        top = (r + 1) * riser
        x = r * row_sp
        col = C.hex_rgb(0x3A1A2C) if r % 2 else C.hex_rgb(0x42203A)
        a.box((row_sp, width, top), at=(x, mid_y, top * 0.5), col=col, bevel=1.5, rough=0.95)
        # brass nosing at the front edge of the step
        a.box((4, width, 2), at=(x - row_sp * 0.5 + 2, mid_y, top - 1), col=C.BRASS, bevel=0.6, rough=0.35, glow=0.3)
        # riser kick plate + step lights (front face of the step, facing -X)
        if r > 0:
            a.box((1.2, width - 8, riser - 8), at=(x - row_sp * 0.5 - 0.3, mid_y, top - riser * 0.5), col=C.shade(col, 0.8), bevel=0.3)
            for y in (8.0, width - 8.0):
                a.box((1.2, 6, 3), at=(x - row_sp * 0.5 - 0.8, mid_y - width * 0.5 + y, top - riser * 0.5), col=C.AMBER, bevel=0.4, glow=4)
    # side cheeks follow the steps: stepped XZ profile extruded 6 cm along Y
    import bmesh
    from ftb.core import bm_extrude
    prof = [(-row_sp * 0.5, 0.0)]
    for r in range(rows):
        top = (r + 1) * riser
        prof += [(r * row_sp - row_sp * 0.5, top), (r * row_sp + row_sp * 0.5, top)]
    prof += [((rows - 1) * row_sp + row_sp * 0.5, 0.0)]
    for side in (-1, 1):
        cheek = bm_extrude(prof, 6)
        deform(cheek, lambda c: Vector((c.x, c.z, c.y)))
        bmesh.ops.reverse_faces(cheek, faces=cheek.faces)
        y0 = mid_y + side * (width * 0.5 - 3) - 3
        a.add(cheek, xf((0, y0, 0)), C.shade(C.hex_rgb(0x3A1A2C), 0.75), rough=0.9)


@asset("SM_Cinema_Seat", F,
       desc="Hall seat: crimson tip-up seat cushion and padded back, brass-capped armrest and a cast pedestal.",
       replaces=[actor("AFTCinemaSeating", "SeatBases + SeatBacks instances (one seat)")],
       pivot="the code's SeatLocal(R, C) point on the tier top; seat faces -X (the screen)",
       integration="Use as a single ISM for all 240 seats at SeatLocal(R, C) instead of the two box ISMs.", view=(-1, 0.6, 0.4))
def hall_seat(a):
    c = bm_box(52, 58, 12, 5, 2)
    bulge(c, 2, axis=2)
    a.add(c, xf((-4, 0, 42)), C.CARPET, rough=0.95)
    b = bm_box(12, 60, 62, 5, 2)
    bulge(b, 2, axis=0)
    a.add(b, xf((24, 0, 74), (-10, 0, 0)), C.FOYER_CARPET, rough=0.95)
    a.box((8, 56, 50), at=(30, 0, 68), rot=(-10, 0, 0), col=C.shade(C.FOYER_CARPET, 0.7), bevel=3)
    a.box((16, 40, 34), at=(8, 0, 18), col=C.CHARCOAL, bevel=3)
    a.box((24, 6, 6), at=(2, 33, 58), col=C.CHARCOAL, bevel=2)
    a.box((6, 6, 4), at=(-10, 33, 61), col=C.BRASS, bevel=1, rough=0.3)
    a.box((6, 4, 50), at=(14, 33, 34), col=C.CHARCOAL, bevel=1.5)


@asset("SM_Cinema_AudienceBody", F,
       desc="Audience torso for the crowd ISM: rounded shirt body with shoulders, arms resting on the lap and a collar (authored in the unit capsule space).",
       replaces=[actor("AFTCinemaSeating", "Bodies ISM (SM_FT_Capsule)")],
       pivot="unit space like SM_FT_Capsule: fits x/y +-25, z -50..50; the code scales it (0.36, 0.42, BodyH/100)",
       integration="Swap the Bodies ISM mesh; transforms and per-instance colours stay (paint with M_FT_MatteISM or a vertex-colour x CPD material).", view=(-1, 0.6, 0.3))
def audience_body(a):
    secs = []
    for (z, rx, ry) in ((-50, 18, 22), (-30, 22, 24), (0, 21, 24), (25, 20, 25), (40, 16, 22), (48, 8, 10)):
        secs.append([(rx * math.cos(2 * math.pi * k / 12), ry * math.sin(2 * math.pi * k / 12), z) for k in range(12)])
    a.loft(secs, col=C.WHITE, rough=0.9)
    for s in (-1, 1):
        a.tube([(0, s * 24, 35), (-6, s * 26, 5), (-20, s * 16, -20)], 6, col=C.WHITE, sides=8)
    a.torus(9, 2.5, at=(0, 0, 46), col=C.shade(C.WHITE, 0.85), major=12, minor=5)


@asset("SM_Cinema_AudienceHead", F,
       desc="Audience head (unit ball space, r 50): face with a nose bump and ears.",
       replaces=[actor("AFTCinemaSeating", "Heads ISM (SM_FT_Ball)")],
       pivot="unit space like SM_FT_Ball (radius 50); the code scales it 0.3", view=(-1, 0.6, 0.3))
def audience_head(a):
    a.sphere(48, rz=50, col=C.WHITE, segs=14, rings=10, rough=0.8)
    a.sphere(9, at=(-46, 0, -2), col=C.WHITE, segs=8, rings=6)
    for s in (-1, 1):
        a.sphere(10, at=(0, s * 47, 0), ry=5, col=C.WHITE, segs=8, rings=6)


@asset("SM_Cinema_AudienceHair", F,
       desc="Audience hair cap (unit ball space): tufted crown with a fringe.",
       replaces=[actor("AFTCinemaSeating", "Hair ISM (SM_FT_Ball)")],
       pivot="unit space like SM_FT_Ball; the code scales it (0.32, 0.32, 0.2) and offsets it (4, 0, 8) from the head", view=(-1, 0.6, 0.6))
def audience_hair(a):
    a.sphere(50, rz=50, col=C.WHITE, segs=14, rings=8, rough=0.9)
    for k in range(7):
        ang = math.radians(-60 + k * 20)
        a.sphere(16, at=(-38 * math.cos(ang), 38 * math.sin(ang), -30), col=C.WHITE, segs=8, rings=5)
