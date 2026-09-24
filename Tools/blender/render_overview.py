"""Overview renders: the code-built layout (blockout of the shell primitives + the actors' constructor parts, replayed
from the C++) next to the Blender assets at their manifest placements. Ceilings are hidden so the interiors show.

    python3 Tools/blender/render_overview.py [--samples 16]
Writes SourceArt/Blender/Previews/Overview/<view>.png (left: code, right: Blender assets).
"""
import importlib
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "layout"))

import bpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import build_all as B  # noqa: E402
import cppactor as CA  # noqa: E402
from ftb import core, export, layout as L, registry, render  # noqa: E402

OUT = os.path.join(B.ART_ROOT, "Previews", "Overview")
def overhead(r):
    """Ceilings/roof slabs (large, thin, high) - hidden so the interiors show from above."""
    sx, sy, sz = r["size"]
    return sz <= 60 and sx >= 400 and sy >= 400 and r["center"][2] >= 450
FLIP = Matrix.Diagonal((1, -1, 1, 1))

# name: (kind, UE target/centre, UE camera position or ortho width, resolution)
VIEWS = {
    "Studio_Plan": ("ortho", (-620, 60, 0), 6600, (1600, 1100)),
    "Studio_Stage4": ("persp", (1500, 150, -60), (-250, -1450, 900), (1280, 720)),
    "Studio_Street": ("persp", (-2000, 0, 250), (-3500, -1900, 650), (1280, 720)),
    "City_Plan": ("ortho", (-9300, -100, 0), 12600, (1800, 760)),
    "City_Boulevard": ("persp", (-11200, -150, 650), (-5400, -420, 330), (1280, 720)),
    "City_Dealership": ("persp", (-4850, 1000, 150), (-4300, -300, 700), (1280, 720)),
    "Cinema_Hall": ("persp", (-14500, -150, 450), (-12900, 850, 1000), (1280, 720)),
}


def ue_to_bl(p):
    return Vector((p[0], -p[1], p[2]))


def camera_for(view):
    kind, target, where, res = VIEWS[view]
    s = bpy.context.scene
    cam = s.camera
    s.render.resolution_x, s.render.resolution_y = res
    cam.data.clip_start = 5.0
    cam.data.clip_end = 1.0e6
    t = ue_to_bl(target)
    if kind == "ortho":
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = where
        cam.location = Vector((t.x, t.y, 6000.0))
        cam.rotation_euler = (0.0, 0.0, 0.0)
    else:
        cam.data.type = 'PERSP'
        cam.data.lens = 24
        p = ue_to_bl(where)
        cam.location = p
        d = (t - p).normalized()
        cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()


# ------------------------------------------------------------------------------------------- code blockout

_UNIT = {}


def _unit_mesh(shape):
    if shape in _UNIT:
        return _UNIT[shape]
    import bmesh
    if shape in ("Cube", "Box", "Plane", "WaterGrid", "Shoreline", "CrewTorso"):
        bm = core.bm_box(100, 100, 100 if shape in ("Cube", "Box") else 1, 0, 1)
    elif shape == "Cylinder":
        bm = core.bm_cyl(50, 100, 10)
    elif shape == "Cone":
        bm = core.bm_cyl(50, 100, 10, r_top=0.001)
    elif shape in ("Sphere", "Ball"):
        bm = core.bm_ico(50, 2)
    elif shape in ("Prism", "Ramp"):
        bm = core.bm_prism([(-50, -50), (50, -50), (-50 if shape == "Ramp" else 0, 50)], 100)
    elif shape == "Torus":
        bm = core.bm_torus(35, 15, 14, 7)
    else:  # Capsule
        prof = [(25 * math.cos(a), -25 + 25 * math.sin(a)) for a in [-math.pi / 2 + (math.pi / 2) * i / 3 for i in range(4)]]
        prof += [(25 * math.cos(a), 25 + 25 * math.sin(a)) for a in [(math.pi / 2) * i / 3 for i in range(4)]]
        bm = core.bm_lathe(prof, 10)
    for v in bm.verts:
        v.co.y = -v.co.y
    bmesh.ops.reverse_faces(bm, faces=bm.faces)
    me = bpy.data.meshes.new("blk_" + shape)
    bm.to_mesh(me)
    bm.free()
    me.materials.append(L.blockout_material())
    _UNIT[shape] = me
    return me


def actor_blockout(collection):
    """Every placed actor's visible constructor parts (grey), replayed from the C++."""
    n = 0
    for sp in CA.spawns():
        cls = sp["cls"]
        if not cls.startswith("AFT") or cls in ("AFTStudioShell", "AFTCityShell", "AFTAmbientRain", "AFTZone") or sp["loc"] is None:
            continue
        it = CA.Interp(CA.source())
        try:
            _, comps = it.run_actor(cls, sp["settings"], construct=("OnConstruction", "BeginPlay"))
        except Exception as e:  # a runtime-only BeginPlay path: fall back to the constructor state
            print("[overview] %s: %s" % (cls, e))
            _, comps = CA.Interp(CA.source()).run_actor(cls, sp["settings"])
        world = CA.mat_xf(sp["loc"].t(), (0, sp["yaw"] or 0.0, 0))
        for c in comps:
            if not c.is_part() or not c.visible or c.size is None or getattr(c, "unknown", False) or c.shape in ("WaterGrid",) \
                    or "Beam" in c.name or c.name == "Cone":   # light cones are VFX, not geometry
                continue
            m = CA.mat_mul(world, c.matrix())
            o = bpy.data.objects.new("act", _unit_mesh(c.shape))
            o.matrix_world = FLIP @ Matrix([row[:] for row in m]) @ FLIP
            o.color = (0.55, 0.57, 0.62, 1.0)
            o["emissive"] = 0.0
            collection.objects.link(o)
            n += 1
    return n


# ------------------------------------------------------------------------------------------- assets

def place_assets(collection):
    objs = {}
    for spec in registry.ASSETS:
        a = core.Asset(spec.name, spec.folder, spec.desc)
        spec.build(a)
        obj = a.build(collection)
        objs[spec.name] = (obj, a, spec)
        pls = spec.placements() if spec.placements else []
        if not pls or "Ceiling" in spec.name:
            obj.hide_render = True
            continue
        for i, p in enumerate(pls):
            o = obj if i == 0 else obj.copy()
            if i:
                collection.objects.link(o)
            o.matrix_world = L.to_blender_matrix(p["loc"], p["rot"], p["scale"])
    # the four cars on the dealership turntables (socket 'Car'), wheels at their Wheel_* sockets
    cars = [s for s in registry.ASSETS if any(t.startswith("wheels:") for t in s.tags)]
    tables = objs.get("SM_Dealer_DisplayTurntable")
    if tables:
        table_spec = tables[2]
        car_socket = tables[1].sockets.get("Car", {"location": [0, 0, 12], "rotation": [0, 0, 0]})
        for spec, p in zip(cars, table_spec.placements()):
            body, a, _ = objs[spec.name]
            base = core.xf(p["loc"], (0, 30, 0)) @ core.xf(car_socket["location"], car_socket["rotation"])
            body.hide_render = False
            body.matrix_world = FLIP @ base @ FLIP
            names = next(t for t in spec.tags if t.startswith("wheels:")).split(":", 1)[1].split("|")
            for sname, s in a.sockets.items():
                if not sname.startswith("Wheel_"):
                    continue
                src = objs[names[0] if (len(names) == 1 or sname[6] == "F") else names[1]][0]
                w = src.copy()
                collection.objects.link(w)
                w.hide_render = False
                w.matrix_world = FLIP @ base @ core.xf(s["location"], s["rotation"]) @ FLIP


def main():
    samples = 16
    if "--samples" in sys.argv:
        samples = int(sys.argv[sys.argv.index("--samples") + 1])
    for m in B.MODULES:
        importlib.import_module(m)
    export.reset_scene()
    render.setup(samples=samples, res=(1280, 720))
    ground = bpy.data.objects.get("ft_ground")
    if ground:
        ground.location = (-6000, 0, -125)
        ground.scale = (40000, 40000, 1)
    code = bpy.data.collections.new("code_blockout")
    new = bpy.data.collections.new("blender_assets")
    for c in (code, new):
        bpy.context.scene.collection.children.link(c)
    L.build_blockout("studio", code, where=lambda r: not overhead(r))
    L.build_blockout("city", code)
    print("[overview] actor parts:", actor_blockout(code))
    place_assets(new)
    # the hall is a closed room: a few warm lights so both versions can be seen from inside
    for i, (x, y) in enumerate(((-13200, -800), (-13200, 500), (-14200, -150), (-12900, -150))):
        ld = bpy.data.lights.new("hall_%d" % i, 'POINT')
        ld.energy = 6.0e7
        ld.color = (1.0, 0.85, 0.7)
        ld.shadow_soft_size = 60
        lo = bpy.data.objects.new("hall_%d" % i, ld)
        lo.location = ue_to_bl((x, y, 1100))
        bpy.context.scene.collection.objects.link(lo)
    views = [v for v in VIEWS if "--views" not in sys.argv or v in sys.argv[sys.argv.index("--views") + 1].split(",")]
    os.makedirs(OUT, exist_ok=True)
    from PIL import Image, ImageDraw
    for view in views:
        camera_for(view)
        shots = []
        for label, show in (("code", code), ("blender", new)):
            for c in (code, new):
                c.hide_render = c is not show
            path = os.path.join(OUT, "%s_%s.png" % (view, label))
            render.render(path)
            shots.append((label, path))
        ims = [Image.open(p).convert("RGB") for _, p in shots]
        w, h = ims[0].size
        sheet = Image.new("RGB", (w * 2 + 12, h + 44), (27, 33, 64))
        d = ImageDraw.Draw(sheet)
        for k, (im, (label, _)) in enumerate(zip(ims, shots)):
            sheet.paste(im, (k * (w + 12), 44))
            d.text((k * (w + 12) + 12, 12), "%s - %s" % (view, "C++ blockout (current)" if label == "code" else "Blender assets"), fill=(255, 214, 90))
        sheet.save(os.path.join(OUT, "%s.png" % view))
        for _, p in shots:
            os.remove(p)
        print("[overview] %s" % view, flush=True)


if __name__ == "__main__":
    main()
