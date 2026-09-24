"""Studio-supply catalogue items (UFTEconomyConfig::Items, Career/FTEconomy.cpp).

In the game every item is a list of FFTItemPart primitives spawned at runtime by FTShop::BuildParts - on the
item actor (props, effects, set pieces), on a wearer's attach point (accessories) or on the shark rig's SharkRoot
(shark kits). Each mesh here replaces one item's part list 1:1: pivot = the parts' origin, same axes, same bounds.
The bounds check reads the part lists straight from the C++ (layout/cppactor.py).
"""
import math
import os
import sys

from ftb import palette as C
from ftb.core import bm_box, bm_extrude, bm_lathe, bm_sphere, deform, wobble, xf
from ftb.registry import ASSETS, asset
from mathutils import Vector

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "layout"))
import cppactor as CA  # noqa: E402

SRC = "Career/FTEconomy.cpp"
ITEMS = {}


def item(item_id, name, folder, desc, pivot, integration="", view=(1.0, 0.85, 0.62), note=""):
    def deco(fn):
        ITEMS[name] = (item_id, note)
        return asset(name, "Studio/ShopItems/" + folder, desc=desc,
                     replaces=["UFTEconomyConfig item '%s' (%s): FFTItemPart list spawned by FTShop::BuildParts" % (item_id, SRC)],
                     pivot=pivot, integration=integration, view=view, tags=["shop:" + item_id])(fn)
    return deco


HEAD = "the wearer's Head anchor (top of the head, +X = face direction), like the item's FFTItemPart coordinates"
FACE = "the wearer's Face anchor (eye line, +X = forward)"
BODY = "the wearer's Body anchor (chest front, +X = forward)"
FLOOR = "item actor origin on the floor (the FFTItemPart origin); front = +X"
SHARK = "SharkRoot of the shark rig (snout along +X), like the kit's FFTItemPart coordinates"
FX_BOX = ("Effects: the mesh includes the charcoal control box BuildLook() adds behind the unit; the red SwitchLamp "
          "stays a code part (it is the interaction highlight). Glowing parts use the glow slot - drive its intensity "
          "with a dynamic material instance where the code toggles PartGlow.")


# ============================================================================ accessories

@item("Acc.CaptainHat", "SM_Acc_CaptainHat", "Accessories",
      "Captain's hat: white crown with a soft flare and seam, navy band with gold braid, glossy black visor and a glowing gold anchor badge.", HEAD)
def captain_hat(a):
    a.cyl(15.2, 11, at=(0, 0, 8.5), r_top=16.2, col=C.WHITE, sides=18, bevel=3, rough=0.7)
    a.cyl(16.3, 2.4, at=(0, 0, 13.2), col=C.WHITE, sides=18, bevel=1.1, rough=0.7)
    a.cyl(16, 5, at=(0, 0, 1), col=C.NAVY, sides=18, bevel=1.2)
    a.tube([(15.5 * math.cos(t), 15.5 * math.sin(t), 2.4) for t in [math.radians(-70 + k * 10) for k in range(15)]], 0.8, col=C.BRASS, sides=5, rough=0.35)
    visor = []
    for k in range(13):
        t = math.radians(-90 + k * 15)
        visor.append((4 + 13 * math.cos(t), 14.5 * math.sin(t)))
    v = bm_extrude(visor, 2.2, 0.8)
    deform(v, lambda c: Vector((c.x, c.y, c.z - 0.018 * max(0.0, c.x - 4) ** 2 - 0.006 * c.y * c.y)))
    a.add(v, xf((4, 0, -2.6), (-8, 0, 0)), C.INK, rough=0.25)
    a.box((1.6, 8, 6.4), at=(16.2, 0, 6), col=C.YELLOW, bevel=0.6, glow=1.5)
    a.cyl(1.2, 5.2, at=(17.2, 0, 6), col=C.BRASS, sides=6, rough=0.3)
    a.torus(2.1, 0.6, at=(17.2, 0, 3.8), rot=(0, 90, 90), col=C.BRASS, major=8, minor=4, rough=0.3)


def star(r_out, r_in, n=5, rot=90.0):
    pts = []
    for k in range(n * 2):
        r = r_out if k % 2 == 0 else r_in
        ang = math.radians(rot + k * 180.0 / n)
        pts.append((r * math.cos(ang), r * math.sin(ang)))
    return pts


@item("Acc.StarShades", "SM_Acc_StarShades", "Accessories",
      "Star sunglasses: two glowing magenta star lenses in chunky yellow frames, bridge and temple arms.", FACE)
def star_shades(a):
    for s in (-1, 1):
        a.poly(star(6.2, 3.1), 1.6, at=(0.4, s * 8, 0.2), col=C.MAGENTA, glow=0.6)
        a.poly(star(7.2, 3.9), 2.2, at=(-0.6, s * 8, 0.2), col=C.YELLOW, rough=0.4)
        a.box((16, 1.6, 1.6), at=(-8, s * 13.8, 2), col=C.YELLOW, bevel=0.6, rough=0.4)
        a.box((2.4, 1.8, 3), at=(-15.4, s * 13.8, 0.8), col=C.YELLOW, bevel=0.6, rough=0.4)
    a.tube([(0, -2.6, 2), (0.6, 0, 2.8), (0, 2.6, 2)], 0.9, col=C.YELLOW, sides=6, rough=0.4)


@item("Acc.DirectorBeret", "SM_Acc_DirectorBeret", "Accessories",
      "Auteur beret: soft coral-dark felt with a rolled band, tilted like the code part, and a coral stalk nub.", HEAD)
def director_beret(a):
    prof = [(0.0, 7.6), (7, 7.4), (12.5, 6.4), (16.3, 4.4), (17.2, 2.2), (15.2, 0.6), (13.8, 0.2), (12.4, 0.6), (0.0, 0.6)]
    b = bm_lathe(prof, 20)
    wobble(b, 0.35, scale=0.3, seed=11)
    a.add(b, xf((2, 0, 0.2), (0, 0, 10)), C.CORAL_DARK, rough=1.0)
    a.torus(13.4, 1.1, at=(2, 0, 0.8), rot=(0, 0, 10), col=C.shade(C.CORAL_DARK, 0.8), major=20, minor=5)
    a.cyl(1.8, 5.2, at=(2, 3, 10), rot=(0, 0, 10), col=C.CORAL, sides=8, bevel=0.8)


@item("Acc.AlienAntennae", "SM_Acc_AlienAntennae", "Accessories",
      "Alien antennae: dark-green headband, springy green stalks and two glowing bobble balls.", HEAD)
def alien_antennae(a):
    a.box((8, 30, 3), at=(0, 0, 1), col=C.GREEN_DARK, bevel=1.2)
    for s in (-1, 1):
        pts = []
        for k in range(9):
            t = k / 8.0
            pts.append((0.9 * math.sin(t * 6 * math.pi), s * (7 + 4.2 * t), 2 + 20 * t))
        a.tube(pts, 1.4, col=C.GREEN, sides=6)
        a.cyl(2.4, 2.4, at=(0, s * 7, 3), col=C.GREEN_DARK, sides=8, bevel=0.8)
        a.sphere(4.1, at=(0, s * 11, 24), col=C.GREEN, segs=10, rings=7, glow=3)
        a.sphere(1.4, at=(2.8, s * 12.2, 25.6), col=C.WHITE, segs=6, rings=4, glow=3)


@item("Acc.RoyalCrown", "SM_Acc_RoyalCrown", "Accessories",
      "Foam royal crown: gold band with a jewelled rim, four points with ball tips and a glowing ruby at the front.", HEAD)
def royal_crown(a):
    a.cyl(13.5, 10, at=(0, 0, 5), r_top=14.2, col=C.YELLOW, sides=20, bevel=1.5, rough=0.5, glow=0.4)
    a.torus(13.9, 1.3, at=(0, 0, 0.6), col=C.AMBER, major=20, minor=6, rough=0.4)
    a.torus(14.5, 1.1, at=(0, 0, 9.6), col=C.AMBER, major=20, minor=6, rough=0.4)
    for k in range(4):
        ang = math.radians(k * 90)
        c, s = math.cos(ang), math.sin(ang)
        a.cone(4, 12, at=(12 * c, 12 * s, 15), col=C.YELLOW, sides=8, rough=0.5)
        a.sphere(1.6, at=(12 * c, 12 * s, 21.2), col=C.AMBER, segs=8, rings=5, rough=0.4)
        mid = math.radians(k * 90 + 45)
        a.sphere(1.5, at=(14.4 * math.cos(mid), 14.4 * math.sin(mid), 5), col=[C.CYAN, C.GREEN, C.MAGENTA, C.CYAN][k], segs=8, rings=5, glow=1.0)
    a.sphere(3, at=(14.2, 0, 6), col=C.RED, segs=8, rings=6, flat=True, glow=2.0)


@item("Acc.HeroCape", "SM_Acc_HeroCape", "Accessories",
      "Hero cape: billowing red cape with folds and a darker lining, gold collar bar, shoulder straps and a round star clasp.", BODY)
def hero_cape(a):
    secs = []
    for k in range(7):
        t = k / 6.0
        z = 8 - 66 * t
        w = 20 + 5 * t
        x = -19.5 - 3.2 * t
        row = []
        for i in range(9):
            u = -1 + 2 * i / 8.0
            fold = 2.8 * math.sin(u * math.pi * 2.5 + 0.6) * (0.25 + 0.75 * t)
            row.append((x + fold - 1.8 * (1 - u * u) * t, u * w, z))
        back = [(p[0] - 2.4, p[1], p[2]) for p in reversed(row)]
        secs.append(row + back)
    a.loft(secs, col=C.RED, rough=0.9)
    a.box((6, 50, 7), at=(-18, 0, 9), col=C.YELLOW, bevel=2, rough=0.45)
    for s in (-1, 1):
        a.box((34, 5, 6), at=(-2, s * 24, 9), col=C.YELLOW, bevel=1.8, rough=0.45)
    a.cyl(4.5, 2.6, at=(17, 0, 9), rot=(0, 90, 90), col=C.YELLOW, sides=14, bevel=0.8, glow=1.0)
    a.poly(star(3.2, 1.4), 1.2, at=(18.6, 0, 9), col=C.RED)


# ============================================================================ props

@item("Prop.TreasureChest", "SM_Item_TreasureChest", "Props",
      "Treasure chest: plank body with gold corner bands and rivets, open curved lid tilted back, heap of glowing gold coins, lock plate.", FLOOR)
def treasure_chest(a):
    a.box((62, 42, 36), at=(0, 0, 18), col=C.WOOD, bevel=2.5)
    for k in range(4):
        a.box((62.6, 0.8, 1.2), at=(0, -21.1 + 0.001, 6 + k * 9), col=C.WOOD_DARK, bevel=0.2)
        a.box((62.6, 0.8, 1.2), at=(0, 21.1, 6 + k * 9), col=C.WOOD_DARK, bevel=0.2)
    a.box((64, 44, 6), at=(0, 0, 22), col=C.YELLOW, bevel=1.5, rough=0.4)
    a.box((64, 44, 4), at=(0, 0, 2), col=C.YELLOW, bevel=1.2, rough=0.4)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.box((5, 5, 38), at=(sx * 30, sy * 20, 19), col=C.YELLOW, bevel=1.2, rough=0.4)
    lid = bm_box(64, 44, 10, 2.5, 2)
    deform(lid, lambda c: Vector((c.x, c.y, c.z + 4 * (1 - (c.y / 22.0) ** 2) * (1 if c.z > 0 else 0))))
    a.add(lid, xf((-4, 0, 46), (-12, 0, 0)), C.WOOD_DARK, rough=0.9)
    for yy in (-14, 14):
        band = bm_box(66, 4, 12, 1, 1)
        deform(band, lambda c, yy=yy: Vector((c.x, c.y, c.z + 4 * (1 - (yy / 22.0) ** 2) * (1 if c.z > 0 else 0))))
        a.add(band, xf((-4, yy, 46), (-12, 0, 0)), C.YELLOW, rough=0.4)
    coins = bm_sphere(24, 16, 3.6, 12, 6)
    wobble(coins, 1.2, scale=0.35, seed=3)
    a.add(coins, xf((0, 0, 40.5)), C.YELLOW, glow=2.0)
    for k, (x, y) in enumerate(((-12, -6), (6, 8), (14, -9), (-4, 10))):
        a.cyl(2.6, 0.9, at=(x, y, 43.6 + (k % 2)), rot=(k * 17, k * 40, 0), col=C.AMBER, sides=10, glow=2.0)
    a.box((4, 9, 12), at=(32, 0, 30), col=C.YELLOW, bevel=1, rough=0.35)
    a.cyl(1.4, 1.2, at=(34.2, 0, 28), rot=(0, 90, 90), col=C.INK, sides=8)


@item("Prop.GiantClam", "SM_Item_GiantClam", "Props",
      "Giant clam: ribbed magenta bottom shell, coral upper shell hinged open, glowing cream pearl on a tongue.", FLOOR)
def giant_clam(a):
    def shell(rx, ry, rz, ribs=9):
        b = bm_sphere(rx, ry, rz, 24, 10)
        def f(c):
            ang = math.atan2(c.y, c.x)
            k = 1.0 + 0.06 * max(0.0, math.cos(ang * ribs))
            return Vector((c.x * k, c.y * k, c.z))
        deform(b, f)
        return b
    lo = shell(38, 38, 14)
    deform(lo, lambda c: Vector((c.x, c.y, min(c.z, 10.0) if c.z > 0 else c.z)))
    a.add(lo, xf((0, 0, 14)), C.MAGENTA, rough=0.6)
    a.cyl(34, 2, at=(0, 0, 24.4), col=C.shade(C.MAGENTA, 0.6), sides=24)
    up = shell(37, 37, 12)
    deform(up, lambda c: Vector((c.x, c.y, max(c.z, -6.0) if c.z < 0 else c.z)))
    a.add(up, xf((-14, 0, 34), (-28, 0, 0)), C.CORAL, rough=0.6)
    a.cyl(10, 3, at=(4, 0, 25.6), col=C.CORAL_DARK, sides=12, ry=13)
    a.sphere(8, at=(10, 0, 26), col=C.CREAM, segs=14, rings=9, glow=2.0)


@item("Prop.LavaLamp", "SM_Item_LavaLamp", "Props",
      "Retro lava lamp: charcoal cone base, glowing magenta glass bottle with yellow blobs, cone cap.", FLOOR)
def lava_lamp(a):
    a.lathe([(0, 0), (12, 0), (11.5, 2), (9, 16), (0, 16)], col=C.CHARCOAL, sides=16, rough=0.4)
    a.torus(9.4, 0.8, at=(0, 0, 15.2), col=C.CHROME, major=16, minor=4, rough=0.25)
    a.lathe([(0, 14.5), (7, 14.5), (9, 24), (9, 42), (6.5, 60), (5.5, 66), (0, 66)], col=C.MAGENTA, sides=16, glow=3)
    for (x, y, z, r) in ((0, 0, 34, 5), (2, -2, 46, 3.2), (-2, 1.5, 55, 2.4), (1, 2, 22, 2.8)):
        a.sphere(r, at=(x, y, z), col=C.YELLOW, segs=10, rings=7, glow=4)
    a.lathe([(0, 64), (8, 64), (5, 72), (2.4, 76), (0, 76)], col=C.CHARCOAL, sides=16, rough=0.4)


@item("Prop.FoamSword", "SM_Item_FoamSword", "Props",
      "Legendary foam sword: rounded cream foam blade with a fuller line and tip, chunky yellow crossguard with gems, glowing cyan grip and pommel.", FLOOR)
def foam_sword(a):
    blade = bm_box(6, 14, 90, 2.6, 2)
    a.add(blade, xf((0, 0, 60)), C.CREAM, rough=0.95)
    a.box((6.4, 2.4, 72), at=(0, 0, 62), col=C.CREAM_DARK, bevel=0.8)
    a.prism((6, 14, 14), at=(0, 0, 110), col=C.CREAM, rough=0.95)
    guard = bm_box(10, 40, 6, 2.5, 2)
    deform(guard, lambda c: Vector((c.x, c.y, c.z + 0.004 * c.y * c.y)))
    a.add(guard, xf((0, 0, 14)), C.YELLOW, rough=0.45)
    for s in (-1, 1):
        a.sphere(3.2, at=(0, s * 20, 15.6), col=C.YELLOW, segs=8, rings=6, rough=0.45)
    a.sphere(2.4, at=(5, 0, 14), col=C.MAGENTA, segs=8, rings=5, flat=True, glow=1.5)
    a.cyl(4, 12, at=(0, 0, 5), col=C.CYAN, sides=10, bevel=1, glow=3)
    for k in range(3):
        a.torus(4.2, 0.6, at=(0, 0, 1.5 + k * 3.5), col=C.shade(C.CYAN, 0.7), major=10, minor=4)
    a.sphere(4, at=(0, 0, -2), col=C.CYAN, segs=10, rings=6, glow=3)


# ============================================================================ practical effects

def control_box(a, x):
    """The charcoal switch box BuildLook() adds at (B.Min.X - 8, 0, 20); the SwitchLamp stays in the code."""
    a.box((12, 26, 30), at=(x, 0, 20), col=C.CHARCOAL, bevel=2.5)
    a.box((1, 18, 4), at=(x + 6.2, 0, 30), col=C.YELLOW, bevel=0.3)
    a.tube([(x + 2, 8, 6), (x + 6, 12, 1.5), (x + 16, 12, 1.5)], 1.2, col=C.RUBBER, sides=5)


def hazard_band(a, size, at):
    a.box(size, at=at, col=C.YELLOW, bevel=1)
    n = int(size[1] / 6)
    for k in range(n):
        a.box((size[0] + 0.4, 2.2, size[2] * 0.9), at=(at[0], -size[1] / 2 + (k + 0.5) * size[1] / n, at[2]), rot=(0, 0, 35), col=C.INK, bevel=0.2)


@item("Fx.Pyro", "SM_Fx_PyroFountain", "Effects",
      "Cold-spark pyro fountain: black base with a hazard band and vents, grey nozzle tower with cooling rings, glowing orange emitter; control box behind.", FLOOR,
      integration=FX_BOX)
def pyro_fountain(a):
    a.box((42, 42, 20), at=(0, 0, 10), col=C.CHARCOAL, bevel=3)
    hazard_band(a, (44, 44, 4), (0, 0, 12))
    for k in range(3):
        a.box((1, 20, 1.6), at=(21.2, 0, 4 + k * 2.6), col=C.INK, bevel=0.2)
    a.cyl(7, 14, at=(0, 0, 26), col=C.GREY_DARK, sides=12, bevel=1.5)
    for k in range(3):
        a.torus(7.2, 0.7, at=(0, 0, 21.5 + k * 3.5), col=C.GREY, major=12, minor=4)
    a.sphere(4, at=(0, 0, 34), col=C.ORANGE, segs=10, rings=6, glow=2)
    control_box(a, -21 - 8)


@item("Fx.Fog", "SM_Fx_FogMachine", "Effects",
      "Deluxe ground-fog machine: purple case with rounded corners and a grille, charcoal nozzle, cream fluid tank with a cap, green ready lamp; control box behind.", FLOOR,
      integration=FX_BOX)
def fog_machine(a):
    a.box((60, 42, 40), at=(0, 0, 20), col=C.PURPLE, bevel=5)
    for k in range(5):
        a.box((1, 30, 2), at=(30.2, 0, 10 + k * 5), col=C.shade(C.PURPLE, 0.6), bevel=0.2)
    a.cyl(8, 16, at=(36, 0, 22), rot=(-90, 0, 0), col=C.CHARCOAL, sides=12, bevel=2)
    a.cyl(5.5, 2, at=(44.2, 0, 22), rot=(-90, 0, 0), col=C.INK, sides=12)
    a.cyl(9, 18, at=(-12, 0, 48), col=C.CREAM, sides=12, bevel=2.5, rough=0.5)
    a.cyl(4, 3, at=(-12, 0, 58), col=C.TEAL, sides=10, bevel=1)
    a.sphere(4, at=(-26, 16, 36), col=C.GREEN, segs=8, rings=6, glow=2)
    for s in (-1, 1):
        a.box((50, 3, 3), at=(0, s * 22, 1.5), col=C.RUBBER, bevel=0.8)
    control_box(a, -30 - 8)


@item("Fx.Confetti", "SM_Fx_ConfettiCannon", "Effects",
      "Confetti cannon: black base plate with bolts, raked coral barrel with gold rings and a yellow muzzle, confetti peeking out; control box behind.", FLOOR,
      integration=FX_BOX)
def confetti_cannon(a):
    a.box((42, 42, 12), at=(0, 0, 6), col=C.CHARCOAL, bevel=2.5)
    for sx in (-1, 1):
        for sy in (-1, 1):
            a.cyl(2, 1.4, at=(sx * 16, sy * 16, 12.4), col=C.GREY, sides=6)
    a.cyl(10, 10, at=(0, 0, 14), col=C.GREY_DARK, sides=12, bevel=2)
    a.cyl(13, 70, at=(8, 0, 42), rot=(-20, 0, 0), col=C.CORAL, sides=14, bevel=3)
    from ftb.core import ue_rot
    ax = ue_rot(-20, 0, 0) @ Vector((0, 0, 1))
    for t in (-22, 0, 22):
        p = Vector((8, 0, 42)) + ax * t
        a.torus(13.2, 1.1, at=p, rot=(-20, 0, 0), col=C.YELLOW, major=14, minor=4, rough=0.4)
    a.cyl(14, 6, at=(20, 0, 74), rot=(-20, 0, 0), col=C.YELLOW, sides=14, bevel=1.5, rough=0.4)
    for k in range(6):
        ang = math.radians(k * 60 + 15)
        p = Vector((20, 0, 74)) + ue_rot(-20, 0, 0) @ Vector((7 * math.cos(ang), 7 * math.sin(ang), 2.5))
        a.box((3, 3, 0.6), at=p, rot=(k * 25, k * 50, k * 15), col=[C.MAGENTA, C.CYAN, C.YELLOW, C.GREEN, C.CORAL, C.WHITE][k], bevel=0.1)
    control_box(a, -21 - 8)


@item("Fx.Bubbles", "SM_Fx_BubbleMachine", "Effects",
      "Bubble machine: cyan body with a fan grille, blue lid, cream bubble-wand ring on a spinner arm; control box behind.", FLOOR,
      integration=FX_BOX)
def bubble_machine(a):
    a.box((40, 40, 36), at=(0, 0, 18), col=C.CYAN, bevel=4)
    a.cyl(11, 1.2, at=(20.3, 0, 18), rot=(-90, 0, 0), col=C.shade(C.CYAN, 0.6), sides=16)
    for k in range(3):
        a.torus(4 + k * 3, 0.5, at=(20.8, 0, 18), rot=(0, 90, 90), col=C.GREY, major=12, minor=3)
    a.box((42, 42, 4), at=(0, 0, 37), col=C.BLUE, bevel=1.5)
    a.torus(11.5, 1.6, at=(10, 0, 46), rot=(0, 0, 90), col=C.CREAM, major=18, minor=6, rough=0.4)
    a.cyl(1.2, 8, at=(10, 0, 37.5), col=C.GREY, sides=6)
    for (x, y, z, r) in ((16, -6, 58, 2.6), (3, 5, 55, 1.8), (12, 7, 53, 1.4)):
        a.sphere(r, at=(x, y, z), col=C.SKY_BLUE, segs=8, rings=5, mat="glass")
    control_box(a, -20 - 8)


# ============================================================================ shark rig kits

@item("Shark.ChromeTeeth", "SM_SharkKit_ChromeTeeth", "SharkKits",
      "Chrome teeth kit: four big polished fangs on a steel gum bracket (fits the rig shark's upper jaw).", SHARK, view=(1, 0.5, -0.3))
def chrome_teeth(a):
    for (x, y, h) in ((192, -30, 30), (196, -10, 32), (196, 10, 32), (192, 30, 30)):
        t = bm_lathe([(0, -h / 2), (3, -h / 2 + h * 0.35), (8.5, h / 2 - 3), (10, h / 2), (0, h / 2)], 12)
        deform(t, lambda c: Vector((c.x, c.y * 0.85, c.z)))
        a.add(t, xf((x, y, -16), (180, 0, 0)), C.CHROME, rough=0.2, glow=1.5)
    a.box((10, 76, 5), at=(193, 0, -2.5), col=C.GREY_DARK, bevel=1.5, rough=0.35)


@item("Shark.GlowEyes", "SM_SharkKit_GlowEyes", "SharkKits",
      "Evil glow eyes: two red LED eye domes with slit pupils and black bezels (fit over the rig shark's eyes).", SHARK, view=(1, 0.2, 0.3))
def glow_eyes(a):
    for s in (-1, 1):
        a.sphere(7.5, at=(164, s * 58, 47), ry=4.5, rz=9, col=C.RED, segs=12, rings=8, glow=8)
        a.box((1.2, 1.2, 10), at=(170.6, s * 58, 47), col=C.INK, bevel=0.3)
        a.torus(7.6, 1.2, at=(164, s * 58.5, 47), rot=(0, 0, 90), col=C.CHARCOAL, major=14, minor=4, rz=0.9)


@item("Shark.BattleScars", "SM_SharkKit_BattleScars", "SharkKits",
      "Battle-scar paint job: raised cream scar strips with stitch marks on the flanks and a bite notch on the back.", SHARK, view=(0.4, 1, 0.5))
def battle_scars(a):
    def scar(at, length, rot, ribs=True, ax="y"):
        a.box((length, 3, 6) if ax == "y" else (length, 6, 3), at=at, rot=rot, col=C.CREAM, bevel=1.2, rough=0.8)
        if ribs:
            from ftb.core import ue_rot
            R = ue_rot(*rot)
            n = int(length / 8)
            for k in range(n):
                off = R @ Vector((-length / 2 + (k + 0.5) * length / n, 0, 0))
                sz = (1.4, 3.6, 9) if ax == "y" else (1.4, 9, 3.6)
                a.box(sz, at=(at[0] + off.x, at[1] + off.y, at[2] + off.z), rot=rot, col=C.shade(C.CREAM, 0.75), bevel=0.3)
    scar((40, -80, 36), 46, (0, 0, 0))
    scar((10, -82, 20), 40, (12, 0, 0))
    scar((30, 80, 28), 50, (-10, 0, 0))
    scar((-30, 0, 80), 30, (0, 0, 0), ax="z")


# ============================================================================ set pieces

@item("Set.TikiTorches", "SM_Set_TikiTorches", "SetPieces",
      "Tiki torch pair: segmented bamboo poles with lashings, woven cups, stylised glowing flames, wooden base plank with rope ties.", FLOOR,
      integration="The flames use the glow slot (the code toggles their emissive with the Glow fx).", view=(1, 0.6, 0.4))
def tiki_torches(a):
    a.box((30, 140, 8), at=(0, 0, 4), col=C.WOOD_DARK, bevel=2)
    for k in range(5):
        a.box((30.4, 1, 8.4), at=(0, -56 + k * 28, 4), col=C.shade(C.WOOD_DARK, 0.8), bevel=0.2)
    for s in (-1, 1):
        y = s * 55
        a.cyl(4.5, 116, at=(0, y, 62), col=C.WOOD_DARK, sides=10, bevel=1)
        for k in range(5):
            a.torus(4.7, 0.9, at=(0, y, 22 + k * 22), col=C.shade(C.WOOD_DARK, 0.75), major=10, minor=4)
        a.cyl(5.5, 5, at=(0, y, 104), col=C.SAND, sides=10, bevel=1)
        a.cyl(10, 16, at=(0, y, 124), r_top=11, col=C.WOOD, sides=12, bevel=2)
        for k in range(2):
            a.torus(10.6, 0.8, at=(0, y, 119 + k * 8), col=C.SAND, major=12, minor=4)
        f = bm_lathe([(0, 0), (8.5, 3), (8, 10), (5, 18), (2.5, 24), (0, 28)], 10)
        deform(f, lambda c: Vector((c.x + 1.6 * math.sin(c.z * 0.35), c.y + 1.0 * math.cos(c.z * 0.3), c.z)))
        a.add(f, xf((0, y, 130)), C.ORANGE, glow=5)
        a.cone(5, 16, at=(0, y, 139), col=C.YELLOW, sides=8, glow=5)
        a.tube([(0, y - 3, 8), (6, y - 8, 1), (12, y - 12, 1)], 0.8, col=C.SAND, sides=4)


@item("Set.NeonPalm", "SM_Set_NeonPalm", "SetPieces",
      "Neon palm sign: rounded navy board with a chrome border, cyan neon trunk and green neon fronds with a coconut dot, two stand legs with feet.", FLOOR,
      integration="Neon tubes use the glow slot (the code toggles their emissive with the Glow fx).", view=(1, 0.4, 0.3))
def neon_palm(a):
    from ftb.core import superellipse
    board = superellipse(130, 100, 28, power=6)
    a.poly(board, 8, at=(0, 0, 120), col=C.NAVY, bevel=1.5)
    a.poly(superellipse(124, 94, 28, power=6), 1, at=(4.3, 0, 120), col=C.NAVY_LIGHT)
    a.tube([(4.6, p[0], 120 + p[1]) for p in superellipse(126, 96, 28, power=6)], 1.1, col=C.CHROME, sides=5, closed=True, caps=False, rough=0.3)
    trunk = [(6, 2 * math.sin(t * 2.4), 74 + 60 * t) for t in [k / 8.0 for k in range(9)]]
    a.tube(trunk, 1.6, col=C.CYAN, sides=6, glow=6)
    for k in range(4):
        a.tube([(6, -2.5, 80 + k * 14), (6, 2.5, 82 + k * 14)], 1.1, col=C.CYAN, sides=5, glow=6)
    top = (6, 1.5, 136)
    for (dy, dz, bend) in ((-44, 6, -8), (44, 6, -8), (-36, -12, -10), (36, -12, -10), (0, 30, 0)):
        pts = []
        for k in range(7):
            t = k / 6.0
            pts.append((6, top[1] + dy * t, top[2] + dz * t + bend * math.sin(t * math.pi) * (-1 if dz > 0 else 1) - 6 * t * t * (1 if dz <= 0 else 0)))
        a.tube(pts, 1.5, col=C.GREEN, sides=6, glow=6)
    a.sphere(3.2, at=(6.5, 6, 131), col=C.ORANGE, segs=8, rings=5, glow=4)
    for s in (-1, 1):
        a.cyl(3.5, 72, at=(0, s * 50, 36), col=C.CHARCOAL, sides=8, bevel=1)
        a.box((30, 8, 4), at=(0, s * 50, 2), col=C.CHARCOAL, bevel=1.2)


@item("Set.ShipwreckBow", "SM_Set_ShipwreckBow", "SetPieces",
      "Shipwreck bow: weathered plank hull (right-angle wedge like the code ramp) with a red waterline stripe, broken deck planks, leaning mast and a tattered black flag.", FLOOR,
      view=(1, -0.8, 0.5))
def shipwreck_bow(a):
    from ftb.core import bm_prism
    hull = bm_prism([(-55, -50), (55, -50), (-55, 50)], 160)
    wobble(hull, 1.2, scale=0.2, seed=7)
    a.add(hull, xf((0, 0, 50)), C.WOOD, rough=0.95)
    ang = math.degrees(math.atan2(100, 110))        # slope of the ramp face (right-angle wedge)
    nrm = Vector((0, math.sin(math.radians(ang)), math.cos(math.radians(ang))))
    for k in range(7):
        a.box((160.6, 2, 1.4), at=(0, -55.3, 8 + k * 13), col=C.WOOD_DARK, bevel=0.3)
    for k in range(6):
        t = (k + 0.5) / 6.0
        p = Vector((0, 55 - 110 * t, 100 * t)) + nrm * 0.5
        a.box((160.6, 2.2, 1.4), at=p, rot=(0, 0, ang), col=C.WOOD_DARK, bevel=0.3)
    a.box((150, 3, 6), at=(0, -56.2, 40), col=C.CORAL_DARK, bevel=1)
    p = Vector((0, 55 - 110 * 0.4, 40)) + nrm * 1.0
    a.box((150, 6, 2), at=p, rot=(0, 0, ang), col=C.CORAL_DARK, bevel=0.6)
    for k in range(7):
        x = -72 + k * 15.5
        ln = 116 if k % 3 else 96
        a.box((14, ln, 3), at=(x, (116 - ln) / 4 * (1 if k % 2 else -1), 98 + (k % 2) * 0.6),
              rot=(0, (k % 3 - 1) * 1.5, 0), col=C.WOOD_DARK, bevel=0.8)
    a.cyl(5, 150, at=(-40, 0, 170), rot=(-10, 0, 0), col=C.WOOD_DARK, sides=10, bevel=1.5)
    a.cyl(2, 60, at=(-46, 0, 204), rot=(-10, 0, 90), col=C.WOOD_DARK, sides=6)
    flag = bm_box(3, 40, 28, 0.5, 1)
    deform(flag, lambda c: Vector((c.x + 2.2 * math.sin(c.y * 0.18), c.y, c.z - (3 if (c.y > 12 and c.z < -8) else 0))))
    a.add(flag, xf((-44, 18, 220)), C.CHARCOAL, rough=1.0)
    a.sphere(4, at=(-42, 18, 222), ry=4, rz=4.5, col=C.CREAM, segs=8, rings=5)


@item("Set.CardboardMoon", "SM_Set_CardboardMoon", "SetPieces",
      "Cardboard full moon: glowing cream disc with a torn edge and grey craters, cardboard brace on the back, wooden stick and a weighted base with a sandbag.", FLOOR,
      integration="The moon face uses the glow slot (the code toggles its emissive with the Glow fx).", view=(1, 0.5, 0.3))
def cardboard_moon(a):
    rim = []
    for k in range(36):
        ang = 2 * math.pi * k / 36
        r = 70 - (2.2 if k % 3 == 1 else 0) - (1.2 if k % 5 == 0 else 0)
        rim.append((r * math.cos(ang), r * math.sin(ang)))
    a.poly(rim, 8, at=(0, 0, 170), col=C.CREAM, glow=0.8)
    for (y, z, r, d) in ((-30, 190, 15, 3), (28, 150, 11, 3), (22, 205, 6, 2), (-12, 138, 7, 2)):
        a.cyl(r, d, at=(4.8, y, z), rot=(0, 90, 90), col=C.CREAM_DARK, sides=14, bevel=0.8)
        a.torus(r, 1, at=(5.8, y, z), rot=(0, 90, 90), col=C.shade(C.CREAM_DARK, 0.85), major=14, minor=4)
    a.box((2, 12, 110), at=(-5, 0, 165), col=C.SAND, bevel=0.5)
    a.box((2, 100, 12), at=(-5, 0, 172), col=C.SAND, bevel=0.5)
    a.cyl(4, 100, at=(-8, 0, 50), col=C.WOOD, sides=8, bevel=1)
    a.box((60, 60, 8), at=(-8, 0, 4), col=C.WOOD_DARK, bevel=2)
    bag = bm_sphere(12, 8, 5, 10, 6)
    wobble(bag, 0.8, scale=0.3, seed=5)
    a.add(bag, xf((-24, 16, 11)), C.SAND, rough=1.0)


# ============================================================================ bounds from the C++ part lists

def _code_items():
    it = CA.Interp(CA.source())
    actor, _ = it.run_actor("UFTEconomyConfig", construct=())
    return {d.members.get("ItemId"): d for d in (actor.members.get("Items") or []) if isinstance(d, CA.Obj)}


def _corners(parts):
    pts = []
    for p in parts:
        c = CA.Comp("Part", "p", 0, [])
        c.shape = p.members["Shape"].split("::")[-1]
        c.loc, c.rot = p.members["Location"], p.members["Rotation"]
        s = p.members["Size"]
        c.scale = CA.V(s.x / 100, s.y / 100, s.z / 100)
        c.size = s
        lo, hi = c.bounds()
        pts += [lo, hi]
    return pts


_DEFS = _code_items()
_BY_NAME = {s.name: s for s in ASSETS}
for _name, (_id, _note) in ITEMS.items():
    _d = _DEFS.get(_id)
    _spec = _BY_NAME.get(_name)
    if _d is None or _spec is None:
        continue
    _parts = list(_d.members.get("Parts") or [])
    _pts = _corners(_parts)
    if str(_d.members.get("Category", "")).endswith("Effect"):
        _minx = min(p[0] for p in _pts)
        box = CA.Obj("FFTItemPart")
        box.members = {"Shape": "EFTShape::Box", "Location": CA.V(_minx - 8, 0, 20), "Size": CA.V(12, 26, 30), "Rotation": CA.R()}
        _parts.append(box)
        _pts = _corners(_parts)
    _lo = [min(p[i] for p in _pts) for i in range(3)]
    _hi = [max(p[i] for p in _pts) for i in range(3)]
    _spec.expect = (lambda lo, hi, n: (lambda: {"min": lo, "max": hi, "parts": n}))(_lo, _hi, len(_parts))
    _spec.code_parts = "%s item %s: %d FFTItemPart%s" % (SRC, _id, len(_d.members.get("Parts") or []), " + BuildLook() control box" if len(_parts) > len(_d.members.get("Parts") or []) else "")
    _spec.fit_note = _note
