"""Crew base character sheet.

Assembles the modular crew meshes (assets/characters.py) exactly the way AFTCharacter::BuildBody places its parts
(component tree replayed by layout/cppactor.py, unit-space meshes scaled by the code sizes), paints them like
AFTCharacter::ApplyLook and renders review sheets; saves SourceArt/Blender/Characters.blend to look at in Blender.

    python3 Tools/blender/character_sheet.py [--res 900] [--samples 32] [--no-blend] [--only hero,face,...]

Renders (SourceArt/Blender/Previews/Characters/):
    Crew_Base_Hero.png         3/4 hero view in a relaxed idle pose
    Crew_Base_Turnaround.png   front / side / back / 3/4 in the code's rest pose
    Crew_Base_Face.png         face close-up and the expression set of AFTCharacter::UpdateFace
    Crew_Base_Modules.png      the same base with its modules swapped (hair, cap, glasses, headphones, bib)
    Crew_FirstPerson.png       the first-person arms seen from FirstPersonCamera
"""
import argparse
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402

import build_all as B  # noqa: E402
from ftb import core, export, palette as C, registry, render  # noqa: E402
from assets import characters as CH  # noqa: E402

OUT = os.path.join(B.ART_ROOT, "Previews", "Characters")
FLIP = Matrix.Diagonal((1, -1, 1, 1))

# FTCharacter.cpp GetCrewLook(): Skin, Hair, Shirt, Pants, Sleeve, Glove, Shoe, Cap, Brim, Bib, Glasses, bCap, bGlasses,
# bPhones, bBib, HairStyle
LOOKS = [
    dict(Skin=C.SKIN_A, Hair=C.HAIR_BROWN, Shirt=C.TEAL, Pants=C.TEAL, Sleeve=C.TEAL, Glove=C.CREAM, Shoe=C.CREAM,
         Cap=C.TEAL, Brim=C.TEAL_DARK, Bib=C.TEAL, Glasses=C.CORAL, cap=True, glasses=False, phones=False, bib=False, hair=0),
    dict(Skin=C.SKIN_D, Hair=C.HAIR_BROWN, Shirt=C.CORAL, Pants=C.CORAL, Sleeve=C.CORAL, Glove=C.SKIN_D, Shoe=C.CREAM,
         Cap=C.CORAL, Brim=C.CORAL, Bib=C.CORAL, Glasses=C.CORAL_DARK, cap=False, glasses=True, phones=False, bib=False, hair=1),
    dict(Skin=C.SKIN_C, Hair=C.HAIR_BLACK, Shirt=C.YELLOW, Pants=C.YELLOW, Sleeve=C.YELLOW, Glove=C.CORAL, Shoe=C.CHARCOAL,
         Cap=C.YELLOW, Brim=C.AMBER, Bib=C.YELLOW, Glasses=C.CHARCOAL, cap=True, glasses=False, phones=True, bib=False, hair=0),
    dict(Skin=C.SKIN_A, Hair=C.HAIR_GINGER, Shirt=C.CREAM, Pants=C.TEAL_LIGHT, Sleeve=C.CREAM, Glove=C.TEAL, Shoe=C.TEAL_LIGHT,
         Cap=C.CREAM, Brim=C.TEAL, Bib=C.TEAL_LIGHT, Glasses=C.CORAL, cap=True, glasses=False, phones=False, bib=True, hair=2),
]

PAINT_KEY = {"Torso": "Shirt", "Pelvis": "Pants", "LegL": "Pants", "LegR": "Pants", "ShoeL": "Shoe", "ShoeR": "Shoe",
             "ArmL": "Sleeve", "ArmR": "Sleeve", "HandL": "Glove", "HandR": "Glove", "Head": "Skin", "EarL": "Skin",
             "EarR": "Skin", "HairTop": "Hair", "HairBack": "Hair", "HairBun": "Hair", "BrowL": "Hair", "BrowR": "Hair",
             "CapCrown": "Cap", "CapBrim": "Brim", "GlassL": "Glasses", "GlassR": "Glasses", "Bib": "Bib",
             "FPSleeveL": "Sleeve", "FPSleeveR": "Sleeve", "FPCuffL": "Glasses", "FPCuffR": "Glasses",
             "FPHandL": "Glove", "FPHandR": "Glove"}

# UpdateFace(): (eye z, pupil scale, mouth size, mouth z, mouth roll, brow z, brow roll L, brow roll R)
EXPRESSIONS = {
    "Neutral": (1.0, 1.0, (3, 11, 3), 12.0, 0, 31.5, 0, 0),
    "Happy": (1.0, 1.0, (3, 14, 5), 12.0, 0, 33.0, 0, 0),
    "Panic": (1.15, 0.65, (4, 9, 10), 10.5, 0, 34.0, -18, 18),
    "Grumpy": (1.0, 1.0, (3, 12, 3), 12.0, 8, 30.5, 14, -14),
    "Smug": (1.0, 1.0, (3, 8, 3), 12.0, -12, 31.5, -10, 0),
    "Sleepy": (0.45, 1.0, (3, 12, 2), 11.0, 0, 29.5, -8, 8),
}

IDLE = {"ShoulderL": (2, 0, 9), "ShoulderR": (-3, 0, -8), "Neck": (-5, 0, 5), "Chest": (2, 0, -2), "HipL": (-4, 0, 0), "HipR": (5, 0, 0)}


def paint_of(comp, look):
    if comp in ("CuffL", "CuffR"):
        return tuple(x * 0.85 for x in look["Pants"])
    if comp == "Collar":
        return C.CREAM
    key = PAINT_KEY.get(comp)
    return look[key] if key else None


def visible(comp, look):
    if comp.startswith("FP"):
        return False
    if comp in ("CapCrown", "CapBrim", "CapBadge"):
        return look["cap"]
    if comp == "HairTop":
        return not look["cap"] and look["hair"] != 1
    if comp == "HairBun":
        return look["hair"] == 1
    if comp in ("GlassL", "GlassR"):
        return look["glasses"]
    if comp in ("PhoneL", "PhoneR", "PhoneBand"):
        return look["phones"]
    if comp == "Bib":
        return look["bib"]
    return True


def crew_specs():
    return [s for s in registry.ASSETS if getattr(s, "crew_comp", None)]


def ue_matrix(m):
    return FLIP @ Matrix([m[0], m[1], m[2], m[3]]) @ FLIP


def bake_paint(mesh, colour):
    """Multiply the M_FT_CrewPaint faces' vertex colours by the crew colour (what the material does in Unreal)."""
    lin = [C.srgb_to_lin(x) for x in colour]
    idx = [i for i, m in enumerate(mesh.materials) if m and m.name == CH.PAINT]
    col = mesh.color_attributes.get("Col")
    if not idx or col is None:
        return
    for poly in mesh.polygons:
        if poly.material_index in idx:
            for li in poly.loop_indices:
                c = col.data[li].color
                col.data[li].color = (c[0] * lin[0], c[1] * lin[1], c[2] * lin[2], c[3])


class Kit:
    """Builds every crew mesh once; hands out painted, placed copies."""

    def __init__(self):
        self.lib = bpy.data.collections.new("Crew_Meshes")
        bpy.context.scene.collection.children.link(self.lib)
        self.lib.hide_render = True
        self.lib.hide_viewport = True
        self.mesh = {}
        self.spec_of = {}
        for spec in crew_specs():
            a = core.Asset(spec.name, spec.folder, spec.desc)
            spec.build(a)
            obj = a.build(self.lib)
            for comp in (spec.crew_comp,) + spec.crew_also:
                self.mesh[comp] = obj.data
                self.spec_of[comp] = spec
            print("[sheet] %-28s tris %6d" % (spec.name, a.tri_count()), flush=True)
        self.painted = {}

    def data(self, comp, colour):
        base = self.mesh[comp]
        if colour is None:
            return base
        key = (base.name, tuple(round(x, 4) for x in colour))
        if key not in self.painted:
            m = base.copy()
            m.name = "%s_%s" % (base.name, "%02x%02x%02x" % tuple(int(x * 255) for x in colour))
            bake_paint(m, colour)
            self.painted[key] = m
        return self.painted[key]


def pose_comps(pose=None, expression="Neutral"):
    """cppactor components with a pose (joint FRotators) and a face expression applied."""
    cs = CH.comps()
    saved = {n: (c.loc, c.rot, c.scale) for n, c in cs.items()}
    for n, r in (pose or {}).items():
        cs[n].rot = CH.CA.R(*r)
    ez, ps, ms, mz, mr, bz, brl, brr = EXPRESSIONS[expression]
    for n in ("EyeL", "EyeR"):
        cs[n].scale = CH.CA.V(0.07, 0.08, 0.11 * ez)
    for n in ("PupilL", "PupilR"):
        cs[n].scale = CH.CA.V(0.03, 0.05 * ps, 0.07 * ps * ez)
    cs["Mouth"].scale = CH.CA.V(ms[0] / 100, ms[1] / 100, ms[2] / 100)
    cs["Mouth"].loc = CH.CA.V(21.5, 0, mz)
    cs["Mouth"].rot = CH.CA.R(0, 0, mr)
    cs["BrowL"].loc = CH.CA.V(20.5, -8.5, bz)
    cs["BrowR"].loc = CH.CA.V(20.5, 8.5, bz)
    cs["BrowL"].rot = CH.CA.R(0, 0, brl)
    cs["BrowR"].rot = CH.CA.R(0, 0, brr)
    mats = {n: c.matrix() for n, c in cs.items() if c.is_part()}
    for n, (l, r, s) in saved.items():
        cs[n].loc, cs[n].rot, cs[n].scale = l, r, s
    return mats


def assemble(kit, col, look, offset=(0, 0, 0), pose=None, expression="Neutral", fp=False, name="Crew"):
    mats = pose_comps(pose, expression)
    objs = []
    off = Matrix.Translation(Vector((offset[0], -offset[1], offset[2])))
    for comp, m in mats.items():
        if comp not in kit.mesh or (fp != comp.startswith("FP")) or (not fp and not visible(comp, look)):
            continue
        o = bpy.data.objects.new("%s_%s" % (name, comp), kit.data(comp, paint_of(comp, look)))
        o.matrix_world = off @ ue_matrix(m)
        col.objects.link(o)
        objs.append(o)
    return objs


def shot(objs, path, direction, lens=60, margin=1.1):
    render.only_visible(objs)
    render.frame(objs, direction=(direction[0], -direction[1], direction[2]), lens=lens, margin=margin)
    render.render(path)


def label_sheet(paths, labels, out, cols, title):
    ims = [Image.open(p).convert("RGB") for p in paths]
    w, h = ims[0].size
    rows = (len(ims) + cols - 1) // cols
    top = 46
    sheet = Image.new("RGB", (w * cols, h * rows + top), (24, 26, 40))
    d = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype(core.FONT_SIGN, 28)
        small = ImageFont.truetype(core.FONT_SIGN, 22)
    except OSError:
        font = small = ImageFont.load_default()
    d.text((14, 8), title, fill=(246, 231, 200), font=font)
    for i, (im, lab) in enumerate(zip(ims, labels)):
        x, y = (i % cols) * w, (i // cols) * h + top
        sheet.paste(im, (x, y))
        d.text((x + 12, y + 8), lab, fill=(20, 19, 31), font=small)
    sheet.save(out)
    for p in paths:
        os.remove(p)


def build(render_sheets=True, res=900, samples=32, want=(), save_blend=True):
    """Assemble the crew collections (and render the sheets); save SourceArt/Blender/Characters.blend."""
    def do(k):
        return render_sheets and (not want or k in want)
    export.reset_scene()
    render.setup(samples=samples, res=(res, res))
    kit = Kit()
    look = LOOKS[0]
    os.makedirs(OUT, exist_ok=True)
    root = bpy.data.collections.new("Crew")
    bpy.context.scene.collection.children.link(root)

    def group(name):
        c = bpy.data.collections.new(name)
        root.children.link(c)
        return c

    hero = assemble(kit, group("Crew_Base_Idle"), look, pose=IDLE, expression="Happy", name="Crew_Idle")
    if do("hero"):
        shot(hero, os.path.join(OUT, "Crew_Base_Hero.png"), (1, 0.62, 0.22), lens=70)

    rest = assemble(kit, group("Crew_Base_Rest"), look, offset=(0, 140, 0), name="Crew_Rest")
    if do("turn"):
        views = [("Front", (1, 0, 0.12)), ("Side", (0, 1, 0.12)), ("Back", (-1, 0, 0.12)), ("3/4", (1, 0.8, 0.3))]
        paths = []
        for i, (lab, d) in enumerate(views):
            p = os.path.join(OUT, "_turn%d.png" % i)
            shot(rest, p, d, lens=70, margin=1.05)
            paths.append(p)
        label_sheet(paths, [v[0] for v in views], os.path.join(OUT, "Crew_Base_Turnaround.png"), 4,
                    "Crew base character - rest pose as built by AFTCharacter::BuildBody")

    face_col = group("Crew_Expressions")
    paths = []
    for i, ex in enumerate(EXPRESSIONS):
        objs = assemble(kit, face_col, look, offset=(0, 280 + 70 * i, 0), expression=ex, name="Crew_%s" % ex)
        if do("face"):
            heads = [o for o in objs if any(k in o.name for k in ("Head", "Eye", "Pupil", "Brow", "Mouth", "Ear", "Cap", "Hair"))]
            p = os.path.join(OUT, "_face%d.png" % i)
            shot(heads, p, (1, 0.35, 0.12), lens=85, margin=0.95)
            paths.append(p)
    if paths:
        label_sheet(paths, list(EXPRESSIONS), os.path.join(OUT, "Crew_Base_Face.png"), 3, "Minimal face - the UpdateFace expression set")

    mod_col = group("Crew_Modules")
    variants = [("Base + short hair", dict(cap=False, glasses=False, phones=False, bib=False, hair=0)),
                ("Crew cap", dict(cap=True, glasses=False, phones=False, bib=False, hair=0)),
                ("Bun + glasses", dict(cap=False, glasses=True, phones=False, bib=False, hair=1)),
                ("Cap + headphones", dict(cap=True, glasses=False, phones=True, bib=False, hair=0)),
                ("Overall bib", dict(cap=True, glasses=False, phones=False, bib=True, hair=0))]
    paths = []
    for i, (lab, v) in enumerate(variants):
        lk = dict(look)
        lk.update(v)
        objs = assemble(kit, mod_col, lk, offset=(0, -200 - 90 * i, 0), pose=IDLE, name="Crew_Mod%d" % i)
        if do("modules"):
            p = os.path.join(OUT, "_mod%d.png" % i)
            shot(objs, p, (1, 0.55, 0.18), lens=70, margin=1.05)
            paths.append(p)
    if paths:
        label_sheet(paths, [v[0] for v in variants], os.path.join(OUT, "Crew_Base_Modules.png"), 5,
                    "One base, swappable modules (same mesh set the code toggles in SetCostumePartsVisible)")

    fp = assemble(kit, group("Crew_FirstPerson"), look, offset=(0, 900, 0), fp=True, name="Crew_FP")
    if do("fp") and fp:
        cam = bpy.context.scene.camera
        m = ue_matrix(CH.comps()["FirstPersonCamera"].matrix())
        # Blender cameras look down -Z with +Y up; the Unreal camera looks down +X with +Z up
        look_x = Matrix.Rotation(math.radians(-90), 4, 'Z') @ Matrix.Rotation(math.radians(90), 4, 'X')
        cam.matrix_world = Matrix.Translation(Vector((0, -900, 0))) @ m @ look_x
        cam.data.lens = 16
        render.only_visible(fp)
        ground = bpy.data.objects.get("ft_ground")
        if ground:
            ground.location = (0, -900, -200)
        render.render(os.path.join(OUT, "Crew_FirstPerson.png"))

    for o in bpy.context.scene.objects:
        o.hide_render = False
    if save_blend:
        import scenes
        scenes.setup_view((0, 150, 60), 1400.0, yaw=90.0, pitch=80.0)
        path = os.path.join(B.ART_ROOT, "Characters.blend")
        scenes.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--res", type=int, default=900)
    ap.add_argument("--samples", type=int, default=32)
    ap.add_argument("--no-blend", action="store_true")
    ap.add_argument("--no-render", action="store_true")
    ap.add_argument("--only", default="")
    args = ap.parse_args()
    build(not args.no_render, args.res, args.samples, [w for w in args.only.split(",") if w], not args.no_blend)


if __name__ == "__main__":
    main()
