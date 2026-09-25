"""Director's office and wardrobe: shells, ceilings and furniture."""

from ftb import detail as D
from ftb import layout as L
from ftb import palette as C
from ftb.core import bm_box, bulge, xf
from ftb.registry import P, asset
from assets.studio_lobby import cracks, exit_sign, extinguisher, moulding_x, outlet, smoke_detector, sprinkler, switch, vent

FO = "Studio/Office"
FW = "Studio/Wardrobe"
OFFICE = "AFTStudioShell::BuildOffice()"
WARD = "AFTStudioShell::BuildWardrobe()"


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


# ============================================================================== office shell

@asset("SM_Office_Shell", FO,
       desc="Director's office: warm plank floor (individual boards), magenta/coral rug with fringe, teal back wall with dark wainscot, chair rail, skirting and the cream stage-window mullions.",
       replaces=[OFFICE + " plank floor + seams, rug, back wall, wainscot, window mullions"],
       placements=lambda: [P((-1200, 1500, 0))], covers=rec((630, 637), (639, 639), (678, 679)),
       pivot="world-aligned, pivot (-1200, 1500, 0) = office centre at floor level",
       integration="Ceiling is SM_Office_Ceiling. The office window glass is part of SM_Stage4_SouthWall.", view=(0.3, -0.9, 1.2))
def office_shell(a):
    pv = (-1200, 1500, 0)
    recs = L.select("studio", lines=[(630, 637), (639, 639), (678, 679)])   # 638 = ceiling -> SM_Office_Ceiling
    seams = [r for r in recs if r["line"] == 633]
    a.records([r for r in recs if r not in seams], pivot=pv, bevel=1.0)
    rng = a.rng
    # boards: 10 rows (code seam every 100) broken into staggered planks
    for row in range(10):
        y = 1050 + row * 100 - pv[1]
        x = -1800 - (row % 3) * 60
        while x < -600:
            ln = rng.choice((160, 220, 280))
            x0, x1 = max(x, -1800), min(x + ln, -600)
            if x1 - x0 > 20:
                a.box((x1 - x0 - 3, 96, 1.2), at=((x0 + x1) / 2 - pv[0], y + 50 - 50, 0.6), col=C.shade(C.WOOD, rng.uniform(0.86, 1.08)), bevel=0.5, segs=1, ao=False)
            x += ln
    # rug fringe
    for x in range(-1390, -900, 16):
        for y in (1200, 1800):
            a.box((3, 10, 0.8), at=(x - pv[0], y + (-5 if y < 1500 else 5) - pv[1], 1.3), col=C.CREAM, bevel=0.2, ao=False)
    a.box((440, 540, 0.6), at=(-1150 - pv[0], 0, 2.0), col=C.shade(C.CORAL, 0.85), bevel=0.2, ao=False)
    a.box((400, 500, 0.6), at=(-1150 - pv[0], 0, 2.3), col=C.CORAL, bevel=0.2, ao=False)
    a.cyl(90, 0.6, at=(-1150 - pv[0], 0, 2.6), col=C.YELLOW, sides=24, bevel=0, ao=False)
    a.cyl(70, 0.6, at=(-1150 - pv[0], 0, 2.9), col=C.MAGENTA, sides=24, bevel=0, ao=False)
    # trims on the back wall (face at Y 1980, room side -Y)
    moulding_x(a, -1800 - pv[0], -600 - pv[0], 1980 - pv[1], 6, 2.5, 12, C.TEAL_DARK, face=-1)
    moulding_x(a, -1800 - pv[0], -600 - pv[0], 1980 - pv[1], 101, 3, 7, C.CREAM, face=-1)
    moulding_x(a, -1800 - pv[0], -600 - pv[0], 1980 - pv[1], 450, 4, 14, C.CREAM, face=-1)
    vent(a, (-1450 - pv[0], 1978.5 - pv[1], 400), rot=(0, -90, 0))
    # window mullions (verticals between the code's cream bars)
    for y in range(1220, 1800, 145):
        a.box((24, 10, 110), at=(-620 - pv[0], y - pv[1], 290), col=C.CREAM, bevel=2)
    # detail pass: sockets and a switch, a radiator under the stage window, framed certificates, plank nail
    # heads and scuffs on the floor, rug wear patch, plaster cracks
    for x in (-1650, -1250, -850):
        outlet(a, (x - pv[0], 1977.5 - pv[1], 30), "-y")
    switch(a, (-1360 - pv[0], 1020.5 - pv[1], 135), "+y")
    for k in range(14):
        a.box((6, 4, 50), at=(-610 - pv[0], -104 + k * 16, 70), col=C.CREAM, bevel=1.2)
    a.box((8, 226, 6), at=(-611 - pv[0], 0, 97), col=C.CREAM_DARK, bevel=1.5)
    a.box((8, 226, 4), at=(-611 - pv[0], 0, 44), col=C.CREAM_DARK, bevel=1.2)
    for y in (-100, 100):
        a.box((10, 6, 6), at=(-605 - pv[0], y, 70), col=C.GREY, bevel=1)
    for (x, w, h) in ((-1700, 40, 30), (-1640, 30, 40)):
        D.plate(a, (x - pv[0], 1977.5 - pv[1], 230), "-y", w, h, t=1.6, col=C.WOOD_DARK, screws=False)
        D.plate(a, (x - pv[0], 1975.8 - pv[1], 230), "-y", w - 6, h - 6, t=0.4, col=C.CREAM, screws=False)
        a.box((w * 0.6, 0.3, 1.2), at=(x - pv[0], 1975.2 - pv[1], 234), col=C.INK, bevel=0, jitter=0, wear=False)
    D.scuffs(a, (-1200 - pv[0], 0, 1.3), "+z", 1100, 900, n=16, col=C.shade(C.WOOD_DARK, 0.8))
    a.cyl(40, 0.4, at=(-1080 - pv[0], -60, 3.3), col=C.shade(C.CORAL, 0.8), sides=16, bevel=0, ao=False)
    cracks(a, (-1100 - pv[0], 1979 - pv[1], 380), "-y", a.rng, n=2)


@asset("SM_Office_Ceiling", FO,
       desc="Office ceiling with exposed wooden beams.",
       replaces=[OFFICE + " ceiling slab"], placements=lambda: [P((-1200, 1500, 0))], covers=rec((638, 638)),
       pivot="world-aligned, pivot (-1200, 1500, 0); underside at Z 460", view=(0.3, 0.5, -0.8))
def office_ceiling(a):
    pv = (-1200, 1500, 0)
    a.records(L.select("studio", lines=[(638, 638)]), pivot=pv, bevel=1.0)
    for x in range(-1700, -600, 200):
        a.box((22, 1000, 24), at=(x - pv[0], 0, 448), col=C.WOOD_DARK, bevel=3)
    # iron straps with bolts where the beams meet the walls, a ceiling rose for the light, sprinklers
    for x in range(-1700, -600, 200):
        for y in (-460, 460):
            a.box((26, 30, 26), at=(x - pv[0], y, 448), col=C.GREY_DARK, bevel=1)
            for dz in (-6, 6):
                D.screw(a, (x - pv[0] - 13.2, y, 448 + dz), "-x", r=1.2, col=C.GREY)
                D.screw(a, (x - pv[0] + 13.2, y, 448 + dz), "+x", r=1.2, col=C.GREY)
    a.cyl(14, 3, at=(-1150 - pv[0], 0, 458.5), col=C.CREAM, sides=16, bevel=1)
    sprinkler(a, (-900 - pv[0], -250, 460))
    smoke_detector(a, (-1400 - pv[0], 250, 460))


# ============================================================================== office desk

@asset("SM_Office_DirectorDesk", FO,
       desc="Chunky wooden director's desk with drawer pedestals, brass pulls, a green blotter, 'GOOD IDEAS LATER' pencil cup, a clapperboard and a yellow banker's lamp.",
       replaces=[OFFICE + " desk body + top, pencil cup, clapperboard, desk lamp"],
       placements=lambda: [P((-1050, 1500, 0))], covers=rec((641, 650)),
       pivot="desk centre on the floor (-1050, 1500, 0); drawers face +X, the director's chair stands at the +Y end",
       integration="AFTScriptBook sits at (-1050, 1480, 86): the blotter leaves that spot free. Keep the lamp PointLight.", view=(1, 0.3, 0.8))
def director_desk(a):
    a.box((166, 316, 7), at=(0, 0, 83), col=C.WOOD_DARK, bevel=2.5, rough=0.5)
    for sy in (-1, 1):
        a.box((140, 90, 78), at=(0, sy * 100, 39), col=C.WOOD, bevel=3, rough=0.7)
        for k in range(3):
            a.box((3, 78, 20), at=(71, sy * 100, 65 - k * 24), col=C.shade(C.WOOD, 1.1), bevel=1.5)
            a.box((3, 18, 4), at=(73.5, sy * 100, 65 - k * 24), col=C.BRASS, bevel=1, rough=0.3)
    a.box((140, 120, 12), at=(0, 0, 73), col=C.WOOD, bevel=2)
    a.box((6, 116, 60), at=(-65, 0, 42), col=C.shade(C.WOOD, 0.9), bevel=2)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(5, 4, at=(sx * 62, sy * 138, 2), col=C.WOOD_DARK, sides=8, bevel=1)
    # blotter + script book spot
    a.box((70, 110, 1), at=(5, -10, 87), col=C.GREEN_DARK, bevel=0.3)
    for sy in (-1, 1):
        a.box((72, 6, 1.6), at=(5, -10 + sy * 55, 87.3), col=C.WOOD_DARK, bevel=0.4)
    # pencil cup (code: cylinder 20x20x28 at (-30, -110, 100))
    a.cyl(10, 26, at=(-30, -110, 99), r_top=10.5, col=C.CREAM, sides=12, bevel=1)
    a.torus(10.2, 1, at=(-30, -110, 112), col=C.CORAL, major=12, minor=5)
    for k, (dx, dy, c) in enumerate(((-3, 2, C.RED), (3, -2, C.BLUE), (0, 4, C.YELLOW), (2, 3, C.TEAL))):
        a.cyl(0.9, 20, at=(-30 + dx, -110 + dy, 115), rot=(dx * 2, 0, dy * 2), col=c, sides=6)
    # clapperboard (code: ink box 50x16x40 yaw 20 at (30, 140, 110) + white stick)
    a.box((50, 6, 36), at=(30, 140, 106), rot=(0, 20, 0), col=C.INK, bevel=1.5)
    for k in range(4):
        a.box((48, 1, 2.5), at=(30, 140, 98 + k * 6), rot=(0, 20, 0), col=C.CREAM, bevel=0.2)
    a.box((52, 7, 7), at=(30, 140, 128), rot=(0, 20, 8), col=C.WHITE, bevel=1)
    for k in range(4):
        a.box((6, 7.5, 7.5), at=(30 - 18 + k * 12, 140, 128), rot=(0, 20, 8), col=C.INK, bevel=0.4)
    a.cyl(2, 8, at=(8, 132, 124), rot=(90, 20, 0), col=C.GREY, sides=6)
    # banker's lamp (code: yellow base, arm, cone shade at (-30, 120, ...))
    a.cyl(11, 6, at=(-30, 120, 90), col=C.YELLOW, sides=14, bevel=2, rough=0.4)
    a.tube([(-30, 120, 92), (-28, 118, 125), (-22, 112, 146)], 2.2, col=C.BRASS, sides=8, rough=0.3)
    a.lathe([(3, 12), (14, 2), (17, 0), (0.1, 0)], at=(-20, 110, 148), rot=(160, 0, 0), col=C.YELLOW, sides=14, rough=0.4)
    a.sphere(5, at=(-19, 108, 142), col=C.CREAM, segs=8, rings=6, glow=8)
    # coffee mug
    a.cyl(5, 9, at=(40, -80, 91.5), col=C.CORAL, sides=10, bevel=1)
    a.torus(3, 1, at=(46, -80, 92), rot=(90, 0, 0), col=C.CORAL, major=8, minor=4)
    # modesty-panel seams, drawer keyholes, brass corners on the top, lamp cable, a framed photo, loose pages
    D.seams(a, (-68, 0, 42), "-x", 116, 60, n=3, along="v", col=C.shade(C.WOOD, 0.7), width=0.8)
    for sy in (-1, 1):
        for k in range(3):
            a.cyl(0.9, 0.6, at=(73.2, sy * 100 + 12, 68 - k * 24), rot=(-90, 0, 0), col=C.BRASS, sides=6)
        D.seams(a, (-70, sy * 100, 39), "-x", 90, 78, n=2, along="u", col=C.shade(C.WOOD, 0.75), width=0.8)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((10, 10, 8), at=(sx * 79, sy * 154, 83), rot=(0, 45, 0), col=C.BRASS, bevel=1, rough=0.3)
    D.cable(a, [(-40, 120, 90), (-60, 130, 88), (-72, 132, 80), (-74, 132, 10), (-70, 140, 1.2)], r=0.7)
    a.box((3, 18, 14), at=(-50, 60, 94), rot=(12, 0, 0), col=C.BRASS, bevel=0.6, rough=0.3)
    a.box((0.4, 14, 10), at=(-48.4, 60, 94.4), rot=(12, 0, 0), col=C.SKY_BLUE, bevel=0.05, jitter=0)
    a.box((2, 4, 10), at=(-54, 60, 91), rot=(-30, 0, 0), col=C.BRASS, bevel=0.4)
    for k in range(3):
        a.box((21, 29, 0.6), at=(50 + k * 2, 60 - k * 3, 87 + k * 0.7), rot=(0, 8 - k * 9, 0), col=C.WHITE, bevel=0.1)


# ============================================================================== director's chair

@asset("SM_Office_DirectorsChair", FO,
       desc="Folding director's chair: crossed wooden legs, navy canvas seat and back, brass hinge bolts and armrests.",
       replaces=[OFFICE + " director's chair (legs, seat, back)"],
       placements=lambda: [P((-1050, 1720, 0))], covers=rec((653, 658)),
       pivot="chair centre on the floor (-1050, 1720, 0); back rest towards +Y",
       integration="'DIRECTOR' stays a TextRender on the backrest.", view=(-0.5, -1, 0.5))
def directors_chair(a):
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((5, 6, 96), at=(sx * 26, sy * 14, 44), rot=(0, 0, sy * 18), col=C.WOOD, bevel=1.4)
        a.cyl(2.5, 6, at=(sx * 29, 0, 44), rot=(0, 0, 90), col=C.BRASS, sides=8, rough=0.3)
        a.box((6, 70, 5), at=(sx * 30, 0, 4), col=C.WOOD, bevel=1.5)
        a.box((6, 6, 70), at=(sx * 30, 38, 90), col=C.WOOD, bevel=1.5)
        a.box((8, 62, 5), at=(sx * 30, 4, 84), col=C.WOOD_DARK, bevel=1.8)
    a.box((56, 62, 3), at=(0, 0, 60), col=C.NAVY, bevel=1.2, rough=0.9)
    for sy in (-1, 1):
        a.cyl(2.2, 58, at=(0, sy * 30, 60), rot=(0, 0, 90), col=C.WOOD, sides=8)
    a.box((58, 3, 38), at=(0, 40, 108), col=C.NAVY, bevel=1.2, rough=0.9)
    a.box((60, 4, 4), at=(0, 40, 90), col=C.YELLOW, bevel=1)
    # canvas stitching, brass bolts at the cross, leg caps, a script tucked into a side pocket
    for sy in (-1, 1):
        a.box((56, 0.6, 0.4), at=(0, sy * 28, 61.8), col=C.shade(C.NAVY, 1.5), bevel=0, jitter=0, wear=False)
    a.box((58, 0.4, 0.6), at=(0, 38.2, 124), col=C.shade(C.NAVY, 1.5), bevel=0, jitter=0, wear=False)
    a.box((58, 0.4, 0.6), at=(0, 38.2, 92.6), col=C.shade(C.NAVY, 1.5), bevel=0, jitter=0, wear=False)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(3.2, 2, at=(sx * 30, sy * 35, 1), col=C.RUBBER, sides=8, bevel=0.4)
        for y in (-20, 20):
            D.screw(a, (sx * 33, y, 84), "+x" if sx > 0 else "-x", r=0.9, col=C.BRASS)
    a.box((3, 22, 18), at=(-33, -6, 70), col=C.NAVY, bevel=0.6)
    a.box((2, 20, 24), at=(-33, -6, 76), rot=(0, 0, 4), col=C.CREAM, bevel=0.2)


# ============================================================================== bookshelf

@asset("SM_Office_Bookshelf", FO,
       desc="Tall wooden bookcase with three shelves of chunky books, leaning volumes, an award statuette and a film can.",
       replaces=[OFFICE + " bookshelf body + 21 books"],
       placements=lambda: [P((-1600, 1950, 0))], covers=rec((661, 667)),
       pivot="bookcase centre at floor (-1600, 1950, 0); open side faces -Y into the office", view=(0.5, -1, 0.4))
def bookshelf(a):
    W, DP, H = 240, 50, 220
    a.box((W, 6, H), at=(0, DP / 2 - 3, H / 2), col=C.WOOD_DARK, bevel=2)
    for sx in (-1, 1):
        a.box((8, DP, H), at=(sx * (W / 2 - 4), 0, H / 2), col=C.WOOD_DARK, bevel=2.5)
    for z in (4, 32, 102, 172, H - 4):
        a.box((W - 10, DP - 4, 6 if z not in (4, H - 4) else 8), at=(0, -1, z), col=C.WOOD if z not in (4, H - 4) else C.WOOD_DARK, bevel=1.5)
    a.box((W + 10, DP + 8, 10), at=(0, -2, H + 3), col=C.WOOD_DARK, bevel=3)
    rng = a.rng
    cols = [C.CORAL, C.TEAL, C.YELLOW, C.NAVY, C.MAGENTA, C.CREAM, C.GREEN]
    # books stand along X (the case is 240 wide), spines face -Y into the office
    for r, zb in enumerate((35, 105, 175)):
        x = -110
        k = 0
        while x < 105:
            h = rng.uniform(38, 58)
            w = rng.uniform(6, 11)
            d = rng.uniform(30, 38)
            c = cols[(k * 3 + r) % len(cols)]
            tilt = -14 if (r == 1 and k == 9) else 0
            a.box((w, d, h), at=(x + w / 2, 4 - d / 2 + 13, zb + h / 2), rot=(tilt, 0, 0), col=c, bevel=1.2, segs=1, rough=0.7)
            a.box((w * 0.9, 0.6, 3), at=(x + w / 2, 4 - d + 13 - 0.3, zb + h * 0.75), col=C.CREAM if c != C.CREAM else C.NAVY, bevel=0.1, ao=False)
            x += w + 0.8
            k += 1
            if r == 2 and k == 8:
                a.cyl(5, 6, at=(x + 10, 0, zb + 3), col=C.CHARCOAL, sides=10, bevel=1)
                a.cyl(2, 14, at=(x + 10, 0, zb + 13), col=C.BRASS, sides=8, rough=0.2)
                a.sphere(3.6, at=(x + 10, 0, zb + 23), col=C.BRASS, segs=8, rings=6, rough=0.2)
                x += 22
    a.cyl(14, 5, at=(90, -4, 40), col=C.GREY, sides=14, bevel=1.2, rough=0.4)
    a.cyl(14, 5, at=(90, -4, 45), col=C.GREY_DARK, sides=14, bevel=1.2, rough=0.4)
    # back-panel planks, brass label holders on the shelf edges, book-ends and a lying stack, feet
    D.seams(a, (0, DP / 2 - 6.2, H / 2), "-y", W - 16, H - 16, n=5, along="v", col=C.shade(C.WOOD_DARK, 0.7), width=0.8)
    for z in (32, 102, 172):
        for x in (-80, 0, 80):
            a.box((8, 0.8, 3), at=(x, -DP / 2 - 0.2, z), col=C.BRASS, bevel=0.2, rough=0.3)
            a.box((6, 0.3, 1.6), at=(x, -DP / 2 - 0.7, z), col=C.CREAM, bevel=0, jitter=0, wear=False)
    for (x, z) in ((104, 35), (-108, 175)):
        a.box((3, 16, 16), at=(x, -6, z + 8), col=C.GREY_DARK, bevel=0.6)
    for k in range(3):
        a.box((30 - k * 3, 22 - k * 2, 5), at=(-70 + k, -8, 175.5 + 2.5 + k * 5), rot=(0, 4 * k - 4, 0), col=[C.NAVY, C.CORAL, C.CREAM][k], bevel=1)
    for sx in (-1, 1):
        D.feet(a, [(sx * (W / 2 - 8), -DP / 2 + 6, -1.6), (sx * (W / 2 - 8), DP / 2 - 6, -1.6)], r=3, h=1.6, col=C.WOOD_DARK)


# ============================================================================== sofa

@asset("SM_Office_Sofa", FO,
       desc="Coral two-seat sofa: pillowed seat cushions, rolled back, tapered legs and a cream throw pillow.",
       replaces=[OFFICE + " couch body, backrest, pillow"],
       placements=lambda: [P((-1650, 1300, 0))], covers=rec((670, 672)),
       pivot="sofa centre on the floor (-1650, 1300, 0); seat faces +X, back towards -X", view=(1, 0.3, 0.45))
def sofa(a):
    a.box((96, 256, 32), at=(0, 0, 26), col=C.CORAL, bevel=6, rough=0.9)
    for sy in (-1, 1):
        c = bm_box(70, 118, 16, 6, 2)
        bulge(c, 3, axis=2)
        a.add(c, xf((8, sy * 62, 49)), C.shade(C.CORAL, 1.05), rough=0.9)
    b = bm_box(22, 250, 64, 8, 2)
    bulge(b, 3, axis=0)
    a.add(b, xf((-40, 0, 72)), C.CORAL_DARK, rough=0.9)
    for sy in (-1, 1):
        a.box((100, 20, 52), at=(0, sy * 130, 40), col=C.CORAL_DARK, bevel=8, rough=0.9)
        for sx in (-1, 1):
            a.cyl(3.2, 12, at=(sx * 40, sy * 120, 5), r_top=4, col=C.WOOD_DARK, sides=8)
    p = bm_box(18, 46, 38, 7, 2)
    bulge(p, 3, axis=0)
    a.add(p, xf((10, -60, 72), (0, 0, 10)), C.CREAM, rough=0.9)
    # piping along the cushions and arms, tufting buttons on the back, a folded throw over the right arm
    for sy in (-1, 1):
        a.cyl(1.2, 118, at=(43, sy * 62, 50), rot=(0, 0, 90), col=C.CORAL_DARK, sides=6)
        a.torus(10, 1.2, at=(0, sy * 140.5, 56), rot=(0, 0, 90), col=C.CORAL, major=16, minor=4)
    for k in range(5):
        for j in range(2):
            a.sphere(1.8, at=(-28.5, -100 + k * 50, 66 + j * 20), col=C.shade(C.CORAL_DARK, 0.85), segs=6, rings=4)
    a.box((70, 30, 4), at=(4, 132, 67), rot=(0, 0, 0), col=C.TEAL, bevel=1.8, rough=0.95)
    a.box((4, 30, 30), at=(38, 132, 52), col=C.TEAL, bevel=1.8, rough=0.95)
    for k in range(5):
        a.box((1.6, 2, 5), at=(40, 120 + k * 6, 36), col=C.CREAM, bevel=0.3)


# ============================================================================== wardrobe shell

@asset("SM_Wardrobe_Shell", FW,
       desc="Wardrobe & make-up room: cream floor, magenta rug with a clapper motif, blush walls with magenta wainscot on both long walls, skirting, chair rail and vents.",
       replaces=[WARD + " floor, rug, south wall, wainscots"],
       placements=lambda: [P((-1200, -1300, 0))], covers=rec((685, 687), (689, 690)),
       pivot="world-aligned, pivot (-1200, -1300, 0) = wardrobe centre at floor level",
       integration="Ceiling is SM_Wardrobe_Ceiling; the accessory/mirror wall is the AFTAccessoryStand actor.", view=(0.3, 0.9, 1.2))
def wardrobe_shell(a):
    pv = (-1200, -1300, 0)
    a.records(L.select("studio", lines=[(685, 687), (689, 690)]), pivot=pv, bevel=1.0)
    for (y, face) in ((-1580, 1), (-1020, -1)):
        moulding_x(a, -1800 - pv[0], -600 - pv[0], y - pv[1], 6, 2.5, 12, C.MAGENTA, face=face)
        moulding_x(a, -1800 - pv[0], -600 - pv[0], y - pv[1], 121, 3, 7, C.CREAM, face=face)
        moulding_x(a, -1800 - pv[0], -600 - pv[0], y - pv[1], 450, 4, 14, C.CREAM, face=face)
        for x in range(-1760, -640, 120):
            a.box((100, 2, 80), at=(x + 50 - pv[0], y - pv[1] + face * 6, 60), col=C.shade(C.MAGENTA, 1.1), bevel=1.2, segs=1)
    # rug border + clapper motif
    a.box((880, 340, 0.6), at=(0, 0, 1.6), col=C.shade(C.MAGENTA, 0.8), bevel=0.2, ao=False)
    a.box((860, 320, 0.6), at=(0, 0, 1.9), col=C.MAGENTA, bevel=0.2, ao=False)
    a.box((120, 90, 0.6), at=(0, 0, 2.3), col=C.INK, bevel=0.3, ao=False)
    for k in range(4):
        a.box((120, 10, 0.6), at=(0, -35 + k * 22, 2.6), col=C.CREAM, bevel=0.2, ao=False)
    vent(a, (-700 - pv[0], -1578.5 - pv[1], 400), rot=(0, 90, 0))
    # detail pass: sockets, a switch, extinguisher, coat hooks with a scarf and a hat, full-length mirror,
    # floor scuffs, cracks
    for x in (-1700, -1200, -800):
        outlet(a, (x - pv[0], -1577.5 - pv[1], 30), "+y")
        outlet(a, (x - pv[0], -1022.5 - pv[1], 30), "-y")
    switch(a, (-640 - pv[0], -1022.5 - pv[1], 135), "-y")
    extinguisher(a, (-1760 - pv[0], -1022.5 - pv[1], 95), "-y")
    a.box((120, 3, 10), at=(-1550 - pv[0], -1021 - pv[1], 180), col=C.WOOD, bevel=1.5)
    for k in range(4):
        x = -1595 + k * 30 - pv[0]
        a.tube([(x, -1022.5 - pv[1], 180), (x, -1030 - pv[1], 178), (x, -1033 - pv[1], 184)], 0.9, col=C.BRASS, sides=6, rough=0.3)
    a.box((4, 12, 70), at=(-1565 - pv[0], -1030 - pv[1], 145), rot=(0, 0, 4), col=C.CORAL, bevel=1.5, rough=0.95)
    a.sphere(12, at=(-1505 - pv[0], -1036 - pv[1], 180), rz=7, col=C.NAVY, segs=10, rings=6)
    a.cyl(16, 1.4, at=(-1505 - pv[0], -1036 - pv[1], 174), col=C.NAVY, sides=14, bevel=0.4)
    D.plate(a, (-1000 - pv[0], -1022.5 - pv[1], 110), "-y", 60, 170, t=2.4, col=C.CREAM, screws=False)
    a.box((52, 1, 160), at=(-1000 - pv[0], -1024.2 - pv[1], 110), col=C.GLASS, mat="glass")
    D.scuffs(a, (0, 0, 1.3), "+z", 1100, 520, n=14, col=C.shade(C.CREAM_DARK, 0.8))
    cracks(a, (-1300 - pv[0], -1578 - pv[1], 400), "+y", a.rng, n=2)


@asset("SM_Wardrobe_Ceiling", FW,
       desc="Wardrobe ceiling with a pink cove strip.", replaces=[WARD + " ceiling slab"],
       placements=lambda: [P((-1200, -1300, 0))], covers=rec((688, 688)),
       pivot="world-aligned, pivot (-1200, -1300, 0); underside at Z 460", view=(0.3, 0.5, -0.8))
def wardrobe_ceiling(a):
    pv = (-1200, -1300, 0)
    a.records(L.select("studio", lines=[(688, 688)]), pivot=pv, bevel=1.0)
    a.box((1100, 500, 6), at=(0, 0, 457), col=C.WARDROBE_WALL, bevel=2)
    # spot track with three glowing spots, sprinklers, smoke detector
    a.box((600, 6, 4), at=(0, 0, 452), col=C.CHARCOAL, bevel=1)
    for x in (-200, 0, 200):
        a.box((6, 6, 6), at=(x, 0, 447), col=C.CHARCOAL, bevel=1)
        a.cyl(6, 14, at=(x + 6, 0, 438), rot=(-30, 0, 0), col=C.CHARCOAL, sides=10, bevel=1)
        a.cyl(4.6, 0.6, at=(x + 9.6, 0, 432), rot=(-30, 0, 0), col=C.WINDOW_WARM, sides=10, glow=6)
    for x in (-400, 400):
        sprinkler(a, (x, -150, 460))
    smoke_detector(a, (-400, 150, 460))


# ============================================================================== lockers + bench

@asset("SM_Wardrobe_LockerBank", FW,
       desc="Five alternating coral/teal lockers with rounded door edges, louvred vents, nameplates, handles, padlock and a sticker or two.",
       replaces=[WARD + " five lockers with vents, name slots and handles"],
       placements=lambda: [P((-780, -1560, 0))], covers=rec((694, 702)),
       pivot="bank centre on the floor (-780, -1560, 0); doors face +Y", view=(0.4, 1, 0.45))
def locker_bank(a):
    pv = (-780, -1560, 0)
    for i in range(5):
        x = -900 + i * 60 - pv[0]
        col = C.TEAL if i % 2 else C.CORAL
        a.box((56, 58, 196), at=(x, -1, 98), col=C.shade(col, 0.85), bevel=2)
        a.box((50, 3, 186), at=(x, 29, 100), col=col, bevel=2.2)
        for v in range(4):
            a.box((26, 2.4, 2.4), at=(x, 31, 163 + v * 7), col=C.shade(col, 0.6), bevel=0.8)
            a.box((26, 2.4, 2.4), at=(x, 31, 26 + v * 7), col=C.shade(col, 0.6), bevel=0.8)
        a.box((22, 2, 7), at=(x, 31, 148), col=C.CREAM, bevel=0.8)
        a.box((20, 1, 5), at=(x, 32, 148), col=C.CHARCOAL, bevel=0.3)
        a.box((5, 5, 20), at=(x - 17, 33, 93), col=C.CREAM_DARK, bevel=1.5, rough=0.4)
        if i == 2:
            a.box((7, 3, 8), at=(x - 17, 36, 84), col=C.BRASS, bevel=1.5, rough=0.3)
            a.torus(3, 0.8, at=(x - 17, 36, 90), rot=(90, 0, 0), col=C.CHROME, major=8, minor=4)
        if i in (1, 4):
            a.cyl(6, 0.6, at=(x + 8, 32, 120), rot=(0, 0, 90), col=[C.YELLOW, C.MAGENTA][i % 2], sides=10, bevel=0)
    a.box((306, 64, 8), at=(0, 0, 200), col=C.CREAM_DARK, bevel=2)
    a.box((306, 60, 8), at=(0, 1, 4), col=C.CHARCOAL, bevel=1.5)
    # stencilled numbers, hinges, shoulder-height scratches, clutter on top (a box and a helmet)
    for i in range(5):
        x = -900 + i * 60 - pv[0]
        D.stencil_number(a, (x + 10, 30.6, 175), "+y", str(i + 1), 9, col=C.CREAM)
        for z in (40, 160):
            D.hinge(a, (x + 25.5, 31, z), "z", 10, r=1.0, col=C.GREY)
        D.scuffs(a, (x, 31, 120), "+y", 44, 60, n=3)
    a.box((44, 36, 12), at=(-60, 0, 210), rot=(0, 8, 0), col=C.CREAM_DARK, bevel=2)
    D.tape(a, (-60, 0, 216), "+z", 8, 38, deg=8, col=C.SAND)
    a.box((40, 28, 6), at=(70, 4, 207), rot=(0, -6, 0), col=C.CYAN, bevel=2.4, rough=0.95)
    D.seams(a, (0, -32, 100), "-y", 300, 190, n=4, along="v", col=C.shade(C.CORAL, 0.6), width=0.8)
    D.label(a, (100, -32, 160), "-y", 20, 12, lines=3)


@asset("SM_Wardrobe_Bench", FW,
       desc="Slatted locker-room bench on chunky steel legs with a stray towel and sneakers.",
       replaces=[WARD + " wooden bench"],
       placements=lambda: [P((-1400, -1150, 0))], covers=rec((704, 704)),
       pivot="bench centre on the floor (-1400, -1150, 0); long axis X", view=(0.4, 1, 0.5))
def wardrobe_bench(a):
    for k in range(4):
        a.box((240, 13, 5), at=(0, -21 + k * 14, 47.5), col=C.shade(C.WOOD, a.rng.uniform(0.9, 1.08)), bevel=1.5, segs=1)
    for sx in (-1, 1):
        a.box((6, 50, 6), at=(sx * 95, 0, 42), col=C.GREY_DARK, bevel=1.5)
        for sy in (-1, 1):
            a.box((6, 6, 42), at=(sx * 95, sy * 22, 21), col=C.GREY_DARK, bevel=1.5)
            a.box((8, 8, 3), at=(sx * 95, sy * 22, 1.5), col=C.RUBBER, bevel=1)
    a.box((50, 40, 5), at=(-60, 0, 52), rot=(0, 8, 0), col=C.CYAN, bevel=2.2)
    a.box((50, 12, 7), at=(-60, 14, 53), rot=(0, 8, 0), col=C.shade(C.CYAN, 1.1), bevel=2.5)
    for k in range(2):
        a.box((28, 11, 9), at=(60 + k * 2, -18 + k * 13, 4.5), rot=(0, 8 * k, 0), col=C.WHITE, bevel=3)
        a.box((29, 12, 2.5), at=(60 + k * 2, -18 + k * 13, 1.2), rot=(0, 8 * k, 0), col=C.RED, bevel=1)
    # bolts through the slats, a dropped sock
    for sx in (-1, 1):
        for k in range(4):
            D.screw(a, (sx * 95, -21 + k * 14, 50), "+z", r=0.7)
    a.box((14, 6, 2), at=(20, 30, 1), rot=(0, 30, 0), col=C.WHITE, bevel=0.8)
    a.box((6, 6, 2), at=(26, 36, 1), rot=(0, 30, 0), col=C.WHITE, bevel=0.8)
