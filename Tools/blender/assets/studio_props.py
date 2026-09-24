"""Doors, interactive props and set pieces (actors). Pivots = the owning actor/scene component."""
import math

from mathutils import Vector

from ftb import palette as C
from ftb.core import FONT_BLOCK, bm_box, bulge, deform, xf
from ftb.registry import P, asset

F = "Studio/Props"
FD = "Studio/Doors"
OBJ = "World/FTStudioObjects.cpp"
PROP = "Props/FTProp.cpp"
SET = "Props/FTSetPieces.cpp"
SHOP = "Career/FTShopItems.cpp"


def actor(name, file, comps):
    return "%s (%s): %s" % (name, file, comps)


def lin(c):
    return tuple((x * 12.92 if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055) for x in c)


# ============================================================================== doors

DOORS = {
    "Stage4": dict(w=600, h=400, col=lin((0.02, 0.2, 0.19)), at=(-600, 0, 0), yaw=0, lock="Keycard"),
    "Office": dict(w=300, h=300, col=lin((0.6, 0.2, 0.12)), at=(-1200, 1000, 0), yaw=90, lock="Free"),
    "Wardrobe": dict(w=250, h=300, col=lin((0.45, 0.08, 0.3)), at=(-600, -1325, 0), yaw=180, lock="Free"),
    "Projection": dict(w=200, h=220, col=lin((0.15, 0.08, 0.35)), at=(250, -1460, 420), yaw=180, lock="Projection"),
}


def _door_panel(name):
    d = DOORS[name]
    W, H, col = d["w"], d["h"], d["col"]

    @asset("SM_Door_%s_Panel" % name, FD,
           desc="Sliding door leaf %dx%d for the %s door." % (W, H, name),
           replaces=[actor("AFTDoor '%s'" % name, OBJ, "Panel (+ PanelStripe)")],
           placements=lambda: [P((d["at"][0], d["at"][1], d["at"][2] + H / 2), (0, d["yaw"], 0), note="closed position")],
           pivot="panel centre (Panel component: (0, slide, H/2)); thickness along X",
           integration="Set as the Panel mesh with scale 1 (OnConstruction scales the cube to W x H: skip ApplyShape for this mesh).", view=(-1, 0.4, 0.2))
    def build(a):
        a.box((20, W, H), col=col, bevel=3)
        if name == "Stage4":
            for k in range(7):
                a.box((24, W - 20, 10), at=(0, 0, -H / 2 + 40 + k * 50), col=C.shade(col, 1.2), bevel=2.5)
            for s in (-1, 1):
                a.box((26, W * 0.36, 60), at=(0, s * W * 0.24, H * 0.18), col=C.GREY_DARK, bevel=3)
                a.box((28, W * 0.33, 50), at=(0, s * W * 0.24, H * 0.18), col=C.GLASS, mat="glass")
            for k in range(12):
                a.box((22, W / 12 * 0.5, 30), at=(0, -W / 2 + (k + 0.5) * W / 12, -H / 2 + 22), rot=(0, 0, 30), col=C.INK if k % 2 else C.YELLOW, bevel=1)
            a.box((26, 60, 30), at=(0, W / 2 - 50, 0), col=C.GREY, bevel=4)
            a.text("4", 120, 4, at=(12, 0, -20), col=C.YELLOW, font=FONT_BLOCK)
            a.text("4", 120, 4, at=(-12, 0, -20), rot=(0, 180, 0), col=C.YELLOW, font=FONT_BLOCK)
        elif name == "Office":
            for sz in (-1, 1):
                a.box((24, W * 0.7, H * 0.32), at=(0, 0, sz * H * 0.22), col=C.shade(col, 0.85), bevel=5)
            a.cyl(34, 26, at=(0, 0, H * 0.22), rot=(90, 0, 0), col=C.BRASS, sides=18, bevel=3, rough=0.35)
            a.cyl(28, 28, at=(0, 0, H * 0.22), rot=(90, 0, 0), col=C.GLASS, sides=18, mat="glass")
            a.box((26, 80, 22), at=(0, 0, -H * 0.05), col=C.BRASS, bevel=2, rough=0.35)
            for s in (-1, 1):
                a.sphere(6, at=(s * 16, -W * 0.36, -10), col=C.BRASS, segs=10, rings=6, rough=0.3)
        elif name == "Wardrobe":
            pts = [(math.cos(math.radians(90 + k * 36)) * (40 if k % 2 == 0 else 17), math.sin(math.radians(90 + k * 36)) * (40 if k % 2 == 0 else 17) + H * 0.2) for k in range(10)]
            a.poly(pts, 24, col=C.YELLOW, bevel=2, glow=0.4)
            a.box((24, W * 0.7, 10), at=(0, 0, -H * 0.2), col=C.shade(col, 1.25), bevel=2)
            for s in (-1, 1):
                a.sphere(6, at=(s * 16, -W * 0.36, -10), col=C.BRASS, segs=10, rings=6, rough=0.3)
        else:
            q = bm_box(24, W - 24, H - 24, 8, 2)
            bulge(q, 3, axis=0)
            a.add(q, None, C.shade(col, 1.15), rough=0.9)
            for k in range(4):
                for j in range(5):
                    a.sphere(2.5, at=(0, -W / 2 + 30 + k * (W - 60) / 3, -H / 2 + 30 + j * (H - 60) / 4), col=C.BRASS, segs=6, rings=4, rough=0.3) if (k, j) != (1, 3) else None
            a.cyl(26, 28, at=(0, 0, H * 0.18), rot=(90, 0, 0), col=C.BRASS, sides=16, bevel=3, rough=0.35)
            a.cyl(20, 30, at=(0, 0, H * 0.18), rot=(90, 0, 0), col=C.GLASS, sides=16, mat="glass")
    return build


def _door_frame(name):
    d = DOORS[name]
    W, H = d["w"], d["h"]

    @asset("SM_Door_%s_Frame" % name, FD,
           desc="Chunky cream door frame with plinth blocks and corner rosettes (%dx%d opening)." % (W, H),
           replaces=[actor("AFTDoor '%s'" % name, OBJ, "FrameL, FrameR, FrameTop (sized in OnConstruction)")],
           placements=lambda: [P(d["at"], (0, d["yaw"], 0))],
           pivot="door actor root (opening centre on the floor)", view=(-1, 0.4, 0.2))
    def build(a):
        for s in (-1, 1):
            a.box((44, 30, H + 20), at=(0, s * (W / 2 + 15), (H + 20) / 2), col=C.CREAM, bevel=4)
            a.box((50, 36, 24), at=(0, s * (W / 2 + 15), 12), col=C.CREAM_DARK, bevel=4)
            for k in (-1, 1):
                a.box((4, 5, H - 40), at=(k * 23, s * (W / 2 + 15), H / 2 + 10), col=C.shade(C.CREAM, 0.9), bevel=1)
        a.box((44, W + 60, 30), at=(0, 0, H + 15), col=C.CREAM, bevel=4)
        for s in (-1, 1):
            for k in (-1, 1):
                a.cyl(10, 6, at=(k * 23, s * (W / 2 + 15), H + 15), rot=(0, 90, 90), col=C.BRASS, sides=12, bevel=2, rough=0.35)
    return build


for _n in DOORS:
    _door_panel(_n)
    _door_frame(_n)


@asset("SM_Door_KeycardReader", FD,
       desc="Keycard reader: charcoal housing with a card slot, keypad and status bezel.",
       replaces=[actor("AFTDoor (Keycard)", OBJ, "KeyPanel")],
       placements=lambda: [P((-626, -360, 130))],
       pivot="KeyPanel location (-26, -W/2-60, 130) relative to the door; faces -X", integration="KeyLamp stays separate (lit by the code).", view=(-1, 0.4, 0.2))
def keycard_reader(a):
    a.box((10, 30, 44), col=C.CHARCOAL, bevel=3)
    a.box((3, 20, 3), at=(-5.5, 0, 12), col=C.INK, bevel=0.5)
    for r in range(3):
        for c in range(3):
            a.box((2, 5, 4), at=(-5.8, -7 + c * 7, -4 - r * 6), col=C.GREY, bevel=0.6)
    a.cyl(6, 3, at=(-5.5, 0, 18), rot=(0, 90, 90), col=C.GREY_DARK, sides=10, bevel=0.5)


# ============================================================================== script book + scene board

@asset("SM_Prop_ScriptBook_Base", F,
       desc="Fat script binder: teal back cover, cream page block with a stepped fore-edge, three bookmark ribbons and coloured tabs.",
       replaces=[actor("AFTScriptBook", OBJ, "Back, Pages, PageEdge, RibbonA-C")],
       placements=lambda: [P((-1050, 1480, 86))], pivot="actor root on the desk", view=(1, 0.8, 0.8))
def script_book_base(a):
    a.box((100, 140, 6), at=(0, 0, 3), col=C.TEAL, bevel=2)
    a.box((92, 132, 14), at=(0, 0, 12.5), col=C.CREAM, bevel=1.5)
    for k in range(5):
        a.box((92, 1, 12), at=(0, 64 - k * 1.2, 12.5), col=C.shade(C.CREAM, 0.9 + (k % 2) * 0.08), bevel=0.2)
    for (x, c) in ((30, C.CORAL), (-5, C.YELLOW), (-35, C.MAGENTA)):
        a.box((10, 16, 2), at=(x, 70, 8), col=c, bevel=0.6)
        a.box((10, 3, 18), at=(x, 78, 0), rot=(0, 0, 0), col=c, bevel=0.6)
    for k, c in enumerate((C.CORAL, C.CYAN, C.YELLOW, C.GREEN)):
        a.box((8, 4, 10), at=(48, -40 + k * 26, 12), col=c, bevel=0.8)
    a.box((6, 4, 16), at=(-50, -66, 10), col=C.GREY, bevel=1)


@asset("SM_Prop_ScriptBook_Cover", F,
       desc="Hinged front cover with a cream title label and a brass corner protector.",
       replaces=[actor("AFTScriptBook", OBJ, "CoverPivot: Cover, Label")],
       pivot="CoverPivot hinge (0, -70, 20); the cover extends along +Y", integration="'SCRIPTS' stays a TextRender on the cover.", view=(1, 0.8, 0.8))
def script_book_cover(a):
    a.box((100, 140, 6), at=(0, 70, 0), col=C.TEAL, bevel=2)
    a.box((60, 100, 1.2), at=(0, 70, 3.4), col=C.CREAM, bevel=0.4)
    a.box((56, 96, 0.6), at=(0, 70, 4.0), col=C.shade(C.CREAM, 0.9), bevel=0.2)
    for sx in (-1, 1):
        a.box((10, 10, 7), at=(sx * 46, 136, 0), rot=(0, 45, 0), col=C.BRASS, bevel=1, rough=0.3)
    a.cyl(3, 100, at=(0, 0, 0), rot=(90, 0, 0), col=C.TEAL_DARK, sides=8)


@asset("SM_Prop_SceneBoard", F,
       desc="Cork scene board: dark wood frame, cork with texture dimples, taped paper sheet, sticky notes and a clothes-peg clip.",
       replaces=[actor("AFTSceneBoard", OBJ, "Frame, Cork, NoteA, NoteB, Paper")],
       placements=lambda: [P((-1150, 1972, 230), (0, -90, 0)), P((-560, -700, 220))],
       pivot="actor root (board centre); faces +X", integration="Pins and the TextRender lines stay.", view=(1, 0.3, 0.2))
def scene_board(a):
    a.box((10, 300, 200), col=C.WOOD_DARK, bevel=3)
    a.box((4, 280, 180), at=(6, 0, 0), col=C.CORK, bevel=1)
    rng = a.rng
    for k in range(40):
        a.box((1, 3, 3), at=(8, rng.uniform(-135, 135), rng.uniform(-85, 85)), col=C.shade(C.CORK, 0.82), bevel=0)
    a.box((2, 220, 150), at=(9, 10, -5), col=C.CREAM, bevel=0.5)
    # notes sit on top of the paper (in the code NoteA/NoteB and Paper share the plane x 8..10)
    a.box((2, 50, 40), at=(10.6, -110, 60), rot=(0, 0, 6), col=C.YELLOW, bevel=0.5)
    a.box((2, 44, 36), at=(10.6, 115, -65), rot=(0, 0, -8), col=C.hex_rgb(0x9FE3D9), bevel=0.5)
    for (y, z) in ((-95, 65), (115, 65), (-95, -75), (115, -75)):
        a.box((2.5, 22, 7), at=(11.6, y, z), rot=(0, 0, 35 if y < 0 else -35), col=C.CREAM_DARK, bevel=0.4)
    a.box((5, 6, 14), at=(11, 60, 72), col=C.WOOD, bevel=1)
    a.box((6, 30, 5), at=(10, 0, -104), col=C.WOOD_DARK, bevel=1.5)


# ============================================================================== hand props

@asset("SM_Prop_HeroHarpoon", F,
       desc="Foam hero harpoon: wooden shaft, chunky steel tip with barbs, yellow foam tank with a coral band, teal pistol grip, trigger and butt.",
       replaces=[actor("AFTProp_Harpoon", PROP, "Shaft, Tip, TipCollar, Foam, Band, Grip, Trigger, Butt")],
       placements=lambda: [P((2250, 1700, -36), (0, 90, 0))], pivot="actor root; lies along +X at Z 8", view=(0.4, 1, 0.5))
def harpoon(a):
    a.cyl(3, 170, at=(0, 0, 8), rot=(-90, 0, 0), col=C.WOOD, sides=8, bevel=0.5)
    a.cone(9, 28, at=(98, 0, 8), rot=(-90, 0, 0), col=C.GREY, sides=8, rough=0.3)
    for s in (-1, 1):
        a.prism((2, 10, 14), at=(88, s * 7, 8), rot=(0, 0, s * 90), col=C.GREY, rough=0.3)
    a.cyl(6, 6, at=(82, 0, 8), rot=(-90, 0, 0), col=C.GREY_DARK, sides=10, bevel=1)
    a.cyl(10, 40, at=(22, 0, 8), rot=(-90, 0, 0), col=C.YELLOW, sides=12, bevel=6, rough=0.6)
    a.cyl(10.5, 6, at=(22, 0, 8), rot=(-90, 0, 0), col=C.CORAL, sides=12, bevel=1.5)
    a.cyl(3, 8, at=(22, 0, 19), col=C.CORAL, sides=8, bevel=0.8)
    a.box((30, 10, 10), at=(-40, 0, 8), col=C.TEAL, bevel=3)
    a.box((10, 9, 22), at=(-44, 0, -4), rot=(-15, 0, 0), col=C.TEAL_DARK, bevel=3)
    a.box((6, 5, 12), at=(-32, 0, -2), rot=(-15, 0, 0), col=C.CORAL, bevel=1.5)
    a.tube([(-28, 0, 3), (-26, 0, -6), (-38, 0, -7)], 1.2, col=C.CHARCOAL, sides=5)
    a.box((16, 9, 12), at=(-78, 0, 8), col=C.TEAL_DARK, bevel=3)


@asset("SM_Prop_LifeRing", F,
       desc="Coral life ring with four white wraps and a grab rope with lashings.",
       replaces=[actor("AFTProp_LifeRing", PROP, "Ring + 4 stripes")],
       placements=lambda: [P((1960, 1000, -30))], pivot="actor root (ring lies flat, centre at Z 9)", view=(1, 0.6, 0.9))
def life_ring(a):
    a.torus(25.2, 10.8, at=(0, 0, 9), col=C.CORAL, major=24, minor=10, rz=10, rough=0.6)
    for k in range(4):
        ang = k * 90 + 45
        a.cyl(11.6, 16, at=(math.cos(math.radians(ang)) * 25.2, math.sin(math.radians(ang)) * 25.2, 9), rot=(90, ang + 90, 0), col=C.WHITE, sides=12, bevel=3)
    pts = []
    for k in range(33):
        ang = math.radians(k * 360 / 32)
        pts.append((math.cos(ang) * 37.5, math.sin(ang) * 37.5, 9 + 1.5 * math.sin(ang * 8)))
    a.tube(pts, 1.1, col=C.CREAM, sides=5, caps=False)


@asset("SM_Prop_PracticalLamp", F,
       desc="Practical film lamp: splayed tripod, pole with an amber collar, charcoal housing with a top barn door and cooling fins.",
       replaces=[actor("AFTProp_Lamp", PROP, "Leg0-2, Pole, Collar, Housing, DoorTop")],
       placements=lambda: [P((650, 900, -120), (0, -120, 0))], pivot="actor root on the floor; lens along +X",
       integration="Lens stays separate (SM_Prop_PracticalLamp_Lens) because the code switches its glow.", view=(1, 0.7, 0.4))
def practical_lamp(a):
    for i in range(3):
        ang = i * 120
        d = Vector((math.cos(math.radians(ang)), math.sin(math.radians(ang)), 0))
        a.tube([d * 3 + Vector((0, 0, 34)), d * 24 + Vector((0, 0, 1))], 1.8, col=C.CHARCOAL, sides=6)
        a.cyl(2.6, 3, at=tuple(d * 25 + Vector((0, 0, 1.5))), col=C.RUBBER, sides=6)
    a.cyl(2.5, 90, at=(0, 0, 62), col=C.CHARCOAL, sides=8)
    a.cyl(4.5, 6, at=(0, 0, 36), col=C.AMBER, sides=10, bevel=1.5)
    a.box((26, 26, 22), at=(4, 0, 112), col=C.CHARCOAL, bevel=4)
    for k in range(3):
        a.box((14, 27, 1.5), at=(-2, 0, 118 - k * 5), col=C.shade(C.CHARCOAL, 1.3), bevel=0.3)
    a.box((12, 26, 2), at=(20, 0, 126), rot=(25, 0, 0), col=C.CHARCOAL, bevel=0.6)
    a.box((4, 30, 8), at=(0, 0, 100), col=C.CHARCOAL, bevel=1)


@asset("SM_Prop_PracticalLamp_Lens", F,
       desc="Lens disc for the practical lamp (neutral amber; the code controls its glow).",
       replaces=[actor("AFTProp_Lamp", PROP, "Lens")], pivot="lens centre (18, 0, 112), facing +X", view=(1, 0.3, 0.2))
def practical_lamp_lens(a):
    a.cyl(10, 3, rot=(-90, 0, 0), col=C.AMBER, sides=14, bevel=0.8, glow=6)
    a.torus(10.5, 1.2, rot=(-90, 0, 0), col=C.GREY_DARK, major=14, minor=4)


@asset("SM_Prop_PropCrate", F,
       desc="Small throwable prop crate (56 cm): slatted sides, X-braces and rope handles.",
       replaces=[actor("AFTProp_Crate", PROP, "Body, SlatA, SlatB, SlatC")],
       placements=lambda: [P((1230, -1050, -120), (0, 20, 0)), P((2050, 1560, -120), (0, -10, 0))], pivot="actor root on the floor", view=(1, 0.7, 0.5))
def prop_crate(a):
    a.box((52, 52, 48), at=(0, 0, 25), col=C.WOOD, bevel=2)
    for yaw in (0, 90, 180, 270):
        r = math.radians(yaw)
        n = Vector((math.cos(r), math.sin(r), 0))
        for k in range(3):
            a.box((2.5, 50, 13), at=tuple(n * 27 + Vector((0, 0, 9 + k * 16))), rot=(0, yaw, 0), col=C.shade(C.WOOD, 1.05 - k * 0.05), bevel=1, segs=1)
        a.box((2, 64, 7), at=tuple(n * 28.5 + Vector((0, 0, 25))), rot=(0, yaw, 42), col=C.WOOD_DARK, bevel=1, segs=1)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((7, 7, 52), at=(sx * 26, sy * 26, 25), col=C.WOOD_DARK, bevel=1.5)
    a.box((58, 58, 6), at=(0, 0, 51), col=C.WOOD_DARK, bevel=2)
    for s in (-1, 1):
        a.tube([(s * 29, -10, 38), (s * 33, -8, 32), (s * 33, 8, 32), (s * 29, 10, 38)], 1.3, col=C.CREAM_DARK, sides=5)


@asset("SM_Prop_FinMarker", F,
       desc="Floating fin marker: coral buoy with a stripe, short pole and a little blue rubber fin.",
       replaces=[actor("AFTProp_FinMarker", PROP, "Float, Pole, Fin, FinEdge")],
       placements=lambda: [P((820, 1150, -120))], pivot="actor root", view=(1, 0.8, 0.4))
def fin_marker(a):
    a.cyl(20, 12, at=(0, 0, 6), col=C.CORAL, sides=14, bevel=4, rough=0.6)
    a.torus(20, 1.5, at=(0, 0, 6), col=C.WHITE, major=16, minor=4)
    a.cyl(2, 26, at=(0, 0, 22), col=C.GREY_DARK, sides=6)
    a.poly([(-22, 0), (22, 0), (8, 16), (-2, 44), (-8, 38), (-14, 18)], 7, at=(0, 0, 30), rot=(0, 90, 0), col=C.BLUE, bevel=1.5)
    a.poly([(4, 0), (22, 0), (8, 16), (2, 26)], 8, at=(-0.5, 0, 30), rot=(0, 90, 0), col=C.DEEP_BLUE, bevel=1)


@asset("SM_Prop_FilmReel", F,
       desc="Upright film reel: grey rim, charcoal flange with three windows, cream hub and a wound film pack.",
       replaces=[actor("AFTProp_Reel", PROP, "Rim, Disc, Hub, Hole0-2"), actor("AFTProjector", OBJ, "SlotReel0-2 (use scale 0.45)")],
       pivot="actor root; reel stands upright in the XZ plane, centre at Z 24", view=(0.3, 1, 0.3))
def film_reel(a):
    a.torus(18, 6, at=(0, 0, 24), rot=(0, 0, 90), col=C.GREY, major=20, minor=8, rough=0.35)
    a.cyl(20, 4, at=(0, 0, 24), rot=(0, 0, 90), col=C.CHARCOAL, sides=20, bevel=1)
    a.cyl(14, 8, at=(0, 0, 24), rot=(0, 0, 90), col=C.shade(C.CHARCOAL, 1.6), sides=18, bevel=1)
    a.cyl(7, 12, at=(0, 0, 24), rot=(0, 0, 90), col=C.CREAM, sides=10, bevel=1.5)
    for i in range(3):
        ang = math.radians(i * 120)
        a.cyl(4.5, 5, at=(math.cos(ang) * 12, 3, 24 + math.sin(ang) * 12), rot=(0, 0, 90), col=C.GREY, sides=8, bevel=0)
    a.box((10, 3, 4), at=(0, 0, 44), col=C.CORAL, bevel=0.5)


@asset("SM_Prop_FilmCase", F,
       desc="Padded film-can case: grey body with a coral band, dark lid, brass corners, carry handle and latches.",
       replaces=[actor("AFTFilmCase", "World/FTCity.cpp", "Body, Lid, Stripe, Handle, Corner x4")],
       placements=lambda: [P((420, -520, -60), (0, 90, 0))], pivot="actor root", integration="Reel0-2 discs stay (visibility per packed reel); label stays a TextRender.", view=(1, 0.8, 0.6))
def film_case(a):
    a.box((84, 50, 36), at=(0, 0, 18), col=C.GREY, bevel=4, rough=0.5)
    a.box((86, 52, 6), at=(0, 0, 38), col=C.GREY_DARK, bevel=2)
    a.box((86, 52, 8), at=(0, 0, 18), col=C.CORAL, bevel=2)
    a.tube([(-15, 0, 41), (-12, 0, 50), (12, 0, 50), (15, 0, 41)], 2.4, col=C.CHARCOAL, sides=8)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((10, 10, 38), at=(sx * 38, sy * 22, 19), col=C.BRASS, bevel=2.5, rough=0.3)
        a.box((8, 3, 10), at=(sx * 20, 26, 33), col=C.CHARCOAL, bevel=1)


# ============================================================================== rescue boat

@asset("SM_Prop_RescueBoat_Slipway", F,
       desc="Boat slipway: two wooden skids with bolts and a grey launch trolley on four rubber wheels.",
       replaces=[actor("AFTRescueBoat", SET, "SlipL, SlipR, Trolley, TrolleyWheel0-3")],
       placements=lambda: [P((850, 250, -120))], pivot="actor root", integration="'BOAT BAY - PUSH INTO TANK' stays a TextRender.", view=(1, 0.7, 0.5))
def slipway(a):
    for s in (-1, 1):
        a.box((420, 14, 12), at=(120, s * 60, 6), col=C.WOOD_DARK, bevel=2.5)
        for x in range(-60, 330, 60):
            a.cyl(2, 1.5, at=(x, s * 60, 12.5), col=C.GREY, sides=6)
    a.box((220, 110, 12), at=(0, 0, 18), col=C.GREY, bevel=3)
    for s in (-1, 1):
        a.box((200, 8, 8), at=(0, s * 40, 27), col=C.RUBBER, bevel=2.5)
    for i in range(4):
        a.cyl(11, 10, at=(-80 if i < 2 else 80, 55 if i % 2 else -55, 12), rot=(0, 0, 90), col=C.RUBBER, sides=12, bevel=3)
        a.cyl(5, 11, at=(-80 if i < 2 else 80, 55 if i % 2 else -55, 12), rot=(0, 0, 90), col=C.GREY, sides=8)


@asset("SM_Prop_RescueBoat_Boat", F,
       desc="Orange inflatable rescue boat: fat tubes with a rounded bow, grab-line loops, deck boards, bench, transom and a coral-capped outboard.",
       replaces=[actor("AFTRescueBoat", SET, "BoatRoot: TubeL/R, Bow, Floor, Seat, Transom, Motor, MotorCap, MotorLeg, RopeL/R")],
       pivot="BoatRoot component (0, 0, 24); bow towards +X", integration="Hull/deck collision stays with the code capsules and floor box.", view=(1, 0.8, 0.5))
def boat(a):
    hull = C.hex_rgb(0xFF7A2E)
    secs = []
    for (x, s) in ((-140, 0.9), (-100, 1.0), (40, 1.0), (100, 0.92), (140, 0.7), (165, 0.35)):
        loop = []
        for k in range(20):
            t = 2 * math.pi * k / 20
            loop.append((x, 58 * s * math.cos(t) + (0 if math.cos(t) > 0 else 0), 28 + 26 * math.sin(t)))
        secs.append(loop)
    for side in (-1, 1):
        tube = []
        for (x, y) in ((-140, 58), (-80, 60), (20, 60), (90, 54), (135, 36), (160, 10)):
            tube.append((x, side * y, 30))
        a.tube(tube, 26, col=hull, sides=12, radii=[24, 26, 26, 25, 22, 16], rough=0.55)
        for k in range(6):
            x = -110 + k * 44
            a.tube([(x - 10, side * 84, 36), (x, side * 88, 28), (x + 10, side * 84, 36)], 1.4, col=C.CREAM, sides=5)
        a.cyl(2, 240, at=(-10, side * 84, 36), rot=(90, 0, 0), col=C.CREAM, sides=5, cap=False)
    a.sphere(30, at=(150, 0, 30), ry=24, rz=24, col=hull, segs=12, rings=8, rough=0.55)
    a.box((250, 100, 12), at=(0, 0, 10), col=C.GREY_DARK, bevel=3)
    for k in range(5):
        a.box((44, 96, 2), at=(-100 + k * 50, 0, 17), col=C.GREY, bevel=0.8)
    a.box((34, 110, 8), at=(10, 0, 36), col=C.WOOD_DARK, bevel=2.5)
    a.box((10, 110, 40), at=(-138, 0, 30), col=C.GREY_DARK, bevel=2.5)
    a.box((36, 30, 44), at=(-160, 0, 60), col=C.CHARCOAL, bevel=6)
    a.box((38, 32, 10), at=(-160, 0, 86), col=C.CORAL, bevel=3)
    a.box((10, 10, 50), at=(-166, 0, 18), col=C.CHARCOAL, bevel=2)
    a.cyl(8, 4, at=(-170, 0, -4), rot=(-90, 0, 0), col=C.GREY_DARK, sides=3)
    a.tube([(-150, 10, 64), (-120, 16, 66), (-100, 18, 66)], 2, col=C.CHARCOAL, sides=6)


# ============================================================================== wardrobe: costume rack + garments

@asset("SM_Wardrobe_CostumeRack", "Studio/Wardrobe",
       desc="Chrome costume rail on castor feet with a magenta WARDROBE header board, five hooks and wooden hangers.",
       replaces=[actor("AFTCostumeRack", SET, "PostL/R, Bar, FootL/R, Header, H0-4 Hook + Hanger")],
       placements=lambda: [P((-1740, -1300, 0))], pivot="actor root; rail along Y, costumes face +X",
       integration="Garments are separate meshes (SM_Costume_*) at (0, -180 + i*90, 0) so the code can hide a worn costume; header text stays.", view=(1, 0.3, 0.3))
def costume_rack(a):
    for s in (-1, 1):
        a.cyl(4, 190, at=(0, s * 230, 95), col=C.CHROME, sides=10, rough=0.25)
        a.box((70, 10, 8), at=(0, s * 230, 6), col=C.GREY_DARK, bevel=3)
        for x in (-30, 30):
            a.cyl(4, 5, at=(x, s * 230, 3), rot=(0, 0, 90), col=C.RUBBER, sides=8)
    a.cyl(4, 470, at=(0, 0, 188), rot=(0, 0, 90), col=C.CHROME, sides=10, rough=0.25)
    for s in (-1, 1):
        a.sphere(6, at=(0, s * 236, 188), col=C.CHROME, segs=8, rings=6, rough=0.25)
    a.box((8, 260, 40), at=(0, 0, 222), col=C.MAGENTA, bevel=3)
    a.box((10, 270, 6), at=(0, 0, 204), col=C.YELLOW, bevel=1.5)
    for s in (-1, 1):
        a.cyl(1.5, 24, at=(0, s * 100, 198), col=C.CHROME, sides=6)
    for i in range(5):
        y = -180 + i * 90
        a.torus(4, 1, at=(0, y, 184), rot=(0, 0, 90), col=C.GREY, major=10, minor=4)
        a.tube([(0, y - 26, 158), (0, y, 170), (0, y + 26, 158)], 1.6, col=C.WOOD, sides=6)
        a.box((3, 56, 3), at=(0, y, 157), col=C.WOOD, bevel=1)


def _garment(name, desc, comps, build_fn, idx):
    @asset("SM_Costume_%s" % name, "Studio/Wardrobe", desc=desc,
           replaces=[actor("AFTCostumeRack", SET, "H%d %s" % (idx, comps))],
           placements=lambda: [P((-1740, -1300 - 180 + idx * 90, 0))],
           pivot="rack floor level under hook %d (actor root + (0, %d, 0))" % (idx, -180 + idx * 90), view=(1, 0.4, 0.3))
    def build(a):
        build_fn(a)
    return build


def _lifeguard(a):
    t = bm_box(10, 50, 56, 4, 2)
    bulge(t, 1.5, axis=0)
    a.add(t, xf((0, 0, 128)), C.WHITE, rough=0.9)
    a.box((12, 54, 30), at=(0, 0, 88), col=C.RED, bevel=4)
    a.box((12.5, 3, 30), at=(0, 12, 88), col=C.WHITE, bevel=0.6)
    a.torus(14, 4.2, at=(8, 0, 120), rot=(0, 0, 90), col=C.CORAL, major=16, minor=7)
    a.box((2, 20, 8), at=(5.5, 0, 140), col=C.RED, bevel=0.8)


def _shark(a):
    b = bm_box(16, 60, 96, 6, 2)
    bulge(b, 2, axis=0)
    a.add(b, xf((0, 0, 110)), C.BLUE, rough=0.8)
    a.box((4, 34, 70), at=(8, 0, 110), col=C.WHITE, bevel=1.5)
    a.poly([(-15, 0), (15, 0), (4, 12), (-2, 28)], 6, at=(-6, 0, 158), rot=(0, 90, 0), col=C.BLUE, bevel=1)
    for k in range(3):
        a.box((2, 3, 14), at=(9, 22, 120 - k * 6), rot=(0, 0, 12), col=C.DEEP_BLUE, bevel=0.3)


def _raincoat(a):
    c = bm_box(14, 56, 104, 5, 2)
    deform(c, lambda v: Vector((v.x, v.y * (1.0 + 0.18 * max(0.0, -v.z) / 52), v.z)))
    a.add(c, xf((0, 0, 105)), C.YELLOW, rough=0.35)
    a.box((4, 4, 60), at=(8, 0, 120), col=C.CHARCOAL, bevel=1)
    for k in range(4):
        a.cyl(2, 2, at=(9.5, 5, 140 - k * 16), rot=(0, 90, 90), col=C.CHARCOAL, sides=8)
    a.sphere(18, at=(-4, 0, 158), ry=22, rz=12, col=C.YELLOW, segs=10, rings=6, rough=0.35)


def _knight(a):
    b = bm_box(18, 56, 80, 6, 2)
    bulge(b, 3, axis=0)
    a.add(b, xf((0, 0, 118)), C.GREY, rough=0.5)
    a.box((6, 30, 40), at=(10, 18, 104), col=C.RED, bevel=3)
    a.box((7, 6, 30), at=(11, 18, 104), col=C.YELLOW, bevel=1)
    a.poly([(-12, 0), (12, 0), (8, 14), (0, 22), (-8, 14)], 6, at=(0, 0, 160), rot=(0, 90, 0), col=C.RED, bevel=1.5)
    for k in range(4):
        a.box((19, 57, 2), at=(0, 0, 90 + k * 16), col=C.shade(C.GREY, 0.8), bevel=0.4)


def _work(a):
    b = bm_box(12, 52, 70, 4, 2)
    bulge(b, 1.5, axis=0)
    a.add(b, xf((0, 0, 120)), C.TEAL, rough=0.9)
    a.box((4, 30, 6), at=(7, 0, 138), col=C.CREAM, bevel=1)
    a.box((3, 14, 12), at=(7, -12, 118), col=C.TEAL_DARK, bevel=1)
    a.cyl(1, 12, at=(8, -12, 126), col=C.RED, sides=5)
    a.box((3, 10, 8), at=(7, 14, 124), col=C.YELLOW, bevel=0.8)


_garment("Lifeguard", "Lifeguard costume on its hanger: white tank top, red shorts with a stripe, coral mini ring.", "A/B/C (top, shorts, ring)", _lifeguard, 0)
_garment("SharkSuit", "Blue shark onesie with a white belly, gill stripes and a floppy dorsal fin.", "A/B/C (suit, belly, fin)", _shark, 1)
_garment("Raincoat", "Flared yellow raincoat with a hood, toggle buttons and a zip line.", "A/B (coat, zip)", _raincoat, 2)
_garment("FoamKnight", "Foam knight tabard with segment lines, a red shield patch and a plume crest.", "A/B/C (tabard, shield, plume)", _knight, 3)
_garment("WorkClothes", "Teal crew jacket with a name tape, pocket pen and a badge.", "A/B (jacket, tape)", _work, 4)


# ============================================================================== lost & found shelf

@asset("SM_Prop_LostAndFoundShelf", F,
       desc="Lost & Found shelf: wooden cabinet with three shelves, cream and teal parcel boxes with tape crosses, a stray sneaker and a yellow sign board.",
       replaces=[actor("AFTPropShelf", SET, "Frame, Shelf0-2, Box0-2, Sign")],
       placements=lambda: [P((1450, 1900, -120), (0, -90, 0))], pivot="actor root; opens towards +X",
       integration="'LOST & FOUND' stays a TextRender.", view=(1, 0.5, 0.4))
def lost_found(a):
    a.box((6, 180, 200), at=(-22, 0, 100), col=C.WOOD, bevel=2)
    for s in (-1, 1):
        a.box((50, 8, 200), at=(0, s * 86, 100), col=C.WOOD, bevel=2.5)
    a.box((50, 180, 8), at=(0, 0, 196), col=C.WOOD, bevel=2.5)
    a.box((50, 180, 10), at=(0, 0, 5), col=C.WOOD_DARK, bevel=2.5)
    for i in range(3):
        a.box((40, 170, 6), at=(10, 0, 40 + i * 60), col=C.WOOD_DARK, bevel=1.5)
        col = C.TEAL if i == 1 else C.CREAM
        a.box((30, 34, 30), at=(10, -50 + i * 45, 60 + i * 60), col=col, bevel=2.5)
        a.box((31, 5, 31), at=(10, -50 + i * 45, 60 + i * 60), col=C.shade(C.SAND, 0.9), bevel=0.6)
    a.box((26, 11, 9), at=(10, 50, 47.5), rot=(0, 20, 0), col=C.WHITE, bevel=3)
    a.box((27, 12, 2.5), at=(10, 50, 43.5), rot=(0, 20, 0), col=C.RED, bevel=1)
    a.box((4, 160, 34), at=(26, 0, 212), col=C.YELLOW, bevel=1.5)
    for s in (-1, 1):
        a.box((3, 3, 20), at=(24, s * 70, 200), col=C.CHARCOAL, bevel=0.5)


# ============================================================================== stand-in

@asset("SM_Prop_StandIn_Base", F,
       desc="Cardboard stand-in: wooden foot and brace, legs, round face with dot eyes and a smile, waving arms (cardboard edges bevelled).",
       replaces=[actor("AFTStandIn", SET, "Base, Brace, Legs, Face, EyeL/R, Smile, ArmL/R")],
       placements=lambda: [P((-1250, -1150, 0))], pivot="actor root (Visual); figure faces +X",
       integration="Costume overlays are SM_Prop_StandIn_<Plain|Lifeguard|Shark|Raincoat|Knight>, toggled like the code's part groups.", view=(1, 0.4, 0.3))
def standin_base(a):
    a.box((60, 60, 6), at=(-10, 0, 3), col=C.WOOD_DARK, bevel=2)
    a.box((6, 10, 140), at=(-24, 0, 70), rot=(-18, 0, 0), col=C.WOOD, bevel=1.5)
    a.box((6, 40, 40), at=(0, 0, 30), col=C.CORAL_DARK, bevel=2)
    for s in (-1, 1):
        a.box((7, 14, 8), at=(2, s * 11, 10), col=C.CHARCOAL, bevel=2)
    a.cyl(26, 6, at=(1, 0, 172), rot=(90, 0, 0), col=C.SKIN_A, sides=18, bevel=2)
    for s in (-1, 1):
        a.cyl(4.5, 2, at=(5, s * 9, 178), rot=(90, 0, 0), col=C.INK, sides=10)
        a.cyl(1.2, 2.4, at=(5.3, s * 9 + 1.5, 180), rot=(90, 0, 0), col=C.WHITE, sides=6)
        a.cyl(3.5, 1.2, at=(4.6, s * 16, 168), rot=(90, 0, 0), col=C.CORAL, sides=8)
    a.poly([(-8, 0), (8, 0), (6, -3), (0, -5), (-6, -3)], 2, at=(5, 0, 162), rot=(0, 0, 0), col=C.CORAL_DARK)
    a.box((6, 12, 70), at=(0, -32, 100), rot=(0, 0, 12), col=C.SKIN_A, bevel=2)
    a.box((6, 12, 70), at=(0, 32, 110), rot=(0, 0, -40), col=C.SKIN_A, bevel=2)
    a.sphere(7, at=(0, -39, 66), ry=7, rz=7, col=C.SKIN_A, segs=8, rings=5)
    a.sphere(7, at=(0, 56, 136), ry=7, rz=7, col=C.SKIN_A, segs=8, rings=5)


def _standin_overlay(name, desc, comps, fn):
    @asset("SM_Prop_StandIn_%s" % name, F, desc=desc, replaces=[actor("AFTStandIn", SET, comps)],
           placements=lambda: [P((-1250, -1150, 0))], pivot="actor root (Visual), like SM_Prop_StandIn_Base", view=(1, 0.4, 0.3))
    def build(a):
        fn(a)
    return build


def _si_plain(a):
    a.box((6, 50, 110), at=(0, 0, 95), col=C.CORAL, bevel=2)
    a.box((6.5, 20, 30), at=(0.5, 0, 125), col=C.CREAM, bevel=1)
    a.box((7, 50, 12), at=(0, 0, 196), col=C.HAIR_BROWN, bevel=3)
    for k in range(5):
        a.sphere(6, at=(0, -20 + k * 10, 200), ry=6, rz=5, col=C.HAIR_BROWN, segs=6, rings=4)


def _si_lifeguard(a):
    a.box((4, 48, 60), at=(4, 0, 110), col=C.WHITE, bevel=1.5)
    a.box((4, 50, 30), at=(4, 0, 58), col=C.RED, bevel=1.5)
    a.box((4, 70, 12), at=(4, 0, 202), col=C.RED, bevel=2)
    a.box((4, 40, 16), at=(4, 0, 214), col=C.RED, bevel=2)
    a.torus(10, 3, at=(7, 0, 95), rot=(90, 0, 0), col=C.CORAL, major=14, minor=6)


def _si_shark(a):
    a.cyl(40, 4, at=(-3, 0, 176), rot=(90, 0, 0), col=C.BLUE, sides=20, bevel=1.5)
    a.poly([(-20, 0), (20, 0), (6, 18), (-4, 34)], 4, at=(-3, 0, 209), rot=(0, 90, 0), col=C.BLUE, bevel=1)
    a.box((4, 54, 112), at=(4, 0, 95), col=C.BLUE, bevel=1.5)
    a.box((3, 30, 80), at=(6, 0, 95), col=C.WHITE, bevel=1)
    a.torus(26, 3, at=(6, 0, 172), rot=(90, 0, 0), col=C.WHITE, major=20, minor=6)
    for k in range(8):
        ang = math.radians(200 + k * 20)
        a.cone(2.5, 6, at=(7, math.cos(ang) * 26, 172 + math.sin(ang) * 26), rot=(0, 0, 0), col=C.WHITE, sides=4)


def _si_raincoat(a):
    a.box((4, 56, 130), at=(4, 0, 85), col=C.YELLOW, bevel=1.5)
    a.cyl(35, 4, at=(-3, 0, 180), rot=(90, 0, 0), col=C.YELLOW, sides=20, bevel=1.5)
    for k in range(4):
        a.cyl(2, 2, at=(6.5, 0, 130 - k * 20), rot=(90, 0, 0), col=C.CHARCOAL, sides=8)


def _si_knight(a):
    a.box((4, 54, 100), at=(4, 0, 100), col=C.GREY, bevel=1.5)
    a.box((4, 64, 66), at=(-2, 0, 180), col=C.GREY, bevel=2)
    a.box((3, 44, 8), at=(4, 0, 184), col=C.INK, bevel=1)
    a.poly([(-14, 0), (14, 0), (8, 14), (0, 26), (-8, 14)], 4, at=(-2, 0, 213), rot=(0, 90, 0), col=C.RED, bevel=1)
    a.box((4, 34, 44), at=(8, -30, 90), col=C.RED, bevel=2)
    a.box((4.5, 8, 30), at=(8.5, -30, 90), col=C.YELLOW, bevel=1)


_standin_overlay("Plain", "Stand-in 'plain' overlay: coral body with a cream shirt panel and brown hair tufts.", "Plain group: Body, Hair", _si_plain)
_standin_overlay("Lifeguard", "Stand-in lifeguard overlay: white top, red shorts, red cap and a coral ring.", "Lifeguard group: LGTop, LGShorts, LGHat, LGCrown", _si_lifeguard)
_standin_overlay("Shark", "Stand-in shark overlay: blue hood with teeth ring, fin, suit and belly.", "Shark group: SHHood, SHFin, SHBody, SHBelly, SHJaw", _si_shark)
_standin_overlay("Raincoat", "Stand-in raincoat overlay: yellow coat with buttons and hood.", "Raincoat group: RCCoat, RCHood", _si_raincoat)
_standin_overlay("Knight", "Stand-in foam knight overlay: tabard, helm with visor, plume and shield.", "Knight group: FKBody, FKHelm, FKVisor, FKPlume, FKShield", _si_knight)


# ============================================================================== studio supply counter + accessory wall

@asset("SM_Lobby_StudioSupplyCounter", "Studio/Lobby",
       desc="Studio Supply shop counter: coral counter with cream top, striped front and kick plate, retro cash register with keys, catalogue stand, featured-item turntable, back shelf wall stocked with colourful boxes and the neon-trimmed STUDIO SUPPLY sign board.",
       replaces=[actor("AFTShopTerminal", SHOP, "Counter, CounterTop, CounterKick, CounterStripe0-2, Register, RegisterTop, RegisterKey0-5, BookStand/BookL/BookR/BookCover, TurntableBase, TurntableRim, ShelfBack, Shelf0-1, Stock*, Sign, SignNeon*")],
       placements=lambda: [P((-830, 860, 0), (0, -90, 0))], pivot="actor root; customers stand on +X",
       integration="RegisterDisplay, the Turntable scene component (featured item) and all TextRenders stay.", view=(1, 0.6, 0.4))
def supply_counter(a):
    a.box((90, 300, 104), at=(0, 0, 52), col=C.CORAL, bevel=5)
    a.box((104, 316, 8), at=(4, 0, 108), col=C.CREAM, bevel=3)
    a.box((12, 304, 16), at=(40, 0, 8), col=C.CORAL_DARK, bevel=3)
    for i in range(3):
        a.box((2, 290, 6), at=(46, 0, 36 + i * 22), col=C.YELLOW if i == 1 else C.CREAM, bevel=0.8, glow=0.6 if i == 1 else None)
    # register
    a.box((44, 50, 32), at=(-4, -105, 128), col=C.CHARCOAL, bevel=4)
    a.box((26, 46, 20), at=(-12, -105, 154), rot=(-20, 0, 0), col=C.GREY_DARK, bevel=3)
    for k in range(6):
        a.box((6, 9, 4), at=(12 + (k // 3) * 8, -118 + (k % 3) * 12, 146), col=C.CORAL if k == 5 else C.CREAM, bevel=1.2)
    a.box((30, 44, 6), at=(10, -105, 113), col=C.GREY, bevel=1.5)
    a.cyl(3, 3, at=(24, -126, 131), rot=(0, 0, 90), col=C.BRASS, sides=8)
    # catalogue stand
    a.box((40, 70, 8), at=(10, 10, 118), rot=(-25, 0, 0), col=C.WOOD_DARK, bevel=2)
    a.box((34, 34, 3), at=(12, -8, 126), rot=(-25, 0, -8), col=C.CREAM, bevel=0.8)
    a.box((34, 34, 3), at=(12, 28, 126), rot=(-25, 0, 8), col=C.CREAM, bevel=0.8)
    a.box((38, 74, 2), at=(10, 10, 123), rot=(-25, 0, 0), col=C.TEAL, bevel=0.6)
    # turntable
    a.cyl(35, 8, at=(0, 100, 116), col=C.NAVY, sides=24, bevel=2)
    a.torus(34, 1.8, at=(0, 100, 120), col=C.YELLOW, major=24, minor=5, glow=1.0)
    for k in range(12):
        ang = math.radians(k * 30)
        a.sphere(2, at=(36 * math.cos(ang), 100 + 36 * math.sin(ang), 116), col=C.YELLOW, segs=6, rings=4, glow=4)
    # shelf wall
    a.box((12, 360, 300), at=(-80, 0, 150), col=C.NAVY_LIGHT, bevel=3)
    for s in range(2):
        z = 150 + s * 70
        a.box((32, 340, 5), at=(-62, 0, z), col=C.WOOD, bevel=1.5)
        for b in range(7):
            bc = [C.TEAL, C.YELLOW, C.CORAL, C.MAGENTA][(b + s) % 4]
            h = 26 + ((b * 7 + s * 3) % 3) * 8
            a.box((24, 34, h), at=(-62, -140 + b * 46, z + 3 + h / 2), col=bc, bevel=2.5)
            a.box((24.6, 20, 5), at=(-62, -140 + b * 46, z + 3 + h * 0.62), col=C.CREAM, bevel=0.8)
    a.box((14, 400, 80), at=(-70, 0, 335), col=C.NAVY, bevel=4)
    for z in (373, 297):
        a.box((4, 392, 5), at=(-62, 0, z), col=C.MAGENTA, glow=4)


def _bust_positions():
    # actor at (-1190, -1560, 0) yaw 90: local (12, HookBase.Y, 108) -> world (-1190 - HookBase.Y, -1548, 108)
    return [(-1190 - (h - 3.5) * 70, -1548, 108) for h in range(8)]


@asset("SM_Wardrobe_AccessoryWall", "Studio/Wardrobe",
       desc="Accessory wall: navy back panel in a cream frame, magenta display counter with a wooden top and yellow light strip, make-up mirror with a cream frame and nine vanity bulbs.",
       replaces=[actor("AFTAccessoryStand", SHOP, "Back, FrameTop/L/R, Shelf, ShelfBody, ShelfTrim, Mirror, MirrorFrame, Bulb0-8")],
       placements=lambda: [P((-1190, -1560, 0), (0, 90, 0))], pivot="actor root; display faces +X",
       integration="Busts are SM_Prop_AccessoryBust at each HookBase (+12, 0, 117); labels, prices and hook anchors stay.", view=(1, 0.4, 0.3))
def accessory_wall(a):
    W = 8 * 70 + 30
    a.box((12, W, 320), at=(-14, 0, 160), col=C.hex_rgb(0x2B2F5E), bevel=3)
    a.box((20, W + 16, 12), at=(-8, 0, 322), col=C.CREAM, bevel=3)
    for s in (-1, 1):
        a.box((20, 10, 326), at=(-8, s * (W / 2 + 4), 160), col=C.CREAM, bevel=3)
    a.box((48, W, 10), at=(12, 0, 104), col=C.WOOD, bevel=3)
    a.box((40, W - 10, 100), at=(8, 0, 50), col=C.MAGENTA, bevel=4)
    a.box((2, W - 14, 4), at=(29, 0, 92), col=C.YELLOW, glow=0.8)
    for k in range(6):
        a.box((2, 60, 60), at=(28.5, -W / 2 + 60 + k * (W - 120) / 5, 45), col=C.shade(C.MAGENTA, 1.15), bevel=1.5)
    a.box((4, 316, 98), at=(-7, 0, 262), col=C.CREAM, bevel=2)
    a.box((4, 300, 84), at=(-5.5, 0, 262), col=C.SKY_BLUE, bevel=1, glow=0.35)
    for i in range(9):
        y = -144 + i * 36
        a.cyl(6, 4, at=(-3, y, 308), rot=(90, 0, 0), col=C.CHROME, sides=10, rough=0.25)
        a.sphere(6, at=(1, y, 308), col=C.CREAM, segs=10, rings=6, glow=10)


@asset("SM_Prop_AccessoryBust", "Studio/Wardrobe",
       desc="Display bust: rounded dummy head on a neck post and a little plinth.",
       replaces=[actor("AFTAccessoryStand", SHOP, "Bust0-7 + BustNeck0-7")],
       placements=lambda: [P(b, (0, 90, 0)) for b in _bust_positions()],
       pivot="HookBase(h) + (12, 0, 108) - i.e. the neck foot; the code lays out the busts per item count", view=(1, 0.4, 0.3))
def accessory_bust(a):
    a.cyl(8, 4, at=(0, 0, 2), col=C.hex_rgb(0xBDB5A8), sides=12, bevel=1)
    a.cyl(5, 18, at=(0, 0, 11), r_top=4, col=C.hex_rgb(0xBDB5A8), sides=10)
    a.sphere(19, at=(0, 0, 36), ry=17, rz=19, col=C.hex_rgb(0xD9D2C5), segs=14, rings=9)
    a.sphere(3, at=(18, 0, 36), ry=2.5, rz=4, col=C.hex_rgb(0xD9D2C5), segs=6, rings=4)


# ============================================================================== delivery bay floor tape (Career/FTShopItems.cpp)

def _delivery_bay(name, label, cols, rows, sx, sy, where):
    @asset(name, "Studio/Decals",
           desc="Painted delivery-bay floor for %s: %d x %d yellow tape frames with worn edges, black-yellow corner chevrons and a darker pallet footprint in each slot." % (where, cols, rows),
           replaces=[actor("AFTDeliveryBay '%s'" % label, "Career/FTShopItems.cpp", "BeginPlay() slot tape (4 yellow strips per slot, spawned at runtime)")],
           pivot="AFTDeliveryBay actor root on the floor; slots laid out like GetSlots() (columns along X, rows along Y)",
           integration="Slot numbers and the title stay TextRenders; the bay logic (GetSlots) is unchanged.", view=(0.4, 0.3, 1))
    def build(a):
        w, h = sx - 24.0, sy - 24.0
        for c in range(cols):
            for r in range(rows):
                x = (c - (cols - 1) * 0.5) * sx
                y = (r - (rows - 1) * 0.5) * sy
                a.box((w - 10, h - 10, 0.4), at=(x, y, 0.35), col=C.shade(C.STAGE_FLOOR, 0.88), bevel=0, ao=False, jitter=0)
                for s in (-1, 1):
                    a.box((6, h + 6, 1), at=(x + s * w * 0.5, y, 0.6), col=C.YELLOW, bevel=0.3, glow=0.4, jitter=0)
                    a.box((w - 6, 6, 1), at=(x, y + s * h * 0.5, 0.6), col=C.YELLOW, bevel=0.3, glow=0.4, jitter=0)
                for sxx in (-1, 1):
                    for syy in (-1, 1):
                        cx, cy = x + sxx * (w * 0.5 - 14), y + syy * (h * 0.5 - 14)
                        for k in range(2):
                            a.box((3, 14, 1.1), at=(cx - sxx * k * 5, cy, 0.7), rot=(0, sxx * syy * 45, 0), col=C.INK, bevel=0.2, jitter=0)
                # scuffs on the tape
                rng = a.rng
                for _ in range(4):
                    a.box((rng.uniform(4, 10), 3, 0.3), at=(x + rng.uniform(-w / 2, w / 2), y + h * 0.5 * rng.choice((-1, 1)), 1.2),
                          col=C.shade(C.YELLOW, 0.75), bevel=0, jitter=0)
    return build


_delivery_bay("SM_Deco_DeliveryBay_Lobby", "DeliveryBay_Lobby", 1, 3, 200.0, 210.0, "the lobby loading bay")
_delivery_bay("SM_Deco_DeliveryBay_Stage4", "DeliveryBay_Stage4", 2, 4, 210.0, 215.0, "the Stage 4 prop depot")
