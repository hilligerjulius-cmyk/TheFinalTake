"""Downtown boulevard: ground, road, street furniture, alleys, pocket park, parking lot, billboard."""
import math

from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_box, bulge, xf
from ftb.registry import P, asset

F = "City/Boulevard"
CITY = "AFTCityShell"


def rec(*ranges, where=None):
    return lambda: L.select("city", lines=list(ranges), where=where)


def near(a, b, tol=1.0):
    return all(abs(x - y) <= tol for x, y in zip(a, b))


@asset("SM_City_Ground", F,
       desc="District ground slab with subtle dirt/grass seams at the edges.",
       replaces=[CITY + "::BuildBoulevard() district ground slab"],
       placements=lambda: [P((-9350, 0, 0))], covers=rec((122, 122)),
       pivot="world-aligned, pivot (-9350, 0, 0) = ground top", view=(0.3, 0.3, 1))
def ground(a):
    a.records(L.select("city", lines=[(122, 122)]), pivot=(-9350, 0, 0), bevel=2)
    rng = a.rng
    for k in range(18):
        a.cyl(rng.uniform(60, 160), 0.6, at=(rng.uniform(-5500, 5500), rng.uniform(-2800, 2800), 0.2), ry=rng.uniform(40, 120),
              col=C.shade(C.GROUND, rng.uniform(0.85, 1.12)), sides=10, bevel=0, ao=False)


@asset("SM_City_Boulevard", F,
       desc="Boulevard: asphalt with lane dashes, patched repairs, manholes and drains, zebra crossing, sidewalks with paving joints and cream curbs with curb stones.",
       replaces=[CITY + "::BuildBoulevard() roadway, lane dashes, sidewalks, curbs, paving joints, zebra crossing"],
       placements=lambda: [P((-7050, -150, 0))], covers=rec((123, 142)),
       pivot="world-aligned, pivot (-7050, -150, 0) = road centre line at ground level", view=(0.2, 0.5, 1))
def boulevard(a):
    pv = (-7050, -150, 0)
    a.records(L.select("city", lines=[(123, 142)]), pivot=pv, bevel=1.0)
    rng = a.rng
    for (y0, sgn) in ((-402, -1), (102, 1)):
        for x in range(-10350, -3750, 200):
            a.box((196, 14, 3), at=(x + 100 - pv[0], y0 - pv[1] + sgn * 4, 5.2), col=C.shade(C.CREAM, rng.uniform(0.9, 1.02)), bevel=1, segs=1)
    for k in range(10):
        x = rng.uniform(-10200, -3900)
        a.box((rng.uniform(120, 260), rng.uniform(80, 180), 0.4), at=(x - pv[0], rng.uniform(-360, 60), 0.7), rot=(0, rng.uniform(-8, 8), 0), col=C.shade(C.ASPHALT, 0.82), bevel=0.2, ao=False)
    for x in (-4800, -6200, -8000, -9600):
        a.cyl(38, 1.2, at=(x - pv[0], rng.uniform(-80, 80), 0.9), col=C.GREY_DARK, sides=16, bevel=0.4, rough=0.5)
    for x in range(-9800, -3800, 1200):
        for y in (-396, 96):
            a.box((80, 8, 3), at=(x - pv[0], y - pv[1], 0.8), col=C.CHARCOAL, bevel=0.5)
            for k in range(5):
                a.box((3, 9, 3.2), at=(x - pv[0] - 30 + k * 15, y - pv[1], 0.9), col=C.GREY, bevel=0.2)


@asset("SM_City_TrafficLight", F,
       desc="Traffic light: cast base, charcoal pole, signal head with sun visors and red/amber/green lenses, pedestrian push-button box.",
       replaces=[CITY + "::BuildBoulevard() traffic light pole, head and three lamps"],
       placements=lambda: [P((-6860, -520, 4)), P((-7140, 220, 4))],
       covers=lambda: [L.select("city", lines=[(146, 150)], where=lambda r, x=x: abs(r["center"][0] - x) < 30) for x in (-6860, -7140)],
       per_placement=True, pivot="pole foot on the sidewalk; lamps face +X", view=(1, 0.6, 0.3))
def traffic_light(a):
    a.lathe([(0, 0), (14, 0), (14, 4), (9, 12), (7, 20), (0, 21)], col=C.CHARCOAL, sides=12)
    a.cyl(6, 360, at=(0, 0, 180), col=C.CHARCOAL, sides=10)
    a.box((30, 30, 90), at=(0, 0, 400), col=C.CHARCOAL, bevel=5)
    for (z, c, g) in ((428, C.RED, 2), (400, C.AMBER, 0.3), (372, C.GREEN, 6)):
        a.cyl(9, 4, at=(16, 0, z), rot=(-90, 0, 0), col=c, sides=14, bevel=1, glow=g)
        a.box((14, 22, 3), at=(22, 0, z + 11), rot=(-15, 0, 0), col=C.CHARCOAL, bevel=1)
        for s in (-1, 1):
            a.box((14, 3, 18), at=(22, s * 11, z + 3), col=C.CHARCOAL, bevel=0.8)
    a.box((10, 16, 22), at=(8, 0, 120), col=C.YELLOW, bevel=2.5)
    a.cyl(4, 2, at=(13.5, 0, 124), rot=(-90, 0, 0), col=C.CHARCOAL, sides=10)
    a.box((1, 10, 6), at=(13.5, 0, 114), col=C.WHITE, bevel=0.2)


def _hydrants():
    return [r for r in L.select("city", lines=[(164, 164)])]


@asset("SM_City_FireHydrant", F,
       desc="Red fire hydrant with a domed cap, side outlets with chains and bolt flange.",
       replaces=[CITY + "::BuildBoulevard() hydrant cylinder + cap sphere"],
       placements=lambda: [P((r["center"][0], r["center"][1], 4)) for r in _hydrants()],
       covers=lambda: [L.select("city", lines=[(164, 165)], where=lambda q, r=r: abs(q["center"][0] - r["center"][0]) < 1 and abs(q["center"][1] - r["center"][1]) < 1) for r in _hydrants()],
       per_placement=True, pivot="foot on the sidewalk", view=(1, 0.6, 0.4))
def hydrant(a):
    a.cyl(15, 6, at=(0, 0, 3), col=C.RED, sides=12, bevel=2)
    a.cyl(11, 56, at=(0, 0, 32), col=C.RED, sides=12, bevel=2, rough=0.5)
    a.torus(11.5, 2, at=(0, 0, 50), col=C.shade(C.RED, 0.8), major=12, minor=5)
    a.sphere(11, at=(0, 0, 62), rz=9, col=C.RED, segs=12, rings=7, rough=0.5)
    a.cyl(3, 6, at=(0, 0, 72), col=C.shade(C.RED, 0.8), sides=6)
    for s in (-1, 1):
        a.cyl(5, 10, at=(0, s * 14, 38), rot=(0, 0, 90), col=C.RED, sides=10, bevel=1.5)
        a.cyl(6, 3, at=(0, s * 19, 38), rot=(0, 0, 90), col=C.shade(C.RED, 0.8), sides=6)
    a.cyl(6, 12, at=(13, 0, 34), rot=(-90, 0, 0), col=C.RED, sides=10, bevel=1.5)
    for k in range(8):
        ang = math.radians(k * 45)
        a.sphere(1.4, at=(12 * math.cos(ang), 12 * math.sin(ang), 6.5), col=C.GREY, segs=5, rings=3)


def _bins():
    return L.select("city", lines=[(166, 166)])


@asset("SM_City_TrashBin", F,
       desc="Teal city trash bin: ribbed drum on feet with a grey dome lid, drop flap and a sticker.",
       replaces=[CITY + "::BuildBoulevard() bin cylinder + lid"],
       placements=lambda: [P((r["center"][0], r["center"][1], 4)) for r in _bins()],
       covers=lambda: [L.select("city", lines=[(166, 167)], where=lambda q, r=r: abs(q["center"][0] - r["center"][0]) < 1 and abs(q["center"][1] - r["center"][1]) < 1) for r in _bins()],
       per_placement=True, pivot="foot on the sidewalk", view=(1, 0.6, 0.4))
def trash_bin(a):
    a.cyl(20, 74, at=(0, 0, 39), r_top=21, col=C.TEAL_DARK, sides=14, bevel=3, rough=0.6)
    for z in (14, 40, 66):
        a.torus(21, 1.8, at=(0, 0, z), col=C.TEAL, major=14, minor=5)
    a.cyl(22, 6, at=(0, 0, 79), col=C.GREY, sides=14, bevel=2, rough=0.4)
    a.sphere(18, at=(0, 0, 82), rz=7, col=C.GREY, segs=14, rings=6, rough=0.4)
    a.box((4, 20, 10), at=(19, 0, 70), col=C.CHARCOAL, bevel=1.5)
    a.cyl(5, 0.6, at=(20.5, 0, 45), rot=(0, 90, 90), col=C.YELLOW, sides=10, bevel=0)
    for k in range(3):
        ang = math.radians(k * 120)
        a.box((6, 6, 4), at=(16 * math.cos(ang), 16 * math.sin(ang), 1), col=C.CHARCOAL, bevel=1)


def _benches():
    return L.select("city", lines=[(171, 171)])


@asset("SM_City_ParkBench", F,
       desc="Park bench: wooden seat slats and backrest on curly cast-iron frames with armrests.",
       replaces=[CITY + "::BuildBoulevard() benches (seat + back boxes)"],
       placements=lambda: [P((r["center"][0], r["center"][1] - 5, 4)) for r in _benches()],
       covers=lambda: [L.select("city", lines=[(171, 172)], where=lambda q, r=r: abs(q["center"][0] - r["center"][0]) < 1) for r in _benches()],
       per_placement=True, pivot="bench centre on the sidewalk; sitter faces -Y (the road)", view=(0.4, -1, 0.5))
def park_bench(a):
    for k in range(4):
        a.box((160, 9, 4), at=(0, -16 + k * 10, 44), col=C.shade(C.WOOD, a.rng.uniform(0.92, 1.08)), bevel=1.5, segs=1)
    for k in range(3):
        a.box((160, 4, 11), at=(0, 30, 58 + k * 14), rot=(-12, 0, 0), col=C.shade(C.WOOD, a.rng.uniform(0.92, 1.08)), bevel=1.5, segs=1)
    for x in (-70, 70):
        a.tube([(x, -20, 2), (x, -18, 42), (x, 22, 42), (x, 28, 2)], 2.5, col=C.CHARCOAL, sides=6)
        a.tube([(x, 22, 42), (x, 32, 90)], 2.5, col=C.CHARCOAL, sides=6)
        a.tube([(x, -22, 60), (x, -10, 64), (x, 20, 62)], 2.2, col=C.CHARCOAL, sides=6)
        a.torus(6, 1.4, at=(x, -20, 52), rot=(0, 0, 90), col=C.CHARCOAL, major=10, minor=4)
        for y in (-20, 28):
            a.box((8, 8, 3), at=(x, y, 1.5), col=C.CHARCOAL, bevel=1)


@asset("SM_City_Signpost", F,
       desc="Wayfinding signpost at the studio end: post with a finial, coral GRAND CINEMA arrow board and navy DREAM CARS board, both with pointed arrow tips.",
       replaces=[CITY + "::BuildBoulevard() signpost pole and two boards"],
       placements=lambda: [P((-3800, 220, 4))], covers=rec((176, 179)),
       pivot="post foot on the sidewalk; boards face +X", integration="Board texts stay TextRenders (or use the SM_Sign_City_* letters).", view=(1, 0.5, 0.3))
def signpost(a):
    pv = (-3800, 220, 4)
    a.records(L.select("city", lines=[(176, 179)]), pivot=pv, bevel=2)
    a.sphere(9, at=(0, 0, 305), col=C.BRASS, segs=10, rings=6, rough=0.3)
    a.box((14, 30, 8), at=(0, 0, 8), col=C.CHARCOAL, bevel=2)
    # arrow tips: 10 cm thick so their caps sit inside the 12 cm boards (no coplanar faces)
    a.prism((10, 60, 76), at=(0, -150, 300), rot=(0, 0, -90), col=C.CORAL)
    a.prism((10, 40, 46), at=(0, -120, 236), rot=(0, 0, -90), col=C.NAVY)
    a.box((14, 304, 4), at=(0, 0, 337), col=C.CREAM, bevel=1)


def _alley(width):
    walls = [r for r in L.select("city", lines=[(287, 287)]) if abs(r["size"][0] - width) < 1]

    def pl():
        return [P((r["center"][0], r["center"][1], 0), (0, 0 if r["center"][1] > 0 else 180, 0)) for r in walls]

    def cov():
        out = []
        for r in walls:
            out.append(L.select("city", lines=[(287, 290)], where=lambda q, r=r: abs(q["center"][0] - r["center"][0]) < 1 and abs(q["center"][1] - r["center"][1]) < 70))
        return out

    @asset("SM_City_AlleyEnd_%d" % width, F,
           desc="Alley dead end %d cm wide: brick back wall with drainpipe and a caged lamp, teal dumpster with a grey lid, trash bags and a crate." % width,
           replaces=[CITY + "::BuildBlocks() alley back wall + dumpster (alleys %d cm wide)" % width],
           placements=pl, covers=cov, per_placement=True,
           pivot="back-wall centre on the ground; the alley opens towards local -Y (yaw 180 for the south side)", view=(0.3, -1, 0.5))
    def build(a):
        a.box((width, 40, 730), at=(0, 0, 335), col=C.ALLEY_WALL, bevel=1.5)
        for row in range(0, 700, 36):
            off = 0 if (row // 36) % 2 == 0 else 20
            for x in range(-int(width / 2) + off, int(width / 2) - 20, 40):
                if a.rng.random() < 0.55:
                    a.box((38, 2, 16), at=(x + 19, -21, row + 12), col=C.shade(C.ALLEY_WALL, a.rng.uniform(1.1, 1.35)), bevel=0, ao=False)
        a.tube([(width / 2 - 20, -24, 0), (width / 2 - 20, -24, 690), (width / 2 - 30, -34, 710)], 5, col=C.GREY_DARK, sides=8)
        a.box((20, 14, 26), at=(0, -28, 300), col=C.CHARCOAL, bevel=3)
        a.cyl(7, 12, at=(0, -38, 288), col=C.WINDOW_WARM, sides=8, glow=4)
        dw = min(150, width - 30)
        a.box((dw, 80, 110), at=(0, -60, 58), col=C.TEAL_DARK, bevel=5, tap=(1.04, 1.06))
        a.box((dw + 6, 86, 8), at=(0, -60, 124), rot=(4, 0, 0), col=C.GREY_DARK, bevel=2.5)
        for s in (-1, 1):
            a.box((6, 90, 8), at=(s * dw / 2, -60, 70), col=C.GREY_DARK, bevel=2)
            a.cyl(6, 5, at=(s * (dw / 2 - 12), -80, 6), rot=(0, 0, 90), col=C.RUBBER, sides=8)
        for k in range(2):
            b = bm_box(40, 40, 42, 16, 2)
            bulge(b, 5, axis=2)
            a.add(b, xf((-dw / 2 + 20 + k * 40, -118 - k * 12, 21), (0, k * 30, 0)), C.CHARCOAL, rough=0.3)
            a.cyl(4, 8, at=(-dw / 2 + 20 + k * 40, -118 - k * 12, 44), col=C.CHARCOAL, sides=6)
    return build


for _w in (200, 300, 400):
    _alley(_w)


@asset("SM_City_PocketPark", F,
       desc="Pocket park: raised lawn bed with a stone edge, grass tufts, flowers, a gravel path, the hedge back wall and a stone bench.",
       replaces=[CITY + "::BuildBlocks() park lawn, hedge wall, stone bench"],
       placements=lambda: [P((-10100, 750, 0))], covers=rec((314, 314), (318, 319)),
       pivot="lawn centre on the ground (-10100, 750, 0)", integration="Palms and the plant are SM_Prop_PalmTree / SM_Prop_PottedPlant placements; the Blocker stays.", view=(0.6, -1, 0.8))
def pocket_park(a):
    pv = (-10100, 750, 0)
    a.records(L.select("city", lines=[(314, 314), (318, 319)]), pivot=pv, bevel=2)
    for y in (-450, 450):
        a.box((610, 14, 12), at=(0, y, 6), col=C.PLAZA_STONE, bevel=2)
    for x in (-300, 300):
        a.box((14, 910, 12), at=(x, 0, 6), col=C.PLAZA_STONE, bevel=2)
    a.box((90, 900, 1), at=(120, 0, 6.3), col=C.shade(C.SAND, 0.85), bevel=0.3, ao=False)
    rng = a.rng
    for k in range(40):
        x, y = rng.uniform(-280, 280), rng.uniform(-430, 430)
        if 70 < x < 170:
            continue
        a.cone(rng.uniform(5, 9), rng.uniform(8, 16), at=(x, y, 10), col=rng.choice((C.GREEN, C.GREEN_DARK, C.LEAF_LIGHT)), sides=5)
    for k in range(14):
        x, y = rng.uniform(-280, 60), rng.uniform(-430, 430)
        a.sphere(4, at=(x, y, 12), col=rng.choice((C.CORAL, C.YELLOW, C.MAGENTA, C.CREAM)), segs=6, rings=4)
    for k in range(10):
        a.sphere(34, at=(-280 + k * 62, 470, 200 + (k % 2) * 8), ry=18, rz=14, col=C.GREEN_DARK if k % 2 else C.HEDGE_GREEN, segs=10, rings=6)
    a.box((170, 50, 8), at=(-150, -330, 48), col=C.PLAZA_STONE, bevel=3)
    for x in (-210, -90):
        a.box((20, 40, 40), at=(x, -330, 22), col=C.shade(C.PLAZA_STONE, 0.9), bevel=3)


@asset("SM_City_ParkingLot", F,
       desc="Parking lot: asphalt apron with bay lines, wheel stops, oil stains, the blue P sign on its pole and the low perimeter walls with caps.",
       replaces=[CITY + "::BuildBlocks() lot asphalt, bay lines, P sign (FTCity.cpp:328, board turned to face +Y like its text - code finding), walls"],
       placements=lambda: [P((-9900, -1150, 0))], covers=rec((322, 331)),
       pivot="lot centre on the ground (-9900, -1150, 0)", integration="'P' stays a TextRender; Blockers stay.", view=(0.6, 0.8, 0.9))
def parking_lot(a):
    pv = (-9900, -1150, 0)
    a.records(L.select("city", lines=[(322, 331)]), pivot=pv, bevel=2)
    for b in range(4):
        x = -10350 + b * 225 + 112 - pv[0]
        a.box((110, 16, 12), at=(x, -1450 - pv[1], 6), col=C.PLAZA_STONE, bevel=3)
        a.cyl(40, 0.4, at=(x + 20, -1300 - pv[1], 1.1), ry=26, col=C.shade(C.ASPHALT, 0.7), sides=10, bevel=0, ao=False)
    a.box((1010, 50, 10), at=(0, -1720 - pv[1], 125), col=C.shade(C.ALLEY_WALL, 1.3), bevel=3)
    a.box((50, 250, 10), at=(-10420 - pv[0], -1620 - pv[1], 125), col=C.shade(C.ALLEY_WALL, 1.3), bevel=3)
    # white frame behind the blue P board (board turned to face +Y like its text, see ftb.layout.BOARD_FIXES)
    a.box((98, 8, 98), at=(-9500 - pv[0], -1607 - pv[1], 380), col=C.WHITE, bevel=3)


@asset("SM_City_RooftopBillboard", F,
       desc="Rooftop billboard 'TONIGHT AT THE GRAND': navy frame, coral poster face, catwalk with railing, lattice legs and three gooseneck floodlights.",
       replaces=[CITY + "::BuildBlocks() rooftop billboard frame + face"],
       placements=lambda: [P((-6100, 1300, 1600))], covers=rec((337, 338)),
       pivot="billboard foot centre on the DINER roof (-6100, 1300, 1600); face towards +X", integration="Texts stay TextRenders.", view=(1, 0.4, 0.3))
def billboard(a):
    pv = (-6100, 1300, 1600)
    a.records(L.select("city", lines=[(337, 338)]), pivot=pv, bevel=3)
    a.box((6, 870, 16), at=(14, 0, 510), col=C.YELLOW, bevel=2)
    for y in (-380, -130, 130, 380):
        a.box((10, 10, 90), at=(-6, y, 45), col=C.GREY_DARK, bevel=2)
        a.tube([(-6, y - 50, 0), (-6, y + 50, 90)], 2, col=C.GREY_DARK, sides=5)
    a.box((60, 900, 6), at=(30, 0, 88), col=C.GREY, bevel=1.5)
    a.box((4, 900, 4), at=(58, 0, 120), col=C.GREY, bevel=1)
    for y in range(-440, 441, 110):
        a.box((4, 4, 32), at=(58, y, 104), col=C.GREY, bevel=0.8)
    for y in (-300, 0, 300):
        a.tube([(20, y, 96), (70, y, 140), (80, y, 170)], 2.2, col=C.CHARCOAL, sides=6)
        a.cone(12, 16, at=(80, y, 176), rot=(140, 0, 0), col=C.CHARCOAL, sides=8)
        a.cyl(8, 2, at=(76, y, 182), rot=(140, 0, 0), col=C.CREAM, sides=8, glow=8)
