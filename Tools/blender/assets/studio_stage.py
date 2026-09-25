"""Stage 4: floor, walls, ceiling, entry wall, landings, camera deck, sound booth, truss, work lights, signs, warehouse."""
import math

from ftb import detail as D
from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_box, bulge, xf
from ftb.registry import P, asset
from assets.studio_lobby import exit_sign, extinguisher, outlet, sprinkler, vent

F = "Studio/Stage4"
FWH = "Studio/Warehouse"
STAGE = "AFTStudioShell::BuildStage()"
WH = "AFTStudioShell::BuildWarehouse()"
PV = (900, 200, -120)


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


def hazard(a, length, at, rot=(0, 0, 0), h=8, depth=1.2, n=None):
    """Yellow/black hazard stripe band along local Y."""
    n = n or max(2, int(length / 20))
    a.box((depth, length, h), at=at, rot=rot, col=C.YELLOW, bevel=0.4)
    from ftb.core import ue_rot
    from mathutils import Vector
    R = ue_rot(*rot)
    for k in range(n):
        if k % 2:
            continue
        off = R @ Vector((depth * 0.5 + 0.2, -length / 2 + (k + 0.5) * length / n, 0))
        a.box((0.6, length / n * 0.7, h), at=(at[0] + off.x, at[1] + off.y, at[2] + off.z), rot=(rot[0], rot[1], rot[2] + 35), col=C.INK, bevel=0)


# ============================================================================== floor

@asset("SM_Stage4_Floor", F,
       desc="Stage 4 floor: grey concrete slab with saw-cut joints, the yellow safe-walkway tape runs, actor T-marks and a few paint scuffs.",
       replaces=[STAGE + " stage floor slab and walkway tape"],
       placements=lambda: [P(PV)], covers=rec((712, 712), (781, 783)),
       pivot="world-aligned, pivot (900, 200, -120) = stage floor top", view=(0.4, 0.3, 1))
def stage_floor(a):
    a.records(L.select("studio", lines=[(712, 712), (781, 783)]), pivot=PV, bevel=1.0)
    for x in range(-500, 2400, 500):
        a.box((2.5, 3590, 0.4), at=(x - PV[0], 0, 0.1), col=C.shade(C.STAGE_FLOOR, 0.78), bevel=0, ao=False)
    for y in range(-1500, 2000, 500):
        a.box((2990, 2.5, 0.4), at=(0, y - PV[1], 0.1), col=C.shade(C.STAGE_FLOOR, 0.78), bevel=0, ao=False)
    for (x, y, c) in ((820, -300, C.CORAL), (1000, 300, C.CYAN), (700, 600, C.MAGENTA), (1180, -200, C.YELLOW)):
        a.box((30, 5, 0.5), at=(x - PV[0], y - PV[1], 0.4), col=c, bevel=0, glow=0.3, ao=False)
        a.box((5, 22, 0.5), at=(x - PV[0] + 12, y - PV[1], 0.4), col=c, bevel=0, glow=0.3, ao=False)
    rng = a.rng
    for k in range(10):
        a.cyl(rng.uniform(6, 16), 0.3, at=(rng.uniform(-500, 1250) - PV[0], rng.uniform(-1500, 1900) - PV[1], 0.15),
              col=C.shade(rng.choice((C.CREAM, C.TEAL, C.CORAL)), 0.7), sides=8, bevel=0, ao=False)
    # detail pass: a cable run with a yellow cable ramp from the sound booth to the camera deck, chalk marks,
    # gaffer-tape spike marks, anchor plates in the slab, worn paint along the walkway tape
    D.cable(a, [(260 - PV[0], -1080 - PV[1], 1.2), (300 - PV[0], -700 - PV[1], 1.2), (310 - PV[0], -300 - PV[1], 1.2), (300 - PV[0], -160 - PV[1], 1.2)], r=1.3)
    D.cable(a, [(266 - PV[0], -1080 - PV[1], 1.2), (306 - PV[0], -700 - PV[1], 1.2), (316 - PV[0], -300 - PV[1], 1.2), (306 - PV[0], -160 - PV[1], 1.2)], r=1.0, col=C.shade(C.RUBBER, 1.5))
    a.box((60, 90, 5), at=(306 - PV[0], -600 - PV[1], 2.5), rot=(0, 5, 0), col=C.YELLOW, bevel=2)
    for k in range(3):
        a.box((58, 8, 0.6), at=(306 - PV[0], -630 - PV[1] + k * 30, 5.2), rot=(0, 5, 0), col=C.INK, bevel=0, jitter=0, wear=False)
    for (x, y) in ((1300, 900), (600, -900), (1500, -300)):
        for s in (-1, 1):
            a.box((40, 2, 0.3), at=(x - PV[0], y - PV[1], 0.35), rot=(0, s * 45, 0), col=C.WHITE, bevel=0, jitter=0, ao=False)
    for (x, y) in ((-300, 1400), (2200, -1300), (2200, 1500), (-300, -1300)):
        a.box((30, 30, 1), at=(x - PV[0], y - PV[1], 0.5), col=C.GREY, bevel=0.3, rough=0.4)
        D.screws_rect(a, (x - PV[0], y - PV[1], 1), "+z", 30, 30, inset=4, r=1.4, col=C.GREY_DARK)
    rng = a.rng
    for k in range(12):
        a.box((rng.uniform(10, 30), rng.uniform(1, 3), 0.3), at=(rng.uniform(-500, 2300) - PV[0], rng.uniform(-1500, 1900) - PV[1], 0.3),
              rot=(0, rng.uniform(0, 180), 0), col=C.shade(C.STAGE_FLOOR, 0.7), bevel=0, jitter=0, ao=False)


# ============================================================================== walls + ceiling

@asset("SM_Stage4_Walls", F,
       desc="Soundstage walls: navy acoustic walls with a teal wainscot and tape line, steel I-beam columns, a ring girder, quilted sound panels up high, conduit runs with junction boxes.",
       replaces=[STAGE + " north/south/east stage walls, teal lower walls and tape stripes"],
       placements=lambda: [P(PV)], covers=rec((713, 715), (717, 721)),
       pivot="world-aligned, pivot (900, 200, -120)",
       integration="Wall collision stays with the code CubeSolids (or use this mesh as complex collision).", view=(-0.6, 0.3, 0.5))
def stage_walls(a):
    a.records(L.select("studio", lines=[(713, 715), (717, 721)]), pivot=PV, bevel=1.5)
    Z = lambda z: z - PV[2]
    # (face coordinate, axis, inward sign, span)
    walls = [(-1600, "y", 1, (-600, 2400)), (2000, "y", -1, (-600, 2400)), (2400, "x", -1, (-1600, 2000))]
    for (pos, ax, s, (s0, s1)) in walls:
        cols = list(range(s0 + 250, s1, 500))
        for c in cols:
            at = (c - PV[0], pos - PV[1] + s * 9, Z(600)) if ax == "y" else (pos - PV[0] + s * 9, c - PV[1], Z(600))
            size = (30, 14, 1440) if ax == "y" else (14, 30, 1440)
            a.box(size, at=at, col=C.GREY_DARK, bevel=2)
            web = (10, 20, 1440) if ax == "y" else (20, 10, 1440)
            at2 = (at[0], at[1] + s * 10, at[2]) if ax == "y" else (at[0] + s * 10, at[1], at[2])
            a.box(web, at=at2, col=C.shade(C.GREY_DARK, 0.85), bevel=1)
        # ring girder
        g_at = ((s0 + s1) / 2 - PV[0], pos - PV[1] + s * 14, Z(1180)) if ax == "y" else (pos - PV[0] + s * 14, (s0 + s1) / 2 - PV[1], Z(1180))
        g_size = (s1 - s0, 24, 40) if ax == "y" else (24, s1 - s0, 40)
        a.box(g_size, at=g_at, col=C.GREY_DARK, bevel=3)
        # quilted panels between girder and ceiling-high band
        for c in range(s0 + 20, s1 - 200, 250):
            cx = c + 115
            p = bm_box(220 if ax == "y" else 12, 12 if ax == "y" else 220, 200, 5, 1)
            bulge(p, 4, axis=1 if ax == "y" else 0)
            at = (cx - PV[0], pos - PV[1] + s * 7, Z(1000)) if ax == "y" else (pos - PV[0] + s * 7, cx - PV[1], Z(1000))
            a.add(p, xf(at), C.NAVY_LIGHT, rough=0.95)
        # conduit + junction boxes above the wainscot
        if ax == "y":
            a.cyl(3, s1 - s0, at=((s0 + s1) / 2 - PV[0], pos - PV[1] + s * 6, Z(95)), rot=(90, 0, 0), col=C.GREY, sides=6, rough=0.4)
            for c in range(s0 + 400, s1, 800):
                a.box((22, 8, 22), at=(c - PV[0], pos - PV[1] + s * 6, Z(95)), col=C.GREY, bevel=2, rough=0.4)
        else:
            a.cyl(3, s1 - s0, at=(pos - PV[0] + s * 6, (s0 + s1) / 2 - PV[1], Z(95)), rot=(0, 0, 90), col=C.GREY, sides=6, rough=0.4)
    # exit sign above the ladder side
    a.box((8, 70, 28), at=(2396 - PV[0] - 8, -1400 - PV[1], Z(420)), col=C.GREEN, bevel=2, glow=2)
    # detail pass: yellow padded guards with hazard stripes around every column foot, base plates with anchor
    # bolts, stiffener plates, outlets on the conduit, extinguishers and NO SMOKING plates, cable hooks with coils
    for (pos, ax, s, (s0, s1)) in walls:
        n = ("+y" if s > 0 else "-y") if ax == "y" else ("+x" if s > 0 else "-x")
        for i, c in enumerate(range(s0 + 250, s1, 500)):
            if ax == "y":
                gc = (c - PV[0], pos - PV[1] + s * 17, 55)
                a.box((40, 34, 110), at=gc, col=C.YELLOW, bevel=4, rough=0.8)
                D.hazard(a, (gc[0], gc[1] + s * 17, 55), n, 36, 96, stripes=5, t=0.6)
                a.box((50, 40, 3), at=(c - PV[0], pos - PV[1] + s * 20, 1.5), col=C.GREY_DARK, bevel=0.8)
                for dx in (-20, 20):
                    D.screw(a, (c - PV[0] + dx, pos - PV[1] + s * 36, 3), "+z", r=1.6, col=C.GREY)
                for z in (Z(400), Z(800)):
                    a.box((32, 4, 12), at=(c - PV[0], pos - PV[1] + s * 17, z), col=C.shade(C.GREY_DARK, 1.2), bevel=1)
            else:
                gc = (pos - PV[0] + s * 17, c - PV[1], 55)
                a.box((34, 40, 110), at=gc, col=C.YELLOW, bevel=4, rough=0.8)
                D.hazard(a, (gc[0] + s * 17, gc[1], 55), n, 36, 96, stripes=5, t=0.6)
                a.box((40, 50, 3), at=(pos - PV[0] + s * 20, c - PV[1], 1.5), col=C.GREY_DARK, bevel=0.8)
                for dy in (-20, 20):
                    D.screw(a, (pos - PV[0] + s * 36, c - PV[1] + dy, 3), "+z", r=1.6, col=C.GREY)
                for z in (Z(400), Z(800)):
                    a.box((4, 32, 12), at=(pos - PV[0] + s * 17, c - PV[1], z), col=C.shade(C.GREY_DARK, 1.2), bevel=1)
            if i % 2 == 0:
                oc = (c + 120 - PV[0], pos - PV[1] + s * 1, Z(95)) if ax == "y" else (pos - PV[0] + s * 1, c + 120 - PV[1], Z(95))
                outlet(a, D.on(oc, n, 0, -24), n, col=C.GREY)
                hc = (c - 120 - PV[0], pos - PV[1] + s * 1, Z(170)) if ax == "y" else (pos - PV[0] + s * 1, c - 120 - PV[1], Z(170))
                a.box(D._size(n, 4, 4, 14), at=D.on(hc, n, 0, 0, 7), col=C.GREY_DARK, bevel=1)
                for k in range(3):
                    a.torus(16 - k * 1.2, 1.3, at=D.on(hc, n, k * 1.5, -18 - k * 1.5, 10), rot=D.NORMAL_ROT[n],
                            col=C.RUBBER, major=16, minor=4)
            if i == 1:
                ec = (c + 60 - PV[0], pos - PV[1] + s * 1, Z(95)) if ax == "y" else (pos - PV[0] + s * 1, c + 60 - PV[1], Z(95))
                extinguisher(a, ec, n)
                sc = (c - 60 - PV[0], pos - PV[1] + s * 1, Z(250)) if ax == "y" else (pos - PV[0] + s * 1, c - 60 - PV[1], Z(250))
                D.plate(a, sc, n, 40, 26, t=0.8, col=C.WHITE, screws=True)
                a.torus(9, 1.4, at=D.on(sc, n, dn=1.2), rot=D.NORMAL_ROT[n], col=C.RED, major=16, minor=4)
                a.box(D._size(n, 22, 2, 0.4), at=D.on(sc, n, dn=1.4), rot=D._rot_in_plane(n, 45), col=C.RED, bevel=0, jitter=0, wear=False)


@asset("SM_Stage4_Ceiling", F,
       desc="Stage ceiling slab with deep steel rafters.",
       replaces=[STAGE + " stage ceiling slab"], placements=lambda: [P(PV)], covers=rec((716, 716)),
       pivot="world-aligned, pivot (900, 200, -120); underside at Z 1320", view=(0.3, 0.4, -0.8))
def stage_ceiling(a):
    a.records(L.select("studio", lines=[(716, 716)]), pivot=PV, bevel=2)
    for x in range(-400, 2400, 400):
        a.box((20, 3600, 50), at=(x - PV[0], 0, 1320 - PV[2] - 25), col=C.GREY_DARK, bevel=3)
    for y in (-900, 200, 1300):
        a.box((3000, 16, 26), at=(0, y - PV[1], 1320 - PV[2] - 13), col=C.shade(C.GREY_DARK, 0.8), bevel=2)
    # red sprinkler mains along three rafters with heads, a cable tray across the rafters
    for x in (0, 800, 1600):
        a.cyl(4, 3500, at=(x - PV[0] + 16, 0, 1320 - PV[2] - 44), rot=(0, 0, 90), col=C.RED, sides=8, rough=0.5)
        for y in range(-1500, 1901, 400):
            a.cyl(1.2, 8, at=(x - PV[0] + 16, y - PV[1], 1320 - PV[2] - 51), col=C.CHROME, sides=6)
            a.cyl(2.4, 1, at=(x - PV[0] + 16, y - PV[1], 1320 - PV[2] - 55.5), col=C.CHROME, sides=8, bevel=0.2)
        for y in range(-1400, 1901, 700):
            a.box((6, 4, 14), at=(x - PV[0] + 16, y - PV[1], 1320 - PV[2] - 38), col=C.GREY_DARK, bevel=0.5)
    for sy in (-1, 1):
        a.box((2900, 2, 10), at=(0, 1000 - PV[1] + sy * 15, 1320 - PV[2] - 60), col=C.GREY, bevel=0.4, rough=0.4)
    a.box((2900, 32, 1.5), at=(0, 1000 - PV[1], 1320 - PV[2] - 65), col=C.GREY, bevel=0.3, rough=0.4)
    for k in range(3):
        a.cyl(1.8 + k * 0.3, 2900, at=(0, 1000 - PV[1] - 8 + k * 8, 1320 - PV[2] - 61), rot=(90, 0, 0), col=[C.RUBBER, C.GREY_DARK, C.CORAL_DARK][k], sides=6)


# ============================================================================== entry wall

@asset("SM_Stage4_EntryWall", F,
       desc="The wall between front-of-house and Stage 4: stage-side steel door frames, big-door track, office window sill, and the finished lobby/office/wardrobe faces with skirting, chair rail and crown moulding.",
       replaces=[STAGE + " south wall pieces around the Stage 4 door, wardrobe door and office window, office glass, front-of-house cladding + wainscots"],
       placements=lambda: [P((-600, 0, 0))], covers=rec((722, 759)),
       pivot="world-aligned, pivot (-600, 0, 0) = Stage 4 door centre at lobby floor level; stage side is +X",
       integration="The AFTDoor actors (Stage 4, wardrobe) keep their own panels/frames.", view=(1, 0.3, 0.4))
def entry_wall(a):
    pv = (-600, 0, 0)
    a.records(L.select("studio", lines=[(722, 759)]), pivot=pv, bevel=1.5)
    sx = 21  # stage face (x = -580)
    # steel frames on the stage side
    for (y0, y1, z0, z1) in ((-300, 300, 0, 400), (-1450, -1200, 0, 300)):
        for y in (y0, y1):
            a.box((10, 22, z1 - z0 + 30), at=(sx + 4, y + (-11 if y == y0 else 11), (z0 + z1) / 2 + 15), col=C.YELLOW, bevel=2)
        a.box((10, y1 - y0 + 44, 22), at=(sx + 4, (y0 + y1) / 2, z1 + 11), col=C.YELLOW, bevel=2)
        hazard(a, y1 - y0 + 44, (sx + 10, (y0 + y1) / 2, z1 + 11), h=10)
    a.box((14, 1300, 16), at=(sx + 8, 300, 440), col=C.GREY, bevel=3, rough=0.4)
    for y in range(-280, 900, 120):
        a.box((10, 10, 30), at=(sx + 6, y, 430), col=C.GREY_DARK, bevel=1.5)
    # office window: sill on both sides
    a.box((34, 610, 10), at=(0, 1500, 96), col=C.CREAM, bevel=2)
    # lobby side trims (x = -620 face -> -X)
    lx = -23
    for (y0, y1) in ((-980, -300), (300, 980)):
        a.box((3, y1 - y0, 12), at=(lx - 1.5, (y0 + y1) / 2, 6), col=C.TEAL_DARK, bevel=1)
        a.box((4, y1 - y0, 7), at=(lx - 3, (y0 + y1) / 2, 121), col=C.CREAM, bevel=1)
        a.box((5, y1 - y0, 16), at=(lx - 3, (y0 + y1) / 2, 548), col=C.CREAM, bevel=1.5)
        for y in range(int(y0) + 20, int(y1) - 100, 120):
            a.box((2, 100, 80), at=(lx - 5, y + 50, 60), col=C.shade(C.TEAL, 1.12), bevel=1.2, segs=1)
    a.box((5, 600, 16), at=(lx - 3, 0, 548), col=C.CREAM, bevel=1.5)
    # wardrobe side trims
    for (y0, y1) in ((-1580, -1450), (-1200, -1020)):
        a.box((3, y1 - y0, 12), at=(lx - 1.5, (y0 + y1) / 2, 6), col=C.MAGENTA, bevel=1)
        a.box((4, y1 - y0, 7), at=(lx - 3, (y0 + y1) / 2, 121), col=C.CREAM, bevel=1)
    # office side skirting under the window
    a.box((3, 960, 12), at=(lx - 1.5, 1500, 6), col=C.TEAL_DARK, bevel=1)
    vent(a, (lx - 2, -600, 470), rot=(0, 180, 0))
    # stage side: QUIET-ON-SET lamp box beside the big door, door-track hangers get bolts, a hose reel cabinet,
    # lobby side: sockets, EXIT sign above the Stage 4 door
    qa = (sx + 6, -420, 360)
    a.box((12, 70, 26), at=qa, col=C.CHARCOAL, bevel=2)
    a.box((1, 62, 18), at=(sx + 12.3, -420, 360), col=C.RED, glow=2.5)
    a.text("QUIET", 11, 0.6, at=(sx + 13, -420, 356), col=C.WHITE, ao=False)
    D.cable(a, [(sx + 3, -380, 360), (sx + 3, -380, 430)], r=1.0)
    for y in range(-280, 900, 120):
        D.screw(a, (sx + 11.2, y, 436), "+x", r=1.0)
    a.box((10, 60, 80), at=(sx + 5, -900, 150), col=C.RED, bevel=2)
    D.plate(a, (sx + 10, -900, 150), "+x", 50, 70, t=0.6, col=C.shade(C.RED, 0.85))
    a.box((0.6, 40, 50), at=(sx + 10.8, -900, 150), col=C.GLASS, mat="glass")
    a.torus(16, 4, at=(sx + 6, -900, 150), rot=(-90, 0, 0), col=C.shade(C.RED, 0.7), major=16, minor=6)
    D.label(a, (sx + 10.6, -900, 196), "+x", 40, 8, lines=1)
    for y in (-800, -500, 500, 800):
        outlet(a, (lx - 0.5, y, 30), "-x")
    exit_sign(a, (lx - 0.5, 0, 470), "-x")
    # the stage face gets the same structure as the other soundstage walls: I-beam columns with padded guards,
    # ring girder, quilted sound panels above the doors and a conduit run
    for y in (-1000, -500, 500, 1000, 1900):
        a.box((14, 30, 1440), at=(sx + 7, y, 600), col=C.GREY_DARK, bevel=2)
        a.box((20, 10, 1440), at=(sx + 17, y, 600), col=C.shade(C.GREY_DARK, 0.85), bevel=1)
        a.box((34, 40, 110), at=(sx + 17, y, -65), col=C.YELLOW, bevel=4, rough=0.8)
        D.hazard(a, (sx + 34, y, -65), "+x", 36, 96, stripes=5, t=0.6)
    a.box((24, 3560, 40), at=(sx + 12, 200, 1180), col=C.GREY_DARK, bevel=3)
    for yc in range(-1450, 1900, 250):
        if -380 < yc < 380:
            continue
        p = bm_box(12, 220, 200, 5, 1)
        bulge(p, 4, axis=0)
        a.add(p, xf((sx + 6, yc, 1000)), C.NAVY_LIGHT, rough=0.95)
    a.cyl(3, 3500, at=(sx + 6, 200, -25), rot=(0, 0, 90), col=C.GREY, sides=6, rough=0.4)
    for y in (-800, 800, 1500):
        a.box((8, 22, 22), at=(sx + 6, y, -25), col=C.GREY, bevel=2, rough=0.4)
        outlet(a, (sx + 1, y, -60), "+x", col=C.GREY)


# ============================================================================== landings

@asset("SM_Stage4_EntryLanding", F,
       desc="Teal entry landing and five-step stair down to the stage floor: nosed treads with tape edges, hazard-striped kick plates, yellow railing posts with ball caps and a mid rail.",
       replaces=[STAGE + " Stage 4 landing, edge tape, stairs, railing posts and rail"],
       placements=lambda: [P((-440, 200, 0))], covers=rec((761, 763), (767, 771)),
       pivot="landing top centre (-440, 200, 0); stairs run down towards +X",
       integration="Landing/stair collision stays with the code CubeSolids.", view=(1, 0.6, 0.6))
def entry_landing(a):
    pv = (-440, 200, 0)
    a.records(L.select("studio", lines=[(761, 763), (767, 771)]), pivot=pv, bevel=1.5)
    hazard(a, 1200, (140 + 0.8, 0, -8), h=14)
    for k in range(5):
        x = -300 + 60 * k - pv[0]
        z = -20 * (k + 1)
        a.box((6, 600, 4), at=(x + 3, -200, z + 1), col=C.GREY, bevel=1, rough=0.4)
    for r in L.select("studio", lines=[(769, 769)]):
        c = r["center"]
        a.sphere(5.5, at=(c[0] - pv[0], c[1] - pv[1], 100), col=C.YELLOW, segs=8, rings=5)
    a.cyl(2.2, 460, at=(-300 - pv[0], 550 - pv[1], 55), rot=(0, 0, 90), col=C.YELLOW, sides=8)
    # anti-slip strips on the landing and treads, stringer bolts, rail brackets
    for k in range(5):
        x = -300 + 60 * k - pv[0]
        z = -20 * (k + 1)
        for dx in (-20, -8):
            a.box((3, 560, 0.4), at=(x + 30 + dx, -200, z + 2.2), col=C.shade(C.GREY_DARK, 0.8), bevel=0, jitter=0)
    for y in range(-500, 501, 100):
        if abs(y) < 460:
            a.box((3, 50, 0.4), at=(100, y, 0.25), col=C.shade(C.GREY_DARK, 0.8), bevel=0, jitter=0)


@asset("SM_Stage4_WardrobeStairs", F,
       desc="Plum landing and stair from the wardrobe door down to the stage floor with tape nosings and a hazard kick plate.",
       replaces=[STAGE + " wardrobe landing + stairs"],
       placements=lambda: [P((-515, -1325, 0))], covers=rec((764, 765)),
       pivot="landing top centre (-515, -1325, 0)", view=(1, 0.6, 0.6))
def wardrobe_stairs(a):
    pv = (-515, -1325, 0)
    a.records(L.select("studio", lines=[(764, 765)]), pivot=pv, bevel=1.5)
    hazard(a, 250, (65 + 0.8, 0, -8), h=14)
    for k in range(5):
        a.box((6, 250, 4), at=(-450 + 60 * k - pv[0] + 3, 0, -20 * (k + 1) + 1), col=C.GREY, bevel=1, rough=0.4)
    for k in range(5):
        x = -450 + 60 * k - pv[0]
        for dx in (18, 30):
            a.box((3, 220, 0.4), at=(x + dx, 0, -20 * (k + 1) + 2.2), col=C.shade(C.GREY_DARK, 0.8), bevel=0, jitter=0)


# ============================================================================== signs

@asset("SM_Stage4_SoundstageBanner", F,
       desc="Yellow SOUNDSTAGE 4 wall banner with a navy border, rivets and wall brackets.",
       replaces=[STAGE + " SOUNDSTAGE 4 banner board"], placements=lambda: [P((-560, 700, 700))], covers=rec((773, 773)),
       pivot="board centre (-560, 700, 700); faces +X into the stage",
       integration="'SOUNDSTAGE 4' stays a TextRender (x -548) or use the matching SM_Sign_* letters.", view=(1, 0.3, 0.2))
def soundstage_banner(a):
    a.records(L.select("studio", lines=[(773, 773)]), pivot=(-560, 700, 700), bevel=3)
    a.box((16, 920, 12), at=(-2, 0, 92), col=C.NAVY, bevel=3)
    a.box((16, 920, 12), at=(-2, 0, -92), col=C.NAVY, bevel=3)
    for y in (-460, 460):
        a.box((16, 12, 196), at=(-2, y, 0), col=C.NAVY, bevel=3)
    for y in range(-440, 441, 110):
        for z in (80, -80):
            a.sphere(3, at=(11, y, z), col=C.BRASS, segs=6, rings=4, rough=0.3)
    for y in (-300, 300):
        a.box((24, 10, 10), at=(-12, y, 100), col=C.GREY_DARK, bevel=2)
    # weather-worn paint edge, bracket bolts and a sagging cable to the wall
    for y in (-300, 300):
        for z in (97, 103):
            D.screw(a, (-2, y, z), "+x", r=1.0)
    D.scuffs(a, (6, 0, 0), "+x", 880, 160, n=8, col=C.shade(C.YELLOW, 0.7))


@asset("SM_Stage4_SafetySign", F,
       desc="'HIGHER TO SAFETY!' flood-drill sign: cream enamel board with a blue wave band, navy arrow, screw heads and a little climbing-figure pictogram.",
       replaces=[STAGE + " safety sign board, wave band and arrow"], placements=lambda: [P((1250, -1600, 380))], covers=rec((775, 779)),
       pivot="board centre on the wall (1250, -1600, 380); faces +Y",
       integration="Text lines stay TextRenders.", view=(0.3, 1, 0.2))
def safety_sign(a):
    pv = (1250, -1600, 380)
    a.records(L.select("studio", lines=[(775, 779)]), pivot=pv, bevel=1.5)
    for k in range(9):
        a.sphere(12, at=(-160 + k * 40, 12, -118), col=C.BLUE, segs=8, rings=5, ry=4)
    for sx in (-1, 1):
        for sz in (-1, 1):
            a.cyl(3, 2, at=(sx * 168, 9, sz * 118), rot=(0, 0, 90), col=C.GREY, sides=8)
    # pictogram: stick figure on steps
    for k in range(3):
        a.box((24, 2, 8), at=(118 + k * 16, 11, -40 + k * 12), col=C.NAVY, bevel=0.5)
    a.sphere(6, at=(146, 12, 12), col=C.NAVY, segs=8, rings=5, ry=1.5)
    a.box((6, 2, 20), at=(144, 12, -6), rot=(0, 0, 10), col=C.NAVY, bevel=0.5)
    D.scuffs(a, (8, 0, 0), "+y", 320, 220, n=6, col=C.shade(C.CREAM, 0.75))
    a.box((6, 2, 20), at=(152, 12, -24), rot=(0, 0, -20), col=C.NAVY, bevel=0.5)
    a.box((6, 2, 18), at=(138, 12, -22), rot=(0, 0, 25), col=C.NAVY, bevel=0.5)


@asset("SM_Stage4_EffectsSign", F,
       desc="Coral PRACTICAL EFFECTS board with bolts and a spark icon.",
       replaces=[STAGE + " PRACTICAL EFFECTS board"], placements=lambda: [P((900, 2000, 260))], covers=rec((811, 811)),
       pivot="board centre on the wall (900, 2000, 260); faces -Y", view=(0.3, -1, 0.2))
def effects_sign(a):
    a.records(L.select("studio", lines=[(811, 811)]), pivot=(900, 2000, 260), bevel=1.5)
    a.box((514, 3, 104), at=(0, -2.5, 0), col=C.CORAL_DARK, bevel=1)
    a.poly([(0, 30), (8, 8), (30, 0), (8, -8), (0, -30), (-8, -8), (-30, 0), (-8, 8)], 3, at=(-230, -9, 0), rot=(0, 90, 0), col=C.YELLOW, glow=0.8)
    for sy in (-1, 1):
        for sz in (-1, 1):
            D.screw(a, (sy * 240, -4.5, sz * 44), "-y", r=1.6)
    D.scuffs(a, (0, -4, 0), "-y", 480, 90, n=5, col=C.shade(C.CORAL, 0.7))


@asset("SM_Stage4_ProjectionSign", F,
       desc="Small navy PROJECTION direction board at the foot of the long stair.",
       replaces=["AFTStudioShell::BuildUpperLevel() PROJECTION hint board (FTStudioShell.cpp:977; board turned to face +Y like its text - code finding)"], placements=lambda: [P((1350, -1585, 120))], covers=rec((977, 977)),
       pivot="board centre (1350, -1585, 120); faces +Y", view=(0.3, 1, 0.2))
def projection_sign(a):
    # navy board from the code (turned to face +Y like its text, see ftb.layout.BOARD_FIXES) on a yellow frame
    a.records(L.select("studio", lines=[(977, 977)]), pivot=(1350, -1585, 120), bevel=2)
    a.box((212, 6, 82), at=(0, 1, 0), col=C.YELLOW, bevel=2)
    for sx in (-1, 1):
        a.cyl(2, 1.4, at=(sx * 92, 10.6, 28), rot=(0, 0, 90), col=C.CHROME, sides=8, rough=0.3)


# ============================================================================== camera deck

@asset("SM_Stage4_CameraDeck", F,
       desc="Raised teal camera deck with tape borders, two-step stair, cable hooks along the fascia and a framed CAMERA board.",
       replaces=[STAGE + " camera deck, tape, steps and CAMERA board"],
       placements=lambda: [P((450, 0, -120))], covers=rec((785, 791)),
       pivot="deck footprint centre on the stage floor (450, 0, -120); deck top at +60",
       integration="AFTFilmCamera (450, 0, -60) runs its dolly track on top; ReelTray, LightingBoard and FilmCase also sit here.", view=(-1, 0.4, 0.6))
def camera_deck(a):
    pv = (450, 0, -120)
    a.records(L.select("studio", lines=[(785, 791)]), pivot=pv, bevel=1.5)
    hazard(a, 1600, (-150 - 0.8, 0, 45), rot=(0, 180, 0), h=12)
    for y in range(-760, 761, 190):
        a.box((6, 10, 6), at=(-153, y, 30), col=C.GREY, bevel=1.5)
        a.tube([(-154, y, 28), (-160, y, 20), (-160, y, 12)], 1.5, col=C.GREY, sides=5)
    a.box((14, 210, 70), at=(-146, -500, 100), col=C.CREAM, bevel=2)
    # deck edge nosing, fascia bolts, a cable ramp over the lip, board screws
    a.box((6, 1600, 3), at=(-151, 0, 61.5), col=C.GREY, bevel=0.8, rough=0.4)
    for y in range(-760, 761, 95):
        D.screw(a, (-153.8, y, 52), "-x", r=1.2)
    a.box((40, 60, 6), at=(-160, -380, 3), rot=(0, 0, 0), col=C.YELLOW, bevel=2)
    D.cable(a, [(-200, -380, 1.2), (-170, -380, 6.4), (-150, -380, 20), (-140, -380, 60), (-120, -380, 61.5)], r=1.2)
    D.screws_rect(a, (-138.8, -500, 100), "-x", 210, 70, inset=4, r=1.2)


# ============================================================================== sound booth

@asset("SM_Stage4_SoundBooth", F,
       desc="Purple sound booth: floor mat, low walls with foam wedge panels, chunky posts up to the balcony slab, header beam with a magenta neon strip and a navy SOUND plate.",
       replaces=[STAGE + " sound booth mat, walls, posts, header, neon strip and sign plate"],
       placements=lambda: [P((150, -1325, -120))], covers=rec((797, 808)),
       pivot="booth mat centre on the stage floor (150, -1325, -120)",
       integration="AFTSoundConsole (230, -1300, -120) stands inside; 'SOUND' stays a TextRender; keep the booth PointLight.", view=(1, 0.6, 0.5))
def sound_booth(a):
    pv = (150, -1325, -120)
    a.records(L.select("studio", lines=[(797, 808)]), pivot=pv, bevel=2)
    # foam wedges on the inner faces of the low walls
    for k in range(9):
        y = -1540 + k * 44 + 22 - pv[1]
        for j in range(2):
            a.prism((8, 40, 44), at=(390 - pv[0] - 6, y, 18 + j * 46), rot=(0, 0, -90), col=C.CHARCOAL, rough=1.0)
    for k in range(8):
        x = -80 + k * 44 + 22 - pv[0]
        for j in range(2):
            a.prism((8, 40, 44), at=(x, -1040 - pv[1] - 16, 18 + j * 46), rot=(0, 90, -90), col=C.CHARCOAL, rough=1.0)
    for y in (-1552, -1158):
        a.box((34, 34, 14), at=(400 - pv[0], y - pv[1], 7), col=C.shade(C.BOOTH_PURPLE, 0.8), bevel=3)
        a.box((34, 34, 14), at=(400 - pv[0], y - pv[1], 383), col=C.shade(C.BOOTH_PURPLE, 0.8), bevel=3)
    a.box((6, 440, 6), at=(417 - pv[0], 0, 390), col=C.MAGENTA, glow=0.8)
    # RECORDING light over the booth header, cable conduit down a post, headphone hook, a patch of gaffer tape
    a.box((10, 40, 16), at=(420 - pv[0], -60, 410), col=C.CHARCOAL, bevel=2)
    a.box((1, 34, 10), at=(425.3 - pv[0], -60, 410), col=C.RED, glow=2.5)
    a.box((4, 4, 370), at=(420 - pv[0], -1552 - pv[1] + 20, 200), col=C.GREY, bevel=0.6)
    a.box((6, 3, 8), at=(412 - pv[0], -1158 - pv[1] - 20, 140), col=C.GREY_DARK, bevel=0.8)
    a.torus(9, 1.6, at=(410 - pv[0], -1158 - pv[1] - 20, 126), rot=(-90, 0, 0), col=C.CHARCOAL, major=14, minor=4)
    for s in (-1, 1):
        a.cyl(4.5, 3, at=(410 - pv[0], -1158 - pv[1] - 20 + s * 9, 118), rot=(0, 0, 90), col=C.CHARCOAL, sides=10, bevel=0.8)


# ============================================================================== truss + work lights + ladder

def _truss_pl():
    return [P((900, c["call"]["args"][0], c["call"]["args"][1]), note="Truss(Y=%g)" % c["call"]["args"][0]) for c in L.kit_calls("studio", "Truss")]


@asset("SM_Stage4_TrussRun", F,
       desc="28 m box truss: four round chords, zig-zag lacing, end plates with bolts, hanging clamps every bay.",
       replaces=[STAGE + " Truss(): 4 chords + rungs + diagonals (FTStudioShell.cpp:383-401)"],
       placements=_truss_pl, covers=lambda: [c["records"] for c in L.kit_calls("studio", "Truss")], per_placement=True,
       pivot="truss centre (900, Y, 1040); runs along X from -500 to 2300", view=(0.5, 1, 0.4))
def truss_run(a):
    L0, L1 = -1400, 1400
    for dy in (-30, 30):
        for dz in (-25, 25):
            a.cyl(4.2, L1 - L0, at=(0, dy, dz), rot=(90, 0, 0), col=C.GREY_DARK, sides=8, bevel=0, rough=0.4)
    for x in range(L0, L1 + 1, 140):
        for dz in (-25, 25):
            a.cyl(2.2, 60, at=(x, 0, dz), rot=(0, 0, 90), col=C.GREY_DARK, sides=6, bevel=0, rough=0.4)
        for dy in (-30, 30):
            a.cyl(2.2, 50, at=(x, dy, 0), col=C.GREY_DARK, sides=6, bevel=0, rough=0.4)
        if x < L1:
            for dy in (-30, 30):
                a.tube([(x, dy, -25), (x + 140, dy, 25)], 1.8, col=C.GREY_DARK, sides=6, rough=0.4)
            for dz in (-25, 25):
                a.tube([(x, -30, dz), (x + 140, 30, dz)], 1.8, col=C.GREY_DARK, sides=6, rough=0.4)
    for x in (L0, L1):
        a.box((6, 76, 66), at=(x + (3 if x == L0 else -3), 0, 0), col=C.GREY, bevel=2, rough=0.4)
        for dy in (-24, 24):
            for dz in (-20, 20):
                a.cyl(2.5, 8, at=(x, dy, dz), rot=(90, 0, 0), col=C.CHROME, sides=6)
    for x in range(L0 + 70, L1, 280):
        a.box((10, 12, 14), at=(x, 0, 40), col=C.CHARCOAL, bevel=2)
    # safety wires at the clamps, a cable bundle with ties along the bottom chords, spigot bolts at every bay,
    # rating labels on the end plates
    for x in range(L0 + 70, L1, 280):
        a.torus(9, 0.6, at=(x + 12, 0, 30), rot=(0, 0, 90), col=C.GREY, major=12, minor=3)
    D.cable_tie_run(a, [(L0 + 20, -26, -31), (0, -26, -32), (L1 - 20, -26, -31)], r=2.0, col=C.RUBBER, ties=18)
    for x in range(L0 + 140, L1, 280):
        for dy in (-30, 30):
            for dz in (-25, 25):
                a.cyl(5.2, 6, at=(x, dy, dz), rot=(90, 0, 0), col=C.GREY, sides=8, bevel=0.8, rough=0.4)
    for x in (L0, L1):
        D.label(a, (x + (6 if x == L0 else -6), 0, 18), "+x" if x == L0 else "-x", 30, 10, lines=2)


def _work_recs():
    return [r for r in L.select("studio", lines=[(826, 831)]) if r["group"] == L.GROUP["CylDeco"]]


@asset("SM_Stage4_WorkLight", F,
       desc="Hanging work light: C-clamp on the truss, yoke, charcoal can with cooling fins and a glowing cream lens.",
       replaces=[STAGE + " nine work lights (box + glowing disc)"],
       placements=lambda: [P(r["center"]) for r in _work_recs()],
       covers=lambda: [[q for q in L.select("studio", lines=[(826, 831)]) if abs(q["center"][0] - r["center"][0]) < 1 and abs(q["center"][1] - r["center"][1]) < 1] for r in _work_recs()],
       per_placement=True, pivot="centre of the lens disc (the code's L, Z 980)",
       integration="Keep the code PointLights 40 cm below the lens.", view=(1, 0.6, -0.3))
def work_light(a):
    a.cyl(24, 26, at=(0, 0, 17), r_top=21, col=C.CHARCOAL, sides=14, bevel=3)
    for k in range(4):
        a.torus(22.5 - k * 0.4, 1.3, at=(0, 0, 10 + k * 6), col=C.shade(C.CHARCOAL, 1.25), major=14, minor=4)
    a.cyl(21, 4, at=(0, 0, 3.5), col=C.GREY_DARK, sides=14, bevel=1)
    a.cyl(19, 3, at=(0, 0, 0), col=C.CREAM, sides=14, bevel=0.5, glow=10)
    for sy in (-1, 1):
        a.box((4, 4, 22), at=(0, sy * 27, 24), col=C.YELLOW, bevel=1)
        a.cyl(4, 5, at=(0, sy * 25, 24), rot=(0, 0, 90), col=C.GREY, sides=8)
    a.box((6, 58, 5), at=(0, 0, 33), col=C.YELLOW, bevel=1.5)
    a.box((12, 16, 8), at=(0, 0, 38), col=C.GREY_DARK, bevel=2)
    # safety wire loop, power cable up to the truss, a label on the can, clamp knob
    a.torus(7, 0.6, at=(8, 0, 36), rot=(0, 0, 90), col=C.GREY, major=12, minor=3)
    D.cable(a, [(-20, 0, 22), (-26, 4, 30), (-24, 6, 44)], r=1.2)
    D.label(a, (0, 22.4, 17), "+y", 12, 8, lines=2)
    a.box((2, 8, 2), at=(0, 0, 44), col=C.YELLOW, bevel=0.5)


@asset("SM_Stage4_WallLadder", F,
       desc="Yellow wall ladder with rounded rungs, stand-off brackets and rubber feet.",
       replaces=[STAGE + " wall ladder rungs and rails"], placements=lambda: [P((2150, -1580, -120))], covers=rec((845, 850)),
       pivot="ladder foot centre (2150, -1580, -120)", view=(0.3, 1, 0.4))
def wall_ladder(a):
    # code: rungs 50 wide along X at X 2150, rails at X 2125/2175, Z -100..320; the wall is 20 cm behind (-Y)
    for x in (-25, 25):
        a.box((7, 6, 420), at=(x, 0, 230), col=C.YELLOW, bevel=1.5)
        a.box((9, 9, 5), at=(x, 0, 2.5), col=C.RUBBER, bevel=1.5)
        for z in (80, 380):
            a.box((5, 20, 5), at=(x, -10, z), col=C.GREY_DARK, bevel=1)
    for i in range(12):
        a.cyl(2.6, 50, at=(0, 0, 40 + i * 35), rot=(90, 0, 0), col=C.YELLOW, sides=8, bevel=0)
    # anchor bolts in the brackets, grip grooves on the rungs, a yellow/black hazard band at the foot
    for x in (-25, 25):
        for z in (80, 380):
            D.screw(a, (x, -20, z), "+y", r=1.2)
    for i in range(12):
        for dx in (-12, 0, 12):
            a.torus(2.65, 0.3, at=(dx, 0, 40 + i * 35), rot=(0, 0, 90), col=C.shade(C.YELLOW, 0.7), major=8, minor=3)
    D.hazard(a, (0, 4, 15), "+y", 56, 14, stripes=5, t=0.4)


# ============================================================================== warehouse

@asset("SM_Warehouse_FloorMat", FWH,
       desc="Rubber warehouse floor mat with a raised dot pattern and yellow corner guards.",
       replaces=[WH + " warehouse floor mat"], placements=lambda: [P((1880, 1750, -120))], covers=rec((983, 983)),
       pivot="mat centre on the stage floor (1880, 1750, -120)", view=(0.3, 0.3, 1))
def floor_mat(a):
    a.box((960, 480, 1.4), at=(0, 0, 0.7), col=C.GREY_DARK, bevel=0.5, ao=False)
    for x in range(-440, 441, 80):
        for y in range(-200, 201, 80):
            a.cyl(6, 0.5, at=(x, y, 1.5), col=C.shade(C.GREY_DARK, 1.2), sides=6, bevel=0, ao=False)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((40, 40, 1), at=(sx * 460, sy * 220, 1.6), col=C.YELLOW, bevel=0.3, ao=False)
    D.scuffs(a, (0, 0, 1.6), "+z", 900, 440, n=12, col=C.shade(C.GREY_DARK, 0.7))
    for sx in (-1, 1):
        for sy in (-1, 1):
            D.stencil_number(a, (sx * 460, sy * 220, 2.2), "+z", "!", 20, col=C.INK)


def _shelf_x():
    return [1600 + u * 260 for u in range(3)]


@asset("SM_Warehouse_ShelfUnit", FWH,
       desc="Heavy wooden prop shelf: square posts, cross-bracing on the sides, three plank shelves with front lips and a label holder.",
       replaces=[WH + " shelf body + 3 shelf boards per unit"],
       placements=lambda: [P((x, 1945, -120)) for x in _shelf_x()],
       covers=lambda: [L.select("studio", lines=[(986, 990)], where=lambda r, x=x: abs(r["center"][0] - x) < 1) for x in _shelf_x()],
       per_placement=True, pivot="unit footprint centre (X, 1945, -120); open front faces -Y",
       integration="The code BoxSolid (a solid block) stays as simple collision.", view=(0.4, -1, 0.5))
def shelf_unit(a):
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((8, 8, 400), at=(sx * 106, sy * 26, 200), col=C.WOOD_DARK, bevel=2)
        a.tube([(sx * 106, -26, 20), (sx * 106, 26, 380)], 1.5, col=C.GREY, sides=5)
        a.tube([(sx * 106, 26, 20), (sx * 106, -26, 380)], 1.5, col=C.GREY, sides=5)
    a.box((212, 3, 396), at=(0, 28, 200), col=C.shade(C.WOOD_DARK, 0.85), bevel=1)
    for z in (76, 186, 296, 396):
        a.box((212, 60, 8), at=(0, 0, z), col=C.WOOD, bevel=2)
        a.box((212, 4, 6), at=(0, -30, z + 5), col=C.WOOD_DARK, bevel=1.5)
        a.box((30, 2, 10), at=(0, -32.5, z - 7), col=C.CREAM, bevel=0.5)
    # bolts at the brace crossings, label lines, scuffed shelf fronts, levelling feet
    for sx in (-1, 1):
        D.screw(a, (sx * 110.2, 0, 200), "+x" if sx > 0 else "-x", r=1.2)
        for sy in (-1, 1):
            a.cyl(5, 3, at=(sx * 106, sy * 26, 1.5), col=C.GREY_DARK, sides=8, bevel=0.6)
    for z in (76, 186, 296, 396):
        a.box((20, 0.3, 1), at=(0, -33.6, z - 6), col=C.INK, bevel=0, jitter=0, wear=False)
        D.scuffs(a, (0, -32, z + 5), "-y", 200, 5, n=4, col=C.shade(C.WOOD_DARK, 0.7))


def _slot_props(kind):
    out = []
    for u in range(3):
        X = 1600 + u * 260
        for s in range(3):
            if (u + s) % 3 == kind:
                out.append(P((X - 50 + s * 40, 1888, -10 + s * 110), note="shelf %d level %d" % (u, s)))
    return out


def _slot_cover(kind):
    out = []
    for p in _slot_props(kind):
        x, y, z = p["loc"]
        out.append(L.select("studio", lines=[(992, 1019)], where=lambda r, x=x, z=z: abs(r["center"][0] - x) < 45 and abs(r["center"][2] - z) < 40))
    return out


@asset("SM_Prop_LensCase", FWH,
       desc="Teal lens case with a grey lid seam, cream latches, a carry handle and a lens-cap sticker.",
       replaces=[WH + " compact lens case with latches (shelf slot type 0)"],
       placements=lambda: _slot_props(0), covers=lambda: _slot_cover(0), per_placement=True,
       pivot="the code's slot point (case centre); case spans Z -22..+22", view=(0.4, -1, 0.5))
def lens_case(a):
    a.box((66, 44, 44), col=C.TEAL_DARK, bevel=4)
    a.box((68, 46, 4), at=(0, 0, 9), col=C.GREY, bevel=1.5, rough=0.35)
    for sx in (-1, 1):
        a.box((8, 4, 13), at=(sx * 21, -24, 9), col=C.CREAM_DARK, bevel=1.2, rough=0.35)
    a.tube([(-12, 0, 22), (-10, 0, 29), (10, 0, 29), (12, 0, 22)], 2, col=C.CHARCOAL, sides=6)
    a.cyl(8, 1, at=(12, -22.5, -6), rot=(0, 0, 90), col=C.YELLOW, sides=12, bevel=0)
    D.hinge(a, (0, 22.8, 9), "x", 50, r=1.0)
    D.scuffs(a, (0, -22, -5), "-y", 60, 26, n=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((6, 6, 44.6), at=(sx * 31, sy * 20, 0), col=C.GREY, bevel=1.2, rough=0.35)


@asset("SM_Prop_FilmCanStand", FWH,
       desc="Film reel standing in a little navy cradle: cream reel with three spokes around a teal hub.",
       replaces=[WH + " film can and reel on a stand (shelf slot type 1)"],
       placements=lambda: _slot_props(1), covers=lambda: _slot_cover(1), per_placement=True,
       pivot="the code's slot point; reel centre at (0, -5, +17)", view=(0.4, -1, 0.3))
def film_can_stand(a):
    a.box((64, 40, 7), at=(0, 0, -20), col=C.NAVY, bevel=2)
    for sx in (-1, 1):
        a.box((6, 24, 16), at=(sx * 14, -5, -10), col=C.NAVY, bevel=2)
    a.torus(22, 5.5, at=(0, -5, 17), rot=(0, 0, 90), col=C.CREAM_DARK, major=20, minor=8, rough=0.35)
    a.cyl(19, 3, at=(0, -5, 17), rot=(0, 0, 90), col=C.GREY_DARK, sides=18, bevel=0.5)
    a.cyl(6.5, 12, at=(0, -5, 17), rot=(0, 0, 90), col=C.TEAL, sides=10, bevel=1)
    for k in range(3):
        a.box((38, 4, 4), at=(0, -8, 17), rot=(k * 60, 0, 0), col=C.GREY, bevel=0.5, rough=0.4)
    for k in range(8):
        ang = math.radians(k * 45)
        a.sphere(1, at=(22 * math.cos(ang), -10.5, 17 + 22 * math.sin(ang)), col=C.shade(C.CREAM_DARK, 0.8), segs=6, rings=3)
    D.label(a, (0, -20, -20), "-y", 30, 5, lines=1)


@asset("SM_Prop_SoundBlankets", FWH,
       desc="Stack of three folded sound blankets (navy/coral/navy) with stitched quilting and a coiled cable on top.",
       replaces=[WH + " folded sound blankets + coiled cable (shelf slot type 2)"],
       placements=lambda: _slot_props(2), covers=lambda: _slot_cover(2), per_placement=True,
       pivot="the code's slot point; stack bottom at Z -22", view=(0.4, -1, 0.5))
def sound_blankets(a):
    for k in range(3):
        b = bm_box(65, 45, 10, 4, 2)
        bulge(b, 1.5, axis=2)
        col = C.CORAL if k % 2 else C.NAVY_LIGHT
        a.add(b, xf((k * 2, 0, -17 + k * 9), (0, a.rng.uniform(-4, 4), 0)), col, rough=0.95)
        for j in range(3):
            a.box((60, 0.8, 0.8), at=(k * 2, -12 + j * 12, -12 + k * 9), col=C.shade(col, 1.3), bevel=0, ao=False)
    a.torus(12, 3, at=(0, 0, 13), col=C.CHARCOAL, major=16, minor=6)
    a.torus(10, 3, at=(1, 1, 17), col=C.CHARCOAL, major=16, minor=6)
    for k in range(3):
        a.box((1.2, 45.5, 10), at=(33 + k * 2, 0, -17 + k * 9), col=C.shade(C.CORAL if k % 2 else C.NAVY_LIGHT, 0.75), bevel=0.4)


@asset("SM_Warehouse_HarpoonRack", FWH,
       desc="Charcoal harpoon rack with a yellow padded top, foam cradles for the hero harpoon and a red HERO HARPOON board on a post.",
       replaces=[WH + " harpoon rack body, top, red board"],
       placements=lambda: [P((2250, 1700, -120))], covers=rec((1027, 1029)),
       pivot="rack footprint centre (2250, 1700, -120); the harpoon prop lies on top at Z -36",
       integration="'HERO HARPOON' stays a TextRender.", view=(-1, 0.4, 0.5))
def harpoon_rack(a):
    pv = (2250, 1700, -120)
    a.records(L.select("studio", lines=[(1027, 1029)]), pivot=pv, bevel=2)
    for y in (-70, 70):
        a.box((30, 16, 8), at=(0, y, 88), col=C.YELLOW, bevel=3)
        a.box((30, 6, 10), at=(0, y, 94), col=C.shade(C.YELLOW, 0.8), bevel=2)
    for y in (-100, 100):
        a.box((8, 8, 126), at=(36, y, 147), col=C.CHARCOAL, bevel=2)
    a.box((6, 226, 66), at=(38, 0, 180), col=C.shade(C.RED, 0.75), bevel=2)
    # bolts on the post feet, foam grain on the cradles, board screws, a tool clipboard
    for y in (-100, 100):
        a.box((20, 20, 3), at=(36, y, 85.5), col=C.GREY_DARK, bevel=0.8)
        D.screws_rect(a, (36, y, 87), "+z", 20, 20, inset=3, r=0.9)
    for y in (-70, 70):
        for k in range(3):
            a.box((30.4, 1, 0.8), at=(0, y - 5 + k * 5, 99.5), col=C.shade(C.YELLOW, 0.75), bevel=0, jitter=0, wear=False)
    D.screws_rect(a, (41.2, 0, 180), "+x", 226, 66, inset=5, r=1.3)
    a.box((2, 26, 34), at=(-18, 60, 50), col=C.WOOD, bevel=0.6)
    a.box((0.4, 22, 28), at=(-19.2, 60, 48), col=C.WHITE, bevel=0.05, jitter=0)


def _spare_recs():
    return L.select("studio", lines=[(1032, 1037)])


def _spare_pl(coral):
    out = []
    for i in range(3):
        if (i == 1) != coral:
            continue
        out.append(P((2100 + i * 70, 1480, -120)))
    return out


def _spare_cov(coral):
    return [[r for r in _spare_recs() if abs(r["center"][0] - p["loc"][0]) < 1] for p in _spare_pl(coral)]


def _spare(a, col):
    # (detail pass: leg feet, clamp knob, cable, vents)
    for k in range(3):
        ang = k * 120
        a.cyl(3, 2, at=(math.cos(math.radians(ang)) * 26, math.sin(math.radians(ang)) * 26, 1), col=C.RUBBER, sides=8, bevel=0.5)
    a.box((1.4, 8, 1.4), at=(6, 0, 40), col=col, bevel=0.4)
    D.cable(a, [(-19, 0, 124), (-18, 4, 100), (-6, 6, 60), (-4, 10, 10), (4, 20, 1.2)], r=0.9)
    D.vent(a, (0, 0, 147), "+z", 26, 22, slats=4, col=C.shade(col, 0.7), depth=0.8)
    for sy in (-1, 1):
        D.screws_rect(a, (0, sy * 17, 130), "+y" if sy > 0 else "-y", 36, 30, inset=3, r=0.7)
    for k in range(3):
        ang = k * 120
        a.cyl(1.8, 44, at=(math.cos(math.radians(ang)) * 14, math.sin(math.radians(ang)) * 14, 18), rot=(34, ang, 0), col=C.CHARCOAL, sides=6)
    a.cyl(3, 110, at=(0, 0, 62), col=C.CHARCOAL, sides=8)
    a.cyl(5, 6, at=(0, 0, 40), col=col, sides=8, bevel=1)
    a.box((40, 34, 34), at=(0, 0, 130), col=col, bevel=5)
    a.cyl(14, 4, at=(21, 0, 130), rot=(90, 0, 0), col=C.GREY_DARK, sides=12, bevel=1)
    a.cyl(11, 2, at=(23, 0, 130), rot=(90, 0, 0), col=C.CREAM, sides=12)
    for dz in (-1, 1):
        a.box((14, 32, 2), at=(26, 0, 130 + dz * 17), rot=(dz * 25, 0, 0), col=C.CHARCOAL, bevel=0.5)


@asset("SM_Prop_SpareLight_Teal", FWH,
       desc="Spare tripod light with a teal head, lens and barn doors.",
       replaces=[WH + " spare light stands (teal)"], placements=lambda: _spare_pl(False), covers=lambda: _spare_cov(False), per_placement=True,
       pivot="tripod centre on the floor", view=(1, 0.6, 0.4))
def spare_light_teal(a):
    _spare(a, C.TEAL)


@asset("SM_Prop_SpareLight_Coral", FWH,
       desc="Spare tripod light with a coral head, lens and barn doors.",
       replaces=[WH + " spare light stand (coral, middle one)"], placements=lambda: _spare_pl(True), covers=lambda: _spare_cov(True), per_placement=True,
       pivot="tripod centre on the floor", view=(1, 0.6, 0.4))
def spare_light_coral(a):
    _spare(a, C.CORAL)


@asset("SM_Prop_BeachBall", FWH,
       desc="Oversized striped beach ball (spare set dressing).",
       replaces=[WH + " coral ball"], placements=lambda: [P((1960, 1540, -130))], covers=rec((1025, 1025)),
       pivot="ball bottom (the code ball is centred at Z -90, radius 40)", view=(1, 0.6, 0.4))
def beach_ball(a):
    cols = [C.CORAL, C.WHITE, C.YELLOW, C.WHITE, C.TEAL, C.WHITE]
    for k in range(6):
        from ftb.core import bm_lathe
        prof = [(0.01, -40)] + [(40 * math.cos(t), 40 * math.sin(t)) for t in [-math.pi / 2 + i * math.pi / 10 for i in range(1, 10)]] + [(0.01, 40)]
        bm = bm_lathe(prof, sides=4, arc=60)
        a.add(bm, xf((0, 0, 40), (0, k * 60, 0)), cols[k], rough=0.35)
    a.cyl(8, 2, at=(0, 0, 80), col=C.CORAL, sides=10)
    a.cyl(3, 1, at=(0, 0, 0.3), col=C.CORAL, sides=10)
    a.cyl(1.6, 3, at=(8, 0, 79), rot=(0, 0, 0), col=C.WHITE, sides=8, bevel=0.5)


@asset("SM_Warehouse_Sign", FWH,
       desc="Yellow PROP WAREHOUSE wall sign with a navy border.",
       replaces=[WH + " PROP WAREHOUSE board"], placements=lambda: [P((1880, 2000, 420))], covers=rec((1038, 1038)),
       pivot="board centre on the wall (1880, 2000, 420); faces -Y", view=(0.3, -1, 0.2))
def warehouse_sign(a):
    # the code board is 4 cm at y -6 (1994); navy backing plate behind, yellow face in front of it, text at y -10
    a.box((612, 3, 112), at=(0, -2.5, 0), col=C.NAVY, bevel=1)
    a.box((600, 5, 100), at=(0, -6.5, 0), col=C.YELLOW, bevel=1.6)
    a.box((580, 1, 80), at=(0, -9.2, 0), col=C.shade(C.YELLOW, 1.08), bevel=0.4)
    for sx in (-1, 1):
        for sz in (-1, 1):
            a.cyl(2.2, 1.6, at=(sx * 290, -9.4, sz * 40), rot=(0, 0, 90), col=C.CHROME, sides=8, rough=0.3)
