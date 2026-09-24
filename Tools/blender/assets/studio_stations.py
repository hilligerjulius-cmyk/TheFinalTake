"""Production stations (actors). Moving/recoloured components are separate meshes whose pivot equals
the owning scene component, so they can be dropped into the existing component hierarchy 1:1."""
import math

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


@asset("SM_Station_StageLight_Pole", F,
       desc="100 cm telescopic pole section (scale Z by StandHeight/100) with a clamp ring.",
       replaces=[actor("AFTStageLight", SRC_ST, "Pole (scaled to StandHeight)")],
       pivot="bottom of the pole; the code places the pole centre at 20 + StandHeight/2", view=(1, 0.6, 0.3))
def stage_light_pole(a):
    a.cyl(3, 100, at=(0, 0, 50), col=C.CHARCOAL, sides=8, bevel=0, rough=0.5)
    a.cyl(3.8, 30, at=(0, 0, 15), col=C.GREY_DARK, sides=8, bevel=0.8, rough=0.5)


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


@asset("SM_Station_StageLight_Lens", F,
       desc="Stepped fresnel lens disc, neutral white (the code tints it with the gel colour).",
       replaces=[actor("AFTStageLight", SRC_ST, "LensPart")],
       pivot="lens centre, facing +X (same transform as LensPart)", view=(1, 0.3, 0.2))
def stage_light_lens(a):
    for k in range(3):
        a.cyl(16 - k * 4, 3 + k * 0.8, at=(0, 0, k * 0.8), rot=(-90, 0, 0), col=C.WHITE, sides=16, bevel=0.5, rough=0.2)


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
    for k in range(12):
        y = -80 + k * 14.5
        if -70 < y < 70 and abs(y) > 8:
            a.box((26, 2, 1.2), at=(10, y, 101), rot=(-18, 0, 0), col=C.INK, bevel=0)
            a.box((4, 6, 4), at=(10 + (k % 3) * 4 - 4, y, 102.5), rot=(-18, 0, 0), col=[C.CREAM, C.YELLOW, C.CORAL][k % 3], bevel=1)
    a.box((6, 70, 22), at=(-38, 0, 118), col=C.CHARCOAL, bevel=2)
    a.tube([(-30, 80, 100), (-32, 80, 130), (-20, 70, 142)], 1.5, col=C.GREY, sides=6)
    a.cone(6, 8, at=(-16, 66, 140), rot=(-140, 0, 0), col=C.CHARCOAL, sides=8)
    a.box((12, 40, 4), at=(-41, 0, 43), col=C.GREY_DARK, bevel=1)


@asset("SM_Station_LightingBoard_Toggle", F,
       desc="Chunky round toggle cap (neutral grey, the code recolours it).",
       replaces=[actor("AFTLightingBoard", SRC_ST, "ToggleCap0-2")], pivot="cap centre (component location, rotation -18 pitch from the code)", view=(1, 0.4, 0.8))
def lighting_toggle(a):
    a.cyl(15, 10, col=C.GREY, sides=16, bevel=3, rough=0.4)
    a.box((22, 4, 5), at=(0, 0, 6), col=C.shade(C.GREY, 1.2), bevel=1.5)


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
    a.box((10, 150, 16), at=(26, 0, 104), col=C.GREY_DARK, bevel=2)
    for s in (-1, 1):
        y = s * 150
        a.box((60, 60, 140), at=(0, y, 70), col=C.CHARCOAL, bevel=5)
        a.cyl(20, 4, at=(-31, y, 55), rot=(-90, 0, 0), col=C.GREY_DARK, sides=16, bevel=1.5)
        a.cyl(8, 6, at=(-32, y, 55), rot=(-90, 0, 0), col=C.CHARCOAL, sides=12, bevel=2)
        a.cyl(10, 4, at=(-31, y, 110), rot=(-90, 0, 0), col=C.GREY_DARK, sides=12, bevel=1)
        a.box((3, 30, 6), at=(-31, y, 20), col=C.INK, bevel=1)
        for sx in (-1, 1):
            for sz in (-1, 1):
                a.sphere(4, at=(sx * 28, y + sz * 28, 138), col=C.GREY, segs=6, rings=4, rough=0.35)
    a.cyl(12, 3, at=(60, -120, 2), col=C.CHARCOAL, sides=12, bevel=1)
    a.cyl(2.5, 180, at=(60, -120, 90), col=C.CHARCOAL, sides=8)
    a.tube([(60, -120, 180), (120, -120, 185), (180, -120, 200)], 2, col=C.CHARCOAL, sides=6)
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


# ============================================================================== breaker + beacon

@asset("SM_Station_Breaker", F,
       desc="Yellow main-breaker cabinet: hazard-striped kick band, hinged door with a handle and vents, conduit junction box, MAIN BREAKER plate.",
       replaces=[actor("AFTBreaker", SRC_ST, "Panel, Stripe0-3, Door")],
       placements=lambda: [P((-575, 650, 0))],
       pivot="actor root on the landing", integration="LeverPivot (SM_Station_Breaker_Lever) and StatusLamp stay separate.", view=(1, 0.4, 0.4))
def breaker(a):
    a.box((22, 100, 150), at=(0, 0, 140), col=C.YELLOW, bevel=4)
    a.box((24, 104, 8), at=(0, 0, 212), col=C.shade(C.YELLOW, 0.85), bevel=2)
    for i in range(6):
        a.box((2, 10, 22), at=(12, -40 + i * 16, 74), rot=(0, 0, 30), col=C.INK, bevel=0.3)
    a.box((3, 80, 100), at=(12, 0, 150), col=C.hex_rgb(0xE8B42F), bevel=1.5)
    for k in range(4):
        a.box((1.5, 40, 2.5), at=(14, 0, 128 - k * 6), col=C.shade(C.YELLOW, 0.7), bevel=0.3)
    a.box((5, 6, 22), at=(14.5, -32, 150), col=C.GREY_DARK, bevel=1.5)
    a.box((3, 64, 14), at=(14.5, 0, 186), col=C.CREAM, bevel=1)
    a.box((14, 30, 8), at=(-2, 30, 220), col=C.GREY, bevel=2)


@asset("SM_Station_Breaker_Lever", F,
       desc="Knife-switch lever: charcoal arm with a fat red grip.",
       replaces=[actor("AFTBreaker", SRC_ST, "LeverPivot: LeverArm, LeverGrip")],
       pivot="LeverPivot (18, 0, 150) - rotate this component to throw the switch", view=(1, 0.4, 0.3))
def breaker_lever(a):
    a.cyl(5, 14, at=(4, 0, 0), rot=(0, 0, 90), col=C.GREY_DARK, sides=10, bevel=1)
    a.box((8, 10, 50), at=(10, 0, 22), col=C.CHARCOAL, bevel=2)
    a.cyl(6, 34, at=(20, 0, 46), rot=(0, 0, 90), col=C.RED, sides=12, bevel=3, rough=0.5)


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


@asset("SM_Station_EmergencyDome", F,
       desc="Faceted red beacon dome with a cage.", replaces=[actor("AFTEmergencyLight", SRC_ST, "Dome")],
       pivot="dome centre (Dome component location (0, 0, 22))", view=(1, 0.5, 0.3))
def emergency_dome(a):
    a.sphere(14, ry=14, rz=16, col=C.RED, segs=10, rings=7, flat=True, rough=0.3)
    for k in range(4):
        a.torus(14.2, 0.8, at=(0, 0, 0), rot=(90, k * 45, 0), col=C.GREY_DARK, major=16, minor=4)


# ============================================================================== effect machines

@asset("SM_Station_WindMachine", F,
       desc="Wind machine: wheeled cart with a teal fan shroud, safety grille rings, motor pod and cable.",
       replaces=[actor("AFTEffectMachine (Wind)", SRC_ST, "BuildLook() Wind parts: base, wheels, stand, torus shroud, motor, cross bars")],
       placements=lambda: [P((1150, 1350, -120), (0, -60, 0))],
       pivot="actor root on the floor; blows along +X", integration="Blades stay on the Spinner component (SM_Station_WindMachine_Blades).", view=(1, 0.7, 0.4))
def wind_machine(a):
    a.box((90, 70, 16), at=(0, 0, 20), col=C.GREY_DARK, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(9, 8, at=(sx * 34, sy * 28, 9), rot=(0, 0, 90), col=C.RUBBER, sides=12, bevel=2)
            a.cyl(4, 9, at=(sx * 34, sy * 28, 9), rot=(0, 0, 90), col=C.GREY, sides=8)
    for sy in (-1, 1):
        a.box((14, 8, 80), at=(0, sy * 30, 64), col=C.CHARCOAL, bevel=2)
    a.torus(62, 12, at=(0, 0, 120), rot=(90, 0, 0), col=C.TEAL, major=24, minor=8, rz=14)
    for k, r in enumerate((56, 38, 20)):
        a.torus(r, 1.5, at=(18, 0, 120), rot=(90, 0, 0), col=C.GREY, major=20, minor=4)
    a.box((2, 118, 3), at=(18, 0, 120), col=C.GREY, bevel=0.5)
    a.box((2, 3, 118), at=(18, 0, 120), col=C.GREY, bevel=0.5)
    a.cyl(24, 40, at=(-26, 0, 120), rot=(90, 0, 0), col=C.TEAL_DARK, sides=14, bevel=4)
    a.cyl(12, 10, at=(-48, 0, 120), rot=(90, 0, 0), col=C.CHARCOAL, sides=12, bevel=3)
    a.tube([(-50, 0, 112), (-50, 10, 60), (-38, 20, 25)], 2, col=C.RUBBER, sides=6)


@asset("SM_Station_WindMachine_Blades", F,
       desc="Four-blade yellow propeller with a spinner cap.", replaces=[actor("AFTEffectMachine (Wind)", SRC_ST, "Blade0-3 on Spinner")],
       pivot="Spinner component (4, 0, 120); rotates about X", view=(1, 0.3, 0.2))
def wind_blades(a):
    for b in range(4):
        blade = bm_box(5, 20, 50, 2.2, 1)
        deform(blade, lambda c: Vector((c.x, c.y * (0.7 + 0.5 * (c.z + 25) / 50), c.z)))
        a.add(blade, xf((0, 0, 0), (0, 20, b * 90)) @ xf((0, 0, 30)), C.YELLOW, rough=0.5)
    a.sphere(11, ry=11, rz=11, at=(6, 0, 0), col=C.CORAL, segs=10, rings=6)


@asset("SM_Station_RainMachine", F,
       desc="Rain pump cart: teal water tank with a coral filler cap, yellow pump drum, gauges, hose reel and wheels.",
       replaces=[actor("AFTEffectMachine (Rain)", SRC_ST, "cart: base, wheels, tank, pump capsule, cap")],
       placements=lambda: [P((950, 1500, -120))],
       pivot="actor root on the floor", integration="The overhead spray rig is SM_Station_RainRig (Rig component at EffectCenter).", view=(1, 0.7, 0.4))
def rain_machine(a):
    a.box((90, 60, 16), at=(0, 0, 22), col=C.GREY_DARK, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(9, 8, at=(sx * 32, sy * 24, 9), rot=(0, 0, 90), col=C.RUBBER, sides=12, bevel=2)
    a.box((50, 54, 50), at=(-12, 0, 55), col=C.TEAL, bevel=8)
    a.cyl(9, 10, at=(-12, 0, 84), col=C.CORAL, sides=12, bevel=2)
    a.cyl(15, 50, at=(24, 0, 58), rot=(0, 0, 90), col=C.YELLOW, sides=14, bevel=5)
    for sy in (-1, 1):
        a.cyl(15.5, 4, at=(24, sy * 20, 58), rot=(0, 0, 90), col=C.shade(C.YELLOW, 0.8), sides=14, bevel=1)
    a.cyl(6, 3, at=(24, -26, 72), rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=0.8)
    a.tube([(38, 0, 58), (45, 2, 68), (42, 6, 80), (30, 10, 86)], 3, col=C.YELLOW, sides=8)


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


@asset("SM_Station_FoamCannon", F,
       desc="Foam cannon: yellow pressure tank on a wheeled base, raked teal barrel with a coral muzzle ring, pressure gauge and trigger box.",
       replaces=[actor("AFTEffectMachine (Foam)", SRC_ST, "BuildLook() Foam parts")],
       placements=lambda: [P((1000, 1760, -120), (0, -65, 0))], pivot="actor root on the floor; fires along +X", view=(1, 0.7, 0.4))
def foam_cannon(a):
    a.box((90, 60, 16), at=(0, 0, 22), col=C.GREY_DARK, bevel=4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(9, 8, at=(sx * 32, sy * 24, 9), rot=(0, 0, 90), col=C.RUBBER, sides=12, bevel=2)
    a.cyl(20, 50, at=(-10, 0, 55), rot=(0, 0, 90), col=C.YELLOW, sides=14, bevel=9, rough=0.4)
    a.cyl(14, 110, at=(34, 0, 92), rot=(-65, 0, 0), col=C.TEAL, sides=14, bevel=3)
    a.cyl(17, 10, at=(80, 0, 112), rot=(-65, 0, 0), col=C.CORAL, sides=14, bevel=3)
    a.box((10, 8, 24), at=(-30, 0, 90), col=C.CORAL, bevel=2)
    a.cyl(6, 3, at=(-10, -21, 64), rot=(0, 0, 90), col=C.CREAM, sides=12, bevel=0.8)


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
        a.cone(5, 14, at=(x, y, 20), col=C.WHITE, sides=6, rough=0.3)


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


@asset("SM_Station_SharkRig_Trolley", F,
       desc="Yellow rail trolley with four guide wheels and a mast collar.",
       replaces=[actor("AFTSharkRig", "Production/FTShark.cpp", "Carriage: Trolley")],
       pivot="Carriage component (same origin as the actor)", view=(1, 0.6, 0.5))
def shark_trolley(a):
    a.box((50, 60, 24), at=(-140, 0, 84), col=C.YELLOW, bevel=5)
    for sy in (-1, 1):
        for sz in (-1, 1):
            a.cyl(6, 6, at=(-140 + sz * 14, sy * 24, 70), rot=(0, 0, 90), col=C.CHARCOAL, sides=10)
    a.cyl(10, 12, at=(-140, 0, 100), col=C.GREY_DARK, sides=10, bevel=2)


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
            a.cyl(11, 10, at=(x, y, 14), rot=(0, 0, 90), col=C.CHARCOAL, sides=14, bevel=3)
            a.cyl(5, 11, at=(x, y, 14), rot=(0, 0, 90), col=C.GREY, sides=10, bevel=1)
    a.tube([(-48, -32, 30), (-48, -32, 70), (-50, 0, 72), (-48, 32, 70), (-48, 32, 30)], 3, col=C.CORAL, sides=8)
    a.cyl(17, 8, at=(0, 0, 36), col=C.TEAL, sides=14, bevel=2)
    a.cyl(10, 84, at=(0, 0, 72), col=C.CHARCOAL, sides=12, bevel=1)
    for z in (60, 90):
        a.torus(10.5, 1.5, at=(0, 0, z), col=C.GREY, major=12, minor=4)
    a.cyl(2, 60, at=(-30, -44, 70), col=C.CHARCOAL, sides=6)
    a.box((10, 46, 30), at=(-32, -44, 108), col=C.CHARCOAL, bevel=3)
    a.box((10, 50, 4), at=(-28, -44, 125), rot=(20, 0, 0), col=C.CHARCOAL, bevel=1)


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


@asset("SM_Station_FilmCamera_PanHead", F,
       desc="Fluid pan head: ribbed turntable with coral lock knobs and a low tilt cradle.", replaces=[actor("AFTFilmCamera", "Production/FTFilmCamera.cpp", "PanHead: HeadBase")],
       pivot="PanHead component (dolly + 118)", view=(1, 0.8, 0.4))
def camera_pan_head(a):
    a.cyl(14, 10, col=C.GREY_DARK, sides=14, bevel=2)
    a.torus(14, 1.2, at=(0, 0, -2), col=C.shade(C.GREY_DARK, 1.3), major=16, minor=4)
    for sy in (-1, 1):
        a.box((14, 4, 6), at=(0, sy * 11, 7), col=C.GREY_DARK, bevel=1.5)
        a.cyl(3.5, 4, at=(0, sy * 15, 2), rot=(0, 0, 90), col=C.CORAL, sides=10, bevel=1)


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


@asset("SM_Station_Projector_Lever", F,
       desc="Red start lever with a ball grip.", replaces=[actor("AFTProjector", "World/FTStudioObjects.cpp", "Lever capsule")],
       pivot="lever centre (-66, 30, 135), tilted 20 pitch by the code", view=(1, 0.6, 0.3))
def projector_lever(a):
    a.cyl(2.5, 36, col=C.GREY, sides=8)
    a.sphere(5.5, at=(0, 0, 20), col=C.RED, segs=10, rings=6)


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


@asset("SM_Prop_ScreenBottomBar", F,
       desc="Weighted bottom bar for the roller screen with pull-cord handle.", replaces=[actor("AFTCinemaScreen", "World/FTStudioObjects.cpp", "BottomBar")],
       pivot="bar centre (moves with the unroll)", view=(1, 0.5, 0.3))
def screen_bottom_bar(a):
    a.cyl(7, 1660, rot=(0, 0, 90), col=C.CHARCOAL, sides=12, bevel=0)
    for s in (-1, 1):
        a.cyl(8, 6, at=(0, s * 830, 0), rot=(0, 0, 90), col=C.GREY, sides=12, bevel=2)
    a.torus(5, 1.2, at=(3, 0, -12), rot=(90, 0, 0), col=C.CREAM, major=10, minor=4)


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
