"""Assembled scenes for looking at the result: .blend files that open ready to view in Blender, and GLB files for
the browser viewer.

    python3 Tools/blender/scenes.py            # SourceArt/Blender/{Studio,City,Catalog}.blend
    python3 Tools/blender/scenes.py --web      # + SourceArt/Blender/Web/{Studio,City}.glb for a browser viewer (not committed)

- Studio.blend / City.blend: every mesh at every level placement (instances share their mesh), the cars on the
  Dream Cars turntables, runtime-only items (shop items, film reel) on a showcase row outside the building.
  Ceilings/roofs sit in their own collection that starts hidden, so the viewport looks straight into the rooms.
- Catalog.blend: each of the meshes once, in a grid, one collection per folder.
The 3D viewports are set up for the cm scale (clip distances, framing, solid shading with vertex colours).
"""
import importlib
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402
from mathutils import Matrix, Quaternion, Vector  # noqa: E402

import build_all as B  # noqa: E402
from ftb import core, export, layout as L, registry  # noqa: E402

ART = B.ART_ROOT
WEB = os.path.join(ART, "Web")
FLIP = Matrix.Diagonal((1, -1, 1, 1))
CITY_X = -3850.0          # placements west of this (Unreal X) belong to the city
OVERHEAD = ("Ceiling", "SM_Cinema_Shell")   # hidden at start so the interiors show


def area_of(loc):
    return "City" if loc[0] < CITY_X else "Studio"


def is_overhead(name):
    return any(k in name for k in OVERHEAD)


def collection(name, parent=None):
    c = bpy.data.collections.new(name)
    (parent or bpy.context.scene.collection).children.link(c)
    return c


def build_mesh(spec, col):
    a = core.Asset(spec.name, spec.folder, spec.desc)
    spec.build(a)
    return a, a.build(col)


def place(obj, col, p, first):
    o = obj if first else obj.copy()
    if not first:
        col.objects.link(o)
    o.matrix_world = L.to_blender_matrix(p["loc"], p["rot"], p["scale"])
    return o


def mount_cars(built, col):
    """The four cars on the Dream Cars turntables, wheels at their sockets."""
    table = built.get("SM_Dealer_DisplayTurntable")
    if not table:
        return
    a_t, _, spec_t = table
    sock = a_t.sockets.get("Car", {"location": [0, 0, 12], "rotation": [0, 0, 0]})
    cars = [s for s in registry.ASSETS if any(t.startswith("wheels:") for t in s.tags)]
    for spec, p in zip(cars, spec_t.placements()):
        a, body, _ = built[spec.name]
        base = core.xf(p["loc"], (0, 30, 0)) @ core.xf(sock["location"], sock["rotation"])
        body.matrix_world = FLIP @ base @ FLIP
        names = next(t for t in spec.tags if t.startswith("wheels:")).split(":", 1)[1].split("|")
        for sname, s in a.sockets.items():
            if sname.startswith("Wheel_"):
                src = built[names[0] if (len(names) == 1 or sname[6] == "F") else names[1]][1]
                w = src.copy()
                col.objects.link(w)
                w.matrix_world = FLIP @ base @ core.xf(s["location"], s["rotation"]) @ FLIP
    # the wheel templates themselves are not needed (the mounted copies share their meshes)
    for name, (_, obj, spec) in built.items():
        if spec.folder == "Vehicles" and "Wheel" in name:
            bpy.data.objects.remove(obj)


def setup_view(center_ue, radius, yaw=-35.0, pitch=58.0):
    """Every 3D viewport in the file: cm-friendly clipping, framed on the content, solid + vertex colours."""
    c = Vector((center_ue[0], -center_ue[1], center_ue[2]))
    rot = (Quaternion((0, 0, 1), math.radians(yaw)) @ Quaternion((1, 0, 0), math.radians(pitch)))
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type != 'VIEW_3D':
                continue
            sp = area.spaces.active
            sp.clip_start = 2.0
            sp.clip_end = 400000.0
            sp.shading.type = 'SOLID'
            sp.shading.color_type = 'VERTEX'
            sp.shading.light = 'STUDIO'
            sp.overlay.show_floor = False
            r3d = sp.region_3d
            r3d.view_perspective = 'PERSP'
            r3d.view_location = c
            r3d.view_distance = radius
            r3d.view_rotation = rot
    s = bpy.context.scene
    cam_data = bpy.data.cameras.new("Overview")
    cam_data.clip_start, cam_data.clip_end, cam_data.lens = 5.0, 400000.0, 24
    cam = bpy.data.objects.new("Overview", cam_data)
    s.collection.objects.link(cam)
    cam.location = c + rot @ Vector((0, 0, radius))
    cam.rotation_mode = 'QUATERNION'
    cam.rotation_quaternion = rot
    s.camera = cam


def area_scene(area):
    export.reset_scene()
    root = collection(area)
    over = collection(area + " - Decken und Dächer (ausgeblendet)")
    show = collection(area + " - Laufzeit-Objekte (Showcase)")
    folders = {}
    built = {}
    shelf = []
    for spec in registry.ASSETS:
        pls = spec.placements() if spec.placements else []
        mine = [p for p in pls if area_of(p["loc"]) == area]
        runtime = not pls and spec.folder.startswith("Studio") and area == "Studio"
        car = any(t.startswith("wheels:") for t in spec.tags) or spec.folder == "Vehicles"
        if not mine and not runtime and not (car and area == "City"):
            continue
        if is_overhead(spec.name):
            col = over
        elif runtime:
            col = show
        else:
            key = spec.folder
            if key not in folders:
                folders[key] = collection(key, root)
            col = folders[key]
        a, obj = build_mesh(spec, col)
        built[spec.name] = (a, obj, spec)
        if runtime:
            shelf.append((obj, a))
            continue
        for i, p in enumerate(mine):
            place(obj, col, p, i == 0)
    if area == "City":
        mount_cars(built, folders.get("Vehicles") or root)
    # showcase row south of the studio building (Unreal -Y), one metre apart
    x = -1800.0
    for obj, a in shelf:
        lo, hi = a.bounds()
        w = max(hi.x - lo.x, 40.0)
        obj.matrix_world = L.to_blender_matrix((x - lo.x, -2500.0 - (lo.y + hi.y) / 2, -lo.z))
        x += w + 60.0
    vl = bpy.context.view_layer
    for lc in vl.layer_collection.children:
        if lc.collection is over:
            lc.hide_viewport = True
            over.hide_render = True
    if area == "Studio":
        setup_view((-600, 0, 0), 7500.0)
    else:
        setup_view((-9300, 0, 0), 12500.0, yaw=-60.0, pitch=55.0)
    return over


def catalog_scene():
    export.reset_scene()
    root = collection("Catalog")
    by_folder = {}
    for spec in registry.ASSETS:
        by_folder.setdefault(spec.folder, []).append(spec)
    y = 0.0
    for folder in sorted(by_folder):
        col = collection(folder, root)
        x, row_h = 0.0, 0.0
        for spec in sorted(by_folder[folder], key=lambda s: s.name):
            a, obj = build_mesh(spec, col)
            lo, hi = a.bounds()
            w, d = hi.x - lo.x, hi.y - lo.y
            s = 3000.0 / max(w, d) if max(w, d) > 3000 else 1.0   # whole buildings/streets: shrink to fit the grid
            if x > 0 and x + w * s > 12000:
                x, y = 0.0, y + row_h + 150.0
                row_h = 0.0
            obj.matrix_world = L.to_blender_matrix((x - lo.x * s, y - lo.y * s, -lo.z * s), (0, 0, 0), (s, s, s))
            x += w * s + 80.0
            row_h = max(row_h, d * s)
        y += row_h + 400.0
    setup_view((6000, y / 2, 0), max(y, 12000.0) * 1.1, yaw=-20.0, pitch=50.0)


def save(path):
    if os.path.exists(path):
        os.remove(path)
    bpy.ops.wm.save_as_mainfile(filepath=path, compress=True)
    print("[scenes] wrote %s (%.1f MB)" % (os.path.relpath(path, B.REPO), os.path.getsize(path) / 1e6), flush=True)


def export_glb(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_visible=False, export_yup=True,
                              export_vertex_color='ACTIVE', export_all_vertex_colors=False, export_normals=True,
                              export_materials='EXPORT', export_cameras=False, export_lights=False, export_apply=False)
    print("[scenes] wrote %s (%.1f MB)" % (os.path.relpath(path, B.REPO), os.path.getsize(path) / 1e6), flush=True)


def main(web=None):
    for m in B.MODULES:
        importlib.import_module(m)
    if web is None:
        web = "--web" in sys.argv
    for area in ("Studio", "City"):
        area_scene(area)
        save(os.path.join(ART, area + ".blend"))
        if web:
            export_glb(os.path.join(WEB, area + ".glb"))
    catalog_scene()
    save(os.path.join(ART, "Catalog.blend"))


if __name__ == "__main__":
    main()
