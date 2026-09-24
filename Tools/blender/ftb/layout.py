"""Access to the shell layout dumped by layout/extract_layout.py (the code-built map).

Records are Unreal-space: prim {group, center, size, rot, color(linear), emissive, gloss},
text {text, loc, yaw, pitch, size, color}, point/spot lights. Each carries the source file/line
of its Add()/Text() call and `ctx`, the stack of helper calls (Plant, Crate, Building, ...).
"""
import json
import math
import os

from mathutils import Matrix, Vector

from .core import ue_rot, xf

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
LAYOUT_PATH = os.path.join(REPO, "SourceArt", "Blender", "layout", "shell_layout.json")

SHAPES = ["Cube", "Cube", "Box", "Box", "Cylinder", "Cylinder", "Sphere", "Ball", "Cone", "Prism",
          "Ramp", "Torus", "Capsule", "Cube", "Cube", "Shoreline"]
GROUP = {n: i for i, n in enumerate(["CubeSolid", "CubeDeco", "BoxSolid", "BoxDeco", "CylSolid", "CylDeco",
                                      "SphereDeco", "BallDeco", "ConeDeco", "PrismDeco", "RampSolid", "TorusDeco",
                                      "CapsuleDeco", "GlassSolid", "Blocker", "ShorelineDeco"])}
SOLID_GROUPS = {0, 2, 4, 10, 13, 14}

_DATA = None


# Sign boards whose size in the C++ is rotated 90 degrees against their own TextRender (the text sits in front of
# the board on the axis it faces, but the board is thin on the other axis). The meshes follow the intended board;
# the C++ is unchanged - see BLENDER_ASSETS.md, "Befunde im bestehenden Code".
BOARD_FIXES = {("FTStudioShell.cpp", 585): "RECEPTION board", ("FTStudioShell.cpp", 977): "PROJECTION hint board",
               ("FTCity.cpp", 302): "DREAM CARS board", ("FTCity.cpp", 328): "parking 'P' sign"}


def data():
    global _DATA
    if _DATA is None:
        with open(LAYOUT_PATH) as f:
            _DATA = json.load(f)
        for area in ("studio", "city"):
            for r in _DATA.get(area, []):
                if r["kind"] == "prim" and (os.path.basename(r["file"]), r["line"]) in BOARD_FIXES:
                    r["size_code"] = list(r["size"])
                    r["size"] = [r["size"][1], r["size"][0], r["size"][2]]
                    r["board_fix"] = BOARD_FIXES[(os.path.basename(r["file"]), r["line"])]
    return _DATA


def records(area):
    return data()[area]


AREA_FILE = {"studio": "FTStudioShell.cpp", "city": "FTCity.cpp"}


def select(area="studio", kind="prim", lines=None, build_lines=None, kit=None, where=None, include_blockers=False):
    """Filter records. lines=(a, b) matches the Add() line OR any enclosing kit call line - only lines of
    the area's own source file count (helpers from FTStudioShell.cpp called by the city share numbers)."""
    own = AREA_FILE[area]
    out = []
    for r in records(area):
        if kind and r["kind"] != kind:
            continue
        if kind == "prim" and not include_blockers and r["group"] == GROUP["Blocker"]:
            continue
        if lines:
            ranges = lines if isinstance(lines[0], (list, tuple)) else [lines]
            call_lines = ([r["line"]] if r["file"].endswith(own) else []) + [c["line"] for c in r["ctx"] if c["file"].endswith(own)]
            if not any(a <= l <= b for (a, b) in ranges for l in call_lines):
                continue
        if kit and not any(c["kit"] == kit for c in r["ctx"]):
            continue
        if where and not where(r):
            continue
        out.append(r)
    return out


def kit_calls(area, kit, lines=None):
    """Unique invocations of a helper (e.g. every Plant()) with their records."""
    calls = {}
    for r in records(area):
        for c in r["ctx"]:
            if c["kit"] != kit:
                continue
            if lines and not (lines[0] <= c["line"] <= lines[1]):
                continue
            key = (c["file"], c["line"], json.dumps(c["args"]))
            calls.setdefault(key, {"call": c, "records": []})["records"].append(r)
            break
    return list(calls.values())


def prim_corners(r):
    """World corners of a primitive's bounding box (unit shapes are 100 cm, centred)."""
    size = Vector(r["size"])
    rot = ue_rot(*r["rot"])
    c = Vector(r["center"])
    pts = []
    for sx in (-0.5, 0.5):
        for sy in (-0.5, 0.5):
            for sz in (-0.5, 0.5):
                pts.append(c + rot @ Vector((sx * size.x, sy * size.y, sz * size.z)))
    return pts


def prim_bounds(r):
    """Shape-aware world AABB of a primitive record (spheres/cylinders/capsules/tori analytically)."""
    import os as _os
    import sys as _sys
    _sys.path.insert(0, _os.path.join(_os.path.dirname(HERE), "layout"))
    import cppactor as _ca
    m = xf(r["center"], r["rot"], [s / 100.0 for s in r["size"]])
    return _ca.shape_bounds(SHAPES[r["group"]] if "group" in r else "Box", [[m[i][j] for j in range(4)] for i in range(3)])


def aabb(recs):
    pts = []
    for r in recs:
        if r["kind"] == "prim":
            lo, hi = prim_bounds(r)
            pts += [Vector(lo), Vector(hi)]
    if not pts:
        return None
    return (Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts))),
            Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))))


def placed_aabb(local_min, local_max, loc, rot=(0, 0, 0), scale=(1, 1, 1)):
    m = xf(loc, rot, scale)
    pts = []
    for x in (local_min.x, local_max.x):
        for y in (local_min.y, local_max.y):
            for z in (local_min.z, local_max.z):
                pts.append(m @ Vector((x, y, z)))
    return (Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts))),
            Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))))


def lin_to_srgb(c):
    return tuple((x * 12.92 if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055) for x in c[:3])


def ref(recs, area="studio"):
    """Human readable 'File.cpp:line-line' ranges for a set of records (Add lines + kit call lines)."""
    by_file = {}
    for r in recs:
        top = r["ctx"][0] if r["ctx"] else None
        f, l = (top["file"], top["line"]) if top else (r["file"], r["line"])
        by_file.setdefault(os.path.basename(f), set()).add(l)
    parts = []
    for f, ls in sorted(by_file.items()):
        ls = sorted(ls)
        spans = []
        a = b = ls[0]
        for l in ls[1:]:
            if l <= b + 2:
                b = l
            else:
                spans.append((a, b))
                a = b = l
        spans.append((a, b))
        parts.append(f + ":" + ",".join(str(a) if a == b else "%d-%d" % (a, b) for a, b in spans))
    return "; ".join(parts)


# ------------------------------------------------------------------------------ blockout

def build_blockout(area, collection, where=None):
    """Recreate the current primitive look (for side-by-side comparison renders)."""
    import bmesh
    import bpy
    from . import core
    unit = {}

    def unit_mesh(shape):
        if shape in unit:
            return unit[shape]
        if shape in ("Cube", "Box"):
            bm = core.bm_box(100, 100, 100, 7 if shape == "Box" else 0, 1)
        elif shape == "Cylinder":
            bm = core.bm_cyl(50, 100, 10)
        elif shape == "Cone":
            bm = core.bm_cyl(50, 100, 10, r_top=0.001)
        elif shape in ("Sphere", "Ball"):
            bm = core.bm_ico(50, 1 if shape == "Sphere" else 2)
        elif shape in ("Prism", "Ramp"):
            bm = core.bm_prism([(-50, -50), (50, -50), (-50 if shape == "Ramp" else 0, 50)], 100)
        elif shape == "Torus":
            bm = core.bm_torus(35, 15, 14, 7)
        elif shape == "Capsule":
            prof = [(25 * math.cos(a), -25 + 25 * math.sin(a)) for a in [-math.pi / 2 + (math.pi / 2) * i / 3 for i in range(4)]]
            prof += [(25 * math.cos(a), 25 + 25 * math.sin(a)) for a in [(math.pi / 2) * i / 3 for i in range(4)]]
            bm = core.bm_lathe(prof, 10)
        else:  # shoreline approximated by a thin slab
            bm = core.bm_box(100, 100, 10, 0, 1)
        # Blender space for the mesh itself: mirror Y + flip winding
        for v in bm.verts:
            v.co.y = -v.co.y
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
        me = bpy.data.meshes.new("blk_" + shape)
        bm.to_mesh(me)
        bm.free()
        me.materials.append(blockout_material())
        unit[shape] = me
        return me

    flip = Matrix.Diagonal((1, -1, 1, 1))
    objs = []
    for r in records(area):
        if r["kind"] != "prim" or r["group"] == GROUP["Blocker"]:
            continue
        if where and not where(r):
            continue
        shape = SHAPES[r["group"]]
        o = bpy.data.objects.new("blk", unit_mesh(shape))
        m = xf(r["center"], r["rot"], [s / 100.0 for s in r.get("size_code", r["size"])])   # as the C++ builds it
        o.matrix_world = flip @ m @ flip
        col = lin_to_srgb(r["color"])
        o.color = (*[srgb_to_lin(x) for x in col], 1.0)
        o["emissive"] = r["emissive"]
        collection.objects.link(o)
        objs.append(o)
    return objs


def srgb_to_lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def blockout_material():
    import bpy
    m = bpy.data.materials.get("ft_blockout")
    if m:
        return m
    m = bpy.data.materials.new("ft_blockout")
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    info = nt.nodes.new("ShaderNodeObjectInfo")
    attr = nt.nodes.new("ShaderNodeAttribute")
    attr.attribute_type = 'OBJECT'
    attr.attribute_name = "emissive"
    nt.links.new(info.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(info.outputs["Color"], bsdf.inputs["Emission Color"])
    mul = nt.nodes.new("ShaderNodeMath")
    mul.operation = 'MULTIPLY'
    mul.inputs[1].default_value = 0.6
    nt.links.new(attr.outputs["Fac"], mul.inputs[0])
    nt.links.new(mul.outputs[0], bsdf.inputs["Emission Strength"])
    bsdf.inputs["Roughness"].default_value = 0.8
    return m


def to_blender_matrix(loc, rot=(0, 0, 0), scale=(1, 1, 1)):
    """Placement (Unreal space) -> Blender object matrix for an asset built by Asset.build()."""
    flip = Matrix.Diagonal((1, -1, 1, 1))
    return flip @ xf(loc, rot, scale) @ flip
