"""Production stations (actors). Moving/recoloured components are separate meshes whose pivot equals
the owning scene component, so they can be dropped into the existing component hierarchy 1:1."""
import math

from ftb import detail as D
from ftb import palette as C
from ftb.core import bm_box, bm_sphere, deform, xf
from ftb.registry import P, asset
from mathutils import Vector

F = "Studio/Stations"
SRC_ST = "Production/FTStations.cpp"


def actor(name, file, comps):
    return "%s (%s): %s" % (name, file, comps)


def grille(a, at, w, h, rot=(0, 0, 0), n=5, col=C.GREY_DARK, depth=2):
    from ftb.core import ue_rot
    R = ue_rot(*rot)
    for k in range(n):
        off = R @ Vector((0, 0, -h / 2 + (k + 0.5) * h / n))
        a.box((depth, w, h / n * 0.45), at=(at[0] + off.x, at[1] + off.y, at[2] + off.z), rot=rot, col=col, bevel=0.4)


# ============================================================================== lighthouse

@asset("SM_Station_Lighthouse", F,
       desc="Striped lighthouse: stone plinth with steps, four tapering red/white drums with rivet bands, arched door, porthole, gallery deck with railing and balusters, lantern cage, red cap with a ball vane; switch box on the side.",
       replaces=[actor("AFTLighthouse", SRC_ST, "Base, Seg0-3, Door, Window, Gallery, Railing, LanternGlass, Roof, Tip, SwitchBox, SwitchPlate")],
       placements=lambda: [P((2150, -700, -10), (0, 180, 0))],
       pivot="actor root (plinth bottom centre); switch box at local (70, 70)",
       integration="Keep the actor; hide the listed parts and attach this mesh to Root. Lamp and Lever stay separate (SM_Station_LighthouseLamp / _LighthouseLever).",
       view=(1, 0.8, 0.4))
def lighthouse(a):
    a.box((150, 150, 36), at=(0, 0, 18), col=C.GREY_DARK, bevel=6)
    a.box((120, 120, 8), at=(0, 0, 40), col=C.shade(C.GREY_DARK, 1.2), bevel=3)
    for k in range(3):
        a.box((24, 50, 12 - k * 4 + 12), at=(84 + k * 8 - 8, 0, (24 - k * 8) / 2), col=C.GREY, bevel=2)
    diam = (112, 100, 90, 80)
    for i in range(4):
        z0 = 36 + i * 80
        r0, r1 = diam[i] / 2 + 1, (diam[i + 1] / 2 if i < 3 else diam[3] / 2 - 3) + 1
        a.cyl(r0, 80, at=(0, 0, z0 + 40), r_top=r1, col=C.RED if i % 2 else C.WHITE, sides=16, bevel=1.5, rough=0.6)
        a.torus(r1 + 0.5, 1.8, at=(0, 0, z0 + 80), col=C.shade(C.GREY, 1.1), major=16, minor=5, rough=0.4)
    a.box((10, 38, 64), at=(56, 0, 68), col=C.WOOD_DARK, bevel=3)
    a.cyl(19, 10, at=(56, 0, 100), rot=(90, 0, 0), col=C.WOOD_DARK, sides=12, bevel=2)
    a.cyl(2, 3, at=(62, -10, 70), rot=(90, 0, 0), col=C.BRASS, sides=6, rough=0.3)
    a.cyl(15, 6, at=(45, 0, 240), rot=(90, 0, 0), col=C.GREY, sides=14, bevel=2, rough=0.4)
    a.cyl(11, 7, at=(46, 0, 240), rot=(90, 0, 0), col=C.NAVY, sides=14, bevel=1)
    a.cyl(63, 10, at=(0, 0, 342), col=C.CHARCOAL, sides=18, bevel=2)
    a.torus(60, 2.2, at=(0, 0, 366), col=C.GREY, major=24, minor=6, rough=0.4)
    for k in range(16):
        ang = math.radians(k * 22.5)
        a.cyl(1.5, 20, at=(60 * math.cos(ang), 60 * math.sin(ang), 356), col=C.GREY, sides=5, rough=0.4)
    a.cyl(36, 6, at=(0, 0, 350), col=C.GREY_DARK, sides=16, bevel=1)
    for k in range(6):
        ang = math.radians(k * 60)
        a.box((5, 5, 62), at=(34 * math.cos(ang), 34 * math.sin(ang), 380), rot=(0, k * 60, 0), col=C.CHARCOAL, bevel=1)
    a.cone(52, 34, at=(0, 0, 426), r_top=6, col=C.RED, sides=16, rough=0.6)
    a.torus(49, 3, at=(0, 0, 410), col=C.shade(C.RED, 0.8), major=18, minor=6)
    a.sphere(8, at=(0, 0, 448), col=C.CHARCOAL, segs=10, rings=6)
    a.cyl(1.4, 26, at=(0, 0, 465), col=C.CHARCOAL, sides=5)
    a.box((2, 16, 8), at=(0, 5, 472), col=C.CHARCOAL, bevel=0.5)
    # switch box (code: at (70, 70, 50) 28x28x40 + plate on +X)
    a.box((28, 28, 40), at=(70, 70, 50), col=C.CHARCOAL, bevel=3)
    a.box((3, 22, 30), at=(85, 70, 50), col=C.YELLOW, bevel=1)
    a.cyl(4, 30, at=(70, 70, 15), col=C.CHARCOAL, sides=8)
    a.cyl(5, 3, at=(86, 70, 58), rot=(0, 90, 90), col=C.GREY, sides=10)
    lighthouse_details(a, diam)


def lighthouse_details(a, diam):
    def rad(z):   # outer radius of the drums at height z
        i = max(0, min(3, int((z - 36) // 80)))
        r0 = diam[i] / 2 + 1
        r1 = (diam[i + 1] / 2 if i < 3 else diam[3] / 2 - 3) + 1
        return r0 + (r1 - r0) * min(1.0, max(0.0, (z - 36 - i * 80) / 80))
    joint = C.shade(C.GREY_DARK, 0.62)
    # plinth: two stone courses with staggered joints on every side, a few chipped stones
    for n, c in (("+x", (75, 0, 18)), ("-x", (-75, 0, 18)), ("+y", (0, 75, 18)), ("-y", (0, -75, 18))):
        D.seams(a, c, n, 138, 36, n=1, along="u", col=joint, width=0.9)
        for du in (-44, 6, 52):
            a.box(D._size(n, 0.9, 16, 0.25), at=D.on(c, n, du, 9, 0.12), col=joint, bevel=0, jitter=0, wear=False)
        for du in (-62, -18, 30):
            a.box(D._size(n, 0.9, 16, 0.25), at=D.on(c, n, du, -9, 0.12), col=joint, bevel=0, jitter=0, wear=False)
    for (x, y, z) in ((74, -60, 33), (-74, 40, 4), (50, -74, 30), (-30, 74, 6)):
        a.box((5, 5, 3), at=(x, y, z), rot=(0, 20, 8), col=C.shade(C.GREY_DARK, 1.18), bevel=0.8)
    # service ladder on the back with wall brackets, hatch in the gallery deck above it
    zt = 334
    for sy in (-1, 1):
        a.tube([(-(rad(44) + 8), sy * 11, 44), (-(rad(zt) + 8), sy * 11, zt)], 1.6, col=C.GREY, sides=6, rough=0.4)
        for z in (80, 190, 300):
            a.box((9, 3, 3), at=(-(rad(z) + 4), sy * 11, z), col=C.GREY_DARK, bevel=0.6)
    for k in range(12):
        z = 60 + k * 23
        a.cyl(1.1, 22, at=(-(rad(z) + 8), 0, z), rot=(0, 0, 90), col=C.GREY, sides=6, rough=0.4)
    a.box((24, 26, 1.2), at=(-50, 0, 347.6), col=C.GREY_DARK, bevel=0.3)
    D.hinge(a, (-38.5, 0, 348.6), "y", 20, r=1.0)
    # corbels carrying the gallery, a drip ring under the deck, deck plate seams, mid rail
    for k in range(8):
        a.poly([(37, 337), (60, 337), (37, 313)], 6, rot=(0, k * 45 + 22.5, 0), col=C.CHARCOAL, bevel=1)
    a.torus(62.5, 1.2, at=(0, 0, 337), col=C.shade(C.CHARCOAL, 1.3), major=24, minor=4)
    for k in range(12):
        ang = math.radians(k * 30 + 15)
        a.box((22, 0.6, 0.3), at=(50 * math.cos(ang), 50 * math.sin(ang), 347.1), rot=(0, k * 30 + 15, 0), col=C.INK, bevel=0, jitter=0, wear=False)
    a.torus(60, 1.2, at=(0, 0, 357), col=C.GREY, major=24, minor=4, rough=0.4)
    # lantern: glazing rings above and below the lens, roof ribs and a vent collar
    a.torus(36, 2.4, at=(0, 0, 347.5), col=C.CHARCOAL, major=20, minor=5)
    a.torus(36, 2.4, at=(0, 0, 410.5), col=C.CHARCOAL, major=20, minor=5)
    slope = math.degrees(math.atan2(34, 46))
    for k in range(8):
        ang = math.radians(k * 45)
        nrm = Vector((math.cos(ang) * math.sin(math.radians(slope)), math.sin(ang) * math.sin(math.radians(slope)), math.cos(math.radians(slope))))
        p = Vector((29 * math.cos(ang), 29 * math.sin(ang), 426)) + nrm * 1.0
        a.box((56, 3, 2.2), at=p, rot=(-slope, k * 45, 0), col=C.shade(C.RED, 0.78), bevel=0.6)
    a.cyl(9, 3, at=(0, 0, 442), col=C.GREY_DARK, sides=10, bevel=0.8)
    # vertical plate seams on the drums, bolted portholes, a second porthole at the back
    for i in range(4):
        z = 36 + i * 80 + 40
        for ang in (35, 145, 215, 325):
            r = rad(z) + 0.2
            a.box((0.5, 0.8, 74), at=(r * math.cos(math.radians(ang)), r * math.sin(math.radians(ang)), z),
                  rot=(0, ang, 0), col=C.shade(C.RED if i % 2 else C.WHITE, 0.72), bevel=0, jitter=0, wear=False)
    D.bolt_ring(a, (48.2, 0, 240), "+x", 13, n=8, r=0.9)
    rb = rad(160) + 1.5
    a.cyl(13, 5, at=(-rb, 0, 160), rot=(90, 0, 0), col=C.GREY, sides=14, bevel=1.5, rough=0.4)
    a.cyl(9, 6, at=(-rb - 0.5, 0, 160), rot=(90, 0, 0), col=C.NAVY, sides=14, bevel=1)
    D.bolt_ring(a, (-rb - 2.5, 0, 160), "-x", 11, n=6, r=0.8)
    # door: planks, strap hinges, kick plate, a name plaque above
    D.seams(a, (61, 0, 68), "+x", 38, 60, n=3, along="v", col=C.shade(C.WOOD_DARK, 0.6), width=0.7)
    for z in (50, 88):
        a.box((1.2, 20, 3), at=(61.6, -8, z), col=C.GREY_DARK, bevel=0.4)
        D.hinge(a, (61.8, -19, z), "z", 7, r=1.2, col=C.GREY_DARK)
    D.plate(a, (61, 0, 42), "+x", 34, 8, t=0.8, col=C.GREY)
    D.plate(a, (rad(128) - 0.5, 0, 128), "+x", 26, 9, t=0.8, col=C.BRASS, rough=0.35)
    a.text("No 4", 5, 0.4, at=(rad(128) + 0.6, 0, 128), col=C.INK)
    # life ring on the +Y side
    a.box((4, 3, 6), at=(0, rad(172) + 1, 172), col=C.GREY_DARK, bevel=0.6)
    ry = rad(152) + 4
    a.torus(13, 3.6, at=(0, ry, 154), rot=(0, 0, 90), col=C.WHITE, major=20, minor=8, rough=0.7)
    for k in range(4):
        ang = k * 90 + 45
        a.cyl(4.1, 5, at=(13 * math.cos(math.radians(ang)), ry, 154 + 13 * math.sin(math.radians(ang))),
              rot=(ang, 0, 0), col=C.CORAL, sides=10, bevel=0.6, rough=0.7)
    D.cable(a, [(0, ry, 170), (-10, ry + 1, 150), (0, ry + 1, 140), (10, ry + 1, 150), (0, ry, 170)], r=0.6, col=C.SAND)
    # switch box: screws, label, conduit to the tower, cable on the floor
    D.screws_rect(a, (86.5, 70, 50), "+x", 22, 30, inset=2, r=0.7)
    D.label(a, (84, 70, 36), "+x", 16, 5, lines=1)
    D.hinge(a, (84.5, 56.5, 50), "z", 26, r=0.9)
    a.tube([(58, 62, 40), (48, 52, 40), (38, 42, 40)], 2.0, col=C.GREY, sides=8, rough=0.4)
    a.cyl(2.8, 5, at=(53, 57, 40), rot=(0, 45, 90), col=C.GREY_DARK, sides=8, bevel=0.5)
    D.cable(a, [(70, 74, 1.2), (80, 80, 1.2), (92, 82, 1.2)], r=1.1)


@asset("SM_Station_LighthouseLamp", F,
       desc="Fresnel lantern lens: ribbed glass drum around a bright core (painted by the code when switched on).",
       replaces=[actor("AFTLighthouse", SRC_ST, "Lamp (ball) + LanternGlass")],
       pivot="lens centre = Lamp component location (0, 0, 378)",
       integration="Keep M_FT_Matte/CPD painting on this component (glow on/off); the vertex colours are neutral cream.", view=(1, 0.6, 0.3))
def lighthouse_lamp(a):
    a.cyl(34, 58, col=C.CREAM, sides=16, bevel=3, rough=0.2)
    for k in range(5):
        a.torus(35, 1.8, at=(0, 0, -22 + k * 11), col=C.WHITE, major=18, minor=5, rough=0.2)
    a.sphere(20, col=C.YELLOW, segs=12, rings=8)
    # bull's-eye panels between the prism rings (front and back) and a lamp holder cap
    for ang in (0, 180):
        c = math.radians(ang)
        a.cyl(11, 2, at=(35 * math.cos(c), 35 * math.sin(c), 0), rot=(-90 if ang == 0 else 90, 0, 0),
              col=C.WHITE, sides=14, bevel=0.8, rough=0.15)
        a.cyl(5, 3, at=(36.5 * math.cos(c), 36.5 * math.sin(c), 0), rot=(-90 if ang == 0 else 90, 0, 0), col=C.CREAM, sides=12, bevel=1, rough=0.15)
    a.cyl(10, 4, at=(0, 0, 31), col=C.GREY_DARK, sides=12, bevel=1)


@asset("SM_Station_LighthouseLever", F,
       desc="Red knob lever for the lighthouse switch box.",
       replaces=[actor("AFTLighthouse", SRC_ST, "Lever cylinder")],
       pivot="lever centre (the Lever component's location); the code tilts it -35 pitch", view=(1, 0.6, 0.3))
def lighthouse_lever(a):
    a.cyl(2.5, 26, col=C.GREY, sides=8)
    a.sphere(5, at=(0, 0, 15), col=C.RED, segs=10, rings=6)


# ============================================================================== stage light

@asset("SM_Station_StageLight_Base", F,
       desc="Tripod base for the stage light: three splayed legs with rubber feet, spider hub, amber locking collar.",
       replaces=[actor("AFTStageLight", SRC_ST, "Leg0-2, Collar")],
       placements=lambda: [P((1500, -1250, -120), (0, 55, 0)), P((1100, 1350, -120), (0, -61, 0)), P((1200, 1300, -120), (0, -49, 0))],
       pivot="actor root on the floor", integration="Pole stays a scaled cylinder (height = StandHeight) or use SM_Station_StageLight_Pole scaled in Z.", view=(1, 0.6, 0.4))
def stage_light_base(a):
    for i in range(3):
        ang = i * 120
        d = Vector((math.cos(math.radians(ang)), math.sin(math.radians(ang)), 0))
        a.tube([d * 4 + Vector((0, 0, 50)), d * 40 + Vector((0, 0, 2))], 2.8, col=C.CHARCOAL, sides=6)
        a.cyl(4, 4, at=tuple(d * 42 + Vector((0, 0, 2))), col=C.RUBBER, sides=8, bevel=1)
        a.tube([d * 4 + Vector((0, 0, 30)), d * 20 + Vector((0, 0, 24))], 1.4, col=C.GREY, sides=5)
    a.cyl(7, 14, at=(0, 0, 48), col=C.GREY_DARK, sides=10, bevel=1.5)
    a.cyl(8, 8, at=(0, 0, 60), col=C.AMBER, sides=10, bevel=2)
    a.cyl(1.5, 10, at=(9, 0, 60), rot=(90, 0, 0), col=C.AMBER, sides=6)
    # detail pass: T-knob on the collar pin, leg knuckles, brace clamps, a shot bag and a coiled cable
    a.box((1.6, 7, 1.6), at=(14.5, 0, 60), col=C.AMBER, bevel=0.5)
    for i in range(3):
        d = Vector((math.cos(math.radians(i * 120)), math.sin(math.radians(i * 120)), 0))
        a.box((6, 5, 6), at=tuple(d * 6.5 + Vector((0, 0, 47))), rot=(0, i * 120, 0), col=C.GREY_DARK, bevel=1)
        a.cyl(1.2, 7, at=tuple(d * 6.5 + Vector((0, 0, 47))), rot=(0, i * 120 + 90, 90), col=C.CHROME, sides=6)
        a.cyl(3.2, 4, at=tuple(d * 20 + Vector((0, 0, 28.7))), rot=(36.9, i * 120, 0), col=C.GREY, sides=8, bevel=0.5)
    d = Vector((math.cos(math.radians(240)), math.sin(math.radians(240)), 0))
    side = Vector((-d.y, d.x, 0))
    bag = C.shade(C.ORANGE, 0.72)
    c = d * 28 + Vector((0, 0, 17))
    a.sphere(6, at=tuple(c + Vector((0, 0, 1.5))), ry=10, rz=4.2, rot=(0, 240, 0), col=bag, segs=10, rings=6, rough=0.95)
    for s_ in (-1, 1):
        a.sphere(6.5, at=tuple(c + side * (s_ * 8.5) + Vector((0, 0, -7))), ry=4.8, rz=8, rot=(0, 240, s_ * 12),
                 col=bag, segs=10, rings=6, rough=0.95)
    a.box((3.2, 21, 1.0), at=tuple(c + Vector((0, 0, 5.4))), rot=(0, 240, 0), col=C.CHARCOAL, bevel=0.3, rough=0.9)
    a.torus(2.2, 0.6, at=tuple(c + Vector((0, 0, 7.2))), rot=(90, 240, 0), col=C.CHARCOAL, major=10, minor=4)
    d = Vector((math.cos(math.radians(60)), math.sin(math.radians(60)), 0))
    side = Vector((-d.y, d.x, 0))
    for k in range(3):
        a.torus(9 - k * 0.6, 0.85, at=tuple(d * 30 + side * 2 + Vector((k * 1.1, -k * 0.8, 0.9 + k * 1.4))), col=C.RUBBER, major=16, minor=4, rough=0.85)
    D.cable_tie_run(a, [tuple(d * 30 + side * -6.5 + Vector((0, 0, 4.5))), tuple(d * 33 + side * 3 + Vector((0, 0, 12))),
                        tuple(d * 18 + side * 3 + Vector((0, 0, 32))), tuple(d * 5 + side * 4 + Vector((0, 0, 44)))], r=0.85, ties=2)


@asset("SM_Station_StageLight_Pole", F,
       desc="100 cm telescopic pole section (scale Z by StandHeight/100) with a clamp ring.",
       replaces=[actor("AFTStageLight", SRC_ST, "Pole (scaled to StandHeight)")],
       pivot="bottom of the pole; the code places the pole centre at 20 + StandHeight/2", view=(1, 0.6, 0.3))
def stage_light_pole(a):
    a.cyl(3, 100, at=(0, 0, 50), col=C.CHARCOAL, sides=8, bevel=0, rough=0.5)
    a.cyl(3.8, 30, at=(0, 0, 15), col=C.GREY_DARK, sides=8, bevel=0.8, rough=0.5)
    # clamp collar with a T-screw, height marks, top ferrule
    a.cyl(4.6, 4, at=(0, 0, 28), col=C.GREY, sides=10, bevel=0.8, rough=0.4)
    a.cyl(0.8, 5, at=(6.5, 0, 28), rot=(-90, 0, 0), col=C.CHROME, sides=6)
    a.box((1.4, 7, 1.4), at=(9.3, 0, 28), col=C.AMBER, bevel=0.4)
    for z in (40, 55, 70, 85):
        a.cyl(3.08, 0.6, at=(0, 0, z), col=C.shade(C.YELLOW, 0.9), sides=8, bevel=0, jitter=0)
    a.cyl(3.5, 3, at=(0, 0, 98.5), col=C.GREY_DARK, sides=8, bevel=0.6)


@asset("SM_Station_StageLight_Head", F,
       desc="Fresnel lamp head: charcoal body with vent slots, amber rim, four hinged barn doors, yoke with tilt knobs and a rear handle.",
       replaces=[actor("AFTStageLight", SRC_ST, "Head: Yoke, Body, Rim, DoorTop/Bottom/Left/Right")],
       pivot="Head scene component (lens axis +X)",
       integration="LensPart stays separate (SM_Station_StageLight_Lens) because the code recolours it per gel.", view=(1, 0.7, 0.3))
def stage_light_head(a):
    a.box((46, 40, 40), col=C.CHARCOAL, bevel=6)
    for k in range(4):
        a.box((24, 1.6, 3), at=(-6, 20.5, 10 - k * 6), col=C.shade(C.CHARCOAL, 0.6), bevel=0.3)
        a.box((24, 1.6, 3), at=(-6, -20.5, 10 - k * 6), col=C.shade(C.CHARCOAL, 0.6), bevel=0.3)
    a.cyl(20, 6, at=(24, 0, 0), rot=(-90, 0, 0), col=C.AMBER, sides=16, bevel=1.5, rough=0.4)
    for (at, size, rot) in (((36, 0, 24), (22, 40, 2), (-30, 0, 0)), ((36, 0, -24), (22, 40, 2), (30, 0, 0)),
                            ((36, -24, 0), (22, 2, 40), (0, 30, 0)), ((36, 24, 0), (22, 2, 40), (0, -30, 0))):
        a.box(size, at=at, rot=rot, col=C.CHARCOAL, bevel=0.8)
    a.box((10, 48, 8), at=(0, 0, -24), col=C.CHARCOAL, bevel=2)
    for sy in (-1, 1):
        a.box((6, 4, 30), at=(0, sy * 23, -12), col=C.CHARCOAL, bevel=1)
        a.cyl(5, 4, at=(0, sy * 26, 0), rot=(0, 0, 90), col=C.AMBER, sides=10, bevel=1)
    a.tube([(-23, -8, 12), (-30, -8, 16), (-30, 8, 16), (-23, 8, 12)], 1.6, col=C.GREY, sides=6)
    # detail pass: top vent, side screws + badge, gel-frame holder with door hinges, rear switch plate and power lead
    D.vent(a, (-4, 0, 20), "+z", 26, 22, slats=6, col=C.shade(C.CHARCOAL, 0.8), depth=1.0)
    for n, y in (("+y", 20), ("-y", -20)):
        D.screws_rect(a, (0, y, 0), n, 38, 34, inset=3.5, r=0.8, col=C.GREY)
    D.plate(a, (13, 20, 12), "+y", 10, 5, t=0.5, col=C.AMBER, screws=False)
    D.border(a, (27, 0, 0), "+x", 42, 42, bar=2.2, t=1.2, col=C.GREY_DARK)
    for z in (20.8, -20.8):
        D.hinge(a, (28, 0, z), "y", 34, r=0.9, col=C.GREY)
    for y in (20.8, -20.8):
        D.hinge(a, (28, y, 0), "z", 34, r=0.9, col=C.GREY)
    D.plate(a, (-23, 0, -3), "-x", 22, 12, t=0.8, col=C.GREY_DARK)
    a.box((1.4, 4, 6), at=(-24.3, -5, -3), col=C.RED, bevel=0.4)
    D.led(a, (-23.8, 4, -1), "-x", r=0.7, col=C.GREEN, glow=2)
    a.cyl(2.2, 3, at=(-24, 8, -12), rot=(90, 0, 0), col=C.RUBBER, sides=8, bevel=0.5)
    D.cable(a, [(-25.5, 8, -12), (-28, 12, -18), (-14, 21, -26), (-4, 21, -27)], r=0.9)
    D.label(a, (-23, 0, 6), "-x", 12, 3.5, lines=1)


@asset("SM_Station_StageLight_Lens", F,
       desc="Stepped fresnel lens disc, neutral white (the code tints it with the gel colour).",
       replaces=[actor("AFTStageLight", SRC_ST, "LensPart")],
       pivot="lens centre, facing +X (same transform as LensPart)", view=(1, 0.3, 0.2))
def stage_light_lens(a):
    for k in range(3):
        a.cyl(16 - k * 4, 3 + k * 0.8, at=(0, 0, k * 0.8), rot=(-90, 0, 0), col=C.WHITE, sides=16, bevel=0.5, rough=0.2)
    for r in (14, 10, 6):
        a.torus(r, 0.45, at=(1.6 + (16 - r) * 0.2, 0, 0), rot=(-90, 0, 0), col=C.WHITE, major=16, minor=3, rough=0.15)
    a.torus(16.2, 1.0, at=(0.2, 0, 0), rot=(-90, 0, 0), col=C.shade(C.WHITE, 0.85), major=20, minor=4, rough=0.3)


# ============================================================================== lighting + sound desks

@asset("SM_Station_LightingBoard", F,
       desc="Navy lighting desk: flight-case shell with aluminium corners, raked charcoal fader panel with fader rows, teal trim, gooseneck lamp and a title plate.",
       replaces=[actor("AFTLightingBoard", SRC_ST, "Desk, Panel, Trim")],
       placements=lambda: [P((430, 640, -60))],
       pivot="actor root on the camera deck", integration="ToggleCap0-2, ColorCap0-2 and PowerLamp stay separate (painted by the code).", view=(-1, 0.4, 0.6))
def lighting_board(a):
    a.box((80, 190, 86), at=(0, 0, 43), col=C.NAVY, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((9, 9, 86), at=(sx * 38, sy * 93, 43), col=C.GREY, bevel=2, rough=0.35)
    a.box((84, 194, 5), at=(0, 0, 88), col=C.TEAL, bevel=1.5)
    a.box((76, 180, 8), at=(-4, 0, 95), rot=(-18, 0, 0), col=C.CHARCOAL, bevel=2)
    # raked panel content in the panel's own frame (top surface at local z 4). The panel dips into the trim
    # beyond local x ~26, the code's ToggleCaps (local x -21..9) and ColorCaps (8..26) sit at Y -55/0/55,
    # the PowerLamp and the gooseneck at Y 80: faders go into the gaps, knobs + LEDs along the front edge
    pn = D.Local(a, (-4, 0, 95), (-18, 0, 0))
    for y in (-88, -80, -34, -27, -20, 20, 27, 34):
        pn.box((24, 2, 0.8), at=(8, y, 4.3), col=C.INK, bevel=0, jitter=0, wear=False)
        pn.box((4, 5.5, 3.5), at=(8 + (int(abs(y)) % 3) * 5 - 5, y, 6), col=[C.CREAM, C.YELLOW, C.CORAL][int(abs(y) / 8) % 3], bevel=1)
    for k in range(14):
        y = -84 + k * 12
        if y > 70:
            continue
        D.knob(pn, (-26, y, 4), "+z", r=2.2, h=1.8, col=C.shade(C.CHARCOAL, 1.5))
        D.led(pn, (-30.5, y + 4, 4), "+z", r=0.55, col=C.GREEN if k % 4 else C.AMBER, glow=1.5)
    pn.box((3.5, 170, 0.5), at=(-35, 0, 4.3), col=C.CREAM, bevel=0.1, rough=0.9, jitter=0)
    for k in range(10):
        pn.box((1.4, 7 + (k * 7) % 5, 0.3), at=(-35, -76 + k * 17, 4.7), col=C.INK, bevel=0, jitter=0, wear=False)
    for y in (-86, 86):
        D.screw(pn, (-35, y, 4), "+z", r=0.8, col=C.GREY)
        D.screw(pn, (20, y, 4), "+z", r=0.8, col=C.GREY)
    # title plate behind the LIGHTING text (the code text sits at x -20 facing -X), on two posts
    a.box((4, 92, 26), at=(-16, 0, 125), col=C.CHARCOAL, bevel=1.5)
    D.border(a, (-18, 0, 125), "-x", 92, 26, bar=1.6, t=0.6, col=C.TEAL)
    for sy in (-1, 1):
        a.box((3, 3, 9), at=(-16, sy * 40, 108.5), col=C.GREY_DARK, bevel=0.6)
    a.tube([(-30, 80, 100), (-32, 80, 130), (-20, 70, 142)], 1.5, col=C.GREY, sides=6)
    for t in (0.2, 0.4, 0.6, 0.8):
        a.torus(1.6, 0.5, at=(-30.4 - t * 1.6, 80, 100 + t * 30), col=C.shade(C.GREY, 0.8), major=8, minor=3)
    a.cone(6, 8, at=(-16, 66, 140), rot=(-140, 0, 0), col=C.CHARCOAL, sides=8)
    a.cyl(4.5, 0.8, at=(-18.5, 68.6, 137), rot=(-140, 0, 0), col=C.WINDOW_WARM, sides=8, glow=2.0)
    a.cyl(4, 3, at=(-30, 80, 100), col=C.GREY_DARK, sides=8, bevel=0.8)
    a.box((12, 40, 4), at=(-41, 0, 43), col=C.GREY_DARK, bevel=1)
    # flight-case shell: lid-panel inset, butterfly latches, recessed side handles, stencil, back connector panel
    D.inset_panel(a, (-40, 0, 44), "-x", 164, 72, C.NAVY, frame_col=C.shade(C.GREY, 0.9), bar=2.2, t=1.0)
    D.stencil_number(a, (-41, 50, 20), "-x", "LX-01", 7, col=C.CREAM)
    for x, n in ((-40, "-x"), (40, "+x")):
        for y in (-60, 60):
            D.plate(a, (x, y, 78), n, 10, 7, t=0.8, col=C.GREY, screws=False)
            a.box(D._size(n, 3, 8, 1.5), at=D.on((x, y, 78), n, 0, -1, 1.5), col=C.CHROME, bevel=0.4, rough=0.3)
            D.screws_rect(a, D.on((x, y, 78), n, dn=0.8), n, 10, 7, inset=1.6, r=0.5)
    for y, n in ((95, "+y"), (-95, "-y")):
        D.plate(a, (0, y, 58), n, 30, 12, t=0.6, col=C.INK, screws=False)
        D.handle(a, (0, y, 58), n, 22, along="u", r=1.2, standoff=3.0)
        D.screws_rect(a, D.on((0, y, 58), n, dn=0.6), n, 30, 12, inset=1.8, r=0.6)
        D.label(a, (0, y, 28), n, 22, 8, lines=2)
    D.plate(a, (40, 0, 50), "+x", 60, 22, t=0.8, col=C.GREY_DARK)
    D.jacks(a, D.on((40, 0, 50), "+x", -12, 2, 0.8), "+x", cols=4, rows=2, pitch=5)
    D.rack_unit(a, D.on((40, 0, 50), "+x", 15, 0, 0.8), "+x", 24, 10, knobs=2, leds=2)
    D.vent(a, (40, -55, 30), "+x", 30, 16, slats=5, col=C.shade(C.NAVY, 0.8))
    D.cable_tie_run(a, [(41.5, -10, 44), (44, -12, 26), (45, -20, 3), (44, -60, 1.5), (41, -80, 1.5)], r=1.4, ties=2)
    D.cable(a, [(41.5, 4, 44), (43.5, 8, 24), (44.5, 16, 3), (43, 40, 1.5)], r=1.1, col=C.shade(C.RUBBER, 1.4))
    D.feet(a, [(sx * 36, sy * 88, 0) for sx in (-1, 1) for sy in (-1, 1)], r=3.2, h=1.2)


@asset("SM_Station_LightingBoard_Toggle", F,
       desc="Chunky round toggle cap (neutral grey, the code recolours it).",
       replaces=[actor("AFTLightingBoard", SRC_ST, "ToggleCap0-2")], pivot="cap centre (component location, rotation -18 pitch from the code)", view=(1, 0.4, 0.8))
def lighting_toggle(a):
    a.cyl(15, 10, col=C.GREY, sides=16, bevel=3, rough=0.4)
    a.box((22, 4, 5), at=(0, 0, 6), col=C.shade(C.GREY, 1.2), bevel=1.5)
    # knurled skirt and an indicator notch
    for k in range(16):
        ang = k * 22.5
        a.box((1.2, 2.2, 5), at=(15 * math.cos(math.radians(ang)), 15 * math.sin(math.radians(ang)), -1), rot=(0, ang, 0), col=C.shade(C.GREY, 0.85), bevel=0.3)
    a.torus(15.2, 0.8, at=(0, 0, -5), col=C.shade(C.GREY, 0.7), major=20, minor=4)
    a.box((5, 1.2, 0.6), at=(8.5, 0, 8.8), col=C.WHITE, bevel=0.1, jitter=0, wear=False)


@asset("SM_Station_SoundConsole", F,
       desc="Charcoal sound console with raked meter bridge, magenta trim, two speaker stacks with grilles and ports, boom stand with a fluffy dead-cat microphone.",
       replaces=[actor("AFTSoundConsole", SRC_ST, "Desk, Top, Trim, TitlePlate, Speaker/Woofer/Tweeter x2, BoomStand/Arm/Mic")],
       placements=lambda: [P((230, -1300, -120))],
       pivot="actor root on the floor", integration="Button0-2, Lamp0-2, Meter0-5 stay separate (lit/pressed by the code).", view=(-1, 0.5, 0.5))
def sound_console(a):
    a.box((80, 200, 86), at=(0, 0, 43), col=C.CHARCOAL, bevel=4)
    a.box((84, 204, 5), at=(0, 0, 88), col=C.MAGENTA, bevel=1.5)
    a.box((76, 190, 8), at=(-4, 0, 95), rot=(-16, 0, 0), col=C.GREY_DARK, bevel=2)
    for k in range(16):
        y = -84 + k * 11.2
        a.box((20, 1.6, 1), at=(14, y, 99), rot=(-16, 0, 0), col=C.INK, bevel=0)
        a.box((3, 5, 3), at=(14 + (k % 4) * 3 - 5, y, 100.5), rot=(-16, 0, 0), col=C.CREAM, bevel=0.8)
    a.box((4, 92, 28), at=(-12, 0, 132), col=C.CHARCOAL, bevel=1)
    D.border(a, (-14, 0, 132), "-x", 92, 28, bar=1.6, t=0.6, col=C.MAGENTA)
    for sy in (-1, 1):
        a.box((3, 3, 19), at=(-12, sy * 40, 109), col=C.GREY_DARK, bevel=0.6)
    # meter bridge behind the code's Meter0-5 (x 23..29), so the lit meters stay visible from the operator side
    a.box((6, 150, 18), at=(33, 0, 104), col=C.GREY_DARK, bevel=2)
    for k in range(6):
        D.border(a, (30, -50 + k * 20, 104), "-x", 14, 12, bar=1.0, t=0.6, col=C.CHARCOAL)
    # lamp stalks + cups under the code's Lamp0-2 (spheres at (-30, Y, 124))
    for y in (-60, 0, 60):
        a.cyl(1.6, 14, at=(-30, y, 112), col=C.GREY, sides=8, rough=0.4)
        a.cyl(5.5, 3, at=(-30, y, 118.5), col=C.CHARCOAL, sides=12, bevel=1)
    pn = D.Local(a, (-4, 0, 95), (-16, 0, 0))
    for k in range(14):
        y = -84 + k * 13
        if min(abs(y - c) for c in (-60, 0, 60)) < 19:
            continue
        for j in range(3):
            D.knob(pn, (-26 + j * 7, y, 4), "+z", r=1.9, h=1.6, col=[C.shade(C.CHARCOAL, 1.6), C.MAGENTA, C.CREAM][j])
    pn.box((4.5, 180, 0.5), at=(-33, 0, 4.3), col=C.CREAM, bevel=0.1, rough=0.9, jitter=0)
    for k in range(12):
        pn.box((1.6, 6 + (k * 5) % 4, 0.3), at=(-33, -80 + k * 14.5, 4.7), col=C.INK, bevel=0, jitter=0, wear=False)
    D.screws_rect(pn, (0, 0, 4), "+z", 74, 186, inset=2.5, r=0.8, col=C.GREY)
    # desk shell: name plates behind the code's Name0-2 texts, kick plate, vents; back: patch bay, rack unit, cables
    for y in (-60, 0, 60):
        D.plate(a, (-40, y, 72), "-x", 38, 13, t=0.6, col=C.INK, screws=False)
    D.inset_panel(a, (-40, 0, 36), "-x", 180, 44, C.CHARCOAL, frame_col=C.GREY_DARK, bar=2.0, t=1.0)
    a.box((2, 196, 8), at=(-40.5, 0, 4), col=C.RUBBER, bevel=0.6, rough=0.9)
    D.vent(a, (-40, 70, 36), "-x", 28, 18, slats=5, col=C.GREY_DARK)
    D.vent(a, (-40, -70, 36), "-x", 28, 18, slats=5, col=C.GREY_DARK)
    D.plate(a, (40, 0, 60), "+x", 120, 24, t=0.8, col=C.GREY_DARK)
    D.jacks(a, D.on((40, 0, 60), "+x", -22, 0, 0.8), "+x", cols=8, rows=3, pitch=4.5, r=1.1)
    D.rack_unit(a, D.on((40, 0, 60), "+x", 35, 0, 0.8), "+x", 36, 14, knobs=3, leds=2)
    D.label(a, (40, 0, 40), "+x", 30, 8, lines=2)
    for y in (-60, 60):
        D.vent(a, (40, y, 30), "+x", 30, 20, slats=6, col=C.CHARCOAL)
    D.stencil_number(a, (41, -60, 78), "+x", "FOH-2", 6, col=C.MAGENTA)
    for s in (-1, 1):
        y = s * 150
        a.box((60, 60, 140), at=(0, y, 70), col=C.CHARCOAL, bevel=5)
        a.cyl(20, 4, at=(-31, y, 55), rot=(90, 0, 0), col=C.GREY_DARK, sides=16, bevel=1.5)
        a.cyl(8, 6, at=(-32, y, 55), rot=(90, 0, 0), col=C.CHARCOAL, sides=12, bevel=2)
        a.cyl(10, 4, at=(-31, y, 110), rot=(90, 0, 0), col=C.GREY_DARK, sides=12, bevel=1)
        a.box((3, 30, 6), at=(-31, y, 20), col=C.INK, bevel=1)
        for sx in (-1, 1):
            for sz in (-1, 1):
                a.sphere(4, at=(sx * 28, y + sz * 28, 138), col=C.GREY, segs=6, rings=4, rough=0.35)
        # baffle frame, cone surrounds and bolts, dust cap, rear amp plate with heat-sink fins, side handles, feet
        D.border(a, (-30, y, 70), "-x", 54, 134, bar=2.4, t=1.2, col=C.shade(C.CHARCOAL, 1.25))
        a.torus(19, 1.6, at=(-33, y, 55), rot=(90, 0, 0), col=C.RUBBER, major=24, minor=5, rough=0.8)
        a.torus(13, 1.0, at=(-33, y, 55), rot=(90, 0, 0), col=C.shade(C.GREY_DARK, 0.8), major=20, minor=4)
        D.bolt_ring(a, (-33, y, 55), "-x", 22, n=8, r=0.9)
        D.bolt_ring(a, (-33, y, 110), "-x", 12, n=4, r=0.7)
        a.sphere(4, at=(-35, y, 55), ry=4, rz=4, col=C.GREY, segs=8, rings=4, rough=0.4)
        D.plate(a, (-30.5, y, 132), "-x", 20, 5, t=0.5, col=C.GREY, screws=False)
        D.plate(a, (30, y, 80), "+x", 40, 70, t=1.0, col=C.GREY_DARK)
        for k in range(7):
            a.box((3.5, 1.4, 44), at=(32.5, y - 15 + k * 5, 86), col=C.GREY, bevel=0.4, rough=0.4)
        D.jacks(a, (31, y + 8, 56), "+x", cols=2, rows=1, pitch=6, r=1.6)
        a.box((1.5, 5, 3), at=(31.6, y - 10, 56), col=C.RED, bevel=0.3)
        D.led(a, (31, y - 10, 62), "+x", r=0.6, col=C.GREEN)
        for n in ("+y", "-y"):
            yy = y + (30 if n == "+y" else -30)
            D.plate(a, (0, yy, 110), n, 22, 9, t=0.5, col=C.INK, screws=False)
            D.handle(a, (0, yy, 110), n, 16, along="u", r=1.0, standoff=2.2)
        D.feet(a, [(sx * 24, y + sy * 24, 0) for sx in (-1, 1) for sy in (-1, 1)], r=3.0, h=1.0)
        D.cable(a, [(31, y + 8, 56), (36, y + 6, 30), (38, y - s * 12, 2), (42, y - s * 60, 1.5), (41, s * 80, 14)], r=1.2)
    # coffee mug and a taped cue sheet on the right speaker
    a.cyl(4.5, 10, at=(12, 138, 145), col=C.WHITE, sides=12, bevel=1, rough=0.4)
    a.torus(3, 0.8, at=(12, 143.5, 145), rot=(-90, 0, 0), col=C.WHITE, major=10, minor=4)
    a.cyl(3.9, 0.4, at=(12, 138, 149.3), col=C.WOOD_DARK, sides=12, bevel=0, jitter=0)
    a.box((22, 16, 0.3), at=(-6, 150, 140.2), rot=(0, 12, 0), col=C.WHITE, bevel=0.05, rough=0.9, jitter=0)
    for k in range(4):
        a.box((14 - (k % 2) * 4, 0.8, 0.2), at=(-6 + 1.5 * (k % 2), 146 + k * 3, 140.4), rot=(0, 12, 0), col=C.INK, bevel=0, jitter=0, wear=False)
    D.tape(a, (-15, 146, 140), "+z", 3, 6, deg=12, col=C.CREAM)
    a.cyl(12, 3, at=(60, -120, 2), col=C.CHARCOAL, sides=12, bevel=1)
    a.cyl(2.5, 180, at=(60, -120, 90), col=C.CHARCOAL, sides=8)
    a.tube([(60, -120, 180), (120, -120, 185), (180, -120, 200)], 2, col=C.CHARCOAL, sides=6)
    # boom stand: weighted base, height clamp, boom knuckle with a counterweight stub, mic cable to the desk
    a.cyl(8, 4, at=(60, -120, 5.5), col=C.GREY_DARK, sides=12, bevel=1)
    a.cyl(3.4, 6, at=(60, -120, 110), col=C.GREY_DARK, sides=8, bevel=0.8)
    a.cyl(0.7, 5, at=(64, -120, 110), rot=(-90, 0, 0), col=C.CHROME, sides=6)
    a.box((1.4, 5, 1.4), at=(66.5, -120, 110), col=C.CHARCOAL, bevel=0.4)
    a.cyl(4.5, 7, at=(60, -120, 182), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=1)
    a.cyl(2.2, 12, at=(60, -128, 182), rot=(0, 0, 90), col=C.CHROME, sides=6, rough=0.3)
    a.tube([(60, -120, 180), (40, -120, 176)], 1.8, col=C.CHARCOAL, sides=6)
    a.cyl(4.5, 9, at=(36, -120, 175), rot=(-80, 0, 0), col=C.GREY_DARK, sides=10, bevel=1.5)
    D.cable_tie_run(a, [(176, -118, 197), (120, -118, 182), (64, -117, 178), (63, -117, 100), (63, -117, 8), (58, -110, 1.5),
                        (44, -100, 1.5), (41, -80, 12)], r=0.8, ties=4)
    fur = bm_sphere(10, 9, 22, 10, 8)
    from ftb.core import wobble
    wobble(fur, 2.2, scale=0.4, seed=5)
    a.add(fur, xf((190, -120, 203), (-70, 0, 0)), C.GREY, rough=1.0, jitter=0)


@asset("SM_Station_ConsoleButton", F,
       desc="Big arcade-style push button with a chrome bezel (neutral; the code colours it per cue).",
       replaces=[actor("AFTSoundConsole / AFTSharkRig", "FTStations.cpp / FTShark.cpp", "Button0-2, RigButton3 (round caps)")],
       pivot="cap centre (component location)", view=(1, 0.4, 0.8))
def console_button(a):
    a.cyl(17, 6, at=(0, 0, -3), col=C.CHROME, sides=16, bevel=2, rough=0.25)
    a.cyl(14, 10, at=(0, 0, 3), col=C.WHITE, sides=16, bevel=4, rough=0.35)
    D.bolt_ring(a, (0, 0, 0), "+z", 15.6, n=4, r=0.8)
    a.torus(9, 0.6, at=(0, 0, 8.1), col=C.shade(C.WHITE, 0.85), major=20, minor=3)
    a.cyl(14.3, 1.2, at=(0, 0, -1.2), col=C.shade(C.WHITE, 0.8), sides=16, bevel=0.3)


# ============================================================================== breaker + beacon

@asset("SM_Station_Breaker", F,
       desc="Yellow main-breaker cabinet: hazard-striped kick band, hinged door with a handle and vents, conduit junction box, MAIN BREAKER plate.",
       replaces=[actor("AFTBreaker", SRC_ST, "Panel, Stripe0-3, Door")],
       placements=lambda: [P((-575, 650, 0))],
       pivot="actor root on the landing", integration="LeverPivot (SM_Station_Breaker_Lever) and StatusLamp stay separate.", view=(1, 0.4, 0.4))
def breaker(a):
    a.box((22, 100, 150), at=(0, 0, 140), col=C.YELLOW, bevel=4)
    a.box((24, 104, 8), at=(0, 0, 212), col=C.shade(C.YELLOW, 0.85), bevel=2)
    D.hazard(a, (11, 0, 74), "+x", 96, 16, stripes=8, t=0.8)
    a.box((3, 80, 100), at=(12, 0, 150), col=C.hex_rgb(0xE8B42F), bevel=1.5)
    for k in range(4):
        a.box((1.5, 40, 2.5), at=(14, 0, 128 - k * 6), col=C.shade(C.YELLOW, 0.7), bevel=0.3)
    a.box((5, 6, 22), at=(14.5, -32, 150), col=C.GREY_DARK, bevel=1.5)
    a.box((3, 64, 14), at=(14.5, 0, 186), col=C.CREAM, bevel=1)
    a.box((14, 30, 8), at=(-2, 30, 220), col=C.GREY, bevel=2)
    # door hinges, lock, screws, plate lettering, warning sign; conduits out of the junction box into the wall
    for z in (115, 185):
        D.hinge(a, (14, 41, z), "z", 12, r=1.3, col=C.shade(C.YELLOW, 0.7))
    a.cyl(2.2, 2, at=(15.5, -32, 166), rot=(-90, 0, 0), col=C.CHROME, sides=10, bevel=0.4, rough=0.3)
    a.box((0.4, 0.6, 2.4), at=(16.6, -32, 166), col=C.INK, bevel=0, jitter=0, wear=False)
    D.screws_rect(a, (13.5, 0, 150), "+x", 80, 100, inset=3, r=0.8, col=C.GREY)
    for k in range(3):
        a.box((0.3, 44 - k * 10, 1.4), at=(16.1, -4 + k * 2, 189 - k * 3.2), col=C.INK, bevel=0, jitter=0, wear=False)
    a.prism((0.8, 14, 12), at=(14, 22, 150), rot=(0, 0, 0), col=C.YELLOW)
    a.prism((0.4, 11, 9.5), at=(14.5, 22, 149.5), col=C.INK)
    a.prism((0.3, 8, 6.5), at=(14.8, 22, 149), col=C.YELLOW)
    a.box((0.3, 1.2, 4.5), at=(15.1, 22, 148.5), rot=(0, 0, 20), col=C.INK, bevel=0, jitter=0, wear=False)
    for y in (22, 38):
        a.cyl(2.2, 8, at=(-12, y, 219), rot=(90, 0, 0), col=C.GREY, sides=8, rough=0.4)
        a.cyl(3, 2.5, at=(-9.6, y, 219), rot=(90, 0, 0), col=C.GREY_DARK, sides=8, bevel=0.6)
    D.screws_rect(a, (-2, 30, 224), "+z", 14, 30, inset=2, r=0.6)
    for n, y in (("-y", -50), ("+y", 50)):
        D.vent(a, (0, y, 190), n, 14, 18, slats=4, col=C.shade(C.YELLOW, 0.75))
    D.label(a, (0, -50, 120), "-y", 14, 12, lines=3)
    a.box((6, 100, 4), at=(-13, 0, 212), col=C.GREY_DARK, bevel=1)
    a.box((6, 100, 4), at=(-13, 0, 70), col=C.GREY_DARK, bevel=1)


@asset("SM_Station_Breaker_Lever", F,
       desc="Knife-switch lever: charcoal arm with a fat red grip.",
       replaces=[actor("AFTBreaker", SRC_ST, "LeverPivot: LeverArm, LeverGrip")],
       pivot="LeverPivot (18, 0, 150) - rotate this component to throw the switch", view=(1, 0.4, 0.3))
def breaker_lever(a):
    a.cyl(5, 14, at=(4, 0, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=1)
    a.box((8, 10, 50), at=(10, 0, 22), col=C.CHARCOAL, bevel=2)
    a.cyl(6, 34, at=(20, 0, 46), rot=(0, 0, 90), col=C.RED, sides=12, bevel=3, rough=0.5)
    for sy in (-1, 1):
        a.cyl(3, 1.5, at=(4, sy * 7.6, 0), rot=(0, 0, 90), col=C.CHROME, sides=6, bevel=0.3, rough=0.3)
        a.cyl(6.4, 1.2, at=(20, sy * 12, 46), rot=(0, 0, 90), col=C.shade(C.RED, 0.75), sides=12, bevel=0.3)
    for k in range(3):
        a.torus(6.1, 0.5, at=(20, -6 + k * 6, 46), rot=(0, 0, 90), col=C.shade(C.RED, 0.8), major=12, minor=3)
    a.box((2, 6, 30), at=(14.8, 0, 22), col=C.GREY_DARK, bevel=0.5)


@asset("SM_Station_EmergencyLight", F,
       desc="Emergency beacon: ribbed charcoal base with screw feet and a faceted red dome with an inner mirror (the dome glow is driven by the code).",
       replaces=[actor("AFTEmergencyLight", SRC_ST, "Base (the Dome is SM_Station_EmergencyDome)")],
       placements=lambda: [P(p) for p in ((2380, -1200, 900), (2380, 1500, 900), (700, -1580, 900), (700, 1980, 900))],
       pivot="actor root (base bottom centre)", integration="Base only: the Dome component keeps its glow toggle and gets SM_Station_EmergencyDome.", view=(1, 0.5, 0.4))
def emergency_light(a):
    a.cyl(17, 12, at=(0, 0, 6), col=C.CHARCOAL, sides=14, bevel=2)
    for k in range(3):
        a.torus(17, 1, at=(0, 0, 3 + k * 3.5), col=C.shade(C.CHARCOAL, 1.3), major=14, minor=4)
    a.cyl(13, 4, at=(0, 0, 13), col=C.GREY_DARK, sides=14, bevel=1)
    a.cyl(19, 1.2, at=(0, 0, 0.6), col=C.GREY_DARK, sides=16, bevel=0.3)
    D.bolt_ring(a, (0, 0, 1.2), "+z", 18, n=4, r=0.9)
    D.bolt_ring(a, (0, 0, 15), "+z", 11, n=6, r=0.6)
    a.cyl(2.6, 5, at=(-17.5, 0, 6), rot=(90, 0, 0), col=C.GREY, sides=8, bevel=0.5)
    D.cable(a, [(-20, 0, 6), (-22, 0, 3), (-22, 0, 0.8)], r=1.0)
    D.label(a, (0, 17, 6), "+y", 8, 3.5, lines=1)


@asset("SM_Station_EmergencyDome", F,
       desc="Faceted red beacon dome with a cage.", replaces=[actor("AFTEmergencyLight", SRC_ST, "Dome")],
       pivot="dome centre (Dome component location (0, 0, 22))", view=(1, 0.5, 0.3))
def emergency_dome(a):
    a.sphere(14, ry=14, rz=16, col=C.RED, segs=10, rings=7, flat=True, rough=0.3)
    for k in range(4):
        a.torus(14.2, 0.8, at=(0, 0, 0), rot=(90, k * 45, 0), col=C.GREY_DARK, major=16, minor=4)
    a.torus(14.3, 0.8, at=(0, 0, 0), col=C.GREY_DARK, major=16, minor=4)
    a.torus(10.6, 0.8, at=(0, 0, 10.5), col=C.GREY_DARK, major=14, minor=4)
    a.cyl(3.2, 3, at=(0, 0, 16), col=C.GREY_DARK, sides=8, bevel=0.8)
    a.cyl(14.5, 2.5, at=(0, 0, -13), col=C.GREY_DARK, sides=16, bevel=0.6)


# ============================================================================== effect machines

def cart_details(a, sx, sy, z_top, col=C.GREY_DARK, handle_x=None, handle_h=80, wheel_r=9, wheels=None):
    """Shared trolley dressing: side skirt panels with screws, corner bumpers, tie-down rings, wheels with hubs and
    treads, and optionally a push handle at the back (x = handle_x)."""
    zc = z_top - 8
    for n, c, w in (("+y", (0, sy / 2, zc), sx), ("-y", (0, -sy / 2, zc), sx), ("+x", (sx / 2, 0, zc), sy), ("-x", (-sx / 2, 0, zc), sy)):
        D.seams(a, c, n, w - 10, 12, n=1, along="u", col=C.shade(col, 0.6), width=0.6)
        for du in (-w / 2 + 7, w / 2 - 7):
            D.screw(a, D.on(c, n, du, 0), n, r=0.8, col=C.GREY)
    for sxx in (-1, 1):
        for syy in (-1, 1):
            a.box((5, 5, 14), at=(sxx * (sx / 2 - 1.5), syy * (sy / 2 - 1.5), zc), col=C.RUBBER, bevel=1.5, rough=0.9)
    for y in (-sy / 2, sy / 2):
        a.torus(2.2, 0.6, at=(0, y + (0.8 if y > 0 else -0.8), z_top - 2), rot=(-90, 0, 0), col=C.CHROME, major=10, minor=4)
    for (x, y) in (wheels or []):
        D.wheel(a, (x, y, wheel_r), wheel_r, 8, "y", treads=10, bolts=4)
    if handle_x is not None:
        for syy in (-1, 1):
            a.tube([(handle_x + 6, syy * (sy / 2 - 8), z_top), (handle_x, syy * (sy / 2 - 8), z_top + handle_h - 6),
                    (handle_x - 1, syy * (sy / 2 - 12), z_top + handle_h)], 1.8, col=C.GREY, sides=8, rough=0.4)
        a.cyl(2.4, sy - 22, at=(handle_x - 1, 0, z_top + handle_h), rot=(0, 0, 90), col=C.RUBBER, sides=10, bevel=0.6)



@asset("SM_Station_WindMachine", F,
       desc="Wind machine: wheeled cart with a teal fan shroud, safety grille rings, motor pod and cable.",
       replaces=[actor("AFTEffectMachine (Wind)", SRC_ST, "BuildLook() Wind parts: base, wheels, stand, torus shroud, motor, cross bars")],
       placements=lambda: [P((1150, 1350, -120), (0, -60, 0))],
       pivot="actor root on the floor; blows along +X", integration="Blades stay on the Spinner component (SM_Station_WindMachine_Blades).", view=(1, 0.7, 0.4))
def wind_machine(a):
    a.box((90, 70, 16), at=(0, 0, 20), col=C.GREY_DARK, bevel=4)
    cart_details(a, 90, 70, 28, wheels=[(sx * 34, sy * 28) for sx in (-1, 1) for sy in (-1, 1)])
    for sy in (-1, 1):
        a.box((14, 8, 80), at=(0, sy * 30, 64), col=C.CHARCOAL, bevel=2)
        a.box((20, 12, 3), at=(0, sy * 30, 29.5), col=C.GREY, bevel=0.8)
        D.bolt_ring(a, (0, sy * 30, 31), "+z", 5, n=4, r=0.6)
        a.cyl(4, 12, at=(0, sy * 30, 104), rot=(0, 0, 90), col=C.GREY, sides=10, bevel=1)
        a.cyl(2.5, 5, at=(0, sy * 37.5, 104), rot=(0, 0, 90), col=C.AMBER, sides=8, bevel=0.6)
        a.box((1.6, 1.6, 9), at=(0, sy * 40.5, 104), col=C.AMBER, bevel=0.4)
    a.torus(62, 12, at=(0, 0, 120), rot=(90, 0, 0), col=C.TEAL, major=24, minor=8, rz=14)
    for k, r in enumerate((56, 38, 20)):
        a.torus(r, 1.5, at=(18, 0, 120), rot=(90, 0, 0), col=C.GREY, major=20, minor=4)
    a.box((2, 118, 3), at=(18, 0, 120), col=C.GREY, bevel=0.5)
    a.box((2, 3, 118), at=(18, 0, 120), col=C.GREY, bevel=0.5)
    a.cyl(24, 40, at=(-26, 0, 120), rot=(90, 0, 0), col=C.TEAL_DARK, sides=14, bevel=4)
    a.cyl(12, 10, at=(-48, 0, 120), rot=(90, 0, 0), col=C.CHARCOAL, sides=12, bevel=3)
    a.tube([(-50, 0, 112), (-50, 10, 60), (-38, 20, 25)], 2, col=C.RUBBER, sides=6)
    # shroud rim bolts, motor cooling ribs, nameplate, grille mounting clips; speed controller on the cart
    for k in range(12):
        ang = math.radians(k * 30 + 15)
        D.screw(a, (12.2, 62 * math.cos(ang), 120 + 62 * math.sin(ang)), "+x", r=1.1, col=C.GREY)
    for k in range(10):
        ang = k * 36
        c = Vector((-26, 24.6 * math.cos(math.radians(ang)), 120 + 24.6 * math.sin(math.radians(ang))))
        a.box((30, 1.6, 2.2), at=c, rot=(0, 0, ang + 90), col=C.shade(C.TEAL_DARK, 0.8), bevel=0.4)
    D.plate(a, (-26, 0, 144.5), "+z", 16, 7, t=0.6, col=C.GREY)
    a.cyl(8, 3, at=(-54, 0, 120), rot=(90, 0, 0), col=C.GREY_DARK, sides=10, bevel=0.8)
    D.grille_holes(a, (-55.5, 0, 120), "-x", 10, 10, pitch=2.6, r=0.6)
    for k in range(8):
        ang = math.radians(k * 45)
        a.box((3, 3.5, 2), at=(17.5, 57 * math.cos(ang), 120 + 57 * math.sin(ang)), rot=(k * 45, 0, 0), col=C.GREY_DARK, bevel=0.4)
    a.box((16, 14, 11), at=(24, -18, 33.5), col=C.CHARCOAL, bevel=2)
    D.knob(a, (32, -18, 35), "+x", r=3.2, h=2.4, col=C.CORAL)
    D.led(a, (32, -12, 37), "+x", r=0.6, col=C.GREEN)
    D.label(a, (24, -18, 39), "+z", 12, 6, lines=1)
    D.cable(a, [(24, -10, 34), (10, 4, 29), (-30, 16, 29), (-38, 20, 25)], r=1.1)
    for s_ in (-1, 1):
        a.box((24, 13, 7), at=(-30, s_ * 22, 31.5), rot=(0, s_ * 6, 0), col=C.shade(C.ORANGE, 0.72), bevel=3.2, rough=0.95)
        a.box((3, 14, 7.4), at=(-30, s_ * 22, 31.6), rot=(0, s_ * 6, 0), col=C.CHARCOAL, bevel=1, rough=0.9)


@asset("SM_Station_WindMachine_Blades", F,
       desc="Four-blade yellow propeller with a spinner cap.", replaces=[actor("AFTEffectMachine (Wind)", SRC_ST, "Blade0-3 on Spinner")],
       pivot="Spinner component (4, 0, 120); rotates about X", view=(1, 0.3, 0.2))
def wind_blades(a):
    for b in range(4):
        blade = bm_box(5, 20, 50, 2.2, 1)
        deform(blade, lambda c: Vector((c.x, c.y * (0.7 + 0.5 * (c.z + 25) / 50), c.z)))
        a.add(blade, xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, 0, 30)), C.YELLOW, rough=0.5)
        # white safety tip, root clamp with two bolts, a balancing weight
        tip = bm_box(5.4, 30.5, 7, 1.2, 1)
        a.add(tip, xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, 0, 51.5)), C.WHITE, rough=0.5)
        a.add(bm_box(7, 12, 8, 1.5, 1), xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, 0, 12)), C.GREY_DARK, rough=0.4)
        for dy in (-3.5, 3.5):
            from ftb.core import bm_cyl
            a.add(bm_cyl(1.0, 9, 6), xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, dy, 12), (-90, 0, 0)), C.CHROME, rough=0.3)
        a.add(bm_box(5.8, 4, 3, 0.8, 1), xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, 7, 36)), C.GREY, rough=0.4)
    a.sphere(11, ry=11, rz=11, at=(6, 0, 0), col=C.CORAL, segs=10, rings=6)
    a.cyl(13, 4, at=(-2, 0, 0), rot=(-90, 0, 0), col=C.GREY_DARK, sides=14, bevel=1)
    D.bolt_ring(a, (0, 0, 0), "+x", 10.5, n=6, r=0.9)


@asset("SM_Station_RainMachine", F,
       desc="Rain pump cart: teal water tank with a coral filler cap, yellow pump drum, gauges, hose reel and wheels.",
       replaces=[actor("AFTEffectMachine (Rain)", SRC_ST, "cart: base, wheels, tank, pump capsule, cap")],
       placements=lambda: [P((950, 1500, -120))],
       pivot="actor root on the floor", integration="The overhead spray rig is SM_Station_RainRig (Rig component at EffectCenter).", view=(1, 0.7, 0.4))
def rain_machine(a):
    a.box((90, 60, 16), at=(0, 0, 22), col=C.GREY_DARK, bevel=4)
    cart_details(a, 90, 60, 30, handle_x=-49, handle_h=62, wheels=[(sx * 32, sy * 24) for sx in (-1, 1) for sy in (-1, 1)])
    a.box((50, 54, 50), at=(-12, 0, 55), col=C.TEAL, bevel=8)
    # tank: weld seam, sight glass, drain cock, stencil, hose coil on top
    a.box((51, 55, 1.6), at=(-12, 0, 55), col=C.shade(C.TEAL, 0.8), bevel=0.6)
    a.cyl(2.2, 36, at=(-24, -28.5, 55), col=C.CYAN, sides=8, bevel=0.4, mat="glass")
    a.cyl(1.3, 22, at=(-24, -28.6, 50), col=C.hex_rgb(0x2E78B8), sides=8, bevel=0.2)
    for z in (37, 73):
        a.box((6, 4, 3), at=(-24, -28, z), col=C.GREY, bevel=0.6)
    for k in range(4):
        a.box((2.4, 0.4, 0.5), at=(-21.4, -29, 42 + k * 8), col=C.INK, bevel=0, jitter=0, wear=False)
    a.cyl(2, 7, at=(-40, 12, 36), rot=(90, 0, 0), col=C.GREY, sides=8)
    a.box((1.5, 8, 2), at=(-44, 12, 38), col=C.RED, bevel=0.5)
    D.stencil_number(a, (-12, 27, 58), "+y", "H2O", 9, col=C.WHITE)
    for k in range(3):
        a.torus(9.5 - k * 0.5, 1.6, at=(-25 + k * 0.6, 12 - k * 0.4, 81.5 + k * 2.6), col=C.YELLOW, major=18, minor=5, rough=0.8)
    D.screws_rect(a, (-12, 0, 80), "+z", 30, 30, inset=2, r=0.8)
    a.cyl(9, 10, at=(-12, 0, 84), col=C.CORAL, sides=12, bevel=2)
    a.cyl(15, 50, at=(24, 0, 58), rot=(0, 0, 90), col=C.YELLOW, sides=14, bevel=5)
    for sy in (-1, 1):
        a.cyl(15.5, 4, at=(24, sy * 20, 58), rot=(0, 0, 90), col=C.shade(C.YELLOW, 0.8), sides=14, bevel=1)
    a.cyl(6, 3, at=(24, -26, 72), rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=0.8)
    a.tube([(38, 0, 58), (45, 2, 68), (42, 6, 80), (30, 10, 86)], 3, col=C.YELLOW, sides=8)
    # pump: end-bell bolts, gauge rim + needle, motor fins, hose coupling and clamp
    for sy in (-1, 1):
        D.bolt_ring(a, (24, sy * 25, 58), "+y" if sy > 0 else "-y", 11, n=6, r=0.9)
    a.torus(6, 0.8, at=(24, -27.6, 72), rot=(0, 0, 90), col=C.CHROME, major=14, minor=4)
    a.box((0.4, 0.3, 4.5), at=(24.8, -28, 72.8), rot=(0, 0, 0), col=C.RED, bevel=0, jitter=0, wear=False)
    a.cyl(1.5, 6, at=(24, -24, 66), col=C.GREY, sides=6)
    for k in range(5):
        ang = 50 + k * 20
        a.box((1.4, 34, 2.4), at=(24 + 15.6 * math.cos(math.radians(ang)), 0, 58 + 15.6 * math.sin(math.radians(ang))),
              rot=(ang - 90, 0, 0), col=C.shade(C.YELLOW, 0.72), bevel=0.3)
    a.cyl(4, 5, at=(30, 10, 86), rot=(-60, 20, 0), col=C.GREY, sides=10, bevel=0.8)
    a.torus(3.4, 0.7, at=(43.5, 2.5, 70), rot=(-20, 0, 0), col=C.CHROME, major=10, minor=4)


@asset("SM_Station_RainRig", F,
       desc="Overhead rain bar: charcoal spray boom with cyan nozzles, cross bar and two drop poles.",
       replaces=[actor("AFTEffectMachine (Rain)", SRC_ST, "RigPart0-5 at EffectCenter (bars, poles, nozzles)")],
       placements=lambda: [P((1800, 100, 400))],
       pivot="Rig component (EffectCenter (850, -1400, 520) is relative to the RainMachine actor -> world (1800, 100, 400) above the tank); modelled for EffectExtent (420, 900): boom 1800 long along Y", view=(0.6, 1, 0.3))
def rain_rig(a):
    ey = 900
    a.cyl(7, ey * 2, rot=(0, 0, 90), col=C.CHARCOAL, sides=10, bevel=0)
    a.cyl(6, 420 * 1.6, rot=(90, 0, 0), col=C.CHARCOAL, sides=10, bevel=0)
    for s in (-1, 1):
        a.cyl(2, 500, at=(0, s * ey * 0.8, 250), col=C.GREY, sides=6)
    for k in range(9):
        y = -ey + 100 + k * 200
        a.cone(8, 14, at=(0, y, -12), rot=(180, 0, 0), col=C.CYAN, sides=8)
        a.cyl(3, 6, at=(0, y, -4), col=C.GREY, sides=6)
        # nozzle: hex nut, spray face with holes
        a.cyl(4.2, 3, at=(0, y, -6.5), col=C.GREY_DARK, sides=6, bevel=0.3, rough=0.4)
        D.grille_holes(a, (0, y, -19.1), "-z", 8, 8, pitch=2.6, r=0.5)
    # pipe couplings along the boom and the cross bar, feed hose on top, pole clamps with bolts, end valves
    for k in range(7):
        y = -ey + 150 + k * 250
        a.cyl(8.6, 7, at=(0, y, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=1)
        D.bolt_ring(a, (0, y + 3.6, 0), "+y", 6.5, n=4, r=0.7)
    for x in (-250, 250):
        a.cyl(7.6, 7, at=(x, 0, 0), rot=(90, 0, 0), col=C.GREY_DARK, sides=10, bevel=1)
    a.box((22, 22, 16), at=(0, 0, 0), col=C.CHARCOAL, bevel=3)
    D.screws_rect(a, (0, 0, 8), "+z", 22, 22, inset=2.5, r=0.9)
    D.cable_tie_run(a, [(0, -ey + 40, 10), (0, 0, 11), (0, ey - 40, 10)], r=2.2, col=C.YELLOW, ties=8)
    D.cable(a, [(0, 0, 11), (-6, -8, 40), (-8, -12, 500)], r=2.2, col=C.YELLOW)
    for s_ in (-1, 1):
        y = s_ * ey * 0.8
        a.box((10, 10, 22), at=(0, y, 4), col=C.GREY, bevel=1.5, rough=0.4)
        for z in (-3, 11):
            a.cyl(0.9, 12, at=(0, y, z), rot=(-90, 0, 0), col=C.CHROME, sides=6)
        a.cyl(2.8, 6, at=(0, y, 30), col=C.GREY_DARK, sides=8, bevel=0.6)
        a.cyl(3, 10, at=(0, s_ * (ey + 4), 0), rot=(0, 0, 90), col=C.GREY, sides=8, bevel=0.8)
        a.cyl(1, 8, at=(0, s_ * (ey + 6), 6), col=C.GREY, sides=6)
        a.box((12, 2, 2.4), at=(0, s_ * (ey + 6), 10), col=C.RED, bevel=0.5)
    for x in (-336, 336):
        a.cyl(7.6, 6, at=(x, 0, 0), rot=(90, 0, 0), col=C.CHARCOAL, sides=10, bevel=1.5)


@asset("SM_Station_SmokeMachine", F,
       desc="Hazer/smoke machine: charcoal box with vent slots, nozzle, fluid bottle, handle and caster wheels.",
       replaces=[actor("AFTEffectMachine (Smoke)", SRC_ST, "BuildLook() Smoke parts")],
       placements=lambda: [P((750, 1650, -120), (0, -80, 0))], pivot="actor root on the floor; nozzle along +X", view=(1, 0.7, 0.4))
def smoke_machine(a):
    a.box((70, 50, 50), at=(0, 0, 45), col=C.CHARCOAL, bevel=6)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(6, 6, at=(sx * 26, sy * 20, 9), rot=(0, 0, 90), col=C.RUBBER, sides=10, bevel=1.5)
            a.box((6, 6, 8), at=(sx * 26, sy * 20, 17), col=C.GREY, bevel=1)
    for k in range(4):
        a.box((24, 1.5, 4), at=(-8, 25.5, 36 + k * 8), col=C.INK, bevel=0.5)
    a.cyl(11, 22, at=(42, 0, 45), rot=(-90, 0, 0), col=C.GREY_DARK, sides=12, bevel=2)
    a.cyl(6, 6, at=(54, 0, 45), rot=(-90, 0, 0), col=C.INK, sides=10)
    a.box((2, 30, 20), at=(-36, 0, 45), col=C.CORAL, bevel=1)
    a.tube([(-20, 0, 70), (-20, 0, 82), (20, 0, 82), (20, 0, 70)], 2.5, col=C.GREY_DARK, sides=8)
    a.cyl(7, 16, at=(-24, 13, 78), col=C.TEAL_LIGHT, sides=10, bevel=2, rough=0.3)
    a.cyl(3, 3, at=(-24, 13, 87), col=C.CORAL, sides=8, bevel=0.8)
    # detail pass: fluid level + tube into the unit, control panel with display, heat warning at the nozzle,
    # nozzle bolt ring, back power inlet and cable, screwed side panels, stencil
    a.cyl(6.2, 7, at=(-24, 13, 75), col=C.hex_rgb(0x9BE0D8), sides=10, bevel=0.5, rough=0.2)
    D.cable(a, [(-24, 13, 86), (-18, 12, 88), (-12, 10, 84), (-12, 10, 70.5)], r=0.6, col=C.WHITE)
    D.plate(a, (8, -13, 70), "+z", 24, 16, t=0.8, col=C.GREY_DARK)
    a.box((10, 6, 0.6), at=(3, -13, 71.2), col=C.hex_rgb(0x2A9D5B), glow=1.2)
    for k in range(3):
        D.knob(a, (12 + k * 5, -15, 70.8), "+z", r=1.6, h=1.4, col=C.CORAL if k == 0 else C.GREY)
    D.led(a, (16, -8, 70.8), "+z", r=0.6, col=C.RED)
    D.bolt_ring(a, (53, 0, 45), "+x", 9, n=6, r=0.8)
    D.hazard(a, (35, 0, 62), "+x", 20, 5, stripes=5)
    a.prism((0.6, 7, 6.5), at=(35.4, -15, 60), col=C.YELLOW)
    a.prism((0.3, 4.8, 4.2), at=(35.8, -15, 59.6), col=C.INK)
    D.plate(a, (-35, 0, 45), "-x", 26, 18, t=0.6, col=C.GREY_DARK, screws=False)
    a.box((1.5, 8, 6), at=(-38.5, -6, 48), col=C.INK, bevel=0.4)
    a.box((1.5, 3, 4), at=(-38.5, 6, 48), col=C.RED, bevel=0.4)
    D.cable(a, [(-39, -6, 46), (-42, -6, 38), (-43, -2, 12), (-41, 6, 1.5), (-30, 20, 1.5)], r=1.1)
    for n, y in (("+y", 25), ("-y", -25)):
        D.screws_rect(a, (0, y, 45), n, 64, 44, inset=4, r=0.8, col=C.GREY)
    D.stencil_number(a, (4, -25, 38), "-y", "HAZE", 8, col=C.CORAL)
    D.label(a, (-20, -25, 58), "-y", 12, 6, lines=2)


@asset("SM_Station_FoamCannon", F,
       desc="Foam cannon: yellow pressure tank on a wheeled base, raked teal barrel with a coral muzzle ring, pressure gauge and trigger box.",
       replaces=[actor("AFTEffectMachine (Foam)", SRC_ST, "BuildLook() Foam parts")],
       placements=lambda: [P((1000, 1760, -120), (0, -65, 0))], pivot="actor root on the floor; fires along +X", view=(1, 0.7, 0.4))
def foam_cannon(a):
    a.box((90, 60, 16), at=(0, 0, 22), col=C.GREY_DARK, bevel=4)
    cart_details(a, 90, 60, 30, wheels=[(sx * 32, sy * 24) for sx in (-1, 1) for sy in (-1, 1)])
    a.cyl(20, 50, at=(-10, 0, 55), rot=(0, 0, 90), col=C.YELLOW, sides=14, bevel=9, rough=0.4)
    # tank saddles with straps, weld bands, relief valve; barrel yoke, bands and hose; muzzle mesh
    for y in (-16, 16):
        a.box((30, 6, 10), at=(-10, y, 34), col=C.CHARCOAL, bevel=1.5)
        a.torus(20.4, 1.0, at=(-10, y, 55), rot=(0, 0, 90), col=C.GREY_DARK, major=18, minor=4)
    a.torus(20.3, 0.7, at=(-10, 0, 55), rot=(0, 0, 90), col=C.shade(C.YELLOW, 0.75), major=18, minor=4)
    a.cyl(2, 8, at=(-2, 0, 78), col=C.GREY, sides=8)
    a.cyl(3.4, 3, at=(-2, 0, 83), col=C.RED, sides=8, bevel=0.8)
    D.stencil_number(a, (-10, -20.5, 55), "-y", "FOAM", 8, col=C.INK)
    fr = (-65, 0, 0)
    from ftb.core import ue_rot
    ax = ue_rot(*fr) @ Vector((0, 0, 1))
    for t in (-30, 5, 40):
        p = Vector((34, 0, 92)) + ax * t
        a.cyl(14.8, 3, at=p, rot=fr, col=C.TEAL_DARK, sides=14, bevel=0.8)
    for sy in (-1, 1):
        a.box((24, 4, 44), at=(18, sy * 16, 58), rot=(-25, 0, 0), col=C.CHARCOAL, bevel=1.5)
        a.cyl(4, 6, at=(26, sy * 16.5, 76), rot=(0, 0, 90), col=C.GREY, sides=10, bevel=1)
    tip = Vector((34, 0, 92)) + ax * 55.1
    D.grille_holes(D.Local(a, tuple(tip + ax * 0.2), fr), (0, 0, 0), "+z", 18, 18, pitch=3.2, r=0.8)
    a.tube([(-10, 10, 76), (-4, 14, 92), (10, 15, 96), (22, 12, 90)], 2.2, col=C.RUBBER, sides=8, rough=0.85)
    a.cyl(14, 110, at=(34, 0, 92), rot=(-65, 0, 0), col=C.TEAL, sides=14, bevel=3)
    a.cyl(17, 10, at=(80, 0, 112), rot=(-65, 0, 0), col=C.CORAL, sides=14, bevel=3)
    a.box((10, 8, 24), at=(-30, 0, 90), col=C.CORAL, bevel=2)
    a.cyl(6, 3, at=(-10, -21, 64), rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=0.8)
    a.torus(6, 0.8, at=(-10, -22.6, 64), rot=(0, 0, 90), col=C.CHROME, major=14, minor=4)
    a.box((0.4, 0.3, 4.2), at=(-9.3, -22.9, 65), rot=(0, 0, 0), col=C.RED, bevel=0, jitter=0, wear=False)
    a.box((6, 3, 8), at=(-30, 0, 104), rot=(0, 0, 0), col=C.CHARCOAL, bevel=1)
    a.box((3, 6, 10), at=(-24, 0, 84), rot=(0, 20, 0), col=C.RED, bevel=0.8)
    D.label(a, (-30, -4, 90), "-y", 7, 10, lines=2)


# ============================================================================== fin glider

@asset("SM_Station_FinGlider_Fin", F,
       desc="Rubber shark fin on a rail shoe: curved fin with a darker trailing edge and a scar.",
       replaces=[actor("AFTFinGlider", SRC_ST, "Fin: FinBody, FinEdge")],
       pivot="Fin scene component (0, 0, -80 in the actor); the fin grows up from Z 0", view=(0.3, 1, 0.3))
def fin_glider_fin(a):
    prof = [(-40, 0), (40, 0), (22, 20), (2, 50), (-6, 72), (-14, 60), (-26, 30)]
    a.poly(prof, 10, col=C.BLUE, bevel=2.5, rough=0.6)
    a.poly([(10, 0), (40, 0), (22, 20), (6, 40), (0, 30)], 12, at=(-0.5, 0, 0), col=C.DEEP_BLUE, bevel=2, rough=0.6)
    a.box((12, 3, 1.5), at=(5.5, -10, 40), rot=(0, 90, 30), col=C.WHITE, bevel=0.3)
    a.box((20, 90, 8), at=(0, 0, -4), col=C.CHARCOAL, bevel=2)
    # detail pass: bolted root clamps both sides, a gaffer-tape repair, a second scar, rail rollers + shoe bolts
    for sx in (-1, 1):
        a.box((1.6, 70, 8), at=(sx * 5.8, -6, 4), col=C.GREY_DARK, bevel=0.5)
        for y in (-34, -14, 6, 24):
            D.screw(a, (sx * 6.6, y, 4), "+x" if sx > 0 else "-x", r=1.0, col=C.GREY)
        D.tape(a, (sx * 5.1, 6, 26), "+x" if sx > 0 else "-x", 16, 5, deg=-18 * sx, col=C.GREY)
    a.box((12, 3, 1.5), at=(-5.5, 14, 22), rot=(0, 90, -20), col=C.WHITE, bevel=0.3)
    for y in (-36, 36):
        for sx in (-1, 1):
            D.wheel(a, (sx * 11.5, y, -5), 3.2, 3, "x", tire=C.GREY, hub=C.CHROME, bolts=0)
    for y in (-40, 40):
        D.screw(a, (0, y, 0.1), "+z", r=1.2, col=C.GREY)
    D.stencil_number(a, (10.2, 30, -4), "+x", "FIN-2", 4, col=C.YELLOW)


@asset("SM_Station_FinGlider_Crank", F,
       desc="Crank post with a coral hand wheel and handle.",
       replaces=[actor("AFTFinGlider", SRC_ST, "CrankPost, CrankWheel")],
       placements=lambda: [P((1700, -750, -50))], pivot="actor root", view=(1, 0.5, 0.4))
def fin_glider_crank(a):
    a.cyl(12, 6, at=(0, -60, 3), col=C.GREY_DARK, sides=12, bevel=2)
    a.cyl(5, 80, at=(0, -60, 40), col=C.CHARCOAL, sides=10)
    a.torus(17, 3.5, at=(0, -60, 84), rot=(0, 0, 90), col=C.CORAL, major=18, minor=7)
    for k in range(3):
        a.box((32, 3, 3), at=(0, -60, 84), rot=(k * 60, 0, 0), col=C.CORAL, bevel=0.5)
    a.cyl(3, 12, at=(0, -54, 100), rot=(0, 0, 90), col=C.CREAM, sides=8, bevel=1)
    # base bolts, gearbox with a ratchet pawl, hub nut, warning label, drive cable down to the floor
    D.bolt_ring(a, (0, -60, 6), "+z", 9, n=4, r=1.0)
    a.box((12, 12, 12), at=(0, -60, 76), col=C.GREY_DARK, bevel=2)
    D.screws_rect(a, (6, -60, 76), "+x", 12, 12, inset=2, r=0.6)
    a.cyl(3, 6, at=(0, -60, 84), rot=(0, 0, 90), col=C.CHROME, sides=6, rough=0.3)
    a.box((2, 6, 1.5), at=(-5, -54.5, 80), rot=(0, 0, 25), col=C.CORAL_DARK, bevel=0.4)
    D.label(a, (5, -60, 40), "+x", 7, 12, lines=3)
    D.cable(a, [(0, -65.5, 70), (0, -66, 20), (0, -63, 3), (0, -52, 1.2)], r=0.8, col=C.GREY)
    for k in range(6):
        ang = math.radians(k * 60 + 30)
        a.sphere(1.6, at=(17 * math.cos(ang), -60, 84 + 17 * math.sin(ang)), col=C.CORAL_DARK, segs=6, rings=3)


# ============================================================================== shark rig + shark

@asset("SM_Shark_Body", F,
       desc="Rubber movie shark: chunky body with a pale belly, gill slits, dorsal/pectoral/tail fins, googly eyes with pupils and angry brows (jaw separate).",
       replaces=["FFTSharkParts::Build (Production/FTShark.cpp:38-70): SharkBody, Belly, Snout, TailStalk, Tail, Dorsal, FinL/R, Eyes, Pupils, Brows"],
       pivot="SharkRoot (scale 1 = rig shark; AFTSharkHazard uses the same mesh scaled)",
       integration="Eyes are baked in; if the code must animate EyeL/EyeR keep the ball components. Jaw = SM_Shark_Jaw on the Jaw component.", view=(0.4, 1, 0.3))
def shark_body(a):
    body = C.hex_rgb(0x6C7FA3)
    dark = C.hex_rgb(0x4E5E82)
    secs = []
    prof = [(-240, 20, 18), (-200, 34, 34), (-140, 60, 60), (-60, 80, 74), (20, 82, 76), (100, 72, 64), (160, 54, 50), (205, 26, 30), (222, 4, 8)]
    for (x, ry, rz) in prof:
        loop = []
        for k in range(14):
            t = 2 * math.pi * k / 14
            z = rz * math.sin(t) * (1.0 if math.sin(t) > 0 else 0.85)
            loop.append((x, ry * math.cos(t), z + (6 if x > 60 else 0)))
        secs.append(loop)
    a.loft(secs, col=body, rough=0.5)
    belly = []
    for (x, ry, rz) in prof[1:-1]:
        loop = []
        for k in range(10):
            t = math.pi + math.pi * k / 9
            loop.append((x + 2, ry * 0.86 * math.cos(t), rz * 0.8 * math.sin(t) - 6 + (6 if x > 60 else 0)))
        belly.append(loop)
    a.loft(belly, col=C.WHITE, caps=False, rough=0.5)
    # profiles are (u, z) with u pointing backwards (-X) after the yaw
    fin = [(-50, 0), (50, 0), (35, 20), (25, 50), (22, 100), (12, 98), (-20, 50)]
    a.poly(fin, 14, at=(-20, 0, 62), rot=(0, 90, 0), col=dark, bevel=3)
    for s in (-1, 1):
        a.poly([(-30, 0), (30, 0), (5, 20), (-30, 50)], 8, at=(40, s * 78, -30), rot=(0, 90 + s * 20, s * 60), col=dark, bevel=2)
    tail = [(0, 12), (55, 85), (78, 90), (50, 18), (58, -42), (40, -46), (0, -12)]
    a.poly(tail, 10, at=(-232, 0, 20), rot=(0, 90, 0), col=dark, bevel=3)
    for k in range(3):
        for s in (-1, 1):
            a.box((3, 2, 34), at=(88 - k * 12, s * 76, 12), rot=(0, s * 12, 0), col=dark, bevel=0.8)
    for s in (-1, 1):
        a.sphere(15, at=(150, s * 50, 44), ry=10, rz=17, col=C.WHITE, segs=12, rings=8)
        a.sphere(7, at=(160, s * 57, 46), ry=4, rz=9, col=C.INK, segs=10, rings=6)
        a.sphere(2, at=(163, s * 58, 51), col=C.WHITE, segs=6, rings=4, glow=0.5)
        a.box((8, 30, 7), at=(158, s * 50, 66), rot=(0, 0, s * -15), col=dark, bevel=2)
        # eyelid rims, nostrils, mould seam along the flank, scars, a gaffer-tape patch
        a.torus(12, 1.6, at=(151, s * 55.5, 44), rot=(0, 0, 90 + s * 18), col=dark, major=16, minor=4, rz=15.5)
        a.sphere(3.2, at=(205, s * 12, 30), ry=1.6, rz=2, rot=(0, s * 25, 0), col=C.INK, segs=8, rings=4)
        for (x, z, deg) in ((40, 30, 20), (-40, 40, -10), (-120, 28, 30)):
            ry = next(p[1] for p in prof if p[0] >= x)
            a.box((14, 1.2, 1.6), at=(x, s * (ry * 0.97), z), rot=(0, s * 4, deg), col=C.WHITE, bevel=0.3)
        seam = []
        for (x, ry, rz) in prof[1:-1]:
            seam.append((x, s * (ry + 0.3), 4 + (6 if x > 60 else 0)))
        a.tube(seam, 0.9, col=dark, sides=5)
    a.box((26, 10, 1.4), at=(-30, -61, 44), rot=(0, 10, -8), col=C.GREY, bevel=0.2, rough=0.95)
    a.box((24, 9, 1.4), at=(-26, -61.5, 47), rot=(0, 10, 32), col=C.shade(C.GREY, 0.9), bevel=0.2, rough=0.95)
    # rig mount under the belly: bolted plate with a collar (the arm plugs in here)
    a.box((50, 36, 4), at=(-20, 0, -64), col=C.GREY_DARK, bevel=1.2, rough=0.4)
    D.screws_rect(a, (-20, 0, -66), "-z", 50, 36, inset=4, r=1.4, col=C.CHROME)
    a.cyl(10, 8, at=(-20, 0, -70), col=C.GREY, sides=10, bevel=1.5, rough=0.4)
    # dorsal-fin nicks and tail stitching
    a.box((6, 16, 3), at=(4, 0, 128), rot=(0, 90, 40), col=body, bevel=1)
    for k in range(5):
        a.box((1.2, 12, 2.4), at=(-226, 0, -8 + k * 7), rot=(0, 0, 0), col=C.INK, bevel=0.2)


@asset("SM_Shark_Jaw", F,
       desc="Lower jaw: pale bone with a red mouth, pink tongue and two rows of chunky triangular teeth.",
       replaces=["FFTSharkParts::Build (Production/FTShark.cpp:63-70): Jaw component with SharkJawBone, SharkMouth"],
       pivot="Jaw scene component (hinge); opens by pitch", view=(0.6, 1, 0.6))
def shark_jaw(a):
    a.sphere(80, ry=52, rz=21, at=(70, 0, -8), col=C.WHITE, segs=16, rings=8)
    a.sphere(64, ry=42, rz=14, at=(64, 0, 6), col=C.CARPET, segs=16, rings=8)
    a.sphere(36, ry=22, rz=8, at=(50, 0, 14), col=C.CORAL, segs=12, rings=6)
    for k in range(9):
        t = -1 + 2 * k / 8
        ang = t * 1.2
        x = 70 + 64 * math.cos(ang) - 10
        y = 42 * math.sin(ang)
        a.cone(5, 14, at=(x, y, 20), rot=(a.rng.uniform(-8, 8), 0, a.rng.uniform(-8, 8)), col=C.WHITE, sides=6, rough=0.3)
    # second row of smaller teeth, gum ridge, lip rim, and the mechanical hinge brackets of the prop
    for k in range(7):
        t = -1 + 2 * k / 6
        ang = t * 1.05
        a.cone(3.4, 9, at=(70 + 52 * math.cos(ang) - 14, 32 * math.sin(ang), 16), rot=(0, 0, a.rng.uniform(-10, 10)), col=C.CREAM, sides=6, rough=0.35)
    for k in range(15):
        ang = -1.35 + 2.7 * k / 14
        a.sphere(4.2, at=(70 + 74 * math.cos(ang) - 10, 48 * math.sin(ang), 11), ry=3.2, rz=2.4, col=C.CORAL_DARK, segs=8, rings=4)
    for sy in (-1, 1):
        a.box((22, 3, 14), at=(6, sy * 44, -2), col=C.GREY_DARK, bevel=1.2, rough=0.4)
        a.cyl(4, 5, at=(0, sy * 46, 0), rot=(0, 0, 90), col=C.CHROME, sides=10, bevel=1, rough=0.3)
        D.screw(a, (12, sy * 45.6, -4), "+y" if sy > 0 else "-y", r=1.0)


@asset("SM_Station_SharkRig_Rail", F,
       desc="Steel rail beam for the shark carriage with yellow end stops and two support legs down to the tank floor.",
       replaces=[actor("AFTSharkRig", "Production/FTShark.cpp", "Rail (scaled in OnConstruction: RailHalfLength 180 -> 460 cm)")],
       placements=lambda: [P((1700, 460, -120), (0, 180, 0))], pivot="actor root",
       integration="Modelled for the level's RailHalfLength 180 (rail 460 cm). For other values scale Y by (2 * RailHalfLength + 100) / 460 instead of the code's RelativeScale3D.",
       view=(1, 0.6, 0.5))
def shark_rail(a):
    half = 230
    a.box((20, 2 * half, 16), at=(-140, 0, 70), col=C.GREY, bevel=3, rough=0.4)
    a.box((10, 2 * half, 6), at=(-140, 0, 80), col=C.GREY_DARK, bevel=1)
    for y in (-half + 8, half - 8):
        a.box((30, 12, 30), at=(-140, y, 74), col=C.YELLOW, bevel=3)
    for y in (-150, 150):
        a.box((12, 12, 62), at=(-140, y, 31), col=C.GREY_DARK, bevel=2)
        a.box((40, 30, 4), at=(-140, y, 2), col=C.GREY_DARK, bevel=1)
        a.box((4, 40, 4), at=(-140, y, 55), rot=(0, 0, 0), col=C.GREY, bevel=1)
        # anchor bolts, diagonal braces, head plate under the beam
        for sx in (-1, 1):
            for sy in (-1, 1):
                D.screw(a, (-140 + sx * 15, y + sy * 10, 4), "+z", r=1.3, col=C.GREY)
        for sy in (-1, 1):
            a.tube([(-140, y + sy * 5, 14), (-140, y + sy * 34, 58)], 1.6, col=C.GREY, sides=6, rough=0.4)
        a.box((24, 20, 3), at=(-140, y, 60.5), col=C.GREY, bevel=0.8)
    # I-beam flanges and web bolts, drag chain for the carriage cables, end-stop bumpers + hazard faces
    for z in (62.5, 77.5):
        a.box((24, 2 * half - 4, 1.6), at=(-140, 0, z), col=C.shade(C.GREY, 0.85), bevel=0.4, rough=0.4)
    for sx in (-1, 1):
        for k in range(12):
            y = -half + 20 + k * (2 * half - 40) / 11
            D.screw(a, (-140 + sx * 10, y, 70), "+x" if sx > 0 else "-x", r=0.9, col=C.GREY)
    for k in range(40):
        y = -half + 30 + k * (2 * half - 60) / 39
        a.box((8, 10, 6), at=(-126, y, 58), col=C.CHARCOAL, bevel=1.2)
        a.box((8.6, 3, 6.4), at=(-126, y + 5, 58), col=C.GREY_DARK, bevel=0.6)
    for y in (-half + 8, half - 8):
        inner = 1 if y < 0 else -1
        a.cyl(6, 6, at=(-140, y + inner * 8, 76), rot=(0, 0, 90), col=C.RUBBER, sides=10, bevel=1.5, rough=0.9)
        D.hazard(a, (-125, y, 74), "+x", 12, 26, stripes=3, t=0.5)


@asset("SM_Station_SharkRig_Trolley", F,
       desc="Yellow rail trolley with four guide wheels and a mast collar.",
       replaces=[actor("AFTSharkRig", "Production/FTShark.cpp", "Carriage: Trolley")],
       pivot="Carriage component (same origin as the actor)", view=(1, 0.6, 0.5))
def shark_trolley(a):
    a.box((50, 60, 24), at=(-140, 0, 84), col=C.YELLOW, bevel=5)
    for sy in (-1, 1):
        for sz in (-1, 1):
            D.wheel(a, (-140 + sz * 14, sy * 24, 70), 6, 6, "y", tire=C.CHARCOAL, hub=C.GREY, bolts=3)
    a.cyl(10, 11, at=(-140, 0, 99.5), col=C.GREY_DARK, sides=10, bevel=2)
    # side plates with bolts, hazard ends, grease nipples, collar bolts, cable junction box
    for n, y in (("+y", 30), ("-y", -30)):
        D.plate(a, (-140, y, 84), n, 40, 16, t=1.0, col=C.shade(C.YELLOW, 0.85))
    for n, x in (("+x", -115), ("-x", -165)):
        D.hazard(a, (x, 0, 84), n, 48, 12, stripes=6, t=0.6)
    for sy in (-1, 1):
        a.cyl(0.9, 3, at=(-128, sy * 30.5, 93), rot=(0, 0, 90), col=C.CHROME, sides=6)
    D.bolt_ring(a, (-140, 0, 96.2), "+z", 13, n=6, r=0.7)
    a.box((12, 10, 8), at=(-152, 22, 100), col=C.CHARCOAL, bevel=1.5)
    D.led(a, (-152, 27, 101), "+y", r=0.6, col=C.GREEN)


@asset("SM_Station_SharkRig_Arm", F,
       desc="Angled steel arm that carries the shark: grey tube with clamp collars and a yellow-black hazard band.",
       replaces=[actor("AFTSharkRig", "Production/FTShark.cpp", "SharkRoot: Arm")],
       pivot="SharkRoot component (the arm rises and dives with the shark)", view=(0.3, 1, 0.3))
def shark_arm(a):
    from ftb.core import ue_rot
    rot = (-30, 0, 0)
    axis = ue_rot(*rot) @ Vector((0, 0, 1))
    c = Vector((-110, 0, 60))
    a.cyl(7, 150, at=c, rot=rot, col=C.GREY_DARK, sides=10, bevel=1)
    for t, col in ((-68, C.GREY), (68, C.GREY), (-20, C.YELLOW), (-8, C.INK), (4, C.YELLOW)):
        p = c + axis * t
        a.cyl(8.5 if col == C.GREY else 7.6, 8 if col == C.GREY else 11, at=p, rot=rot, col=col, sides=10, bevel=1.5)
    # clamp bolts on the collars, hydraulic hose with clips along the arm, a weld bead at the top flange
    arm = D.Local(a, tuple(c), rot)
    for t in (-68, 68):
        for k in range(4):
            ang = math.radians(k * 90 + 45)
            arm.cyl(1.2, 3, at=(9 * math.cos(ang), 9 * math.sin(ang), t), rot=(-90, k * 90 + 45, 0), col=C.CHROME, sides=6)
    D.cable_tie_run(arm, [(0, 9, -70), (0, 9.5, -30), (0, 9.5, 30), (0, 9, 66)], r=1.2, col=C.RUBBER, ties=3)
    arm.torus(7.4, 0.8, at=(0, 0, 60), col=C.shade(C.GREY_DARK, 1.3), major=12, minor=4)


@asset("SM_Station_SharkRig_Desk", F,
       desc="Shark-rig control desk: teal desk with raked panel, button guards, labelled strip and a magenta upgrade-kit switch panel.",
       replaces=[actor("AFTSharkRig", "Production/FTShark.cpp", "Station: Desk, DeskTop, KitPanel")],
       pivot="Station component (StationOffset/StationYaw in the level)", integration="RigButton0-4 and KitLamp stay separate (lit by the code).", view=(-1, 0.4, 0.6))
def shark_desk(a):
    a.box((70, 220, 86), at=(0, 0, 43), col=C.TEAL_DARK, bevel=4)
    a.box((66, 214, 7), at=(-4, 0, 94), rot=(-15, 0, 0), col=C.CHARCOAL, bevel=2)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((8, 8, 86), at=(sx * 33, sy * 108, 43), col=C.GREY, bevel=2, rough=0.35)
    a.box((6, 200, 10), at=(-36, 0, 88), col=C.shade(C.TEAL, 1.2), bevel=1.5)
    for i in range(5):
        y = -84 + i * 42
        a.box((30, 38, 2), at=(-4, y, 99), rot=(-15, 0, 0), col=C.INK, bevel=0.5)
    a.box((40, 12, 60), at=(0, 118, 70), col=C.MAGENTA, bevel=3)
    a.box((30, 4, 44), at=(0, 124, 70), col=C.shade(C.MAGENTA, 0.75), bevel=1.5)
    a.tube([(20, -100, 90), (26, -100, 106), (16, -94, 116)], 2, col=C.GREY, sides=6)
    a.cyl(3, 7, at=(13, -93, 117), rot=(-60, -30, 0), col=C.CHARCOAL, sides=8, bevel=1)
    # detail pass: cue labels under the buttons, front inset + kick plate, kit-panel hazard frame, lock and
    # screws, back: patch panel, vents, hydraulic lines to the rig, side handles
    for i in range(5):
        y = -84 + i * 42
        D.label(a, (-35, y, 76), "-x", 26, 7, lines=1)
        D.screw(a, (-35, y - 16, 82), "-x", r=0.7)
        D.screw(a, (-35, y + 16, 82), "-x", r=0.7)
    D.inset_panel(a, (-35, 0, 40), "-x", 200, 50, C.TEAL_DARK, frame_col=C.shade(C.TEAL_DARK, 1.25), bar=2.2, t=1.0)
    a.box((2, 214, 8), at=(-35.5, 0, 4), col=C.RUBBER, bevel=0.6, rough=0.9)
    D.stencil_number(a, (-36.6, -60, 40), "-x", "SHARK CTRL", 7, col=C.YELLOW)
    D.border(a, (0, 124, 70), "+y", 40, 60, bar=3, t=0.8, col=C.YELLOW)
    for k in range(6):
        a.box((3, 0.6, 5), at=(-16 + k * 6.4, 125.2, 99), rot=(35, 0, 0), col=C.INK, bevel=0, jitter=0, wear=False)
        a.box((3, 0.6, 5), at=(-16 + k * 6.4, 125.2, 41), rot=(35, 0, 0), col=C.INK, bevel=0, jitter=0, wear=False)
    D.screws_rect(a, (0, 126, 70), "+y", 30, 44, inset=2, r=0.8)
    a.cyl(1.8, 1.5, at=(10, 126.8, 56), rot=(0, 0, 90), col=C.CHROME, sides=10, bevel=0.3)
    a.box((0.5, 0.4, 2.2), at=(10, 127.6, 56), col=C.INK, bevel=0, jitter=0, wear=False)
    D.plate(a, (35, -40, 55), "+x", 70, 24, t=0.8, col=C.GREY_DARK)
    D.jacks(a, D.on((35, -40, 55), "+x", -12, 0, 0.8), "+x", cols=5, rows=2, pitch=4.5)
    D.rack_unit(a, D.on((35, -40, 55), "+x", 22, 0, 0.8), "+x", 22, 12, knobs=2, leds=1)
    for y in (40, 80):
        D.vent(a, (35, y, 40), "+x", 26, 20, slats=5, col=C.shade(C.TEAL_DARK, 0.8))
    for k, y in enumerate((-70, -64)):
        D.cable_tie_run(a, [(36, y, 44 - k * 4), (39, y + 2, 20), (40, y + 6, 1.8), (41, y - 20, 1.8), (40, y - 34, 1.8)],
                        r=1.3, col=C.YELLOW if k == 0 else C.RUBBER, ties=2)
    D.plate(a, (0, -110, 58), "-y", 30, 10, t=0.6, col=C.INK, screws=False)
    D.handle(a, (0, -110, 58), "-y", 22, along="u", r=1.1, standoff=2.6)


# ============================================================================== film camera

@asset("SM_Station_FilmCamera_Track", F,
       desc="Dolly track for TrackLength 1400: two round aluminium rails on wooden sleepers with rail clamps and yellow end stops.",
       replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "Track: RailL, RailR, Sleeper0-n (TrackLength 1400)")],
       placements=lambda: [P((450, 0, -60))], pivot="actor root (track centre); rails along Y", view=(1, 0.3, 0.5))
def camera_track(a):
    Ltr = 1460
    for x in (-34, 34):
        a.cyl(4.5, Ltr, at=(x, 0, 7), rot=(0, 0, 90), col=C.GREY, sides=10, bevel=0, rough=0.3)
        for y in (-Ltr / 2, Ltr / 2):
            a.box((14, 12, 16), at=(x, y, 8), col=C.YELLOW, bevel=3)
    n = 16
    for i in range(n):
        y = -1400 / 2 - 20 + (1440 * i / (n - 1))
        a.box((96, 12, 4), at=(0, y, 2), col=C.shade(C.WOOD_DARK, a.rng.uniform(0.9, 1.1)), bevel=1.2)
        for x in (-34, 34):
            a.box((10, 8, 5), at=(x, y, 5), col=C.GREY_DARK, bevel=1)
            D.screw(a, (x + 3.5, y, 7.5), "+z", r=0.7, col=C.GREY)
            D.screw(a, (x - 3.5, y, 7.5), "+z", r=0.7, col=C.GREY)
        # wood grain lines, the odd levelling wedge, a numbered position mark every few sleepers
        for gy in (-3, 2.5):
            a.box((80 - abs(gy) * 4, 0.5, 0.2), at=(a.rng.uniform(-4, 4), y + gy, 4.05), col=C.shade(C.WOOD_DARK, 0.65), bevel=0, jitter=0, wear=False)
        if i % 5 == 2:
            a.prism((14, 10, 3), at=(46, y, 1.5), rot=(0, 90, 0), col=C.WOOD, right=True)
        if i % 3 == 0:
            D.tape(a, (-44, y, 4), "+z", 5, 9, deg=0, col=[C.YELLOW, C.CORAL, C.CYAN][(i // 3) % 3])
    # rail joints and end-stop bumpers
    for x in (-34, 34):
        for y in (-240, 240):
            a.cyl(5.4, 8, at=(x, y, 7), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=1, rough=0.35)
            D.screw(a, (x, y, 12.4), "+z", r=0.8)
        for s_ in (-1, 1):
            a.cyl(4.5, 4, at=(x, s_ * (Ltr / 2 - 8), 8), rot=(0, 0, 90), col=C.RUBBER, sides=10, bevel=1, rough=0.9)
            D.hazard(a, (x, s_ * Ltr / 2, 16), "+z", 14, 12, stripes=3, t=0.3)


@asset("SM_Station_FilmCamera_Dolly", F,
       desc="Camera dolly: grey platform with a diamond-plate top, four chunky wheels on the rails, coral push bar, teal pedestal ring and the monitor arm with a hooded monitor.",
       replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "Dolly: Platform, PlatformTrim, Wheel0-3, PushBar/Posts, Pedestal, PedestalRing, MonitorArm, MonitorBox")],
       pivot="Dolly component (slides along the track)", integration="MonitorScreen (render target plane) stays.", view=(1, 0.8, 0.5))
def camera_dolly(a):
    a.box((96, 78, 14), at=(0, 0, 22), col=C.GREY_DARK, bevel=4)
    a.box((90, 72, 3), at=(0, 0, 30), col=C.GREY, bevel=1, rough=0.35)
    for k in range(8):
        for j in range(6):
            a.box((5, 2, 0.8), at=(-38 + k * 11, -28 + j * 11, 31.8), rot=(0, 45, 0), col=C.shade(C.GREY, 1.15), bevel=0, ao=False)
    for x in (-34, 34):
        for y in (-34, 34):
            D.wheel(a, (x, y, 14), 11, 10, "y", tire=C.CHARCOAL, hub=C.GREY, treads=0, bolts=5)
            a.cyl(4, 4, at=(x, y - 7 if y > 0 else y + 7, 14), rot=(0, 0, 90), col=C.GREY_DARK, sides=8, bevel=0.8)
    # skirt panels with bolts, battery box, tape marks, grip handle wraps, bumper corners
    for n, c, w in (("+x", (48, 0, 22), 78), ("-x", (-48, 0, 22), 78), ("+y", (0, 39, 22), 96), ("-y", (0, -39, 22), 96)):
        for du in (-w / 2 + 6, -w / 6, w / 6, w / 2 - 6):
            D.screw(a, D.on(c, n, du, 0), n, r=0.9, col=C.GREY)
    a.box((16, 22, 12), at=(-34, 20, 38), col=C.CHARCOAL, bevel=2)
    D.label(a, (-26, 20, 38), "+x", 14, 6, lines=1)
    D.led(a, (-26, 12, 42), "+x", r=0.6, col=C.GREEN)
    D.cable(a, [(-34, 9, 38), (-26, 4, 32.6), (-12, 6, 32.6), (-8, 8, 40)], r=0.8)
    for y in (-18, 18):
        D.tape(a, (44, y, 31.8), "+z", 3, 10, col=C.CORAL)
    for y in (-14, 14):
        a.cyl(3.6, 9, at=(-50, y, 72), rot=(0, 0, 90), col=C.RUBBER, sides=10, bevel=1, rough=0.9)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((6, 6, 12), at=(sx * 47, sy * 38, 22), col=C.RUBBER, bevel=2, rough=0.9)
    a.tube([(-48, -32, 30), (-48, -32, 70), (-50, 0, 72), (-48, 32, 70), (-48, 32, 30)], 3, col=C.CORAL, sides=8)
    a.cyl(17, 8, at=(0, 0, 36), col=C.TEAL, sides=14, bevel=2)
    a.cyl(10, 84, at=(0, 0, 72), col=C.CHARCOAL, sides=12, bevel=1)
    for z in (60, 90):
        a.torus(10.5, 1.5, at=(0, 0, z), col=C.GREY, major=12, minor=4)
    a.cyl(2, 60, at=(-30, -44, 70), col=C.CHARCOAL, sides=6)
    a.box((10, 46, 30), at=(-32, -44, 108), col=C.CHARCOAL, bevel=3)
    a.box((10, 50, 4), at=(-28, -44, 125), rot=(20, 0, 0), col=C.CHARCOAL, bevel=1)
    # monitor: side hood flaps, back vents + screws, mount knuckle, cable down the arm
    for sy in (-1, 1):
        a.box((10, 2, 26), at=(-26, -44 + sy * 24, 110), rot=(0, sy * 12, 0), col=C.CHARCOAL, bevel=0.6)
    D.vent(a, (-37, -44, 112), "-x", 30, 12, slats=4, col=C.GREY_DARK)
    D.screws_rect(a, (-37, -44, 108), "-x", 44, 28, inset=2.5, r=0.7)
    a.cyl(3.5, 8, at=(-30, -44, 96), rot=(0, 0, 90), col=C.GREY_DARK, sides=8, bevel=0.8)
    D.knob(a, (-30, -48.5, 96), "-y", r=2.4, h=2, col=C.CORAL)
    D.cable_tie_run(a, [(-35, -40, 94), (-32, -41, 70), (-30, -41, 44), (-20, -30, 33)], r=0.8, ties=2)
    D.label(a, (-37, -44, 99), "-x", 16, 4, lines=1)


@asset("SM_Station_FilmCamera_Body", F,
       desc="Retro film camera body: teal housing with a dark stripe, coral badge, vented sides, lens barrel with focus ring, open matte box with french flag, cream focus wheel, viewfinder and coral handle.",
       replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "TiltHead: Body, BodyStripe, Badge, LensBarrel, LensRing, MatteBox x4, FrenchFlag, FocusWheel/Knob, BodyVents, Handle, Viewfinder")],
       pivot="TiltHead component (pan head + 22 cm); lens along +X",
       integration="ReelA/ReelB (SM_Station_FilmCamera_Reel), LensGlass and RecLamp stay separate (spun/lit by the code).", view=(1, 0.8, 0.4))
def camera_body(a):
    a.box((56, 30, 34), col=C.TEAL, bevel=5, rough=0.45)
    a.box((58, 32, 6), at=(0, 0, -12), col=C.TEAL_DARK, bevel=2)
    a.box((12, 2, 8), at=(10, 15.5, 4), col=C.CORAL, bevel=1)
    for k in range(4):
        a.box((3, 1.5, 12), at=(-18 + k * 6, -15.8, 2), col=C.TEAL_DARK, bevel=0.5)
    a.cyl(11, 28, at=(40, 0, 0), rot=(-90, 0, 0), col=C.CHARCOAL, sides=14, bevel=1.5)
    a.cyl(12.5, 5, at=(50, 0, 0), rot=(-90, 0, 0), col=C.GREY_DARK, sides=14, bevel=1)
    for k in range(10):
        ang = math.radians(k * 36)
        a.box((5, 2, 2), at=(34, 11.5 * math.cos(ang), 11.5 * math.sin(ang)), rot=(k * 36, 0, 0), col=C.GREY, bevel=0.3)
    for (at, size) in (((62, 0, 15), (12, 38, 4)), ((62, 0, -15), (12, 38, 4)), ((62, -17, 0), (12, 4, 28)), ((62, 17, 0), (12, 4, 28))):
        a.box(size, at=at, col=C.CHARCOAL, bevel=1)
    a.box((24, 43, 2), at=(65, 0, 19), rot=(12, 0, 0), col=C.GREY_DARK, bevel=0.6)
    a.cyl(6.5, 7, at=(31, 20, -5), rot=(0, 0, 90), col=C.CREAM, sides=14, bevel=1.5)
    a.cyl(3, 4, at=(31, 25, -5), rot=(0, 0, 90), col=C.CORAL, sides=8, bevel=0.8)
    a.box((22, 10, 12), at=(-30, -18, 8), col=C.CHARCOAL, bevel=2.5)
    a.cyl(5, 6, at=(-42, -18, 8), rot=(-90, 0, 0), col=C.RUBBER, sides=10, bevel=1.5)
    a.cyl(4, 36, at=(-44, 22, -6), rot=(-90, 0, 0), col=C.CORAL, sides=10, bevel=2)
    a.box((6, 6, 16), at=(-26, 22, -6), col=C.GREY_DARK, bevel=1.5)
    a.box((20, 26, 6), at=(0, 0, 20), col=C.GREY_DARK, bevel=2)
    # detail pass: panel seams + screws, side door hinge, timecode display at the back, tally window,
    # lens focus scale, top handle screws, a strip of camera tape with a scribble
    D.seams(a, (0, 15, 2), "+y", 50, 26, n=1, along="v", col=C.shade(C.TEAL, 0.6))
    D.screws_rect(a, (0, 15, 2), "+y", 52, 28, inset=2.5, r=0.7, col=C.GREY)
    D.screws_rect(a, (0, -15, 2), "-y", 52, 28, inset=2.5, r=0.7, col=C.GREY)
    D.hinge(a, (-22, -15.6, 2), "z", 20, r=1.0, col=C.TEAL_DARK)
    D.plate(a, (-28, 6, 4), "-x", 12, 8, t=0.6, col=C.INK, screws=False)
    a.box((0.4, 9, 4.4), at=(-28.9, 6, 4), col=C.hex_rgb(0xFF5A3C), glow=1.6)
    D.knob(a, (-28, 6, -8), "-x", r=1.8, h=1.4, col=C.CORAL)
    D.knob(a, (-28, 12, -8), "-x", r=1.4, h=1.4, col=C.GREY)
    for k in range(7):
        ang = math.radians(-60 + k * 20)
        a.box((2.2 if k % 3 else 3.4, 0.4, 0.3), at=(43, 11.05 * math.sin(ang), 11.05 * math.cos(ang)), rot=(0, 0, math.degrees(ang)),
              col=C.WHITE, bevel=0, jitter=0, wear=False)
    for sx in (-6, 6):
        D.screw(a, (sx, 0, 23), "+z", r=0.8)
    D.tape(a, (10, 0, 17.2), "+z", 18, 5, deg=4, col=C.WHITE)
    a.box((10, 0.6, 0.2), at=(9, 0, 17.6), rot=(0, 4, 0), col=C.INK, bevel=0, jitter=0, wear=False)


@asset("SM_Station_FilmCamera_Reel", F,
       desc="Magazine reel: charcoal drum with a grey hub and five cut-outs (spun by the code).",
       replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "ReelA / ReelB + hubs")],
       pivot="reel centre; axis along Y (the code rolls the cylinder 90)", view=(0.3, 1, 0.3))
def camera_reel(a):
    a.cyl(17, 8, rot=(0, 0, 90), col=C.CHARCOAL, sides=18, bevel=2)
    a.cyl(5, 10, rot=(0, 0, 90), col=C.GREY, sides=10, bevel=1)
    for k in range(5):
        ang = math.radians(k * 72)
        a.cyl(3.6, 9, at=(10 * math.cos(ang), 0, 10 * math.sin(ang)), rot=(0, 0, 90), col=C.GREY_DARK, sides=8, bevel=0)
    for sy in (-1, 1):
        a.torus(16.4, 0.8, at=(0, sy * 4, 0), rot=(0, 0, 90), col=C.shade(C.CHARCOAL, 1.4), major=18, minor=4)
        D.bolt_ring(a, (0, sy * 5, 0), "+y" if sy > 0 else "-y", 3.2, n=3, r=0.6)


@asset("SM_Station_FilmCamera_PanHead", F,
       desc="Fluid pan head: ribbed turntable with coral lock knobs and a low tilt cradle.", replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "PanHead: HeadBase")],
       pivot="PanHead component (dolly + 118)", view=(1, 0.8, 0.4))
def camera_pan_head(a):
    a.cyl(14, 10, col=C.GREY_DARK, sides=14, bevel=2)
    a.torus(14, 1.2, at=(0, 0, -2), col=C.shade(C.GREY_DARK, 1.3), major=16, minor=4)
    for sy in (-1, 1):
        a.box((14, 4, 6), at=(0, sy * 11, 7), col=C.GREY_DARK, bevel=1.5)
        a.cyl(3.5, 4, at=(0, sy * 15, 2), rot=(0, 0, 90), col=C.CORAL, sides=10, bevel=1)
        a.box((1.4, 1.4, 6), at=(0, sy * 17.5, 2), col=C.CORAL, bevel=0.3)
    # degree scale around the turntable, bubble level, pan-lock lever
    for k in range(24):
        ang = math.radians(k * 15)
        a.box((1.4 if k % 6 else 2.4, 0.4, 0.8), at=(14.05 * math.cos(ang), 14.05 * math.sin(ang), 2), rot=(0, k * 15, 0), col=C.WHITE, bevel=0, jitter=0, wear=False)
    a.cyl(2.6, 1.4, at=(8, -6, 5.7), col=C.GREY, sides=10, bevel=0.3)
    a.sphere(1.9, at=(8, -6, 6.4), rz=0.9, col=C.hex_rgb(0x9BE0D8), segs=10, rings=4, mat="glass")
    a.tube([(-12, 0, 0), (-15, 0, -1), (-17, 0, -2.5)], 1.0, col=C.GREY, sides=6)
    a.cyl(1.6, 4, at=(-18, 0, -3), rot=(-70, 0, 0), col=C.CORAL, sides=8, bevel=0.5)


# ============================================================================== flood valve (World/FTFloodController.cpp)

@asset("SM_Station_FloodValve_Pipe", F,
       desc="Flood valve pipe stub: dark steel pipe with bolted flanges, a coupling sleeve and a yellow hazard band (modelled in the component's own frame: axis along local Z).",
       replaces=[actor("AFTFloodController", "World/FTFloodController.cpp", "ValvePipe")],
       pivot="ValvePipe component (ValveLocation, set in BeginPlay); like the code cylinder the axis is local Z - keep the component rotation (90, 0, 0)",
       integration="Swap the mesh on ValvePipe and set its scale to 1 (ApplyShape sets 0.26/0.26/0.8).", view=(1, 0.6, 0.4))
def flood_valve_pipe(a):
    a.cyl(13, 80, col=C.GREY_DARK, sides=14, bevel=1.5, rough=0.45)
    for z in (-38, 38):
        a.cyl(17, 4, at=(0, 0, z), col=C.GREY, sides=14, bevel=1, rough=0.4)
        for k in range(6):
            ang = math.radians(k * 60 + 30)
            a.cyl(1.6, 5.4, at=(14.6 * math.cos(ang), 14.6 * math.sin(ang), z), col=C.CHROME, sides=6, rough=0.3)
    a.cyl(14.2, 12, at=(0, 0, 6), col=C.GREY, sides=14, bevel=1.5, rough=0.4)
    a.cyl(13.4, 6, at=(0, 0, -18), col=C.YELLOW, sides=14, bevel=0.8)
    for k in range(4):
        a.box((2.2, 27.2, 1.4), at=(0, 0, -20.2 + k * 1.5), rot=(0, k * 45, 0), col=C.INK, bevel=0.2)
    # weld beads at the flanges, a pressure gauge on the sleeve, rust streaks and a stencilled flow arrow
    for z in (-35.6, 35.6):
        a.torus(13.2, 0.7, at=(0, 0, z), col=C.shade(C.GREY_DARK, 1.3), major=16, minor=4)
    a.cyl(1.2, 6, at=(0, -16, 8), rot=(0, 0, 90), col=C.GREY, sides=6)
    a.cyl(4, 2.4, at=(0, -19.5, 8), rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=0.6)
    a.torus(4, 0.6, at=(0, -20.8, 8), rot=(0, 0, 90), col=C.CHROME, major=12, minor=4)
    a.box((0.4, 0.3, 3), at=(0.6, -21, 8.6), rot=(0, 0, 30), col=C.RED, bevel=0, jitter=0, wear=False)
    rust = C.hex_rgb(0x8A4A2E)
    for (ang, z, h) in ((20, 22, 14), (140, -2, 10), (250, 26, 12), (300, -28, 8)):
        c = math.radians(ang)
        a.box((0.3, 1.6 + (h % 3) * 0.4, h), at=(13.05 * math.cos(c), 13.05 * math.sin(c), z), rot=(0, ang, 0), col=rust, bevel=0, jitter=0, wear=False)
    a.box((0.3, 6, 1.6), at=(13.1, 0, 20), col=C.WHITE, bevel=0, jitter=0, wear=False)
    a.prism((0.3, 4, 3.4), at=(13.1, 0, 23), rot=(0, 0, 0), col=C.WHITE)


@asset("SM_Station_FloodValve_Wheel", F,
       desc="Red flood-valve handwheel: rim with grip bumps, five spokes and a chrome hub nut (in the torus frame: wheel plane XY, axis local Z).",
       replaces=[actor("AFTFloodController", "World/FTFloodController.cpp", "ValveWheel")],
       pivot="ValveWheel component (ValveLocation + (30, 0, 0)); axis local Z like the code torus - the code keeps rotating it with FRotator(90, 0, Spin)",
       integration="Swap the mesh on ValveWheel and set its scale to 1 (ApplyShape sets 0.44/0.44/0.08); the spin code stays unchanged.", view=(0.3, 0.3, 1))
def flood_valve_wheel(a):
    a.torus(19.5, 2.6, col=C.RED, major=28, minor=8, rough=0.5)
    for k in range(10):
        ang = math.radians(k * 36)
        a.sphere(2.5, at=(19.5 * math.cos(ang), 19.5 * math.sin(ang), 0), col=C.shade(C.RED, 0.8), segs=6, rings=4)
    for k in range(5):
        ang = math.radians(k * 72)
        a.box((17, 3, 2.4), at=(9.5 * math.cos(ang), 9.5 * math.sin(ang), 0), rot=(0, math.degrees(ang), 0), col=C.RED, bevel=0.8)
    a.cyl(5, 5, col=C.GREY_DARK, sides=10, bevel=1)
    a.cyl(2.6, 7, at=(0, 0, 1), col=C.CHROME, sides=6, rough=0.3)
    # spoke ribs, worn paint on the rim grip
    for k in range(5):
        ang = k * 72
        a.box((15, 1.0, 3.2), at=(9.5 * math.cos(math.radians(ang)), 9.5 * math.sin(math.radians(ang)), 0), rot=(0, ang, 0), col=C.shade(C.RED, 0.8), bevel=0.3)
    for k in range(5):
        ang = math.radians(k * 72 + 36)
        a.torus(2.7, 0.9, at=(19.5 * math.cos(ang), 19.5 * math.sin(ang), 0), rot=(90, k * 72 + 36 + 90, 0), col=C.shade(C.RED, 1.25), major=10, minor=4)


# ============================================================================== projector + screen + reel tray

@asset("SM_Station_Projector", F,
       desc="35 mm projector: charcoal pedestal with vents, dark housing with amber trim and lamp-house chimney, lens barrel, reel arms, reel-slot pegs, lever base and 'REELS' plate.",
       replaces=[actor("AFTProjector", "World/FTStudioObjects.cpp", "Stand, Housing, HousingTrim, Barrel, ArmA/B, SlotPeg0-2, LeverBase")],
       placements=lambda: [P((120, -1125, 420)), P((-12380, -900, 424), (0, 180, 0))],
       pivot="actor root on the floor; lens along +X",
       integration="ReelSpinA/B -> SM_Station_Projector_Reel, SlotReel0-2 -> SM_Prop_FilmReel scaled 0.45, Lever -> SM_Station_Projector_Lever, PowerPanel -> SM_Station_ProjectorPowerPanel; LensGlow stays (lit by the code).",
       view=(1, 0.8, 0.4))
def projector(a):
    a.box((70, 70, 90), at=(0, 0, 45), col=C.CHARCOAL, bevel=6)
    grille(a, (35.5, 0, 45), 40, 34, n=5)
    a.box((76, 76, 8), at=(0, 0, 4), col=C.GREY_DARK, bevel=2)
    a.box((100, 56, 70), at=(0, 0, 125), col=C.GREY_DARK, bevel=7)
    a.box((104, 60, 6), at=(0, 0, 162), col=C.AMBER, bevel=2)
    a.cyl(12, 28, at=(-20, 0, 176), col=C.GREY_DARK, sides=12, bevel=2)
    a.cyl(14, 5, at=(-20, 0, 192), col=C.CHARCOAL, sides=12, bevel=1.5)
    for sy in (-1, 1):
        grille(a, (10, sy * 28.5, 125), 40, 30, rot=(0, 90, 0), n=4, col=C.CHARCOAL)
    a.cyl(17, 40, at=(68, 0, 125), rot=(-90, 0, 0), col=C.CHARCOAL, sides=14, bevel=2)
    a.cyl(19, 6, at=(84, 0, 125), rot=(-90, 0, 0), col=C.GREY, sides=14, bevel=1.5, rough=0.3)
    a.box((8, 8, 50), at=(22, 0, 185), rot=(-20, 0, 0), col=C.GREY, bevel=2)
    a.box((8, 8, 50), at=(-26, 0, 185), rot=(20, 0, 0), col=C.GREY, bevel=2)
    for i in range(3):
        a.cyl(3, 20, at=(-10 + i * 24, -36, 110), rot=(0, 0, 90), col=C.YELLOW, sides=8, bevel=0.8)
    a.box((20, 20, 20), at=(-60, 30, 110), col=C.CHARCOAL, bevel=3)
    a.box((50, 2, 16), at=(0, -29, 140), col=C.CHARCOAL, bevel=1)
    # detail pass: housing screws + seams, side door hinges and latch, lamp-house handle, chimney rain cap,
    # lens focus knob + hood ring, film path from the reels into the gate, pedestal bolts, rear inlet + cable
    for n, y in (("+y", 28), ("-y", -28)):
        D.screws_rect(a, (0, y, 125), n, 92, 62, inset=4, r=0.9, col=C.GREY)
    D.seams(a, (0, 28, 125), "+y", 92, 62, n=1, along="v", col=C.shade(C.GREY_DARK, 0.6))
    for z in (104, 146):
        D.hinge(a, (-47, 28.6, z), "z", 10, r=1.3, col=C.GREY)
    a.box((6, 2, 3), at=(40, 29, 125), col=C.CHROME, bevel=0.5, rough=0.3)
    D.handle(a, (-50, 0, 140), "-x", 26, along="u", r=1.3, standoff=4)
    a.cyl(16, 1.6, at=(-20, 0, 196), col=C.GREY_DARK, sides=12, bevel=0.4)
    for k in range(4):
        a.box((2, 2, 4), at=(-20 + 10 * math.cos(math.radians(k * 90 + 45)), 10 * math.sin(math.radians(k * 90 + 45)), 193.5), col=C.GREY_DARK, bevel=0.4)
    D.knob(a, (62, 17.5, 125), "+y", r=2.6, h=2.4, col=C.AMBER)
    a.torus(18.2, 1.0, at=(80, 0, 125), rot=(-90, 0, 0), col=C.shade(C.GREY, 0.8), major=16, minor=4)
    film = C.hex_rgb(0x3A2418)
    D.cable(a, [(34, 6, 193), (32, 6, 178), (26, 6, 170), (20, 6, 164)], r=0.7, col=film)
    D.cable(a, [(-26, 6, 164), (-30, 6, 170), (-36, 6, 180), (-38, 6, 193)], r=0.7, col=film)
    for x in (26, -30):
        a.cyl(2.2, 4, at=(x, 6, 170), rot=(0, 0, 90), col=C.GREY, sides=8, bevel=0.5)
    for sx in (-1, 1):
        for sy in (-1, 1):
            D.screw(a, (sx * 33, sy * 33, 8), "+z", r=1.2, col=C.GREY)
    D.plate(a, (-35, 0, 60), "-x", 24, 16, t=0.8, col=C.GREY_DARK)
    a.box((1.5, 8, 5), at=(-36.5, -4, 62), col=C.INK, bevel=0.4)
    a.box((1.5, 3, 4), at=(-36.5, 6, 62), col=C.RED, bevel=0.4)
    D.cable(a, [(-37, -4, 60), (-42, -4, 40), (-42, 0, 8), (-40, 10, 1.5), (-30, 30, 1.5)], r=1.3)
    D.label(a, (35.5, 0, 72), "+x", 28, 8, lines=2)
    D.stencil_number(a, (0, -35.5, 30), "-y", "35MM", 9, col=C.AMBER)


@asset("SM_Station_Projector_Reel", F,
       desc="Big projector spool: grey rim with six round windows around a hub (spun by the code).",
       replaces=[actor("AFTProjector", "World/FTStudioObjects.cpp", "ReelSpinA/B: ReelA/B torus + spoke")],
       pivot="reel centre (ReelSpinA (34, 0, 222) / ReelSpinB (-38, 0, 222)); axis along Y", view=(0.3, 1, 0.3))
def projector_reel(a):
    a.torus(29, 6, rot=(0, 0, 90), col=C.GREY, major=24, minor=8, rough=0.35)
    a.cyl(27, 4, rot=(0, 0, 90), col=C.GREY_DARK, sides=24, bevel=1)
    a.cyl(7, 10, rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=1.5)
    for k in range(6):
        ang = math.radians(k * 60)
        a.cyl(6.5, 5, at=(17 * math.cos(ang), 0, 17 * math.sin(ang)), rot=(0, 0, 90), col=C.INK, sides=10, bevel=0)
        # raised spokes between the windows
        ang2 = k * 60 + 30
        a.box((13, 5.4, 2.2), at=(17 * math.cos(math.radians(ang2)), 0, 17 * math.sin(math.radians(ang2))), rot=(ang2, 0, 0),
              col=C.shade(C.GREY_DARK, 1.2), bevel=0.4)
    for sy in (-1, 1):
        D.bolt_ring(a, (0, sy * 5, 0), "+y" if sy > 0 else "-y", 4.5, n=3, r=0.7)
        a.torus(27, 0.8, at=(0, sy * 2.2, 0), rot=(0, 0, 90), col=C.shade(C.GREY, 1.15), major=24, minor=4)


@asset("SM_Station_Projector_Lever", F,
       desc="Red start lever with a ball grip.", replaces=[actor("AFTProjector", "World/FTStudioObjects.cpp", "Lever capsule")],
       pivot="lever centre (-66, 30, 135), tilted 20 pitch by the code", view=(1, 0.6, 0.3))
def projector_lever(a):
    a.cyl(2.5, 36, col=C.GREY, sides=8)
    a.sphere(5.5, at=(0, 0, 20), col=C.RED, segs=10, rings=6)
    a.cyl(3.4, 4, at=(0, 0, -16), col=C.GREY_DARK, sides=8, bevel=0.8)
    for z in (13, 15):
        a.torus(2.6, 0.5, at=(0, 0, z), col=C.shade(C.GREY, 0.75), major=8, minor=3)


@asset("SM_Station_ProjectorPowerPanel", F,
       desc="Yellow wall power box with a conduit stub, hazard label, knife switch and a lamp socket.",
       replaces=[actor("AFTProjector", "World/FTStudioObjects.cpp", "PowerPanel: PowerBox, PowerSwitchPart")],
       placements=lambda: [P((-130, -1455, 420))],
       pivot="PowerPanel component (PowerPanelOffset from the projector)", integration="PowerLampPart stays separate (lit by the code).", view=(1, 0.5, 0.4))
def projector_power(a):
    a.box((20, 70, 90), at=(0, 0, 140), col=C.YELLOW, bevel=4)
    a.box((2, 50, 14), at=(10.5, 0, 108), col=C.INK, bevel=0.5)
    for k in range(4):
        a.box((1, 8, 14), at=(11.5, -18 + k * 12, 108), rot=(0, 0, 30), col=C.YELLOW, bevel=0)
    a.box((8, 16, 30), at=(14, -6, 135), col=C.CHARCOAL, bevel=2)
    a.cyl(4, 10, at=(-12, 0, 172), rot=(90, 0, 0), col=C.GREY, sides=8)
    a.cyl(6, 4, at=(11, 22, 170), rot=(0, 90, 90), col=C.GREY_DARK, sides=10)
    # door seam + hinges, corner screws, padlocked hasp, danger label, knock-outs, conduit coupling
    D.border(a, (10, 0, 140), "+x", 66, 86, bar=1.2, t=0.6, col=C.shade(C.YELLOW, 0.8))
    for z in (110, 170):
        D.hinge(a, (10.8, -34, z), "z", 10, r=1.2, col=C.shade(C.YELLOW, 0.7))
    D.screws_rect(a, (10.6, 0, 140), "+x", 66, 86, inset=3.5, r=0.8)
    a.box((1.5, 4, 8), at=(11, 30, 140), col=C.GREY_DARK, bevel=0.4)
    a.box((3, 5, 5), at=(13, 30, 135), col=C.BRASS, bevel=1, rough=0.35)
    a.torus(1.8, 0.5, at=(13, 30, 138.4), rot=(-90, 0, 0), col=C.CHROME, major=8, minor=3)
    D.label(a, (10.6, -6, 160), "+x", 22, 9, lines=2)
    for n, y in (("+y", 35), ("-y", -35)):
        for z in (120, 160):
            a.cyl(3, 0.5, at=(0, y + (0.25 if y > 0 else -0.25), z), rot=(0, 0, 90), col=C.shade(C.YELLOW, 0.8), sides=10, bevel=0.1)
    a.cyl(5.2, 4, at=(-15, 0, 172), rot=(90, 0, 0), col=C.GREY_DARK, sides=8, bevel=0.8)


@asset("SM_Prop_ScreenRoller", F,
       desc="Motorised roller-screen housing: charcoal cassette with a teal glow trim and coral end caps with motor bumps.",
       replaces=[actor("AFTCinemaScreen", "World/FTStudioObjects.cpp", "Housing, HousingTrim, CapL, CapR")],
       placements=lambda: [P((2360, 100, 760), (0, 180, 0)), P((-14720, -150, 800))],
       pivot="actor root (housing centre at Z 500 above it)", integration="Sheet stays a scaled box; BottomBar -> SM_Prop_ScreenBottomBar; 'PREMIERE SCREEN' stays a TextRender.", view=(1, 0.5, 0.3))
def screen_roller(a):
    a.box((56, 1760, 64), at=(0, 0, 500), col=C.CHARCOAL, bevel=10)
    a.box((3, 1720, 12), at=(29, 0, 500), col=C.TEAL, glow=0.6)
    for s in (-1, 1):
        a.box((64, 34, 76), at=(0, s * 895, 500), col=C.CORAL, bevel=8)
        a.cyl(14, 10, at=(0, s * 915, 500), rot=(0, 0, 90), col=C.CORAL_DARK, sides=12, bevel=3)
    a.box((20, 1700, 4), at=(0, 0, 467), col=C.INK, bevel=1)
    # cassette joints and screws, hanger brackets on top, motor cap vents + cable, service label
    for y in (-580, 0, 580):
        a.box((57, 1.2, 65), at=(0, y, 500), col=C.shade(C.CHARCOAL, 0.7), bevel=0.3)
        for z in (486, 514):
            D.screw(a, (28.2, y + 8, z), "+x", r=1.2, col=C.GREY)
            D.screw(a, (28.2, y - 8, z), "+x", r=1.2, col=C.GREY)
    for y in (-720, -240, 240, 720):
        a.box((30, 14, 4), at=(0, y, 534), col=C.GREY_DARK, bevel=1)
        a.box((4, 10, 8), at=(0, y, 540), col=C.GREY_DARK, bevel=0.8)
        a.cyl(1.4, 12, at=(0, y, 544), rot=(-90, 0, 0), col=C.CHROME, sides=6)
    for s_ in (-1, 1):
        D.vent(a, (0, s_ * 912, 500), "+y" if s_ > 0 else "-y", 40, 30, slats=5, col=C.CORAL_DARK)
        D.screws_rect(a, (32, s_ * 895, 500), "+x", 30, 70, inset=3, r=1.0)
    D.cable(a, [(-20, 918, 480), (-24, 920, 440), (-26, 918, 400)], r=1.2)
    D.label(a, (28.5, 820, 486), "+x", 40, 12, lines=2)


@asset("SM_Prop_ScreenBottomBar", F,
       desc="Weighted bottom bar for the roller screen with pull-cord handle.", replaces=[actor("AFTCinemaScreen", "World/FTStudioObjects.cpp", "BottomBar")],
       pivot="bar centre (moves with the unroll)", view=(1, 0.5, 0.3))
def screen_bottom_bar(a):
    a.cyl(7, 1660, rot=(0, 0, 90), col=C.CHARCOAL, sides=12, bevel=0)
    for s in (-1, 1):
        a.cyl(8, 6, at=(0, s * 830, 0), rot=(0, 0, 90), col=C.GREY, sides=12, bevel=2)
    a.torus(5, 1.2, at=(3, 0, -12), rot=(90, 0, 0), col=C.CREAM, major=10, minor=4)
    for y in (-600, -300, 300, 600):
        a.cyl(7.3, 3, at=(0, y, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=12, bevel=0.5)
    for s_ in (-1, 1):
        D.screw(a, (0, s_ * 833.2, 0), "+y" if s_ > 0 else "-y", r=1.4)
    a.cyl(1.2, 6, at=(3, 0, -5), col=C.GREY, sides=6)
    a.box((6, 4, 3), at=(0, 0, -7), col=C.GREY_DARK, bevel=0.8)


@asset("SM_Station_ReelTray", F,
       desc="'Finished reels' tray table: teal top with three yellow reel cradles, charcoal trestle legs and a cream sign.",
       replaces=[actor("AFTReelTray", "Props/FTSetPieces.cpp", "Table, LegA/B, Slot0-2, Sign")],
       placements=lambda: [P((400, -700, -60))], pivot="actor root on the camera deck", integration="'FINISHED REELS' stays a TextRender.", view=(1, 0.6, 0.5))
def reel_tray(a):
    a.box((60, 190, 10), at=(0, 0, 40), col=C.TEAL_DARK, bevel=3)
    for s in (-1, 1):
        a.box((50, 8, 36), at=(0, s * 85, 18), col=C.CHARCOAL, bevel=2)
        a.box((56, 12, 4), at=(0, s * 85, 2), col=C.CHARCOAL, bevel=1)
    for i in range(3):
        y = (i - 1) * 55
        for sx in (-1, 1):
            a.box((10, 14, 8), at=(sx * 14, y, 48), col=C.YELLOW, bevel=2)
    a.box((4, 120, 36), at=(-28, 0, 80), col=C.CREAM, bevel=1.5)
    for s in (-1, 1):
        a.box((3, 3, 34), at=(-28, s * 50, 60), col=C.CHARCOAL, bevel=0.5)
    # edge banding, stretcher between the trestles, felt pads in the cradles, sign frame + screws,
    # scuffs on the top, a checklist clipboard
    for sy in (-1, 1):
        a.box((61, 1.6, 4), at=(0, sy * 95.2, 40), col=C.shade(C.TEAL_DARK, 0.75), bevel=0.4)
    for sx in (-1, 1):
        a.box((1.6, 191, 4), at=(sx * 30.2, 0, 40), col=C.shade(C.TEAL_DARK, 0.75), bevel=0.4)
    a.box((6, 164, 6), at=(0, 0, 12), col=C.CHARCOAL, bevel=1.5)
    for sy in (-1, 1):
        D.bracket(a, (0, sy * 81, 12), "+y" if sy < 0 else "-y", w=5, h=6, col=C.GREY)
    for i in range(3):
        y = (i - 1) * 55
        for sx in (-1, 1):
            a.box((6, 10, 1), at=(sx * 14, y, 52.4), col=C.CARPET, bevel=0.2, rough=0.95)
    D.border(a, (-30.2, 0, 80), "-x", 120, 36, bar=2, t=0.8, col=C.TEAL)
    D.screws_rect(a, (-31, 0, 80), "-x", 120, 36, inset=3, r=0.8)
    D.scuffs(a, (0, 0, 45), "+z", 58, 186, n=7)
    a.box((22, 16, 1), at=(12, 70, 45.5), rot=(0, -10, 0), col=C.WOOD, bevel=0.3)
    a.box((18, 13, 0.3), at=(12.5, 70, 46.2), rot=(0, -10, 0), col=C.WHITE, bevel=0.05, jitter=0)
    a.box((4, 6, 1.2), at=(19, 69, 46.4), rot=(0, -10, 0), col=C.CHROME, bevel=0.3)
    for k in range(4):
        a.box((1.2, 9, 0.2), at=(10 - k * 3, 71, 46.4), rot=(0, -10, 0), col=C.INK, bevel=0, jitter=0, wear=False)
