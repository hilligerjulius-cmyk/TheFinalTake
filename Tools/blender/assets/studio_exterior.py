"""Studio street front: road, sidewalk, facade, marquee, entrance, carpet, STAGE 4 sign, blocks across the street."""
import math

from ftb import layout as L
from ftb import palette as C
from ftb.core import FONT_BLOCK
from ftb.registry import P, asset

F = "Studio/Exterior"
SHELL = "AFTStudioShell::BuildExterior()"


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


# ============================================================================== street + sidewalk

@asset("SM_Studio_Street", F,
       desc="Studio street: asphalt with patch repairs, manholes and storm drains, sidewalk with paving joints, curb stones, lane dashes, rain puddles.",
       replaces=[SHELL + " street slab, sidewalk, lane dashes, curb, puddles"],
       placements=lambda: [P((-2750, 200, 0))], covers=rec((429, 441)),
       pivot="world-aligned, pivot (-2750, 200, 0) = road surface level; road top Z 0, sidewalk top Z 4",
       integration="Visual replacement of the CubeSolid street/sidewalk instances; keep the invisible street Blockers (FTStudioShell.cpp:541-544).",
       view=(0.2, 0.6, 1.0))
def street(a):
    pv = (-2750, 200, 0)
    a.records(L.select("studio", lines=[(429, 441)]), pivot=pv, bevel=1.0)
    rng = a.rng
    # sidewalk: paving slabs as shallow raised tiles with dark joints (X -2250..-1800, Y -2200..2600)
    for iy in range(40):
        y = -2200 + 60 + iy * 120 - pv[1]
        a.box((438, 3, 0.6), at=(-2025 - pv[0], y, 4.1), col=C.shade(C.SIDEWALK, 0.7), bevel=0, ao=False)
    a.box((3, 4790, 0.6), at=(-2100 - pv[0], 0, 4.1), col=C.shade(C.SIDEWALK, 0.7), bevel=0, ao=False)
    # curb stones on top of the code's curb strip
    for iy in range(24):
        y = -2200 + 100 + iy * 200 - pv[1]
        a.box((16, 196, 3), at=(-2253 - pv[0], y, 4.0), col=C.shade(C.CREAM, rng.uniform(0.9, 1.02)), bevel=1.2, segs=1, rough=0.9)
    # asphalt patches and tar seams
    for (x, y, w, d) in ((-3350, -1300, 260, 180), (-2700, 700, 220, 160), (-3150, 1700, 300, 140), (-2450, -800, 160, 120)):
        a.box((w, d, 0.8), at=(x - pv[0], y - pv[1], 0.2), rot=(0, rng.uniform(-6, 6), 0), col=C.shade(C.ASPHALT, 0.82), bevel=0.3, ao=False)
    for k in range(6):
        x0 = rng.uniform(-3600, -2350)
        y0 = rng.uniform(-2100, 2400)
        pts = [(x0 - pv[0] + i * 25, y0 - pv[1] + rng.uniform(-18, 18), 0.25) for i in range(6)]
        a.tube(pts, 1.4, col=C.shade(C.ASPHALT, 0.6), sides=4, ao=False)
    # manholes
    for (x, y) in ((-2850, -300), (-3250, 1250)):
        a.cyl(42, 1.6, at=(x - pv[0], y - pv[1], 0.3), col=C.GREY_DARK, sides=18, bevel=0.5, rough=0.5)
        for k in range(5):
            a.box((60 - abs(k - 2) * 12, 4, 0.8), at=(x - pv[0], y - pv[1] - 16 + k * 8, 1.3), col=C.shade(C.GREY_DARK, 0.7), bevel=0.2, ao=False)
    # storm drains in the curb face
    for y in (-1500, 150, 1900):
        a.box((6, 90, 10), at=(-2263 - pv[0], y - pv[1], 1.0), col=C.CHARCOAL, bevel=1)
        for k in range(6):
            a.box((7, 3, 11), at=(-2263 - pv[0], y - pv[1] - 35 + k * 14, 1.0), col=C.GREY, bevel=0.4, rough=0.4)


# ============================================================================== facade

@asset("SM_Studio_Facade", F,
       desc="Teal art-deco facade with cream cornice and belt course, fluted pilasters, lit windows with cream mullions and sills, wall sconces, drainpipes and a founding plaque.",
       replaces=[SHELL + " facade walls, cornice, belt course, base band, five lit windows"],
       placements=lambda: [P((-1800, 0, 0))], covers=rec((444, 457)),
       pivot="world-aligned, pivot (-1800, 0, 0): centre of the entrance opening at street level; street face at local X -20",
       integration="Walls keep their code collision (CubeSolid); this mesh can carry complex-as-simple collision instead.",
       view=(-1, -0.55, 0.35))
def facade(a):
    pv = (-1800, 0, 0)
    a.records(L.select("studio", lines=[(444, 457)]), pivot=pv, bevel=1.5)
    X = -1820 - pv[0]  # street face
    # fluted pilasters between windows and at the entrance
    for y in (-1600, -975, -425, 425, 975, 1500, 2000):
        a.box((10, 46, 700), at=(X - 5, y, 450), col=C.TEAL_LIGHT, bevel=3)
        for k in (-1, 0, 1):
            a.box((4, 6, 660), at=(X - 11, y + k * 13, 450), col=C.shade(C.TEAL_LIGHT, 0.85), bevel=1.5)
        a.box((16, 58, 30), at=(X - 8, y, 810), col=C.CREAM, bevel=3)
        a.box((16, 58, 24), at=(X - 8, y, 112), col=C.CREAM_DARK, bevel=3)
    # stepped art-deco crown over the entrance
    for k, (w, h) in enumerate(((620, 30), (460, 30), (300, 30))):
        a.box((24 - k * 4, w, h), at=(X - 12, 0, 850 + k * 30), col=C.CREAM if k % 2 == 0 else C.TEAL_LIGHT, bevel=4)
    a.cyl(40, 10, at=(X - 14, 0, 950), rot=(90, 0, 0), col=C.YELLOW, sides=20, bevel=2, glow=0.6)
    for k in range(7):
        ang = math.radians(-60 + k * 20)
        a.box((6, 8, 60), at=(X - 16, 50 * math.sin(ang) * 1.3, 950 + 50 * math.cos(ang)), rot=(0, 0, -math.degrees(ang)), col=C.YELLOW, bevel=1.5, glow=0.4)
    # window flower boxes + sills (under the five lower windows)
    for y in (-1250, -700, 700, 1250, 1750):
        a.box((34, 300, 26), at=(X - 18, y, 128), col=C.WOOD, bevel=4)
        for k in range(9):
            a.sphere(16, at=(X - 20, y - 130 + k * 32.5, 146), col=C.GREEN if k % 2 else C.GREEN_DARK, segs=8, rings=5, rz=12, rough=0.8)
            if k % 2 == 0:
                a.sphere(5, at=(X - 30, y - 130 + k * 32.5, 158), col=C.CORAL if k % 4 == 0 else C.YELLOW, segs=6, rings=4)
    # wall sconces left/right of the entrance
    for y in (-330, 330):
        a.box((10, 30, 50), at=(X - 5, y, 300), col=C.BRASS, bevel=3, rough=0.3)
        a.cyl(13, 36, at=(X - 22, y, 300), col=C.CREAM, sides=10, bevel=3, glow=6)
        a.cone(14, 12, at=(X - 22, y, 324), col=C.BRASS, sides=10, rough=0.3)
    # drainpipes on the building corners
    for y in (-1605, 2005):
        a.tube([(X - 8, y, 0), (X - 8, y, 790), (X - 20, y, 815), (X - 34, y, 826)], 6, col=C.GREY_DARK, sides=8, rough=0.5)
        for z in (150, 400, 650):
            a.box((10, 18, 6), at=(X - 6, y, z), col=C.GREY, bevel=1.5)
    # plaque + intercom next to the doors
    a.box((4, 70, 46), at=(X - 2, -410, 190), col=C.BRASS, bevel=1.5, rough=0.3)
    a.text("EST. 1938", 11, 0.8, at=(X - 4.5, -410, 196), rot=(0, 180, 0), col=C.NAVY, font=FONT_BLOCK)
    a.text("FINAL TAKE", 8, 0.8, at=(X - 4.5, -410, 180), rot=(0, 180, 0), col=C.NAVY, font=FONT_BLOCK)
    a.box((6, 20, 34), at=(X - 3, 410, 150), col=C.CHARCOAL, bevel=2)
    a.cyl(3.5, 3, at=(X - 7, 410, 158), rot=(90, 0, 0), col=C.RED, sides=8, glow=3)
    a.box((2, 12, 10), at=(X - 7, 410, 142), col=C.GREY, bevel=0.5)
    # roof parapet cap
    a.box((12, 3640, 10), at=(X + 22, 200, 845), col=C.CREAM_DARK, bevel=2)


# ============================================================================== marquee

@asset("SM_Studio_Marquee", F,
       desc="Coral marquee box with a navy letter field, 32 socketed chase bulbs, cream pylons with glowing prism tips, an art-deco sun fan, and the teal entrance canopy with its amber bulb row and tie rods.",
       replaces=[SHELL + " marquee box/board, 32 bulbs, pylons + prism tips, canopy + 9 bulbs"],
       placements=lambda: [P((-1800, 0, 0))], covers=rec((460, 477)),
       pivot="same pivot as SM_Studio_Facade (-1800, 0, 0)",
       integration="The title stays a UTextRenderComponent (x -1868, z 690) or use SM_Sign_Studio_THE_FINAL_TAKE_STUDIOS. Canopy keeps its code collision.",
       view=(-1, -0.6, 0.25))
def marquee(a):
    pv = (-1800, 0, 0)
    recs = L.select("studio", lines=[(460, 477)])
    bulbs = [r for r in recs if r["group"] == L.GROUP["SphereDeco"]]
    a.records([r for r in recs if r not in bulbs and r["group"] != L.GROUP["PrismDeco"]], pivot=pv, bevel=3)
    # frame around the letter field
    for z in (604, 756):
        a.box((10, 960, 14), at=(-1866 - pv[0], 0, z), col=C.CREAM, bevel=3)
    for y in (-476, 476):
        a.box((10, 14, 166), at=(-1866 - pv[0], y, 680), col=C.CREAM, bevel=3)
    # bulbs in chrome cups
    for r in bulbs:
        c = r["center"]
        big = r["size"][0] >= 12
        a.cyl(7 if big else 6, 5, at=(c[0] - pv[0] + 3, c[1], c[2]), rot=(90, 0, 0) if big else (0, 0, 0), col=C.CHROME, sides=8, bevel=0.8, rough=0.3)
        a.sphere(6.5 if big else 5.5, at=(c[0] - pv[0] - 2, c[1], c[2] - (0 if big else 3)), col=C.YELLOW if big else C.AMBER, segs=8, rings=6, glow=r["emissive"])
    # pylon tips as faceted jewels (code prisms at z 930)
    for y in (-560, 560):
        a.cone(26, 60, at=(-1850 - pv[0], y, 930), col=C.YELLOW, sides=6, glow=1.0)
        a.box((50, 50, 14), at=(-1850 - pv[0], y, 905), col=C.BRASS, bevel=3, rough=0.3)
        for z in (560, 640, 720, 800):
            a.box((44, 44, 8), at=(-1850 - pv[0], y, z), col=C.CREAM_DARK, bevel=2)
    # sun fan on top of the marquee
    a.poly([(math.cos(math.radians(t)) * 150 * (1 if k % 2 == 0 else 0.62), math.sin(math.radians(t)) * 150 * (1 if k % 2 == 0 else 0.62) + 776)
            for k, t in enumerate(range(0, 181, 10))], 12, at=(-1850 - pv[0], 0, 0), col=C.YELLOW, bevel=2, glow=0.5)
    a.cyl(46, 16, at=(-1856 - pv[0], 0, 780), rot=(0, 0, 90), col=C.CORAL, sides=16, bevel=3)
    # canopy: ribbed underside, trim, tie rods back to the facade
    cx = -1960 - pv[0]
    a.box((10, 770, 18), at=(cx - 161, 0, 380), col=C.CREAM, bevel=3)
    for y in (-381, 381):
        a.box((322, 10, 18), at=(cx, y, 380), col=C.CREAM, bevel=3)
    for k in range(7):
        a.box((300, 10, 8), at=(cx, -330 + k * 110, 366), col=C.shade(C.TEAL_DARK, 0.8), bevel=2)
    for y in (-360, 360):
        a.tube([(cx - 150, y, 395), (-1830 - pv[0], y, 560)], 3, col=C.BRASS, sides=6, rough=0.3)
        a.sphere(6, at=(-1826 - pv[0], y, 562), col=C.BRASS, segs=8, rings=5, rough=0.3)
    for r in [r for r in bulbs if r["size"][0] < 12]:
        pass


# ============================================================================== entrance

@asset("SM_Studio_EntranceDoors", F,
       desc="Brass-framed entrance: two glass door leaves parked in the wall pockets, brass jambs with rope moulding, header with a clapper-board emblem, push bars and a welcome mat.",
       replaces=[SHELL + " glass doors and brass door posts"],
       placements=lambda: [P((-1800, 0, 0))], covers=rec((480, 483)),
       pivot="same pivot as SM_Studio_Facade (-1800, 0, 0)",
       integration="Glass panels are GlassSolid in the code (collision stays there).", view=(-1, -0.4, 0.3))
def entrance(a):
    pv = (-1800, 0, 0)
    a.records(L.select("studio", lines=[(480, 483)]), pivot=pv, bevel=2)
    for y in (-330, 330):
        a.box((14, 150, 10), at=(-1790 - pv[0], y, 345), col=C.BRASS, bevel=2, rough=0.3)
        a.box((14, 10, 340), at=(-1790 - pv[0], y + (-70 if y < 0 else 70), 170), col=C.BRASS, bevel=2, rough=0.3)
        a.box((14, 150, 12), at=(-1790 - pv[0], y, 6), col=C.BRASS, bevel=2, rough=0.3)
        a.cyl(2.5, 110, at=(-1799 - pv[0], y, 110), rot=(0, 0, 90), col=C.CHROME, sides=8, rough=0.2)
    for y in (-262, 262):
        for k in range(8):
            a.torus(9, 2.6, at=(-1826 - pv[0], y, 20 + k * 44), col=C.shade(C.BRASS, 0.85), major=10, minor=5, rough=0.3)
    a.box((20, 540, 30), at=(-1826 - pv[0], 0, 372), col=C.BRASS, bevel=4, rough=0.3)
    a.box((6, 90, 56), at=(-1838 - pv[0], 0, 372), col=C.INK, bevel=2)
    for k in range(4):
        a.box((3, 90, 7), at=(-1842 - pv[0], 0, 392 - k * 13), rot=(0, 0, 0), col=C.WHITE if k % 2 == 0 else C.INK, bevel=0.6)


@asset("SM_Studio_RedCarpet", F,
       desc="Red carpet runner with brass stair-rod edges, gold star inlays and a stitched border.",
       replaces=[SHELL + " red carpet"],
       placements=lambda: [P((-2030, 0, 4))], covers=rec((485, 485)),
       pivot="carpet centre on the sidewalk (-2030, 0, 4)", view=(-0.4, 0.2, 1))
def red_carpet(a):
    a.box((460, 300, 2.2), at=(0, 0, 1.1), col=C.CARPET, bevel=1, rough=0.95)
    for y in (-138, 138):
        a.box((456, 6, 0.6), at=(0, y, 2.3), col=C.shade(C.CARPET, 1.25), bevel=0.2, ao=False)
    for y in (-152, 152):
        a.cyl(2.2, 462, at=(0, y, 1.6), rot=(90, 0, 0), col=C.BRASS, sides=8, rough=0.3)
        for x in (-231, 231):
            a.sphere(3.2, at=(x, y, 1.6), col=C.BRASS, segs=8, rings=5, rough=0.3)
    for x in (-150, 0, 150):
        pts = []
        for k in range(10):
            ang = math.radians(90 + k * 36)
            rr = 38 if k % 2 == 0 else 16
            pts.append((x + math.cos(ang) * rr, math.sin(ang) * rr))
        a.slab(pts, 0.6, at=(0, 0, 2.2), col=C.BRASS, ao=False, rough=0.35)


@asset("SM_Studio_Stage4StreetSign", F,
       desc="Navy STAGE 4 sidewalk sign: bolted foot plate, square post with a brass collar, framed board, glowing yellow arrow and two gooseneck sign lights.",
       replaces=[SHELL + " STAGE 4 sign stand, board, arrow shaft + head"],
       placements=lambda: [P((-2080, -820, 4))], covers=rec((497, 502)),
       pivot="post foot on the sidewalk (-2080, -820, 4); board faces -X (street)",
       integration="Text 'STAGE 4' stays a TextRender (or SM_Sign_Studio_STAGE_4).", view=(-1, -0.5, 0.3))
def stage4_street_sign(a):
    pv = (-2080, -820, 4)
    a.records(L.select("studio", lines=[(497, 502)]), pivot=pv, bevel=2)
    a.box((60, 60, 5), at=(0, 0, 2.5), col=C.GREY_DARK, bevel=1.5)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(3, 3, at=(sx * 22, sy * 22, 6), col=C.CHROME, sides=6, rough=0.3)
    a.box((28, 28, 10), at=(0, 0, 150), col=C.BRASS, bevel=2, rough=0.3)
    a.box((26, 310, 150), at=(2, 0, 226), col=C.BRASS, bevel=3, rough=0.35)
    a.box((6, 290, 130), at=(-9, 0, 226), col=C.NAVY, bevel=2)
    for y in (-110, 110):
        a.tube([(2, y, 296), (-6, y, 308), (-24, y, 312)], 2, col=C.CHARCOAL, sides=6)
        a.cone(8, 11, at=(-27, y, 306), rot=(-40, 0, 0), col=C.CHARCOAL, sides=8)
        a.cyl(5.5, 2, at=(-30, y, 302), rot=(-40, 0, 0), col=C.CREAM, sides=8, glow=8)


# ============================================================================== blocks across the street

_BLOCKS = [(-1700, 700, 900, C.NAVY_LIGHT), (-900, 600, 1300, C.BLD_NAVY_B), (700, 800, 1150, C.NAVY_LIGHT),
           (1600, 700, 950, C.BLD_NAVY_B), (2300, 600, 1400, C.BLD_NAVY_C)]


def _block(idx):
    by, bw, bh, col = _BLOCKS[idx]
    name = "SM_Studio_StreetBlock_%s" % "ABCDE"[idx]

    def covers():
        return L.select("studio", lines=[(530, 530), (536, 536)], where=lambda r: abs(r["center"][1] - by) <= bw / 2 + 1 and (r["line"] == 536 or abs(r["center"][1] - by) < 1))

    @asset(name, F,
           desc="Building across the studio street (%d cm wide, %d cm tall): window grid with frames and sills (lit pattern from the code), cornice, ground-floor shop front, roof clutter." % (bw, bh),
           replaces=[SHELL + " building block at Y %d (wall + window grid)" % by],
           placements=lambda: [P((-3600, by, 0))], covers=covers,
           pivot="world-aligned, pivot (-3600, %d, 0): centre of the facade that faces the studio (+X)" % by,
           view=(1, 0.5, 0.35))
    def build(a):
        pv = (-3600, by, 0)
        recs = covers()
        a.records(recs, pivot=pv, bevel=2)
        # window frames, sills, lintels
        for r in recs:
            if r["line"] != 536:
                continue
            y, z = r["center"][1] - by, r["center"][2]
            a.box((6, 84, 6), at=(3, y, z + 48), col=C.CREAM_DARK, bevel=1.5)
            a.box((10, 88, 7), at=(5, y, z - 49), col=C.CREAM, bevel=2)
            a.box((4, 4, 90), at=(3, y, z), col=C.shade(col, 0.7), bevel=0.8)
        # cornice + parapet
        a.box((24, bw + 20, 30), at=(10, 0, bh - 15), col=C.shade(col, 1.5), bevel=4)
        a.box((14, bw + 10, 14), at=(5, 0, bh - 44), col=C.shade(col, 1.3), bevel=3)
        # ground floor shop front (below the first window row)
        a.box((8, bw - 40, 130), at=(4, 0, 72), col=C.shade(col, 0.75), bevel=2)
        a.box((4, bw * 0.5, 90), at=(8, -bw * 0.12, 70), col=C.WINDOW_WARM, glow=0.8)
        for k in range(4):
            a.box((6, 6, 92), at=(10, -bw * 0.12 - bw * 0.25 + k * bw * 0.5 / 3, 70), col=C.CREAM, bevel=1)
        a.box((5, 90, 120), at=(8, bw * 0.3, 60), col=C.CHARCOAL, bevel=2)
        stripe = [C.CORAL, C.TEAL, C.YELLOW, C.MAGENTA, C.CYAN][idx]
        a.box((8, bw * 0.62, 26), at=(9, -bw * 0.12, 138), col=C.NAVY, bevel=2)
        a.box((3, bw * 0.6, 4), at=(13, -bw * 0.12, 149), col=stripe, glow=3)
        a.box((3, bw * 0.6, 4), at=(13, -bw * 0.12, 127), col=stripe, glow=3)
        # roof: low clutter only (keeps the code's skyline)
        for k in range(2):
            yy = -bw * 0.25 + k * bw * 0.4
            a.box((110, 90, 60), at=(-120, yy, bh + 30), col=C.GREY, bevel=5)
            a.cyl(30, 6, at=(-120, yy, bh + 62), col=C.GREY_DARK, sides=12)
            for g in range(4):
                a.box((3, 70, 40), at=(-64, yy - 26 + g * 17, bh + 30), col=C.GREY_DARK, bevel=0.8)
        a.tube([(-60, bw * 0.35, bh), (-60, bw * 0.35, bh + 45), (-80, bw * 0.35, bh + 55)], 6, col=C.GREY_DARK, sides=8)
        a.box((120, 70, 40), at=(-230, bw * 0.1, bh + 20), col=C.shade(col, 1.2), bevel=4)
        a.box((6, 50, 26), at=(-168, bw * 0.1, bh + 20), col=C.CHARCOAL, bevel=1.5)
        # fire escape on the taller blocks
        if bh >= 1150:
            for z in range(380, int(bh) - 250, 180):
                a.box((70, bw * 0.3, 5), at=(38, bw * 0.22, z), col=C.CHARCOAL, bevel=1)
                a.box((4, bw * 0.3, 4), at=(72, bw * 0.22, z + 45), col=C.CHARCOAL, bevel=0.8)
                for k in range(5):
                    a.box((3, 3, 45), at=(72, bw * 0.22 - bw * 0.15 + k * bw * 0.075, z + 22), col=C.CHARCOAL, bevel=0)
                a.box((60, 6, 190), at=(38, bw * 0.22 + (bw * 0.12 if (z // 180) % 2 else -bw * 0.12), z + 90), rot=(0, 0, 0), col=C.GREY_DARK, bevel=1)
    return build


for _i in range(5):
    _block(_i)
