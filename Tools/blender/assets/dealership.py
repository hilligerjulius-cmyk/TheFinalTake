"""Dream Cars dealership (planned work package 4): lot, sign gantry, showroom canopy, turntables, sales kiosk,
price stands, bunting, flood poles and a waving tube man. Laid out inside the lot the city shell reserves
(X -5600..-3900, Y 300..1500) without touching the code's walls, bay lines, sign or pillars."""
import math

from ftb import detail as D
from ftb import layout as L
from ftb import palette as C
from ftb.core import FONT_BLOCK
from ftb.registry import P, asset

F = "Dealership"
LOT = "AFTCityShell::BuildBlocks() Dream Cars lot (FTCity.cpp:293-311)"
BAYS = [(-5300, 900), (-5000, 900), (-4700, 900), (-4400, 900)]  # centres between the code's bay lines


def rec(*ranges, where=None):
    return lambda: L.select("city", lines=list(ranges), where=where)


@asset("SM_Dealer_Lot", F,
       desc="Dream Cars lot: pale showroom paving with a checker border, the four painted display bays (code lines), tyre-mark scuffs, planter strip along the side wall, cream wall caps and the lavender back and side walls with car-silhouette murals.",
       replaces=[LOT + ": lot slab, bay lines, back wall, side wall"],
       placements=lambda: [P((-4750, 900, 0))], covers=rec((294, 300)),
       pivot="world-aligned, pivot (-4750, 900, 0) = lot centre at ground level; the boulevard is on -Y",
       integration="Walls keep the code collision; the Blocker above the back wall stays.", view=(0.5, -1, 0.9))
def lot(a):
    pv = (-4750, 900, 0)
    a.records(L.select("city", lines=[(294, 300)]), pivot=pv, bevel=2)
    for k in range(34):
        for (y0, sgn) in ((300, 1),):
            x = -5600 + k * 50 + 25
            if k % 2 == 0:
                a.box((50, 50, 0.6), at=(x - pv[0], y0 + 25 - pv[1], 4.3), col=C.NAVY, bevel=0, ao=False)
    for (x, y) in BAYS:
        a.box((250, 6, 0.6), at=(x - pv[0], 460 - pv[1], 4.5), col=C.YELLOW, bevel=0, glow=0.3, ao=False)
        for k in range(3):
            a.box((80, 10, 0.4), at=(x - pv[0] + a.rng.uniform(-40, 40), y - pv[1] + a.rng.uniform(-250, 250), 4.4), rot=(0, a.rng.uniform(-20, 20), 0), col=C.shade(C.LOT_GREY, 0.8), bevel=0, ao=False)
    # wall caps + murals (back wall face at Y 1500, side wall face at X -5600)
    a.box((1710, 50, 10), at=(0, 1520 - pv[1], 265), col=C.CREAM, bevel=3)
    a.box((50, 1250, 10), at=(-5620 - pv[0], 920 - pv[1], 265), col=C.CREAM, bevel=3)
    for (x, c) in ((-5250, C.CORAL), (-4600, C.TEAL), (-4100, C.YELLOW)):
        prof = [(-120, 0), (120, 0), (125, 30), (80, 40), (50, 75), (-40, 75), (-80, 40), (-125, 30)]
        a.poly([(p[0], p[1] + 120) for p in prof], 2, at=(x - pv[0], 1499 - pv[1], 0), rot=(0, -90, 0), col=c, bevel=0.5)
        for s in (-1, 1):
            a.cyl(22, 3, at=(x - pv[0] + s * 70, 1497 - pv[1], 120), rot=(0, 0, 90), col=C.NAVY, sides=12)
    # planter strip along the west wall
    a.box((60, 900, 40), at=(-5570 - pv[0], 900 - pv[1], 20), col=C.TERRACOTTA, bevel=4)
    for k in range(9):
        a.sphere(26, at=(-5570 - pv[0], 500 + k * 100 - pv[1], 50), ry=34, rz=22, col=C.GREEN if k % 2 else C.GREEN_DARK, segs=10, rings=6)
    # detail pass: drain channel with a grate, expansion joints in the paving, bay numbers, wall-cap joints,
    # a hose reel on the side wall, mural outlines
    a.box((1600, 20, 1), at=(0, 1470 - pv[1], 4.8), col=C.GREY_DARK, bevel=0.3)
    for x in range(-5580, -3920, 20):
        a.box((2, 18, 0.4), at=(x - pv[0], 1470 - pv[1], 5.4), col=C.INK, bevel=0, jitter=0, ao=False)
    for x in range(-5500, -3950, 300):
        a.box((1.2, 1150, 0.3), at=(x - pv[0], 900 - pv[1], 4.7), col=C.shade(C.LOT_GREY, 0.7), bevel=0, jitter=0, ao=False)
    for i, (x, y) in enumerate(BAYS):
        D.stencil_number(a, (x - pv[0] + 90, 520 - pv[1], 4.8), "+z", str(i + 1), 40, col=C.WHITE)
    for x in range(-5500, -3950, 200):
        a.box((1.2, 52, 10.4), at=(x - pv[0], 1520 - pv[1], 265), col=C.CREAM_DARK, bevel=0, jitter=0, wear=False)
    a.cyl(20, 8, at=(-5588 - pv[0], 1350 - pv[1], 120), rot=(90, 0, 0), col=C.RED, sides=14, bevel=2)
    a.torus(16, 2.4, at=(-5582 - pv[0], 1350 - pv[1], 120), rot=(90, 0, 0), col=C.GREEN_DARK, major=16, minor=5)
    a.box((8, 30, 40), at=(-5596 - pv[0], 1350 - pv[1], 120), col=C.GREY_DARK, bevel=1.5)


@asset("SM_Dealer_SignGantry", F,
       desc="DREAM CARS sign gantry: two chrome pillars with brass collars, the coral sign board in a navy frame spanning the pillars, cyan neon tubes, chrome car emblems and ten socketed bulbs along the top.",
       replaces=[LOT + ": sign board, bulbs, pillars (FTCity.cpp:302, 307-310)"],
       placements=lambda: [P((-4750, 1480, 0))], covers=rec((302, 302), (305, 310)),
       pivot="world-aligned, pivot (-4750, 1480, 0); the sign faces -Y (towards the boulevard)",
       integration=("CODE FINDING: FTCity.cpp:302 builds the board as FVector(20, 1200, 240) - 90 degrees off its own text (yaw -90), "
                    "bulb row and pillars, so it sticks through the back wall. This mesh spans the board across X between the pillars "
                    "(1200 x 20 x 240, as the text and bulbs imply); hide the code board when swapping. 'DREAM CARS' and the tagline stay "
                    "TextRenders (or use the SM_Sign_City_* letters); keep the PointLight."), view=(0.3, -1, 0.3))
def sign_gantry(a):
    pv = (-4750, 1480, 0)
    recs = L.select("city", lines=[(305, 310)])
    bulbs = [r for r in recs if r["group"] == L.GROUP["SphereDeco"]]
    a.records([r for r in recs if r not in bulbs], pivot=pv, bevel=3)
    # board across X (see the integration note), front face at local y -10 like the code's text at y -12
    a.box((1200, 20, 240), at=(0, 0, 520), col=C.CORAL, bevel=5, glow=0.2)
    for z in (393, 647):
        a.box((1230, 30, 14), at=(0, 0, z), col=C.NAVY, bevel=4)
    for x in (-610, 610):
        a.box((14, 30, 268), at=(x, 0, 520), col=C.NAVY, bevel=4)
    for z in (410, 630):
        a.box((1160, 4, 5), at=(0, -13, z), col=C.CYAN, glow=5)
    car = [(-60, 0), (60, 0), (62, 14), (40, 18), (24, 36), (-24, 36), (-40, 18), (-62, 14)]
    for x in (-515, 515):
        a.poly([(p[0], p[1] - 18) for p in car], 4, at=(x, -12, 520), rot=(0, -90, 0), col=C.CHROME, bevel=1, rough=0.2)
        for s in (-1, 1):
            a.cyl(11, 5, at=(x + s * 36, -14, 502), rot=(0, 0, 90), col=C.CHARCOAL, sides=12, bevel=1)
    for r in bulbs:
        c = r["center"]
        a.cyl(8, 5, at=(c[0] - pv[0], c[1] - pv[1] + 4, c[2]), rot=(0, 0, 90), col=C.CHROME, sides=10, rough=0.25)
        a.sphere(7, at=(c[0] - pv[0], c[1] - pv[1], c[2]), col=C.YELLOW, segs=10, rings=6, glow=10)
    for x in (-5300, -4200):
        for z in (10, 200, 390):
            a.torus(9, 3, at=(x - pv[0], 0, z), col=C.BRASS, major=12, minor=6, rough=0.3)
    # back bracing of the board (towards the wall), cable conduits up the pillars, bolts on the frame
    for x in (-400, -130, 130, 400):
        a.box((8, 12, 230), at=(x, 16, 520), col=C.GREY_DARK, bevel=1.5)
    a.tube([(-560, 16, 420), (560, 16, 620)], 2.5, col=C.GREY_DARK, sides=6)
    a.tube([(-560, 16, 620), (560, 16, 420)], 2.5, col=C.GREY_DARK, sides=6)
    for x in (-5300, -4200):
        a.cyl(2.6, 380, at=(x - pv[0] + 26, 10, 200), col=C.GREY, sides=8)
    for x in range(-600, 601, 100):
        D.screw(a, (x, -15.4, 393), "-y", r=1.4)
        D.screw(a, (x, -15.4, 647), "-y", r=1.4)


@asset("SM_Dealer_ShowroomCanopy", F,
       desc="Showroom canopy over the four display bays: slim chrome columns on the bay lines, a white steel roof frame with glass panels, a coral fascia with bulbs and hanging display spotlights.",
       replaces=["new for work package 4 (Dream Cars showroom)"],
       placements=lambda: [P((-4850, 1075, 0))],
       pivot="world-aligned, pivot (-4850, 1075, 0); roof at Z 360 below the sign board, columns on the code bay lines",
       integration="Visual only; columns stand on the bay lines so the bays stay drivable.", view=(0.5, -1, 0.7))
def showroom_canopy(a):
    xs = [-5450, -5150, -4850, -4550, -4250]
    y0, y1 = 700 - 1075, 1450 - 1075
    for x in xs:
        for y in (y0, y1):
            a.cyl(7, 360, at=(x + 4850, y, 180), col=C.CHROME, sides=10, bevel=0, rough=0.25)
            a.cyl(14, 6, at=(x + 4850, y, 3), col=C.GREY_DARK, sides=12, bevel=2)
    a.box((1240, 20, 24), at=(0, y0, 366), col=C.WHITE, bevel=4)
    a.box((1240, 20, 24), at=(0, y1, 366), col=C.WHITE, bevel=4)
    for x in xs:
        a.box((16, y1 - y0 + 20, 20), at=(x + 4850, (y0 + y1) / 2, 366), col=C.WHITE, bevel=3)
    for k in range(4):
        a.box((284, y1 - y0 - 10, 3), at=(-450 + k * 300, (y0 + y1) / 2, 374), col=C.GLASS, mat="glass")
    a.box((1250, 8, 44), at=(0, y0 - 14, 360), col=C.CORAL, bevel=3)
    for k in range(25):
        a.sphere(4.5, at=(-600 + k * 50, y0 - 19, 360), col=C.YELLOW, segs=6, rings=4, glow=8)
    for x in xs[:-1]:
        for y in (y0 + 150, y1 - 150):
            cx = x + 150 + 4850
            a.cyl(1.5, 30, at=(cx, y, 339), col=C.CHARCOAL, sides=6)
            a.cone(12, 20, at=(cx, y, 318), rot=(180, 0, 0), r_top=6, col=C.CHARCOAL, sides=10)
            a.cyl(8, 2, at=(cx, y, 308), col=C.CREAM, sides=10, glow=10)
    # base-plate bolts, a gutter with a downpipe, glass clips on the roof panels, cable trays for the spots
    for x in xs:
        for y in (y0, y1):
            for k in range(4):
                ang = math.radians(k * 90 + 45)
                D.screw(a, (x + 4850 + 10 * math.cos(ang), y + 10 * math.sin(ang), 6), "+z", r=1.2)
    a.box((1240, 12, 10), at=(0, y1 + 16, 358), col=C.GREY, bevel=2)
    a.tube([(xs[-1] + 4850 + 10, y1 + 16, 355), (xs[-1] + 4850 + 12, y1 + 12, 10)], 3.2, col=C.GREY, sides=8)
    for k in range(4):
        for y in (y0 + 60, (y0 + y1) / 2, y1 - 60):
            a.box((10, 10, 3), at=(-450 + k * 300, y, 376.5), col=C.CHROME, bevel=0.5)


@asset("SM_Dealer_DisplayTurntable", F,
       desc="Car display turntable: chrome-rimmed round platform with a diamond-plate top, a glowing cyan uplight ring and a brand medallion.",
       replaces=["new for work package 4 (one per display bay)"],
       placements=lambda: [P((x, y, 4)) for (x, y) in BAYS],
       pivot="platform centre on the lot paving (top at Z 12)", integration="Attach the displayed car (SM_Veh_*_Body) at the socket 'Car' and spin this mesh.", view=(0.6, 0.7, 0.6))
def turntable(a):
    a.cyl(142, 10, at=(0, 0, 5), col=C.GREY, sides=32, bevel=3, rough=0.35)
    a.torus(142, 3.5, at=(0, 0, 8), col=C.CHROME, major=40, minor=6, rough=0.2)
    a.torus(136, 1.8, at=(0, 0, 2), col=C.CYAN, major=40, minor=5, glow=4)
    for k in range(12):
        for j in range(3):
            ang = math.radians(k * 30 + j * 10)
            r = 40 + j * 35
            a.box((12, 4, 0.8), at=(r * math.cos(ang), r * math.sin(ang), 10.4), rot=(0, k * 30 + 45, 0), col=C.shade(C.GREY, 1.2), bevel=0, ao=False)
    a.cyl(22, 1.6, at=(0, 0, 10.8), col=C.CORAL, sides=16, bevel=0.5)
    a.socket("Car", (0, 0, 12))
    # rim bolts, a motor access hatch, sponsor decal ring
    for k in range(24):
        ang = math.radians(k * 15)
        D.screw(a, (138 * math.cos(ang), 138 * math.sin(ang), 10), "+z", r=1.2)
    D.plate(a, (100, 0, 10), "+z", 30, 20, t=0.6, col=C.shade(C.GREY, 0.9))
    a.torus(60, 1.2, at=(0, 0, 10.2), col=C.CORAL, major=40, minor=4, rz=0.3)


@asset("SM_Dealer_SalesKiosk", F,
       desc="Sales kiosk: cream booth with teal corners, sliding service window facing the lot and a hatch to the sidewalk, striped coral awning, cash register, key board with car keys, brochure rack, a roof sign with a spinning-car icon and little pennant flags.",
       replaces=["new for work package 4 (sales desk / purchase point)"],
       placements=lambda: [P((-4080, 560, 4))],
       pivot="booth footprint centre on the lot (-4080, 560, 4); service window faces -X (into the lot), hatch faces -Y",
       integration="Put the purchase UFTInteractableComponent at the socket 'Service' (a shop terminal like AFTShopTerminal).", view=(-1, -0.7, 0.5))
def sales_kiosk(a):
    W, DK, H = 240, 200, 300
    a.box((W, DK, 10), at=(0, 0, 5), col=C.GREY_DARK, bevel=3)
    for s in (-1, 1):
        a.box((W, 14, H), at=(0, s * (DK / 2 - 7), 10 + H / 2), col=C.CREAM, bevel=4)
    a.box((14, DK, H), at=(W / 2 - 7, 0, 10 + H / 2), col=C.CREAM, bevel=4)
    a.box((14, DK, 100), at=(-W / 2 + 7, 0, 60), col=C.CREAM, bevel=4)
    a.box((14, DK, 70), at=(-W / 2 + 7, 0, 10 + H - 35), col=C.CREAM, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((20, 20, H + 4), at=(sx * (W / 2 - 6), sy * (DK / 2 - 6), 12 + H / 2), col=C.TEAL, bevel=5)
    a.box((W + 30, DK + 30, 16), at=(0, 0, 18 + H), col=C.TEAL_DARK, bevel=5)
    a.box((W + 10, DK + 10, 12), at=(0, 0, 32 + H), col=C.CORAL, bevel=4)
    # service window + counter
    a.box((6, DK - 30, 130), at=(-W / 2 + 4, 0, 175), col=C.GLASS, mat="glass")
    a.box((40, DK - 20, 8), at=(-W / 2 - 10, 0, 112), col=C.WOOD, bevel=3)
    for k in range(3):
        a.box((6, 8, 132), at=(-W / 2 + 2, -50 + k * 50, 175), col=C.TEAL, bevel=1.5)
    a.box((30, 34, 22), at=(-W / 2 + 30, -50, 127), col=C.CHARCOAL, bevel=3)
    a.box((2, 24, 8), at=(-W / 2 + 14, -50, 136), col=C.GREEN, glow=3)
    a.box((6, 60, 50), at=(W / 2 - 20, 20, 190), col=C.WOOD_DARK, bevel=2)
    for k in range(6):
        a.box((2, 4, 8), at=(W / 2 - 24, -4 + (k % 3) * 20, 200 - (k // 3) * 20), col=[C.RED, C.YELLOW, C.CYAN][k % 3], bevel=0.5)
    # sidewalk hatch (-Y) with a little shelf
    a.box((110, 6, 90), at=(20, -DK / 2 - 1, 170), col=C.GLASS, mat="glass")
    a.box((110, 30, 6), at=(20, -DK / 2 - 14, 125), col=C.WOOD, bevel=2)
    # awning over the service window
    n = 8
    for k in range(n):
        a.box((70, DK / n + 1, 6), at=(-W / 2 - 34, -DK / 2 + (k + 0.5) * DK / n, 262), rot=(18, 0, 0), col=C.CORAL if k % 2 == 0 else C.WHITE, bevel=1.5)
    # brochure rack outside
    a.box((30, 50, 110), at=(-W / 2 - 20, DK / 2 + 40, 55), col=C.CHROME, bevel=3, rough=0.25)
    for k in range(3):
        a.box((6, 44, 26), at=(-W / 2 - 36, DK / 2 + 40, 40 + k * 32), rot=(0, 0, 0), col=[C.CORAL, C.TEAL, C.YELLOW][k], bevel=1)
    # roof sign
    a.box((20, 180, 70), at=(0, 0, 90 + H), col=C.NAVY, bevel=4)
    car = [(-60, 0), (60, 0), (62, 14), (40, 18), (22, 36), (-24, 36), (-40, 18), (-62, 14)]
    a.poly([(p[0], p[1] - 18) for p in car], 4, at=(-12, 0, 90 + H), rot=(0, 90, 0), col=C.CHROME, bevel=1, rough=0.2)
    a.text("SALES", 26, 3, at=(-12, 0, 70 + H), rot=(0, 180, 0), col=C.YELLOW, font=FONT_BLOCK)
    for s in (-1, 1):
        a.cyl(1.5, 80, at=(W / 2, s * (DK / 2), H + 80), col=C.GREY, sides=6)
        a.prism((1.5, 30, 20), at=(W / 2, s * (DK / 2) + 15, H + 110), rot=(0, 0, 90), col=C.MAGENTA if s < 0 else C.CYAN)
    a.socket("Service", (-W / 2 - 30, 0, 110))
    # detail pass: back door with a handle, AC unit on the roof, electric meter, posters on the side, gutter,
    # a bell push by the window, a trash bin
    D.plate(a, (W / 2, 50, 110), "+x", 80, 200, t=1.2, col=C.shade(C.CREAM, 0.92), screws=False)
    D.handle(a, (W / 2, 20, 110), "+x", 20, along="v", r=1.2, standoff=3)
    a.box((60, 50, 30), at=(60, 40, 60 + H), col=C.GREY, bevel=3)
    D.vent(a, (90.5, 40, 60 + H), "+x", 40, 22, slats=4, col=C.GREY_DARK)
    D.plate(a, (40, DK / 2 - 0.2, 150), "+y", 30, 40, t=4, col=C.GREY, screws=True)
    for k, c in enumerate((C.CORAL, C.YELLOW)):
        a.box((50, 0.6, 70), at=(-40 + k * 60, -DK / 2 - 0.4, 190 - k * 10), rot=(0, 0, k * 3 - 1.5), col=c, bevel=0.1, jitter=0)
    a.cyl(3, 1.4, at=(-W / 2 - 1, 80, 140), rot=(90, 0, 0), col=C.BRASS, sides=10, bevel=0.3)
    a.cyl(16, 70, at=(W / 2 + 22, -DK / 2 + 30, 45), col=C.TEAL_DARK, sides=12, bevel=2)
    a.cyl(17, 5, at=(W / 2 + 22, -DK / 2 + 30, 82), col=C.GREY, sides=12, bevel=1.5)


@asset("SM_Dealer_PriceStand", F,
       desc="A-frame price stand: cream board with a coral header, star burst and chrome legs.",
       replaces=["new for work package 4 (one per bay, price shown by a TextRender)"],
       placements=lambda: [P((x, 430, 4)) for (x, y) in BAYS],
       pivot="stand foot centre; the front board faces -Y (the boulevard)", view=(1, 0.5, 0.3))
def price_stand(a):
    # two boards leaning together (tilt about X), coral header on the front board
    for s in (-1, 1):
        a.box((70, 4, 110), at=(0, s * 12, 55), rot=(0, 0, -s * 12), col=C.CREAM, bevel=1.5)
    a.box((72, 5, 26), at=(0, -14, 98), rot=(0, 0, 12), col=C.CORAL, bevel=1.5)
    pts = []
    for k in range(16):
        ang = math.radians(k * 22.5)
        rr = 16 if k % 2 == 0 else 10
        pts.append((math.cos(ang) * rr, math.sin(ang) * rr + 60))
    a.poly(pts, 2, at=(22, -16, 0), rot=(0, 90, 0), col=C.YELLOW, glow=0.5)
    for s in (-1, 1):
        for sx in (-1, 1):
            a.box((3, 3, 112), at=(sx * 36, s * 12, 55), rot=(0, 0, -s * 12), col=C.CHROME, bevel=0.8, rough=0.25)
    a.tube([(-34, -18, 30), (0, 0, 24), (34, 18, 30)], 0.5, col=C.GREY, sides=4)


@asset("SM_Dealer_Bunting", F,
       desc="600 cm pennant string: sagging cord with alternating coral/cyan/yellow flags.",
       replaces=["new for work package 4"],
       placements=lambda: [P((-4750, 460, 380), (0, 90, 0)), P((-4750, 1160, 380), (0, 90, 0))],
       pivot="string mid-span (hangs down 60 cm); spans local Y -300..300", view=(1, 0.3, 0.2))
def bunting(a):
    pts = []
    for i in range(21):
        t = i / 20
        pts.append((0, -300 + 600 * t, -60 * math.sin(math.pi * t)))
    a.tube(pts, 0.8, col=C.GREY_DARK, sides=5, caps=False)
    cols = [C.CORAL, C.CYAN, C.YELLOW, C.MAGENTA]
    for k in range(20):
        t = (k + 0.5) / 20
        y = -300 + 600 * t
        z = -60 * math.sin(math.pi * t)
        a.prism((1, 24, 30), at=(0, y, z - 15), rot=(0, 0, 180), col=cols[k % 4])


@asset("SM_Dealer_FloodPole", F,
       desc="Lot flood-light pole with two angled floodlights and a bunting hook.",
       replaces=["new for work package 4"],
       placements=lambda: [P((-5460, 460, 4)), P((-4040, 1160, 4))],
       pivot="pole foot", view=(1, 0.5, 0.3))
def flood_pole(a):
    a.cyl(12, 10, at=(0, 0, 5), col=C.GREY_DARK, sides=12, bevel=3)
    a.cyl(5, 420, at=(0, 0, 215), col=C.GREY, sides=10, rough=0.35)
    a.box((10, 80, 8), at=(0, 0, 420), col=C.GREY_DARK, bevel=2)
    for s in (-1, 1):
        a.box((24, 26, 18), at=(8, s * 36, 410), rot=(-30, 0, 0), col=C.CHARCOAL, bevel=3)
        a.box((2, 22, 14), at=(20, s * 36, 405), rot=(-30, 0, 0), col=C.CREAM, glow=10)
    a.torus(4, 1, at=(0, 6, 380), rot=(0, 0, 90), col=C.GREY, major=8, minor=4)
    for k in range(4):
        ang = math.radians(k * 90 + 45)
        D.screw(a, (9 * math.cos(ang), 9 * math.sin(ang), 10), "+z", r=1.2)
    a.box((10, 14, 24), at=(6, 0, 100), col=C.GREY_DARK, bevel=1.5)
    a.cyl(1.4, 300, at=(5, 3, 250), col=C.GREY_DARK, sides=6)
    for s in (-1, 1):
        D.vent(a, (-4, s * 36, 416), "-x", 18, 10, slats=3, col=C.GREY_DARK)


@asset("SM_Dealer_TubeMan", F,
       desc="Wacky waving inflatable tube man: coral tube body with a grinning face, arm tubes and a blower base.",
       replaces=["new for work package 4 (entrance eye-catcher)"],
       placements=lambda: [P((-3960, 380, 4))],
       pivot="blower foot on the paving", integration="Static pose; animate with a simple wobble material or a sway component if desired.", view=(1, 0.6, 0.3))
def tube_man(a):
    a.cyl(34, 40, at=(0, 0, 20), col=C.GREY_DARK, sides=14, bevel=5)
    a.cyl(28, 6, at=(0, 0, 42), col=C.CHARCOAL, sides=14)
    pts = []
    radii = []
    for k in range(10):
        t = k / 9
        pts.append((12 * math.sin(t * 5), 18 * math.sin(t * 3 + 1), 44 + 260 * t))
        radii.append(26 - 6 * t)
    a.tube(pts, 24, col=C.CORAL, sides=12, radii=radii, rough=0.4)
    top = pts[-1]
    for s in (-1, 1):
        a.tube([(top[0], top[1], top[2] - 60), (top[0] + 10, top[1] + s * 60, top[2] - 20), (top[0] + 5, top[1] + s * 100, top[2] + 30)], 10, col=C.CORAL, sides=10)
        a.sphere(7, at=(top[0] + 22, top[1] + s * 9, top[2] - 20), col=C.WHITE, segs=8, rings=5)
        a.sphere(3.5, at=(top[0] + 27, top[1] + s * 9, top[2] - 20), col=C.INK, segs=6, rings=4)
    a.torus(10, 2.5, at=(top[0] + 22, top[1], top[2] - 42), rot=(90, 0, 0), col=C.INK, major=12, minor=5)
    for k in range(5):
        a.cone(6, 22, at=(top[0] - 4 + k * 2, top[1] - 16 + k * 8, top[2] + 22), rot=(k * 6 - 12, 0, 0), col=C.YELLOW, sides=6)
    # blower grille, power cable with a plug, a sandbag on the blower rim, a warning label
    D.grille_holes(a, (34, 0, 20), "+x", 30, 24, pitch=4, r=1)
    D.cable(a, [(-34, 0, 6), (-50, 10, 1.2), (-80, 30, 1.2)], r=1.2)
    a.box((8, 6, 5), at=(-84, 32, 2.5), col=C.YELLOW, bevel=1)
    a.box((24, 14, 8), at=(-24, 22, 43), rot=(0, 30, 0), col=C.shade(C.ORANGE, 0.72), bevel=3.2, rough=0.95)
    D.label(a, (0, -34, 22), "-y", 20, 10, lines=2)
