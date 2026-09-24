"""Arcade cars from FTEconomy.cpp (Car.StudioVan, Car.CheckerTaxi, Car.MuscleCar, Car.StarLimo).

Every car = <Name>_Body (pivot: ground under the middle of the wheelbase, nose +X) + <Name>_Wheel
(pivot: wheel centre, axle along Y) placed at the Wheel_* sockets. Paint lives in its own material
slots (M_FT_CarBody / M_FT_CarTrim) so the FFTVehicleDef Body/Trim colours can recolour it; the
vertex colours already carry the catalogue defaults. Sizes are chosen for the 176 cm crew capsule,
the 300 x 900 cm Dream Cars bays and the 225 x 500 cm city parking bays."""
import math


from ftb import palette as C
from ftb.core import FONT_BLOCK
from ftb.registry import P, asset

F = "Vehicles"
BODY = "M_FT_CarBody"
TRIM = "M_FT_CarTrim"
ECON = "Career/FTEconomy.cpp"


def _se(t, p):
    c, s = math.cos(t), math.sin(t)
    return math.copysign(abs(c) ** (2 / p), c), math.copysign(abs(s) ** (2 / p), s)


def section(x, wb, zb, zt, wt=None, p=3.2, n=18):
    """Rounded (tumblehome) cross-section at X: bottom width wb, top width wt, from zb to zt."""
    wt = wb if wt is None else wt
    out = []
    for k in range(n):
        u, v = _se(2 * math.pi * k / n - math.pi / 2, p)
        z = (zb + zt) / 2 + (zt - zb) / 2 * v
        t = (z - zb) / max(zt - zb, 1e-3)
        w = wb + (wt - wb) * t
        out.append((x, w / 2 * u, z))
    return out


def loft_profile(a, prof, col, mat, p=3.2, n=18, **kw):
    """prof: [(x, wb, zb, zt, wt)] from rear to front."""
    secs = [section(x, wb, zb, zt, wt, p, n) for (x, wb, zb, zt, wt) in prof]
    a.loft(secs, col=col, mat=mat, **kw)


def top_stripe(a, prof, y, width, col, mat, lift=0.6, thick=1.2, x_range=None):
    """Thin strip hugging the top centre line of a loft profile [(x, wb, zb, zt, wt)] at lateral offset y."""
    secs = []
    xs = [q[0] for q in prof]
    lo, hi = (x_range or (xs[0], xs[-1]))
    n = 16
    for i in range(n + 1):
        x = lo + (hi - lo) * i / n
        for k in range(len(prof) - 1):
            if prof[k][0] <= x <= prof[k + 1][0]:
                t = (x - prof[k][0]) / max(prof[k + 1][0] - prof[k][0], 1e-6)
                zt = prof[k][3] + (prof[k + 1][3] - prof[k][3]) * t
                break
        z = zt + lift
        secs.append([(x, y - width / 2, z), (x, y + width / 2, z), (x, y + width / 2, z + thick), (x, y - width / 2, z + thick)])
    a.loft(secs, col=col, mat=mat, rough=0.4)


def side_window(a, pts_xz, y, col=C.GLASS):
    """Flat side glass in the XZ plane at +/-y (profile given as (x, z))."""
    for s in (-1, 1):
        a.poly([(-x, z) for (x, z) in pts_xz], 1.2, at=(0, s * y, 0), rot=(0, 90, 0), col=col, mat="glass")


def arch(a, x, r, y, col, thick=4.5, mat="opaque"):
    for s in (-1, 1):
        pts = [(x + (r + 5) * math.cos(math.radians(t)), s * y, r + (r + 5) * math.sin(math.radians(t))) for t in range(0, 181, 15)]
        a.tube(pts, thick, col=col, sides=8, mat=mat)


def wheel_sockets(a, wb, track, r, rear_r=None):
    rr = rear_r or r
    a.socket("Wheel_FL", (wb / 2, -track, r))
    a.socket("Wheel_FR", (wb / 2, track, r))
    a.socket("Wheel_RL", (-wb / 2, -track, rr))
    a.socket("Wheel_RR", (-wb / 2, track, rr))


def seats(a, spots, col, back_col=None):
    for i, (x, y, z) in enumerate(spots):
        a.box((44, 46, 12), at=(x, y, z), col=col, bevel=4)
        a.box((12, 46, 50), at=(x - 22, y, z + 28), rot=(-10, 0, 0), col=back_col or col, bevel=4)
        a.socket("Seat_%d" % i, (x, y, z + 6))


def wheel(a, r, width, rim_col, hub_col=C.CHROME, whitewall=False, spokes=0):
    """Wheel with the axle along Y (tyre, tread, rim, hubcap, lug nuts)."""
    w = width
    prof = [(r * 0.6, -w / 2), (r - 4, -w / 2), (r - 0.5, -w / 2 + 4), (r, 0), (r - 0.5, w / 2 - 4), (r - 4, w / 2), (r * 0.6, w / 2)]
    a.lathe(prof, rot=(0, 0, 90), col=C.RUBBER, sides=24, rough=0.85)
    for k in range(24):
        ang = math.radians(k * 15 + 7.5)
        a.box((5, w - 6, 2.4), at=(math.cos(ang) * (r - 0.3), 0, math.sin(ang) * (r - 0.3)), rot=(math.degrees(ang) + 90, 0, 0), col=C.shade(C.RUBBER, 1.3), bevel=0.6, segs=1)
    if whitewall:
        for s in (-1, 1):
            a.torus(r * 0.78, 2.4, at=(0, s * (w / 2 - 0.6), 0), rot=(0, 0, 90), col=C.WHITE, major=24, minor=5, rz=0.8)
    a.cyl(r * 0.6, w - 2, rot=(0, 0, 90), col=rim_col, sides=18, bevel=2, rough=0.4)
    for s in (-1, 1):
        a.cyl(r * 0.36, 3, at=(0, s * (w / 2 - 0.5), 0), rot=(0, 0, 90), col=hub_col, sides=14, bevel=1, rough=0.2)
        a.sphere(r * 0.12, at=(0, s * (w / 2 + 1.5), 0), col=hub_col, segs=8, rings=5, rough=0.2)
        for k in range(5):
            ang = math.radians(k * 72)
            a.cyl(1.6, 2, at=(math.cos(ang) * r * 0.24, s * (w / 2 + 0.8), math.sin(ang) * r * 0.24), rot=(0, 0, 90), col=C.GREY_DARK, sides=6)
        for k in range(spokes):
            ang = k * 360 / spokes
            a.box((r * 0.5, 2, 4), at=(math.cos(math.radians(ang)) * r * 0.36, s * (w / 2 - 1.2), math.sin(math.radians(ang)) * r * 0.36), rot=(-ang, 0, 0), col=hub_col, bevel=0.8, rough=0.2)


def lights(a, x, y, z, r, col, glow, n_side=(1, -1), rot=(-90, 0, 0), bezel=C.CHROME):
    for s in n_side:
        a.cyl(r + 2, 3, at=(x, s * y, z), rot=rot, col=bezel, sides=14, bevel=1, rough=0.2)
        a.cyl(r, 3.5, at=(x + (0.8 if rot[0] < 0 else -0.8), s * y, z), rot=rot, col=col, sides=14, bevel=0.8, glow=glow)


def plate(a, x, z, txt, face=1):
    a.box((1.5, 36, 14), at=(x, 0, z), col=C.CREAM, bevel=0.6)
    a.text(txt, 7, 0.6, at=(x + face * 1.0, 0, z - 3), rot=(0, 0 if face > 0 else 180, 0), col=C.NAVY, font=FONT_BLOCK)


# ============================================================================== Studio Van

VAN = dict(wb=300, track=86, r=38)


@asset("SM_Veh_StudioVan_Body", F,
       desc="Studio Van (Car.StudioVan, 4 seats, starter car): rounded retro box van, cream body with a teal V-nose, belt stripe and bumpers, big round headlights, split windscreen, sliding side door, roof rack with road cases, a tripod light and a rolled cable, rear ladder, 'FINAL TAKE STUDIOS' side lettering with a clapper logo, visible seats.",
       replaces=["new: FFTVehicleDef 'Car.StudioVan' (%s:312-313) - no vehicle mesh exists in the code yet" % ECON],
       placements=lambda: [P((-4000, 1000, 4), (0, -90, 0), note="parked in the Dream Cars lot (starter car); final parking is up to the vehicle work package")],
       pivot="ground under the middle of the wheelbase, nose +X; wheels at the Wheel_* sockets (r 38)",
       integration="Paint slots: M_FT_CarBody = FFTVehicleDef.Body (Cream), M_FT_CarTrim = FFTVehicleDef.Trim (Teal).", view=(1, 0.9, 0.5), tags=["wheels:SM_Veh_StudioVan_Wheel"])
def van_body(a):
    wb, tr, r = VAN["wb"], VAN["track"], VAN["r"]
    L, W, H = 500, 206, 236
    z0 = r + 6
    loft_profile(a, [(-250, W - 16, z0, H - 30, W - 30), (-244, W, z0 - 2, H - 6, W - 14), (0, W, z0 - 2, H, W - 10),
                     (190, W, z0 - 2, H - 4, W - 16), (236, W - 8, z0, H - 40, W - 40), (250, W - 30, z0 + 10, H - 90, W - 70)],
                 C.CREAM, BODY, p=3.6, n=20, rough=0.45)
    # teal V nose + belt stripe + lower trim
    a.poly([(-70, 118), (70, 118), (0, 160)], 5, at=(252, 0, 0), col=C.TEAL, mat=TRIM, bevel=1.5)
    a.box((5, 150, 58), at=(252, 0, 90), col=C.TEAL, mat=TRIM, bevel=2)
    for s in (-1, 1):
        a.box((440, 3, 18), at=(-10, s * (W / 2 + 0.5), 128), col=C.TEAL, mat=TRIM, bevel=1)
        a.box((440, 3, 26), at=(-10, s * (W / 2 + 0.2), z0 + 14), col=C.TEAL, mat=TRIM, bevel=1)
    # skirts + bumpers
    for s in (-1, 1):
        a.box((wb - 2 * r - 26, 12, 30), at=(0, s * (W / 2 - 4), z0 - 6), col=C.TEAL_DARK, mat=TRIM, bevel=4)
    for x in (256, -256):
        a.box((20, W + 6, 22), at=(x, 0, 46), col=C.TEAL, mat=TRIM, bevel=7)
    arch(a, wb / 2, r, W / 2 + 1, C.TEAL_DARK, mat=TRIM)
    arch(a, -wb / 2, r, W / 2 + 1, C.TEAL_DARK, mat=TRIM)
    # windows: split windscreen, side glass, rear glass
    for s in (-1, 1):
        a.box((3, 78, 62), at=(241, s * 44, 182), rot=(0, -12, 0), col=C.GLASS, mat="glass", bevel=1)
    a.box((6, 8, 70), at=(243, 0, 182), col=C.CREAM, mat=BODY, bevel=2)
    side_window(a, [(150, 150), (225, 150), (215, 215), (150, 215)], W / 2 + 0.3)
    side_window(a, [(-230, 150), (-110, 150), (-110, 215), (-225, 215)], W / 2 + 0.3)
    side_window(a, [(-95, 150), (130, 150), (130, 215), (-95, 215)], W / 2 + 0.3)
    a.box((3, 140, 58), at=(-251, 0, 180), col=C.GLASS, mat="glass", bevel=1)
    # sliding door seam + handle, mirrors, headlights, indicators, grille slots, plates
    for s in (-1, 1):
        a.box((3, 3, 150), at=(-100, s * (W / 2 + 1), 118), col=C.shade(C.CREAM, 0.7), bevel=0.3)
        a.box((3, 3, 150), at=(135, s * (W / 2 + 1), 118), col=C.shade(C.CREAM, 0.7), bevel=0.3)
        a.box((14, 4, 5), at=(-80, s * (W / 2 + 2), 135), col=C.CHROME, bevel=1.5, rough=0.2)
        a.tube([(210, s * (W / 2), 170), (218, s * (W / 2 + 16), 172)], 1.5, col=C.CHROME, sides=6)
        a.box((6, 12, 18), at=(218, s * (W / 2 + 20), 172), col=C.CHARCOAL, bevel=3)
    lights(a, 250, 62, 110, 15, C.CREAM, 6)
    lights(a, 252, 88, 78, 5, C.AMBER, 3)
    lights(a, -254, 80, 78, 8, C.RED, 4, rot=(90, 0, 0))
    for k in range(4):
        a.box((3, 70, 4), at=(252, 0, 60 + k * 7), col=C.TEAL_DARK, bevel=1)
    plate(a, 258, 46, "FT-001")
    plate(a, -258, 46, "FT-001", face=-1)
    # side lettering + clapper logo
    for s in (-1, 1):
        a.text("FINAL TAKE", 20, 1.2, at=(-10, s * (W / 2 + 1.2), 105), rot=(0, 90 * s, 0), col=C.TEAL, font=FONT_BLOCK)
        a.text("STUDIOS", 12, 1.2, at=(-10, s * (W / 2 + 1.2), 85), rot=(0, 90 * s, 0), col=C.CORAL, font=FONT_BLOCK)
        a.box((34, 2, 24), at=(-150, s * (W / 2 + 1.2), 95), col=C.INK, bevel=1)
        a.box((36, 2.4, 6), at=(-150, s * (W / 2 + 1.4), 110), rot=(0, 0, s * 8), col=C.WHITE, bevel=0.8)
    # roof rack with cargo
    for x in (-180, 150):
        for s in (-1, 1):
            a.box((8, 8, 14), at=(x, s * 88, H + 6), col=C.GREY_DARK, bevel=2)
    for s in (-1, 1):
        a.cyl(3, 380, at=(-15, s * 88, H + 14), rot=(90, 0, 0), col=C.CHROME, sides=8, rough=0.25)
    for x in range(-170, 150, 50):
        a.cyl(2.5, 180, at=(x, 0, H + 14), rot=(0, 0, 90), col=C.CHROME, sides=6, rough=0.25)
    a.box((90, 60, 44), at=(-100, -30, H + 38), col=C.CHARCOAL, bevel=5)
    a.box((92, 62, 4), at=(-100, -30, H + 38), col=C.GREY, bevel=1, rough=0.3)
    a.box((70, 50, 30), at=(10, 40, H + 31), col=C.TEAL_DARK, bevel=5)
    a.torus(22, 5, at=(90, -40, H + 22), col=C.RUBBER, major=16, minor=6)
    for k in range(3):
        ang = k * 120
        a.cyl(2, 70, at=(60 + math.cos(math.radians(ang)) * 10, 40 + math.sin(math.radians(ang)) * 10, H + 20), rot=(90, ang, 0), col=C.CHARCOAL, sides=5)
    a.box((24, 22, 22), at=(60, 40, H + 22), col=C.YELLOW, bevel=3)
    # rear ladder
    for s in (-1, 1):
        a.box((4, 4, 150), at=(-254, s * 30, 150), col=C.CHROME, bevel=1, rough=0.25)
    for k in range(6):
        a.cyl(1.8, 60, at=(-255, 0, 90 + k * 25), rot=(0, 0, 90), col=C.CHROME, sides=6)
    # interior
    seats(a, [(150, -45, 70), (150, 45, 70), (40, -40, 70), (40, 40, 70)], C.CORAL, C.CORAL_DARK)
    a.box((60, W - 30, 20), at=(215, 0, 120), col=C.CHARCOAL, bevel=4)
    a.torus(14, 2.5, at=(185, -45, 130), rot=(60, 0, 0), col=C.INK, major=14, minor=5)
    wheel_sockets(a, wb, tr, r)
    a.socket("Exhaust", (-250, 60, 30))
    a.meta["seats"] = 4
    a.meta["dimensions_cm"] = [L, W, H]


# ============================================================================== Checker Taxi

TAXI = dict(wb=290, track=82, r=34)


@asset("SM_Veh_CheckerTaxi_Body", F,
       desc="Checker Cab (Car.CheckerTaxi, 4 seats): upright yellow city cab with a tall glasshouse, checker band along the flanks, chrome bumpers and grille, round headlights, TAXI roof light, meter light and a charcoal lower trim.",
       replaces=["new: FFTVehicleDef 'Car.CheckerTaxi' (%s:314-315)" % ECON],
       placements=lambda: [P((-4700, 900, 16), (0, -90, 0), note="on the Dream Cars turntable (bay 3)")],
       pivot="ground under the middle of the wheelbase, nose +X; wheels at the Wheel_* sockets (r 34)",
       integration="Paint slots: M_FT_CarBody = Body (Yellow), M_FT_CarTrim = Trim (Charcoal).", view=(1, 0.9, 0.5), tags=["wheels:SM_Veh_CheckerTaxi_Wheel"])
def taxi_body(a):
    wb, tr, r = TAXI["wb"], TAXI["track"], TAXI["r"]
    L, W = 490, 196
    z0 = r + 4
    loft_profile(a, [(-245, W - 20, z0 + 6, 100, W - 30), (-238, W, z0, 108, W - 8), (-150, W, z0 - 2, 110, W - 4),
                     (80, W, z0 - 2, 108, W - 4), (200, W, z0, 104, W - 8), (240, W - 10, z0 + 4, 96, W - 24), (246, W - 30, z0 + 10, 88, W - 44)],
                 C.YELLOW, BODY, p=3.8, n=20, rough=0.4)
    # glasshouse + roof
    loft_profile(a, [(-150, W - 24, 104, 152, W - 50), (-140, W - 20, 104, 156, W - 44), (60, W - 20, 104, 156, W - 44), (90, W - 26, 104, 150, W - 60)],
                 C.GLASS, "glass", p=4.5, n=16)
    loft_profile(a, [(-148, W - 44, 152, 162, W - 52), (78, W - 44, 152, 162, W - 52)], C.YELLOW, BODY, p=5, n=16, rough=0.4)
    for x in (-146, -40, 70):
        for s in (-1, 1):
            a.box((10, 6, 50), at=(x, s * (W / 2 - 20), 130), rot=(0, 0, -s * 10) if x != 70 else (0, 20, -s * 10), col=C.YELLOW, mat=BODY, bevel=2)
    # checker band
    for s in (-1, 1):
        for k in range(19):
            for row in range(2):
                a.box((20, 2.2, 8), at=(-190 + k * 20, s * (W / 2 + 0.6), 86 + row * 8), col=C.INK if (k + row) % 2 == 0 else C.WHITE, bevel=0.3)
        a.box((440, 3, 16), at=(0, s * (W / 2 - 2), z0 + 2), col=C.CHARCOAL, mat=TRIM, bevel=2)
        for x in (-80, 40):
            a.box((3, 3, 60), at=(x, s * (W / 2 + 0.5), 75), col=C.shade(C.YELLOW, 0.65), bevel=0.3)
            a.box((12, 4, 4), at=(x - 20, s * (W / 2 + 1.5), 100), col=C.CHROME, bevel=1.2, rough=0.2)
        a.tube([(75, s * (W / 2 - 14), 110), (82, s * (W / 2 + 10), 114)], 1.4, col=C.CHROME, sides=6)
        a.box((5, 10, 14), at=(83, s * (W / 2 + 14), 114), col=C.CHARCOAL, mat=TRIM, bevel=2)
    arch(a, wb / 2, r, W / 2 - 2, C.CHARCOAL, mat=TRIM)
    arch(a, -wb / 2, r, W / 2 - 2, C.CHARCOAL, mat=TRIM)
    for x in (252, -252):
        a.box((14, W + 8, 18), at=(x, 0, 44), col=C.CHROME, bevel=6, rough=0.2)
    a.box((6, 120, 34), at=(247, 0, 76), col=C.CHROME, bevel=3, rough=0.2)
    for k in range(5):
        a.box((3, 110, 3), at=(250, 0, 64 + k * 6), col=C.CHARCOAL, bevel=0.5)
    lights(a, 246, 72, 80, 11, C.CREAM, 6)
    lights(a, 248, 72, 58, 4, C.AMBER, 3)
    lights(a, -247, 76, 80, 7, C.RED, 4, rot=(90, 0, 0))
    plate(a, 260, 44, "TAXI 7")
    plate(a, -260, 44, "TAXI 7", face=-1)
    # roof light + meter
    a.box((50, 70, 8), at=(-40, 0, 166), col=C.CHARCOAL, mat=TRIM, bevel=2)
    a.box((34, 60, 22), at=(-40, 0, 181), col=C.CREAM, bevel=5, glow=2)
    for s in (-1, 1):
        a.text("TAXI", 12, 1, at=(-40 + s * 17.8, 0, 176), rot=(0, 0 if s > 0 else 180, 0), col=C.INK, font=FONT_BLOCK)
    a.box((10, 16, 8), at=(55, 30, 110), col=C.RED, glow=3)
    seats(a, [(20, -40, 60), (20, 40, 60), (-80, -40, 60), (-80, 40, 60)], C.CHARCOAL, C.GREY_DARK)
    a.box((50, W - 40, 16), at=(80, 0, 100), col=C.CHARCOAL, bevel=4)
    a.torus(14, 2.5, at=(55, -40, 110), rot=(60, 0, 0), col=C.INK, major=14, minor=5)
    wheel_sockets(a, wb, tr, r)
    a.socket("Exhaust", (-250, 55, 28))
    a.meta["seats"] = 4
    a.meta["dimensions_cm"] = [L, W, 187]


# ============================================================================== Muscle Car

MUSCLE = dict(wb=280, track=84, r_front=33, r_rear=37)


@asset("SM_Veh_MuscleCar_Body", F,
       desc="Coral Muscle Car (Car.MuscleCar, 2 seats): long hood with a scoop, set-back fastback cabin, twin cream racing stripes over hood, roof and deck, ducktail spoiler, chrome bumpers, quad headlights, side pipes and fat rear arches.",
       replaces=["new: FFTVehicleDef 'Car.MuscleCar' (%s:316-317)" % ECON],
       placements=lambda: [P((-4400, 900, 16), (0, -90, 0), note="on the Dream Cars turntable (bay 4)")],
       pivot="ground under the middle of the wheelbase, nose +X; wheels: _WheelFront (r 33) front, _WheelRear (r 37) rear",
       integration="Paint slots: M_FT_CarBody = Body (Coral), M_FT_CarTrim = Trim (Cream stripes).", view=(1, 0.9, 0.45), tags=["wheels:SM_Veh_MuscleCar_WheelFront|SM_Veh_MuscleCar_WheelRear"])
def muscle_body(a):
    wb, tr = MUSCLE["wb"], MUSCLE["track"]
    rf, rr = MUSCLE["r_front"], MUSCLE["r_rear"]
    L, W = 470, 196
    z0 = 34
    prof = [(-235, W - 16, z0 + 12, 88, W - 30), (-228, W, z0 + 4, 96, W - 12), (-140, W + 6, z0, 98, W - 6),
            (0, W - 4, z0, 94, W - 12), (140, W - 2, z0, 90, W - 12), (215, W - 8, z0 + 4, 82, W - 24), (236, W - 22, z0 + 10, 72, W - 40)]
    loft_profile(a, prof, C.CORAL, BODY, p=3.8, n=20, rough=0.35)
    # glasshouse with the roof sitting on it, A- and C-pillars
    loft_profile(a, [(-150, W - 34, 90, 116, W - 70), (-120, W - 30, 90, 128, W - 64), (-30, W - 30, 90, 130, W - 64), (20, W - 34, 90, 118, W - 76)],
                 C.GLASS, "glass", p=4.2, n=16)
    roof = [(-122, W - 62, 125, 134, W - 72), (-34, W - 62, 127, 135, W - 72)]
    loft_profile(a, roof, C.CORAL, BODY, p=5, n=16, rough=0.35)
    for sy in (-1, 1):
        a.tube([(22, sy * 80, 92), (-32, sy * 64, 130)], 3.2, col=C.CORAL, sides=8, mat=BODY)
        a.tube([(-152, sy * 80, 92), (-120, sy * 64, 128)], 5, col=C.CORAL, sides=8, mat=BODY)
        a.tube([(-70, sy * 82, 92), (-72, sy * 66, 130)], 2.6, col=C.CORAL, sides=8, mat=BODY)
    # racing stripes following hood, roof and deck
    for sy in (-1, 1):
        top_stripe(a, prof, sy * 18, 16, C.CREAM, TRIM, x_range=(40, 234))
        top_stripe(a, roof, sy * 18, 16, C.CREAM, TRIM, x_range=(-120, -36))
        top_stripe(a, prof, sy * 18, 16, C.CREAM, TRIM, x_range=(-226, -156))
    # hood scoop, spoiler, arches, pipes
    a.box((70, 50, 12), at=(110, 0, 98), col=C.CORAL, mat=BODY, bevel=4)
    a.box((4, 40, 7), at=(146, 0, 99), col=C.INK, bevel=1)
    a.box((22, W - 20, 6), at=(-226, 0, 99), rot=(-12, 0, 0), col=C.CORAL, mat=BODY, bevel=2.5)
    arch(a, wb / 2, rf, W / 2 + 1, C.CORAL_DARK, thick=5, mat=TRIM)
    arch(a, -wb / 2, rr, W / 2 + 3, C.CORAL_DARK, thick=7, mat=TRIM)
    for s in (-1, 1):
        a.cyl(5, 150, at=(0, s * (W / 2 + 4), 30), rot=(90, 0, 0), col=C.CHROME, sides=10, bevel=1, rough=0.2)
        a.cyl(6, 6, at=(-76, s * (W / 2 + 4), 30), rot=(90, 0, 0), col=C.CHARCOAL, sides=10)
        a.tube([(-60, s * (W / 2 - 12), 104), (-54, s * (W / 2 + 10), 106)], 1.4, col=C.CHROME, sides=6)
        a.box((5, 10, 10), at=(-52, s * (W / 2 + 14), 106), col=C.CORAL, mat=BODY, bevel=2)
        a.box((3, 3, 44), at=(-10, s * (W / 2 + 0.5), 70), col=C.shade(C.CORAL, 0.6), bevel=0.3)
    for x in (243, -241):
        a.box((12, W + 6, 16), at=(x, 0, 44), col=C.CHROME, bevel=6, rough=0.2)
    a.box((5, 110, 24), at=(238, 0, 64), col=C.INK, bevel=3)
    for s in (-1, 1):
        for k in (0, 1):
            a.cyl(9, 3, at=(239, s * (38 + k * 26), 66), rot=(-90, 0, 0), col=C.CHROME, sides=12, rough=0.2)
            a.cyl(7, 3.5, at=(240, s * (38 + k * 26), 66), rot=(-90, 0, 0), col=C.CREAM, sides=12, glow=6)
    lights(a, -236, 70, 76, 7, C.RED, 5, rot=(90, 0, 0))
    lights(a, -236, 48, 76, 7, C.RED, 5, rot=(90, 0, 0))
    plate(a, 250, 44, "ZOOM")
    plate(a, -248, 44, "ZOOM", face=-1)
    seats(a, [(-60, -40, 50), (-60, 40, 50)], C.CREAM, C.CORAL_DARK)
    a.box((44, W - 50, 14), at=(-10, 0, 92), col=C.CHARCOAL, bevel=4)
    a.torus(13, 2.5, at=(-30, -40, 102), rot=(60, 0, 0), col=C.INK, major=14, minor=5)
    a.socket("Wheel_FL", (wb / 2, -tr, rf))
    a.socket("Wheel_FR", (wb / 2, tr, rf))
    a.socket("Wheel_RL", (-wb / 2, -tr, rr))
    a.socket("Wheel_RR", (-wb / 2, tr, rr))
    a.socket("Exhaust_L", (-80, -W / 2 - 4, 30))
    a.socket("Exhaust_R", (-80, W / 2 + 4, 30))
    a.meta["seats"] = 2
    a.meta["dimensions_cm"] = [L, W, 133]


# ============================================================================== Premiere Limousine

LIMO = dict(wb=480, track=84, r=34)


@asset("SM_Veh_StarLimo_Body", F,
       desc="Premiere Limousine (Car.StarLimo, 4 seats): stretched navy limo with a magenta pinstripe, big chrome waterfall grille, tinted long glasshouse with an opera window, gold star door emblems, fender flag staffs with magenta pennants, whitewall-ready arches and a lounge interior.",
       replaces=["new: FFTVehicleDef 'Car.StarLimo' (%s:318-319)" % ECON],
       placements=lambda: [P((-5150, 900, 16), (0, -90, 0), note="on the Dream Cars turntable (bay 2)")],
       pivot="ground under the middle of the wheelbase, nose +X; wheels at the Wheel_* sockets (r 34)",
       integration="Paint slots: M_FT_CarBody = Body (Navy), M_FT_CarTrim = Trim (Magenta pinstripe/pennants).", view=(1, 0.9, 0.45), tags=["wheels:SM_Veh_StarLimo_Wheel"])
def limo_body(a):
    wb, tr, r = LIMO["wb"], LIMO["track"], LIMO["r"]
    L, W = 720, 204
    z0 = r + 4
    loft_profile(a, [(-360, W - 20, z0 + 8, 96, W - 30), (-352, W, z0, 104, W - 10), (-200, W, z0 - 2, 106, W - 6),
                     (150, W, z0 - 2, 106, W - 6), (300, W, z0, 100, W - 10), (350, W - 12, z0 + 4, 92, W - 26), (360, W - 30, z0 + 10, 84, W - 44)],
                 C.NAVY, BODY, p=4.0, n=20, rough=0.3)
    loft_profile(a, [(-230, W - 26, 100, 140, W - 56), (-210, W - 22, 100, 148, W - 50), (140, W - 22, 100, 148, W - 50), (190, W - 30, 100, 138, W - 66)],
                 C.hex_rgb(0x2E4A6E), "glass", p=4.6, n=16)
    loft_profile(a, [(-208, W - 50, 144, 154, W - 60), (160, W - 50, 144, 154, W - 60)], C.NAVY, BODY, p=5, n=16, rough=0.3)
    for x in (-205, -60, 60, 175):
        for s in (-1, 1):
            a.box((12, 6, 48), at=(x, s * (W / 2 - 22), 124), rot=(0, 20 if x == 175 else 0, -s * 10), col=C.NAVY, mat=BODY, bevel=2)
    for s in (-1, 1):
        a.box((620, 2.4, 3), at=(-30, s * (W / 2 + 0.7), 92), col=C.MAGENTA, mat=TRIM, bevel=0.5)
        a.box((600, 2.4, 3), at=(-5, s * (W / 2 + 0.2), z0 + 10), col=C.CHROME, bevel=0.5, rough=0.2)
        for x in (-150, 20, 160):
            a.box((3, 3, 62), at=(x, s * (W / 2 + 0.5), 72), col=C.shade(C.NAVY, 0.6), bevel=0.3)
        for x in (-60, 100):
            pts = []
            for k in range(10):
                ang = math.radians(90 + k * 36)
                rr = 12 if k % 2 == 0 else 5
                pts.append((-(x + math.cos(ang) * rr), 72 + math.sin(ang) * rr))
            a.poly(pts, 1.2, at=(0, s * (W / 2 + 1), 0), rot=(0, 90, 0), col=C.BRASS, rough=0.3)
        a.cyl(1.2, 40, at=(300, s * (W / 2 - 20), 116), col=C.CHROME, sides=6)
        a.prism((1, 26, 16), at=(300, s * (W / 2 - 20) - 13 * s, 128), rot=(0, 0, 90 * s), col=C.MAGENTA, mat=TRIM)
        a.tube([(170, s * (W / 2 - 16), 112), (176, s * (W / 2 + 10), 116)], 1.4, col=C.CHROME, sides=6)
        a.box((5, 10, 12), at=(178, s * (W / 2 + 14), 116), col=C.NAVY, mat=BODY, bevel=2)
    arch(a, wb / 2, r, W / 2 - 1, C.CHROME, thick=3)
    arch(a, -wb / 2, r, W / 2 - 1, C.CHROME, thick=3)
    for x in (366, -364):
        a.box((14, W + 8, 18), at=(x, 0, 46), col=C.CHROME, bevel=6, rough=0.2)
    a.box((8, 130, 44), at=(360, 0, 72), col=C.CHROME, bevel=4, rough=0.2)
    for k in range(9):
        a.box((3, 3, 40), at=(364.5, -52 + k * 13, 72), col=C.INK, bevel=0.5)
    a.sphere(8, at=(356, 0, 100), col=C.CHROME, segs=10, rings=6, rough=0.2)
    lights(a, 354, 78, 78, 10, C.CREAM, 6)
    lights(a, -360, 78, 80, 8, C.RED, 5, rot=(90, 0, 0))
    plate(a, 374, 46, "PREMIER")
    plate(a, -372, 46, "PREMIER", face=-1)
    # lounge interior: two facing sofas
    for (x, face) in ((-150, 1), (40, -1)):
        a.box((50, W - 50, 16), at=(x, 0, 62), col=C.CORAL, bevel=5)
        a.box((14, W - 50, 44), at=(x - face * 26, 0, 84), col=C.MAGENTA, bevel=5)
    a.box((40, 70, 30), at=(-55, 0, 58), col=C.WOOD_DARK, bevel=4)
    a.cyl(4, 12, at=(-55, 20, 80), col=C.GLASS, sides=8, mat="glass")
    a.socket("Seat_0", (140, -40, 66))
    a.socket("Seat_1", (-150, -40, 70))
    a.socket("Seat_2", (-150, 40, 70))
    a.socket("Seat_3", (40, 0, 70))
    a.box((50, W - 40, 16), at=(200, 0, 96), col=C.CHARCOAL, bevel=4)
    a.torus(14, 2.5, at=(180, -40, 106), rot=(60, 0, 0), col=C.INK, major=14, minor=5)
    wheel_sockets(a, wb, tr, r)
    a.socket("Exhaust", (-360, 60, 28))
    a.meta["seats"] = 4
    a.meta["dimensions_cm"] = [L, W, 154]


# ============================================================================== wheels

@asset("SM_Veh_StudioVan_Wheel", F, desc="Van wheel r 38: chunky tyre with tread blocks, teal steel rim, chrome hubcap and lug nuts.",
       replaces=["new: Car.StudioVan wheel"], pivot="wheel centre, axle along Y (use for all four sockets)", view=(0.3, 1, 0.3))
def van_wheel(a):
    wheel(a, 38, 26, C.TEAL)


@asset("SM_Veh_CheckerTaxi_Wheel", F, desc="Taxi wheel r 34: whitewall tyre, charcoal rim, chrome dog-dish hubcap.",
       replaces=["new: Car.CheckerTaxi wheel"], pivot="wheel centre, axle along Y", view=(0.3, 1, 0.3))
def taxi_wheel(a):
    wheel(a, 34, 22, C.CHARCOAL, whitewall=True)


@asset("SM_Veh_MuscleCar_WheelFront", F, desc="Muscle car front wheel r 33 with a five-spoke chrome mag.",
       replaces=["new: Car.MuscleCar front wheel"], pivot="wheel centre, axle along Y", view=(0.3, 1, 0.3))
def muscle_wheel_f(a):
    wheel(a, 33, 22, C.GREY_DARK, spokes=5)


@asset("SM_Veh_MuscleCar_WheelRear", F, desc="Muscle car rear wheel r 37: fat tyre with a five-spoke chrome mag.",
       replaces=["new: Car.MuscleCar rear wheel"], pivot="wheel centre, axle along Y", view=(0.3, 1, 0.3))
def muscle_wheel_r(a):
    wheel(a, 37, 32, C.GREY_DARK, spokes=5)


@asset("SM_Veh_StarLimo_Wheel", F, desc="Limo wheel r 34: whitewall tyre with a chrome wire-look rim.",
       replaces=["new: Car.StarLimo wheel"], pivot="wheel centre, axle along Y", view=(0.3, 1, 0.3))
def limo_wheel(a):
    wheel(a, 34, 22, C.CHROME, whitewall=True, spokes=10)
