"""Stage 4 water tank beach set: basin, island, ramp, dock, painted backdrop, beach dressing."""
import math

from mathutils import Vector

from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_sphere, xf
from ftb.registry import P, asset

F = "Studio/TankSet"
TANK = "AFTStudioShell::BuildTankSet()"


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


@asset("SM_Tank_Basin", F,
       desc="Tank A basin: blue walls with a white rounded coping, tiled waterline band, depth stripes, chrome pool ladder and drain grates.",
       replaces=[TANK + " tank walls, coping and outside stripes"],
       placements=lambda: [P((1800, 100, -120))], covers=rec((856, 866)),
       pivot="world-aligned, pivot (1800, 100, -120) = tank centre on the stage floor; coping top at +106",
       integration="TankWater (WaterGrid component) and the wall collision stay as they are; 'TANK A - NO DIVING' stays a TextRender.",
       view=(-0.8, 0.5, 0.8))
def basin(a):
    pv = (1800, 100, -120)
    a.records(L.select("studio", lines=[(856, 866)]), pivot=pv, bevel=1.5)
    # coping nosing
    for (at, size) in (((1320, 100), (58, 2016)), ((2280, 100), (58, 2016)), ((1800, -880), (1004, 58)), ((1800, 1080), (1004, 58))):
        a.box((size[0], size[1], 5), at=(at[0] - pv[0], at[1] - pv[1], 106), col=C.WHITE, bevel=2, rough=0.5)
    # waterline tiles on the inner faces (x 1340 / 2260, y -860 / 1060)
    for y in range(-840, 1060, 40):
        for x, s in ((1340, 1), (2260, -1)):
            a.box((2, 36, 14), at=(x - pv[0] + s * 1, y + 20 - pv[1], 72), col=C.SKY_BLUE if (y // 40) % 2 else C.WHITE, bevel=0.5, segs=1)
    for x in range(1360, 2260, 40):
        for y, s in ((-860, 1), (1060, -1)):
            if 1860 <= x <= 2260 and y == -860:
                continue  # island sits here
            a.box((36, 2, 14), at=(x + 20 - pv[0], y - pv[1] + s * 1, 72), col=C.SKY_BLUE if (x // 40) % 2 else C.WHITE, bevel=0.5, segs=1)
    # pool ladder on the west wall inside (near the lunge mark)
    for y in (220, 280):
        a.tube([(1300 - pv[0], y - pv[1], 108), (1330 - pv[0], y - pv[1], 128), (1352 - pv[0], y - pv[1], 110), (1352 - pv[0], y - pv[1], 20)], 2.6, col=C.CHROME, sides=8, rough=0.2)
    for z in (40, 70, 95):
        a.box((6, 64, 3), at=(1352 - pv[0], 250 - pv[1], z), col=C.CHROME, bevel=1, rough=0.2)
    for (x, y) in ((1500, -600), (2100, 900)):
        a.box((40, 40, 1), at=(x - pv[0], y - pv[1], 0.6), col=C.GREY, bevel=0.5)
        for k in range(4):
            a.box((34, 3, 1.2), at=(x - pv[0], y - pv[1] - 12 + k * 8, 1.2), col=C.GREY_DARK, bevel=0)


@asset("SM_Tank_Island", F,
       desc="Sand island: lumpy sand mass, the long ramp-slope down into the water, rounded sand dune, a continuous shoreline apron, shells, a starfish, a sand castle with a flag, bucket and spade, seaweed and a driftwood log.",
       replaces=[TANK + " island block, sand ramp, dune, shoreline apron"],
       placements=lambda: [P((2060, -420, -10))], covers=rec((870, 874)),
       pivot="island top centre (2060, -420, -10); the sand ramp slopes down to -X",
       integration="Collision stays with the code island CubeSolid + RampSolid (the shoreline was visual-only already).", view=(-1, 0.4, 0.7))
def island(a):
    pv = (2060, -420, -10)
    a.records(L.select("studio", lines=[(870, 874)]), pivot=pv, bevel=4, jitter=0)
    rng = a.rng
    # lumps on top so the sand reads soft
    for k in range(9):
        b = bm_sphere(rng.uniform(40, 80), rng.uniform(30, 60), rng.uniform(4, 9), 10, 5)
        a.add(b, xf((rng.uniform(-170, 170), rng.uniform(-420, 400), 0), (0, rng.uniform(0, 180), 0)), C.shade(C.SAND, rng.uniform(0.95, 1.05)), rough=1.0)
    # shells + starfish
    for k in range(8):
        x, y = rng.uniform(-180, 170), rng.uniform(-420, 420)
        a.sphere(4, at=(x, y, 1.5), col=rng.choice((C.CREAM, C.CORAL, C.WHITE)), segs=6, rings=4, rz=2)
    star = []
    for k in range(10):
        ang = math.radians(90 + k * 36)
        rr = 14 if k % 2 == 0 else 6
        star.append((math.cos(ang) * rr, math.sin(ang) * rr))
    a.slab(star, 2.5, at=(-120, 230, 0.5), col=C.CORAL, bevel=1)
    # sand castle
    cx, cy = 90, 220
    a.box((56, 56, 26), at=(cx, cy, 13), col=C.shade(C.SAND, 0.95), bevel=3)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(10, 40, at=(cx + sx * 26, cy + sy * 26, 20), r_top=8, col=C.shade(C.SAND, 0.92), sides=8, bevel=2)
            a.cone(10, 10, at=(cx + sx * 26, cy + sy * 26, 45), col=C.shade(C.SAND, 0.9), sides=8)
    a.cyl(1, 30, at=(cx, cy, 40), col=C.WOOD_DARK, sides=5)
    a.prism((1, 14, 10), at=(cx, cy + 7, 50), rot=(0, 0, 90), col=C.RED)
    # bucket and spade
    a.cyl(10, 16, at=(40, 300, 8), r_top=12, col=C.YELLOW, sides=12, bevel=1)
    a.torus(10, 0.8, at=(40, 300, 18), rot=(90, 0, 0), col=C.BLUE, major=12, minor=4)
    a.box((6, 16, 1.5), at=(65, 290, 2), rot=(0, 30, 20), col=C.BLUE, bevel=0.5)
    a.cyl(1.2, 30, at=(72, 305, 10), rot=(60, 30, 0), col=C.BLUE, sides=6)
    # seaweed + driftwood where the sand meets the water
    for k in range(6):
        y = -380 + k * 150
        a.leaf(30, 8, at=(-205, y, -30), rot=(60, rng.uniform(-180, 180), 0), col=C.GREEN_DARK, droop=0.3, thick=1, segs=5)
    a.cyl(8, 120, at=(-120, -250, 3), rot=(90, 25, 0), r_top=6, col=C.shade(C.WOOD, 0.8), sides=8, bevel=2)
    a.cyl(3, 30, at=(-110, -225, 12), rot=(40, 60, 0), col=C.shade(C.WOOD, 0.8), sides=6)


@asset("SM_Prop_BeachTowel", F,
       desc="Rumpled coral beach towel with a white stripe, fringed ends, flip-flops and star sunglasses.",
       replaces=[TANK + " beach towel + stripe"], placements=lambda: [P((1960, -300, -10))], covers=rec((881, 882)),
       pivot="towel centre on the sand (1960, -300, -10)", view=(0.5, 0.5, 1))
def beach_towel(a):
    from ftb.core import bm_box
    b = bm_box(160, 90, 1.4, 0.4, 1)
    a.add(b, xf((0, 0, 0.7)), C.CORAL, rough=1.0, jitter=0.6)
    a.box((160, 20, 0.6), at=(0, 0, 1.6), col=C.WHITE, bevel=0.2, ao=False)
    for x in (-82, 82):
        for k in range(9):
            a.box((5, 2, 0.6), at=(x, -40 + k * 10, 0.6), col=C.CREAM, bevel=0.1, ao=False)
    for k, y in enumerate((-18, 4)):
        a.box((26, 10, 1.5), at=(-50 + k * 6, y, 2.2), rot=(0, 10 - k * 20, 0), col=C.YELLOW, bevel=0.6)
    for s in (-1, 1):
        a.cyl(4.5, 1.2, at=(40, s * 5, 2.4), rot=(0, 0, 0), col=C.INK, sides=10)
    a.box((2, 18, 1), at=(40, 0, 2.6), col=C.MAGENTA, bevel=0.3)


@asset("SM_Prop_BeachUmbrella", F,
       desc="Little magenta/white striped beach umbrella with a scalloped rim and a ball finial.",
       replaces=[TANK + " umbrella pole + cone"], placements=lambda: [P((1900, -150, -10))], covers=rec((883, 884)),
       pivot="pole foot on the sand (1900, -150, -10)", view=(1, 0.6, 0.5))
def beach_umbrella(a):
    a.cyl(2, 70, at=(0, 0, 35), col=C.CREAM, sides=8)
    from ftb.core import bm_lathe
    for k in range(8):
        bm = bm_lathe([(45, 55), (30, 64), (12, 71), (0.01, 74)], sides=3, arc=45)
        a.add(bm, xf((0, 0, 0), (0, k * 45, 0)), C.MAGENTA if k % 2 else C.WHITE, rough=0.7)
    for k in range(16):
        ang = math.radians(k * 22.5 + 11)
        a.sphere(4, at=(math.cos(ang) * 44, math.sin(ang) * 44, 54), col=C.MAGENTA if (k // 2) % 2 else C.WHITE, segs=6, rings=4, rz=2.5)
    a.sphere(3, at=(0, 0, 76), col=C.YELLOW, segs=8, rings=5)


@asset("SM_Tank_IslandRamp", F,
       desc="Wooden plank ramp from the stage floor up over the west tank wall onto the island, with cleats, side stringers and a rope rail on stanchions.",
       replaces=[TANK + " ramp wedge + slats"], placements=lambda: [P((1970, -1125, -120))], covers=rec((886, 890)),
       pivot="ramp footprint centre on the stage floor (1970, -1125, -120); rises towards +Y (island)",
       integration="Ramp collision stays with the code RampSolid.", view=(-1, -0.6, 0.6))
def island_ramp(a):
    pv = (1970, -1125, -120)
    a.records(L.select("studio", lines=[(886, 890)]), pivot=pv, bevel=2)
    # stringers
    for x in (-82, 82):
        a.poly([(-225, 0), (225, 0), (225, 110), (205, 110)], 8, at=(x, 0, 0), col=C.WOOD_DARK, bevel=1.5)
    # extra cleats between the code slats
    for i in range(12):
        t = (i + 0.5) / 12
        y = -225 + 450 * t
        a.box((150, 5, 2.5), at=(0, y, 110 * t + 1.5), rot=(0, 0, 0), col=C.shade(C.WOOD, 1.08), bevel=0.8, segs=1)
    for x in (-86, 86):
        pts = []
        for k in range(4):
            t = k / 3
            y = -210 + 420 * t
            z = 110 * (y + 225) / 450
            a.cyl(3, 70, at=(x, y, z + 35), col=C.WOOD_DARK, sides=8, bevel=1)
            a.sphere(4, at=(x, y, z + 72), col=C.WOOD_DARK, segs=6, rings=4)
            pts.append((x, y, z + 62))
        sag = []
        for i in range(len(pts) - 1):
            p0, p1 = Vector(pts[i]), Vector(pts[i + 1])
            for s in range(5):
                q = p0.lerp(p1, s / 5)
                q.z -= 6 * math.sin(math.pi * s / 5)
                sag.append(tuple(q))
        sag.append(pts[-1])
        a.tube(sag, 1.4, col=C.CREAM_DARK, sides=6)


@asset("SM_Tank_Dock", F,
       desc="Wooden dock inside the tank: individual planks, four posts with tyre fenders, mooring cleats, a rope coil, the taped HERO MARK cross and the plank ramp up from the east.",
       replaces=[TANK + " dock deck, planks, posts, east ramp, hero mark tape"],
       placements=lambda: [P((1825, 890, -30))], covers=rec((892, 903)),
       pivot="dock deck top centre (1825, 890, -30)",
       integration="Collision stays with the code dock CubeSolid, posts and RampSolid; 'HERO MARK' stays a TextRender.", view=(-0.8, -0.8, 0.8))
def dock(a):
    pv = (1825, 890, -30)
    a.records(L.select("studio", lines=[(892, 903)]), pivot=pv, bevel=2)
    rng = a.rng
    for i, x in enumerate(range(1502, 2150, 30)):
        a.box((27, 338, 3), at=(x + 14 - pv[0], 0, 0.8), col=C.shade(C.WOOD, rng.uniform(0.9, 1.1)), bevel=0.8, segs=1)
    for (x, y) in ((1510, 730), (1510, 1050), (1830, 730), (2140, 730)):
        a.torus(13, 5, at=(x - pv[0], y - pv[1], -18), col=C.RUBBER, major=14, minor=6, rough=0.8)
        a.cyl(12, 4, at=(x - pv[0], y - pv[1], 32), col=C.WOOD_DARK, sides=10, bevel=2)
    for x in (1600, 2000):
        a.box((24, 8, 5), at=(x - pv[0], 730 - pv[1] + 14, 3), col=C.GREY_DARK, bevel=1.5)
        a.box((8, 8, 6), at=(x - pv[0], 730 - pv[1] + 14, 1), col=C.GREY_DARK, bevel=1.5)
    a.torus(16, 3, at=(2080 - pv[0], 1000 - pv[1], 3), col=C.CREAM_DARK, major=16, minor=6)
    a.torus(13, 3, at=(2080 - pv[0], 1000 - pv[1], 7), col=C.CREAM_DARK, major=16, minor=6)
    # east ramp planks
    for i in range(10):
        t = (i + 0.5) / 10
        y = 1100 + 400 * t - pv[1]
        a.box((214, 30, 3), at=(1825 - pv[0], y, (-120 + 90 * (1 - t)) - pv[2] + 1.5), col=C.shade(C.WOOD, rng.uniform(0.9, 1.1)), bevel=0.8, segs=1)


@asset("SM_Tank_Backdrop", F,
       desc="Painted sky flat behind the tank: layered cut-out clouds, a sun disc with rays, distant teal islands, wave strips, gulls and the coral SHARK! banner, all in a wooden frame.",
       replaces=[TANK + " sky backdrop, sea band, wave strips, clouds, sun, islands, SHARK! banner"],
       placements=lambda: [P((2390, 200, -120))], covers=rec((906, 926)),
       pivot="backdrop foot centre (2390, 200, -120); painted side faces -X",
       integration="'SHARK!' stays a TextRender; the AFTCinemaScreen rolls down in front of this flat.", view=(-1, 0.3, 0.2))
def backdrop(a):
    pv = (2390, 200, -120)
    a.records(L.select("studio", lines=[(906, 926)]), pivot=pv, bevel=1.0)
    X = -6
    for (y, z, w, h) in ((-1100, 580, 260, 90), (-300, 660, 260, 90), (700, 550, 260, 90), (1500, 640, 260, 90)):
        for (dy, dz, sw, sh) in ((0, 0, 1, 1), (90, 30, 0.65, 1.1), (-80, 20, 0.58, 0.9), (30, 45, 0.5, 0.9)):
            a.cyl(sw * w / 2, 3, at=(X - 5, y + dy - pv[1], z + dz), rot=(0, 90, 90), ry=sh * h / 2, col=C.WHITE, sides=18, bevel=0.8, glow=0.4)
    for k in range(12):
        ang = math.radians(k * 30)
        a.box((3, 18, 70), at=(X - 3, 1100 - pv[1] + math.cos(ang) * 125, 420 + math.sin(ang) * 125), rot=(0, 0, -k * 30 + 90), col=C.YELLOW, bevel=1, glow=1.0)
    for (y, z) in ((-600, 520), (-500, 560), (300, 600), (400, 570)):
        for s in (-1, 1):
            a.box((2, 22, 3), at=(X - 4, y - pv[1] + s * 9, z + 4), rot=(0, 0, s * 25), col=C.NAVY, bevel=0.5)
    for (y, z) in ((-700, 170), (1500, 160)):
        a.poly([(-300, -60), (-160, 20), (-40, 50), (60, 30), (200, -10), (300, -60)], 3, at=(X - 8, y - pv[1], z), col=C.TEAL, bevel=1)
        a.cyl(4, 90, at=(X - 10, y - pv[1] + 40, z + 80), rot=(0, 0, 8), col=C.WOOD, sides=6)
        for th in (15, 55, 125, 165):
            a.leaf(50, 14, at=(X - 12, y - pv[1] + 48, z + 124), rot=(th, 90, 0), col=C.GREEN_DARK, droop=0.4)
    # wooden frame
    a.box((16, 3580, 14), at=(X + 2, 0, 767), col=C.WOOD_DARK, bevel=3)
    for y in (-1780, 1780):
        a.box((16, 14, 770), at=(X + 2, y, 385), col=C.WOOD_DARK, bevel=3)
