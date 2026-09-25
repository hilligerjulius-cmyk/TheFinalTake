"""Studio lobby: room shell, ceiling, office door surround, reception, bench, STAGE 4 header, pendant lamps."""
import math

from ftb import detail as D
from ftb import layout as L
from ftb import palette as C
from ftb.registry import P, asset

F = "Studio/Lobby"
SHELL = "AFTStudioShell::BuildLobby()"


def rec(*ranges, where=None):
    return lambda: L.select("studio", lines=list(ranges), where=where)


def moulding_x(a, x0, x1, y, z, depth, height, col, face=1, **kw):
    """Profiled wall trim running along X on a wall face at Y=y (face=+1: trim sits on +Y side)."""
    a.box((x1 - x0, depth, height), at=((x0 + x1) / 2, y + face * depth / 2, z), col=col, bevel=min(1.2, depth * 0.4), segs=1, **kw)
    a.box((x1 - x0, depth * 0.5, height * 0.35), at=((x0 + x1) / 2, y + face * depth * 1.2, z + height * 0.3), col=C.shade(col, 1.08), bevel=0.5, segs=1, **kw)


def moulding_y(a, y0, y1, x, z, depth, height, col, face=1, **kw):
    a.box((depth, y1 - y0, height), at=(x + face * depth / 2, (y0 + y1) / 2, z), col=col, bevel=min(1.2, depth * 0.4), segs=1, **kw)
    a.box((depth * 0.5, y1 - y0, height * 0.35), at=(x + face * depth * 1.2, (y0 + y1) / 2, z + height * 0.3), col=C.shade(col, 1.08), bevel=0.5, segs=1, **kw)


def vent(a, at, rot=(0, 0, 0), w=60, h=30, col=C.CREAM_DARK):
    a.box((3, w, h), at=at, rot=rot, col=col, bevel=1.0)
    from ftb.core import ue_rot
    from mathutils import Vector
    R = ue_rot(*rot)
    for k in range(5):
        off = R @ Vector((-1.8, 0, -h * 0.35 + k * h * 0.175))
        a.box((2, w - 8, 2.2), at=(at[0] + off.x, at[1] + off.y, at[2] + off.z), rot=rot, col=C.shade(col, 0.7), bevel=0.4)


def outlet(a, at, normal, col=C.CREAM, double=True):
    """Wall socket plate with one or two sockets (normal: the room side of the wall)."""
    D.plate(a, at, normal, 16 if double else 9, 9, t=0.8, col=col, screws=False)
    for du in ((-4, 4) if double else (0,)):
        p = D.on(at, normal, du, 0, 0.8)
        a.cyl(2.6, 0.5, at=D.on(p, normal, dn=0.25), rot=D.NORMAL_ROT[normal], col=C.shade(col, 0.9), sides=10, bevel=0.1)
        for dv in (-0.9, 0.9):
            a.cyl(0.35, 0.3, at=D.on(p, normal, dv * 0.9, 0, 0.55), rot=D.NORMAL_ROT[normal], col=C.INK, sides=5, bevel=0, jitter=0, wear=False)


def switch(a, at, normal, col=C.CREAM):
    D.plate(a, at, normal, 8, 12, t=0.8, col=col, screws=False)
    a.box(D._size(normal, 2.4, 4, 1.2), at=D.on(at, normal, 0, 0.6, 1.4), col=C.shade(col, 0.85), bevel=0.3)


def extinguisher(a, at, normal):
    """Red extinguisher on a wall bracket with a sign above (at = bracket on the wall, ~1 m up)."""
    n = D.frame(normal)[0]
    base = D.on(at, normal, dn=9)
    a.cyl(8, 46, at=base, col=C.RED, sides=12, bevel=3, rough=0.4)
    a.cyl(3.4, 6, at=base + D.Vector((0, 0, 26)), col=C.GREY_DARK, sides=8, bevel=0.8)
    a.box(D._size(normal, 3, 2, 10), at=base + D.Vector((0, 0, 30)) + n * 2, col=C.GREY_DARK, bevel=0.5)
    D.cable(a, [base + D.Vector((0, 0, 28)) - n * 2, base + D.Vector((0, 0, 20)) - n * 7.5, base + D.Vector((0, 0, -10)) - n * 8.5], r=1.0)
    a.box(D._size(normal, 12, 26, 0.8), at=base + D.Vector((0, 0, 4)), col=C.WHITE, bevel=0.2, rough=0.4)
    D.bracket(a, D.on(at, normal, 0, 10), normal, w=8, h=8, col=C.GREY_DARK)
    D.plate(a, D.on(at, normal, 0, 58), normal, 22, 22, t=0.6, col=C.RED, screws=False)
    a.box(D._size(normal, 10, 12, 0.3), at=D.on(at, normal, 0, 58, 0.8), col=C.WHITE, bevel=0, jitter=0, wear=False)


def exit_sign(a, at, normal):
    D.plate(a, at, normal, 40, 16, t=4, col=C.GREY_DARK, screws=False)
    a.box(D._size(normal, 36, 12, 0.6), at=D.on(at, normal, dn=4.2), col=C.GREEN, glow=3.0)
    a.box(D._size(normal, 8, 8, 0.3), at=D.on(at, normal, -12, 0, 4.7), col=C.WHITE, bevel=0, jitter=0, wear=False)


def cracks(a, at, normal, rng, n=3):
    """Hairline plaster cracks: short dark zig-zag strokes."""
    for _ in range(n):
        du, dv = rng.uniform(-40, 40), rng.uniform(-30, 30)
        ang = rng.uniform(-70, 70)
        for k in range(3):
            a.box(D._size(normal, rng.uniform(5, 11), 0.35, 0.12), at=D.on(at, normal, du + k * 6, dv + k * rng.uniform(-3, 3), 0.06),
                  rot=D._rot_in_plane(normal, ang + rng.uniform(-35, 35)), col=C.shade(C.INK, 2.5), bevel=0, jitter=0, wear=False)


def sprinkler(a, at):
    a.cyl(3, 1, at=(at[0], at[1], at[2] - 0.5), col=C.CHROME, sides=10, bevel=0.3, rough=0.3)
    a.cyl(0.8, 4, at=(at[0], at[1], at[2] - 3), col=C.CHROME, sides=6)
    a.cyl(2.2, 0.8, at=(at[0], at[1], at[2] - 5.2), col=C.RED, sides=8, bevel=0.2)


def smoke_detector(a, at):
    a.cyl(7, 3, at=(at[0], at[1], at[2] - 1.5), col=C.WHITE, sides=14, bevel=1, rough=0.5)
    a.cyl(1, 0.6, at=(at[0] + 3, at[1], at[2] - 3.2), col=C.RED, sides=6, glow=1.5)


# ============================================================================== shell

@asset("SM_Lobby_Shell", F,
       desc="Lobby floor with bevelled checker tiles and the brass-edged red carpet, cream walls with teal wainscot, chair rail, skirting, crown moulding, air vents and a wall clock.",
       replaces=[SHELL + " floor, checker tiles, carpet + brass edges, walls, wainscot, RECEPTION wall board (FTStudioShell.cpp:585, turned to face +Y like its text - code finding)"],
       placements=lambda: [P((-1200, 0, 0))], covers=rec((552, 573), (585, 585)),
       pivot="world-aligned, pivot (-1200, 0, 0) = lobby centre at floor level",
       integration="Walls/floor keep the code collision (CubeSolid); the ceiling is SM_Lobby_Ceiling.", view=(0.2, 0.9, 1.3))
def lobby_shell(a):
    pv = (-1200, 0, 0)
    a.records(L.select("studio", lines=[(552, 573), (585, 585)]), pivot=pv, bevel=1.0)
    # skirting + chair rail + crown on the lobby faces of the side walls
    for (x0, x1) in ((-1800, -600),):
        moulding_x(a, x0 - pv[0], x1 - pv[0], -980, 6, 2.5, 12, C.TEAL_DARK, face=1)
        moulding_x(a, x0 - pv[0], x1 - pv[0], -980, 121, 3, 7, C.CREAM, face=1)
        moulding_x(a, x0 - pv[0], x1 - pv[0], -980, 548, 4, 16, C.CREAM, face=1)
    for (x0, x1) in ((-1800, -1380), (-1020, -600)):
        moulding_x(a, x0 - pv[0], x1 - pv[0], 980, 6, 2.5, 12, C.TEAL_DARK, face=-1)
        moulding_x(a, x0 - pv[0], x1 - pv[0], 980, 121, 3, 7, C.CREAM, face=-1)
    moulding_x(a, -1800 - pv[0], -600 - pv[0], 980, 548, 4, 16, C.CREAM, face=-1)
    # wainscot panels (raised rectangles) on the teal band
    for x in range(-1760, -640, 120):
        a.box((100, 2, 80), at=(x + 50 - pv[0], -977 + 3, 60), col=C.shade(C.TEAL, 1.12), bevel=1.2, segs=1)
    for x in list(range(-1760, -1400, 120)) + list(range(-1000, -640, 120)):
        a.box((100, 2, 80), at=(x + 50 - pv[0], 977 - 3, 60), col=C.shade(C.TEAL, 1.12), bevel=1.2, segs=1)
    # reception wall board: frame + brass film-reel emblems
    a.box((6, 420, 110), at=(-1300 - pv[0], -971, 300), col=C.CREAM, bevel=2)
    for y in (-176, 176):
        a.cyl(24, 4, at=(-1300 - pv[0] + y, -964, 300), rot=(0, 0, 90), col=C.BRASS, sides=14, bevel=1, rough=0.3)
        for k in range(5):
            ang = math.radians(k * 72)
            a.cyl(4.5, 5, at=(-1300 - pv[0] + y + 12 * math.cos(ang), -962, 300 + 12 * math.sin(ang)), rot=(0, 0, 90), col=C.NAVY, sides=8, bevel=0)
    # clock above the office door side
    a.cyl(28, 6, at=(-1560 - pv[0], 975, 390), rot=(0, 0, 90), col=C.CREAM, sides=18, bevel=2)
    a.torus(28, 3, at=(-1560 - pv[0], 972, 390), rot=(0, 0, 90), col=C.BRASS, major=18, minor=6, rough=0.3)
    a.box((1.5, 3, 18), at=(-1560 - pv[0], 971, 396), rot=(0, 0, 0), col=C.INK, bevel=0.3)
    a.box((1.5, 14, 3), at=(-1554 - pv[0], 971, 390), col=C.INK, bevel=0.3)
    # air vents high on the walls
    for x in (-1650, -1000):
        vent(a, (x - pv[0], -978.5, 470), rot=(0, 90, 0))
    vent(a, (-700 - pv[0], 978.5, 470), rot=(0, -90, 0))
    # carpet end caps (brass nosing where the runner meets the tiles)
    for x in (-1800, -600):
        a.box((6, 312, 3), at=(x + (3 if x < -1000 else -3) - pv[0], 0, 2), col=C.BRASS, bevel=1, rough=0.3)
    # detail pass: sockets on the skirting, light switches by the doors, an extinguisher + EXIT signs,
    # thermostat, surface conduit with a junction box, hairline cracks and scuffs along the skirting
    for x in (-1700, -1400, -1100, -800):
        outlet(a, (x - pv[0], -977.5, 30), "+y")
    for x in (-1700, -800):
        outlet(a, (x - pv[0], 977.5, 30), "-y")
    switch(a, (-1410 - pv[0], 977.5, 135), "-y")
    switch(a, (-640 - pv[0], -977.5, 135), "+y")
    extinguisher(a, (-660 - pv[0], 977.5, 95), "-y")
    exit_sign(a, (-1200 - pv[0], 977.5, 520), "-y")
    exit_sign(a, (-1200 - pv[0], -977.5, 520), "+y")
    D.plate(a, (-1480 - pv[0], -977.5, 150), "+y", 9, 12, t=1.6, col=C.CREAM)
    a.box((6, 0.4, 3), at=(-1480 - pv[0], -975.6, 152), col=C.hex_rgb(0x2A9D5B), glow=1.0)
    a.box((2, 3, 250), at=(-1100 - pv[0], -976.5, 330), col=C.GREY, bevel=0.6)
    a.box((12, 4, 12), at=(-1100 - pv[0], -976, 460), col=C.GREY, bevel=1)
    D.screws_rect(a, (-1100 - pv[0], -974, 460), "+y", 12, 12, inset=1.5, r=0.5)
    for z in (250, 350, 420):
        a.box((4, 2.4, 2), at=(-1100 - pv[0], -976, z), col=C.GREY_DARK, bevel=0.4)
    cracks(a, (-1500 - pv[0], -979, 420), "+y", a.rng, n=2)
    cracks(a, (-900 - pv[0], 979, 380), "-y", a.rng, n=2)
    D.scuffs(a, (-1200 - pv[0], -977, 18), "+y", 1100, 12, n=14)
    D.scuffs(a, (-1200 - pv[0], 977, 18), "-y", 1100, 12, n=10)


@asset("SM_Lobby_Ceiling", F,
       desc="Lobby ceiling slab with a coffered beam grid and a central art-deco medallion.",
       replaces=[SHELL + " ceiling slab"],
       placements=lambda: [P((-1200, 0, 0))], covers=rec((574, 574)),
       pivot="world-aligned, pivot (-1200, 0, 0); slab underside at Z 560", view=(0.3, 0.5, -0.8))
def lobby_ceiling(a):
    pv = (-1200, 0, 0)
    a.records(L.select("studio", lines=[(574, 574)]), pivot=pv, bevel=1.0)
    for x in (-1500, -1200, -900):
        a.box((24, 2000, 18), at=(x - pv[0], 0, 551), col=C.shade(C.CEILING, 0.93), bevel=3)
    for y in (-500, 0, 500):
        a.box((1200, 24, 18), at=(0, y, 551), col=C.shade(C.CEILING, 0.93), bevel=3)
    a.cyl(90, 6, at=(0, 0, 557), col=C.CREAM, sides=24, bevel=2)
    for k in range(12):
        ang = k * 30
        a.box((70, 8, 3), at=(math.cos(math.radians(ang)) * 60, math.sin(math.radians(ang)) * 60, 553), rot=(0, ang, 0), col=C.BRASS, bevel=1, rough=0.3)
    # sprinklers and smoke detectors in the coffers, a return-air grille and an access hatch
    for x in (-1350, -1050):
        for y in (-750, -250, 250, 750):
            sprinkler(a, (x - pv[0], y, 560))
    smoke_detector(a, (-1350 - pv[0], 0, 560))
    smoke_detector(a, (-1050 - pv[0], 0, 560))
    D.vent(a, (-1650 - pv[0], -750, 560), "-z", 80, 40, slats=6, col=C.CREAM_DARK)
    D.border(a, (-750 - pv[0], 750, 560), "-z", 70, 70, bar=2.5, t=0.8, col=C.shade(C.CEILING, 0.85))
    D.screws_rect(a, (-750 - pv[0], 750, 559.2), "-z", 70, 70, inset=3, r=0.8)


# ============================================================================== office door surround

@asset("SM_Lobby_OfficeDoorSurround", F,
       desc="Coral art-deco door surround for the Director's Office with stepped capitals, keystone and a framed navy name board.",
       replaces=[SHELL + " office door frame posts, header and sign board"],
       placements=lambda: [P((-1200, 1000, 0))], covers=rec((576, 579)),
       pivot="door opening centre on the lobby wall (-1200, 1000, 0); lobby side is -Y",
       integration="AFTDoor 'Door_Office' keeps its own sliding panel and frame; 'DIRECTOR'S OFFICE' stays a TextRender.", view=(0.3, -1, 0.3))
def office_door_surround(a):
    pv = (-1200, 1000, 0)
    a.records(L.select("studio", lines=[(576, 579)]), pivot=pv, bevel=3)
    for x in (-165, 165):
        for k in range(3):
            a.box((6, 64, 10 - k * 2), at=(x, -2, 290 - k * 9), col=C.CORAL_DARK, bevel=2)
        a.box((40, 66, 16), at=(x, 0, 8), col=C.CORAL_DARK, bevel=3)
        for k in range(3):
            a.box((4, 4, 250), at=(x - 8 + k * 8, -31, 150), col=C.shade(C.CORAL, 1.15), bevel=1.2)
    a.poly([(-18, 0), (18, 0), (12, 28), (-12, 28)], 8, at=(0, -32, 300), rot=(0, 90, 0), col=C.YELLOW, bevel=1.5, glow=0.5)
    # the navy name board comes from the code record (line 579); brass frame behind it
    a.box((310, 4, 80), at=(0, -33, 380), col=C.BRASS, bevel=2, rough=0.3)
    # name-board screws, sunburst rays on the capitals, brass kick caps on the plinths
    D.screws_rect(a, (0, -35, 380), "-y", 310, 80, inset=4, r=1.2, col=C.BRASS)
    for x in (-165, 165):
        for k in range(5):
            ang = -60 + k * 30
            a.box((1.2, 1, 22), at=(x + 8 * D.math.sin(D.math.radians(ang)), -34, 262 + 8 * D.math.cos(D.math.radians(ang))),
                  rot=(0, 0, ang), col=C.YELLOW, bevel=0.2)
        a.box((42, 1.6, 6), at=(x, -33.8, 13), col=C.BRASS, bevel=0.5, rough=0.3)


# ============================================================================== reception desk

@asset("SM_Lobby_ReceptionDesk", F,
       desc="Curved-front coral reception desk with cream fluting, rounded cream top, a retro CRT, desk bell, rotary phone, papers and a pen cup.",
       replaces=[SHELL + " reception desk body, top and monitor box"],
       placements=lambda: [P((-1300, -700, 0))], covers=rec((582, 584)),
       pivot="desk centre on the floor (-1300, -700, 0); visitors stand on the +Y side",
       view=(0.6, 1, 0.55))
def reception_desk(a):
    # body with a bowed front (+Y) and fluting
    prof = [(-150, -55)] + [(-150 + 300 * t, 55 + 12 * math.sin(math.pi * t)) for t in [i / 10 for i in range(11)]] + [(150, -55)]
    a.slab(prof, 104, at=(0, 0, 0), col=C.CORAL, bevel=3, rough=0.7)
    for k in range(13):
        t = (k + 0.5) / 13
        y = 55 + 12 * math.sin(math.pi * t)
        a.box((5, 4, 86), at=(-150 + 300 * t, y + 1.5, 50), rot=(0, math.degrees(math.atan(12 * math.pi / 300 * math.cos(math.pi * t))), 0), col=C.CREAM, bevel=1.5)
    a.box((306, 116, 8), at=(0, 2, 6), col=C.CORAL_DARK, bevel=2)
    # top (code: 320x130x10 at z 114)
    top = [(-160, -65)] + [(-160 + 320 * t, 65 + 12 * math.sin(math.pi * t)) for t in [i / 12 for i in range(13)]] + [(160, -65)]
    a.slab(top, 10, at=(0, 0, 109), col=C.CREAM, bevel=3, rough=0.5)
    a.slab([(p[0] * 0.97, p[1] * 0.97) for p in top], 2, at=(0, 0, 107), col=C.BRASS, rough=0.3)
    # CRT monitor (code box 40x30x30 at (40, -20, 135) from the desk centre)
    a.box((36, 34, 30), at=(40, -24, 134), col=C.CREAM_DARK, bevel=5, rough=0.6)
    a.box((24, 26, 22), at=(40, -40, 136), col=C.CREAM_DARK, bevel=4, rough=0.6)
    a.box((30, 2, 22), at=(40, -6, 135), col=C.TEAL_DARK, bevel=2, glow=0.8)
    a.box((20, 1, 3), at=(34, -4.8, 140), col=C.TEAL_LIGHT, bevel=0.4, glow=1.5)
    a.box((34, 16, 3), at=(40, 14, 115.5), col=C.GREY, bevel=1)
    for k in range(3):
        a.box((30, 3, 1.5), at=(40, 8 + k * 4.5, 117.5), col=C.CREAM, bevel=0.3)
    # desk bell, phone, papers, pens
    a.cyl(7, 2, at=(-60, 45, 115), col=C.CHARCOAL, sides=12, bevel=0.5)
    a.sphere(6.5, at=(-60, 45, 118), col=C.BRASS, segs=12, rings=7, rz=5, rough=0.2)
    a.cyl(1, 3, at=(-60, 45, 124), col=C.BRASS, sides=6, rough=0.2)
    a.box((22, 18, 8), at=(-110, 10, 118), col=C.RED, bevel=3, rough=0.4)
    a.box((24, 8, 5), at=(-110, 10, 125), col=C.RED, bevel=2.2, rough=0.4)
    a.cyl(5, 2, at=(-110, 14, 122.5), col=C.CREAM, sides=10, bevel=0.5)
    for k in range(3):
        a.box((21, 29, 0.8), at=(-10 + k * 1.5, 25 - k * 2, 114.5 + k * 0.8), rot=(0, -6 + k * 7, 0), col=C.WHITE if k < 2 else C.YELLOW, bevel=0.2)
    a.cyl(4, 10, at=(95, 30, 119), col=C.TEAL, sides=10, bevel=0.6)
    for k in range(3):
        a.cyl(0.6, 12, at=(95 + (k - 1) * 1.6, 30, 125), rot=(0, 0, (k - 1) * 8), col=[C.RED, C.BLUE, C.YELLOW][k], sides=5)
    # receptionist side (-Y): drawer bank with pulls, knee recess; keyboard, CRT cable and sticky notes,
    # a small succulent, a stack of forms; the visitor side gets a brass kick rail
    for sx in (-1, 1):
        x = sx * 100
        for k in range(3):
            a.box((70, 2, 24), at=(x, -56, 88 - k * 28), col=C.shade(C.CORAL, 1.08), bevel=1.2)
            a.box((16, 3, 3), at=(x, -58, 88 - k * 28), col=C.BRASS, bevel=0.8, rough=0.3)
    a.box((110, 2, 80), at=(0, -54, 52), col=C.shade(C.CORAL, 0.6), bevel=1)
    a.box((32, 12, 2), at=(40, -2, 115.5), rot=(0, 0, 6), col=C.CREAM_DARK, bevel=0.8)
    for r in range(3):
        for c in range(8):
            a.box((3, 2.6, 0.8), at=(28 + c * 3.6, -6 + r * 3.4, 116.8), rot=(0, 0, 6), col=C.shade(C.CREAM_DARK, 0.85), bevel=0.2, jitter=0)
    D.cable(a, [(40, -40, 124), (44, -52, 118), (46, -58, 108), (50, -58, 60)], r=0.8)
    a.box((0.4, 6, 6), at=(58.3, -30, 142), rot=(0, 0, 4), col=C.YELLOW, bevel=0.05, jitter=0)
    a.box((0.4, 6, 6), at=(58.3, -21, 138), rot=(0, 0, -6), col=C.hex_rgb(0x9FE3D9), bevel=0.05, jitter=0)
    a.cyl(5, 7, at=(125, -30, 117.5), r_top=6, col=C.TERRACOTTA, sides=10, bevel=0.8)
    for k in range(5):
        ang = k * 72
        a.sphere(3, at=(125 + 2.4 * D.math.cos(D.math.radians(ang)), -30 + 2.4 * D.math.sin(D.math.radians(ang)), 123), rz=4.5,
                 rot=(20, ang, 0), col=C.GREEN, segs=6, rings=4)
    for k in range(4):
        a.box((22, 30, 0.9), at=(-70, -30, 114.5 + k * 1), rot=(0, (k % 2) * 4 - 2, 0), col=C.WHITE, bevel=0.1)
    a.slab([(p[0] * 0.95, p[1] * 0.95 + 4) for p in top[1:-1]] + [(150, 60), (-150, 60)], 3, at=(0, 0, 12), col=C.BRASS, rough=0.3)


# ============================================================================== waiting bench

@asset("SM_Lobby_WaitingBench", F,
       desc="Upholstered waiting bench: teal base on brass feet, tufted coral cushion with buttons, rolled armrests and a stray magazine.",
       replaces=[SHELL + " waiting bench + cushion"],
       placements=lambda: [P((-1650, -470, 0))], covers=rec((589, 590)),
       pivot="bench centre on the floor (-1650, -470, 0); long axis along Y", view=(1, 0.4, 0.5))
def waiting_bench(a):
    a.box((78, 256, 40), at=(0, 0, 25), col=C.TEAL, bevel=5, rough=0.8)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(4, 8, at=(sx * 30, sy * 115, 4), col=C.BRASS, sides=8, bevel=1, rough=0.3)
    from ftb.core import bm_box, bulge, xf
    cush = bm_box(90, 262, 12, 5, 2)
    bulge(cush, 2.5, axis=2)
    a.add(cush, xf((0, 0, 55)), C.CORAL, rough=0.9)
    for k in range(6):
        for j in range(2):
            a.sphere(2.2, at=(-20 + j * 40, -105 + k * 42, 62), col=C.CORAL_DARK, segs=6, rings=4)
    for sy in (-1, 1):
        a.cyl(12, 90, at=(0, sy * 133, 62), rot=(90, 0, 0), col=C.TEAL_DARK, sides=12, bevel=4)
        a.torus(12, 1.5, at=(0, sy * 133, 62), rot=(90, 0, 0), col=C.BRASS, major=14, minor=5)
    a.box((24, 30, 1.2), at=(10, 60, 62), rot=(0, 16, 3), col=C.MAGENTA, bevel=0.3)
    a.box((20, 26, 0.4), at=(10, 60, 62.8), rot=(0, 16, 3), col=C.CREAM, bevel=0.1)
    # piping on the cushion edges, brass corner caps, a folded newspaper
    for sx in (-1, 1):
        a.cyl(1.1, 262, at=(sx * 45, 0, 60), rot=(0, 0, 90), col=C.CORAL_DARK, sides=6)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((7, 7, 38), at=(sx * 37.5, sy * 126, 25), col=C.BRASS, bevel=1.5, rough=0.3)
    a.box((26, 34, 1.6), at=(-15, -80, 62.6), rot=(0, -12, 2), col=C.hex_rgb(0xE9E4D8), bevel=0.3)
    for k in range(5):
        a.box((20, 0.6, 0.2), at=(-15, -92 + k * 5, 63.6), rot=(0, -12, 2), col=C.shade(C.INK, 2.5), bevel=0, jitter=0, wear=False)


# ============================================================================== STAGE 4 header

@asset("SM_Lobby_Stage4Header", F,
       desc="Navy STAGE 4 header over the big door: framed board with 24 socketed marquee bulbs and film-reel end caps.",
       replaces=[SHELL + " STAGE 4 sign board and its 24 bulbs"],
       placements=lambda: [P((-625, 0, 470))], covers=rec((606, 611)),
       pivot="board centre (-625, 0, 470); faces -X into the lobby",
       integration="'STAGE 4' stays a TextRender at (-634, 0, 474) or use SM_Sign_Studio_STAGE_4_2.", view=(-1, 0.4, 0.2))
def stage4_header(a):
    pv = (-625, 0, 470)
    recs = L.select("studio", lines=[(606, 611)])
    board = [r for r in recs if r["group"] == L.GROUP["BoxDeco"]]
    a.records(board, pivot=pv, bevel=3)
    a.box((6, 740, 10), at=(-7, 0, 71), col=C.BRASS, bevel=2, rough=0.3)
    a.box((6, 740, 10), at=(-7, 0, -71), col=C.BRASS, bevel=2, rough=0.3)
    for r in recs:
        if r["group"] != L.GROUP["SphereDeco"]:
            continue
        c = r["center"]
        a.cyl(6, 4, at=(c[0] - pv[0] + 4, c[1], c[2] - pv[2]), rot=(90, 0, 0), col=C.CHROME, sides=8, rough=0.3)
        a.sphere(5.5, at=(c[0] - pv[0], c[1], c[2] - pv[2]), col=C.YELLOW, segs=8, rings=6, glow=r["emissive"])
    for y in (-380, 380):
        a.cyl(40, 10, at=(-6, y, 0), rot=(90, 0, 0), col=C.CHARCOAL, sides=18, bevel=2)
        a.cyl(10, 12, at=(-8, y, 0), rot=(90, 0, 0), col=C.BRASS, sides=10, bevel=1, rough=0.3)
        for k in range(6):
            ang = math.radians(k * 60)
            a.cyl(7, 12, at=(-8, y + 24 * math.cos(ang), 24 * math.sin(ang)), rot=(90, 0, 0), col=C.GREY_DARK, sides=8, bevel=0)
    # frame screws, top hanger brackets with rods into the wall, a cable feed at the side
    for y in range(-360, 361, 90):
        D.screw(a, (-10.2, y, 71), "-x", r=1.2, col=C.BRASS)
        D.screw(a, (-10.2, y, -71), "-x", r=1.2, col=C.BRASS)
    for y in (-250, 0, 250):
        a.box((20, 8, 6), at=(4, y, 80), col=C.GREY_DARK, bevel=1)
        a.cyl(1.4, 16, at=(12, y, 80), rot=(-90, 0, 0), col=C.GREY, sides=6)


# ============================================================================== pendant lamp

def _pendant_recs():
    return L.select("studio", lines=[(620, 625)], kind="prim")


def _pendant_placements():
    out = []
    for r in _pendant_recs():
        if r["group"] == L.GROUP["ConeDeco"]:
            out.append(P((r["center"][0], r["center"][1], 560)))
    return out


def _pendant_covers():
    recs = _pendant_recs()
    out = []
    for p in _pendant_placements():
        out.append([r for r in recs if abs(r["center"][0] - p["loc"][0]) < 1 and abs(r["center"][1] - p["loc"][1]) < 1])
    return out


@asset("SM_Lobby_PendantLamp", F,
       desc="Art-deco pendant: brass ceiling rose, cord, teal fluted shade with a cream lip and a glowing opal bulb.",
       replaces=[SHELL + " four pendant lamps (cord, cone shade, bulb)"],
       placements=_pendant_placements, covers=_pendant_covers, per_placement=True,
       pivot="ceiling attachment (Z 560 = lobby ceiling underside); hangs down to Z ~490", view=(1, 0.6, 0.1))
def pendant_lamp(a):
    a.cyl(9, 3, at=(0, 0, -1.5), col=C.BRASS, sides=12, bevel=1, rough=0.3)
    a.cyl(1.2, 26, at=(0, 0, -16), col=C.CHARCOAL, sides=6)
    a.cyl(5, 6, at=(0, 0, -30), col=C.BRASS, sides=10, bevel=1, rough=0.3)
    a.lathe([(4, -32), (12, -34), (26, -44), (34, -52), (36, -54), (0.1, -54)], col=C.TEAL, sides=16, rough=0.5)
    a.torus(35, 2, at=(0, 0, -54), col=C.CREAM, major=18, minor=6)
    for k in range(8):
        ang = k * 45
        a.box((16, 2, 2), at=(math.cos(math.radians(ang)) * 21, math.sin(math.radians(ang)) * 21, -43), rot=(-32, ang, 0), col=C.shade(C.TEAL, 1.2), bevel=0.4)
    a.sphere(13, at=(0, 0, -56), col=C.CREAM, segs=12, rings=8, glow=14)
    # link chain over the cord, gallery ring and screws on the shade, bulb collar
    for k in range(8):
        a.torus(1.4, 0.45, at=(0, 0, -4.5 - k * 3), rot=(90, (k % 2) * 90, 0), col=C.BRASS, major=8, minor=3, rough=0.3)
    a.torus(4.6, 0.9, at=(0, 0, -33), col=C.BRASS, major=12, minor=4, rough=0.3)
    for k in range(4):
        ang = D.math.radians(k * 90 + 45)
        a.sphere(0.8, at=(25 * D.math.cos(ang), 25 * D.math.sin(ang), -45.5), col=C.BRASS, segs=6, rings=3)
    a.cyl(6, 3, at=(0, 0, -45), col=C.GREY, sides=10, bevel=0.6)
