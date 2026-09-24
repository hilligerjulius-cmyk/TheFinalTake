"""Build every The Final Take mesh in Blender, export FBX for Unreal, verify and document it.

Run (either works; the second is what blender-mcp / a Blender GUI session would execute):
    python3 Tools/blender/build_all.py [--only Lobby,SM_Prop_Plant] [--no-render] [--no-blend]
    blender -b --factory-startup --python Tools/blender/build_all.py -- [same options]

Outputs
    Content/TheFinalTake/Meshes/<Folder>/<SM_Name>.fbx        one FBX per asset (Unreal axes, cm, pivot = origin)
    SourceArt/Blender/<Category>.blend                        all assets of a category, assembled at their map positions
    SourceArt/Blender/asset_manifest.json                     what each asset replaces + every placement (Unreal world)
    SourceArt/Blender/Previews/...                            thumbnails and contact sheets
    SourceArt/Blender/fit_report.md                           bounds of each asset vs. the code-built geometry it replaces
"""
import argparse
import fnmatch
import importlib
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

from ftb import core, export, layout as L, registry, render  # noqa: E402

REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
MESH_ROOT = os.path.join(REPO, "Content", "TheFinalTake", "Meshes")
ART_ROOT = os.path.join(REPO, "SourceArt", "Blender")
MODULES = ["assets.shared", "assets.studio_exterior", "assets.studio_lobby", "assets.studio_rooms", "assets.studio_stage",
           "assets.studio_tank", "assets.studio_upper", "assets.studio_props", "assets.studio_stations",
           "assets.city_boulevard", "assets.city_buildings", "assets.city_cinema", "assets.dealership", "assets.vehicles", "assets.signs", "assets.shop_items", "assets.actor_parts"]


def parse():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", default="", help="comma separated name/folder patterns")
    ap.add_argument("--no-render", action="store_true")
    ap.add_argument("--no-export", action="store_true")
    ap.add_argument("--no-blend", action="store_true")
    ap.add_argument("--samples", type=int, default=20)
    ap.add_argument("--fit-only", action="store_true", help="rebuild meshes in memory only; refresh fit/placements in the manifest + fit report")
    ap.add_argument("--blend-only", action="store_true", help="rebuild meshes in memory and write the per-category .blend files")
    return ap.parse_args(argv)


def wanted(spec, pats):
    if not pats:
        return True
    return any(fnmatch.fnmatch(spec.name, p) or fnmatch.fnmatch(spec.folder, p) or p in spec.name or p in spec.folder for p in pats)


def fit_expect(spec, a):
    """Actors: local asset bounds vs. the constructor parts it replaces (replayed by layout/cppactor.py)."""
    e = spec.expect()
    exp = (Vector(e["min"]), Vector(e["max"]))
    lo, hi = a.bounds()
    dev = max(max(abs(exp[0][i] - lo[i]), abs(exp[1][i] - hi[i])) for i in range(3))
    ext = max(exp[1][i] - exp[0][i] for i in range(3))
    rec = {"dev_cm": round(dev, 1), "extent_cm": round(ext, 1), "rel": round(dev / max(ext, 1.0), 3),
           "expected": [[round(x, 1) for x in exp[0]], [round(x, 1) for x in exp[1]]],
           "actual": [[round(x, 1) for x in lo], [round(x, 1) for x in hi]], "primitives": e["parts"],
           "code_refs": getattr(spec, "code_parts", "")}
    rec["status"] = "ok" if (rec["dev_cm"] <= 10 or rec["rel"] <= 0.08) else ("close" if rec["rel"] <= 0.2 else "check")
    return rec


def save_blends(cats):
    """One .blend per category that holds only that category: a scene with the category collection, written with
    bpy.data.libraries.write so nothing else (other categories, preview rig) ends up in the file."""
    main = bpy.context.scene
    for cat, col in cats.items():
        path = os.path.join(ART_ROOT, cat + ".blend")
        sc = bpy.data.scenes.new("FT_" + cat)
        sc.unit_settings.system = main.unit_settings.system
        sc.unit_settings.scale_length = main.unit_settings.scale_length
        sc.unit_settings.length_unit = main.unit_settings.length_unit
        sc.world = main.world
        sc.collection.children.link(col)
        if os.path.exists(path):
            os.remove(path)
        bpy.data.libraries.write(path, {sc}, compress=True)
        sc.collection.children.unlink(col)
        bpy.data.scenes.remove(sc)
        print("[ftb] wrote %s" % os.path.relpath(path, REPO), flush=True)


def blend_only():
    """Rebuild every mesh in memory and write the per-category .blend files (manifest untouched)."""
    cats = {}
    for spec in registry.ASSETS:
        a = core.Asset(spec.name, spec.folder, spec.desc)
        spec.build(a)
        cat = spec.folder.split("/")[0]
        if cat not in cats:
            cats[cat] = bpy.data.collections.new(cat)
            bpy.context.scene.collection.children.link(cats[cat])
        obj = a.build(cats[cat])
        pls = spec.placements() if spec.placements else []
        if pls:
            obj.matrix_world = L.to_blender_matrix(pls[0]["loc"], pls[0]["rot"], pls[0]["scale"])
    save_blends(cats)


def placed_bounds(a, p):
    """Exact world bounds of the placed mesh (vertices, not a rotated box)."""
    m = core.xf(p["loc"], p["rot"], p["scale"])
    pts = [m @ v for v in a.v] or [Vector((0, 0, 0))]
    return (Vector([min(q[i] for q in pts) for i in range(3)]), Vector([max(q[i] for q in pts) for i in range(3)]))


def fit_only(pats):
    """Refresh fit, placements and code references of existing manifest entries without rendering/exporting."""
    manifest_path = os.path.join(ART_ROOT, "asset_manifest.json")
    with open(manifest_path) as f:
        data = json.load(f)
    manifest = {e["name"]: e for e in data["assets"]}
    for spec in registry.ASSETS:
        if not wanted(spec, pats) or spec.name not in manifest:
            continue
        a = core.Asset(spec.name, spec.folder, spec.desc)
        spec.build(a)
        placements = spec.placements() if spec.placements else []
        e = manifest[spec.name]
        e.update({"placements": placements, "hookup": getattr(spec, "hookup", ""), "code_parts": getattr(spec, "code_parts", ""),
                  "replaces": list(spec.replaces), "pivot": spec.pivot, "integration": spec.integration, "desc": spec.desc})
        f = fit(spec, a, placements)
        if f:
            if getattr(spec, "fit_note", ""):
                f["note"] = spec.fit_note
            e["fit"] = f
        else:
            e.pop("fit", None)
        print("[ftb] fit %-40s %s" % (spec.name, (f or {}).get("status", "-")), flush=True)
    ordered = [manifest[n] for n in sorted(manifest, key=lambda n: (manifest[n]["folder"], n))]
    data["assets"] = ordered
    with open(manifest_path, "w") as f:
        json.dump(data, f, indent=1)
    write_fit_report(ordered)


def fit(spec, a, placements):
    """Compare the placed asset bounds with the code primitives it replaces."""
    if spec.expect and spec.check:
        return fit_expect(spec, a)
    if not spec.covers or not spec.check or not placements:
        return None
    lo, hi = a.bounds()
    covers = spec.covers()
    pairs = []
    if spec.per_placement:
        pairs = [(p, recs) for p, recs in zip(placements, covers)]
    else:
        pairs = [(None, covers)]
    worst = None
    for p, recs in pairs:
        exp = L.aabb(recs)
        if not exp:
            continue
        if p is None:
            boxes = [placed_bounds(a, q) for q in placements]
            act = (Vector([min(b[0][i] for b in boxes) for i in range(3)]), Vector([max(b[1][i] for b in boxes) for i in range(3)]))
        else:
            act = placed_bounds(a, p)
        dev = max(max(abs(exp[0][i] - act[0][i]), abs(exp[1][i] - act[1][i])) for i in range(3))
        ext = max(exp[1][i] - exp[0][i] for i in range(3))
        rec = {"dev_cm": round(dev, 1), "extent_cm": round(ext, 1), "rel": round(dev / max(ext, 1.0), 3),
               "expected": [[round(x, 1) for x in exp[0]], [round(x, 1) for x in exp[1]]],
               "actual": [[round(x, 1) for x in act[0]], [round(x, 1) for x in act[1]]]}
        if worst is None or rec["rel"] > worst["rel"]:
            worst = rec
    if worst:
        worst["status"] = "ok" if (worst["dev_cm"] <= 10 or worst["rel"] <= 0.08) else ("close" if worst["rel"] <= 0.2 else "check")
        recs_all = [r for group in (covers if spec.per_placement else [covers]) for r in group]
        worst["code_refs"] = L.ref(recs_all)
        worst["primitives"] = len(recs_all)
    return worst


def main():
    args = parse()
    pats = [p.strip() for p in args.only.split(",") if p.strip()]
    for m in MODULES:
        try:
            importlib.import_module(m)
        except ModuleNotFoundError as e:
            if e.name != m:
                raise
    if args.fit_only:
        fit_only(pats)
        return
    if args.blend_only:
        export.reset_scene()
        blend_only()
        return
    export.reset_scene()
    render.setup(samples=args.samples, res=(384, 384))
    cats = {}
    manifest_path = os.path.join(ART_ROOT, "asset_manifest.json")
    manifest = {}
    if pats and os.path.exists(manifest_path):
        with open(manifest_path) as f:
            manifest = {e["name"]: e for e in json.load(f)["assets"]}
    t0 = time.time()
    built = []
    for spec in registry.ASSETS:
        if not wanted(spec, pats):
            continue
        a = core.Asset(spec.name, spec.folder, spec.desc)
        spec.build(a)
        cat = spec.folder.split("/")[0]
        if cat not in cats:
            cats[cat] = bpy.data.collections.new(cat)
            bpy.context.scene.collection.children.link(cats[cat])
        obj = a.build(cats[cat])
        placements = spec.placements() if spec.placements else []
        fbx_rel = os.path.join("Content", "TheFinalTake", "Meshes", spec.folder, spec.name + ".fbx")
        entry = {
            "name": spec.name,
            "fbx": fbx_rel.replace(os.sep, "/"),
            "unreal_path": "/Game/TheFinalTake/Meshes/%s/%s.%s" % (spec.folder, spec.name, spec.name),
            "folder": spec.folder,
            "desc": spec.desc,
            "replaces": list(spec.replaces),
            "pivot": spec.pivot,
            "integration": spec.integration,
            "tris": a.tri_count(),
            "bounds_cm": {"min": [round(x, 2) for x in a.bounds()[0]], "max": [round(x, 2) for x in a.bounds()[1]]},
            "materials": [m.name for m in obj.data.materials],
            "sockets": a.sockets,
            "meta": a.meta,
            "placements": placements,
            "tags": spec.tags,
            "hookup": getattr(spec, "hookup", ""),
            "code_parts": getattr(spec, "code_parts", ""),
        }
        f = fit(spec, a, placements)
        if f:
            if getattr(spec, "fit_note", ""):
                f["note"] = spec.fit_note
            entry["fit"] = f
        if not args.no_export:
            path = os.path.join(REPO, fbx_rel)
            export.export_fbx(obj, path)
            back = export.reimport_check(path)
            lo, hi = a.bounds()
            ok = bool(back) and all(abs(back[0]["min"][i] - (lo[i] if i != 1 else -hi[1])) < 0.05 and
                                    abs(back[0]["max"][i] - (hi[i] if i != 1 else -lo[1])) < 0.05 for i in range(3))
            entry["fbx_check"] = {"ok": ok, "tris": back[0]["tris"] if back else 0, "materials": back[0]["materials"] if back else []}
            entry["fbx_bytes"] = os.path.getsize(path)
        if not args.no_render:
            png = os.path.join(ART_ROOT, "Previews", spec.folder, spec.name + ".png")
            render.only_visible([obj])
            render.frame([obj], direction=(spec.view[0], -spec.view[1], spec.view[2]))
            render.render(png)
            entry["preview"] = os.path.relpath(png, REPO).replace(os.sep, "/")
        if placements:
            obj.matrix_world = L.to_blender_matrix(placements[0]["loc"], placements[0]["rot"], placements[0]["scale"])
        manifest[spec.name] = entry
        built.append(spec.name)
        status = entry.get("fit", {}).get("status", "-")
        print("[ftb] %-42s tris %6d  fit %-6s %s" % (spec.name, entry["tris"], status, entry.get("fbx_check", {}).get("ok", "")), flush=True)

    if not args.no_render:
        vehicle_previews(manifest)
    for o in bpy.context.scene.objects:
        o.hide_render = False
    if not args.no_blend and not pats:
        save_blends(cats)
    os.makedirs(ART_ROOT, exist_ok=True)
    ordered = [manifest[n] for n in sorted(manifest, key=lambda n: (manifest[n]["folder"], n))]
    with open(manifest_path, "w") as f:
        json.dump({"generator": "Tools/blender/build_all.py", "units": "cm", "space": "Unreal (X forward, Y right, Z up)",
                   "assets": ordered}, f, indent=1)
    write_fit_report(ordered)
    if not args.no_render:
        sheets(ordered)
    print("[ftb] built %d assets in %.1fs" % (len(built), time.time() - t0))


def vehicle_previews(manifest):
    """Re-render car bodies with their wheels mounted at the Wheel_* sockets."""
    objs = {o.name: o for o in bpy.data.objects}
    for spec in registry.ASSETS:
        tag = next((t for t in spec.tags if t.startswith("wheels:")), None)
        if not tag or spec.name not in objs or spec.name not in manifest:
            continue
        names = tag.split(":", 1)[1].split("|")
        if any(n not in objs for n in names):
            continue
        body = objs[spec.name]
        saved = body.matrix_world.copy()
        from mathutils import Matrix
        body.matrix_world = Matrix.Identity(4)
        extra = []
        for sname, s in manifest[spec.name]["sockets"].items():
            if not sname.startswith("Wheel_"):
                continue
            src = objs[names[0] if (len(names) == 1 or sname[6] == "F") else names[1]]
            o = src.copy()
            bpy.context.scene.collection.objects.link(o)
            o.matrix_world = L.to_blender_matrix(s["location"], s["rotation"])
            extra.append(o)
        png = os.path.join(REPO, manifest[spec.name]["preview"])
        render.only_visible([body] + extra)
        render.frame([body] + extra, direction=(spec.view[0], -spec.view[1], spec.view[2]))
        render.render(png)
        for o in extra:
            bpy.data.objects.remove(o)
        body.matrix_world = saved


def write_fit_report(entries):
    lines = ["# Fit report: Blender assets vs. code-built geometry", "",
             "Generated by `Tools/blender/build_all.py`. *Expected* = world bounding box of the primitives the asset replaces",
             "(from `SourceArt/Blender/layout/shell_layout.json`, i.e. the original C++ builders); *actual* = the asset's bounds at its",
             "placement(s). Actors: *expected* = the constructor parts the mesh replaces, replayed from the C++ by",
             "`Tools/blender/layout/cppactor.py`, in the mesh's local space. Status: ok (<= 10 cm or <= 8 % of the extent),",
             "close (<= 20 %), check (review the silhouette). *Note* explains deliberate deviations.", "",
             "| Asset | Replaces (code) | Parts | Max deviation | Status | Note |", "|---|---|---|---|---|---|"]
    for e in entries:
        f = e.get("fit")
        if not f:
            continue
        lines.append("| %s | %s | %d | %.1f cm (%.0f %%) | %s | %s |" % (e["name"], f["code_refs"], f["primitives"], f["dev_cm"], f["rel"] * 100,
                                                                        f["status"], f.get("note", "")))
    with open(os.path.join(ART_ROOT, "fit_report.md"), "w") as fh:
        fh.write("\n".join(lines) + "\n")


def sheets(entries):
    by_cat = {}
    for e in entries:
        if e.get("preview"):
            key = "/".join(e["folder"].split("/")[:2])
            by_cat.setdefault(key, []).append((os.path.join(REPO, e["preview"]), e["name"].replace("SM_", "")))
    for cat, items in by_cat.items():
        render.contact_sheet(items, os.path.join(ART_ROOT, "Previews", "Sheet_%s.png" % cat.replace("/", "_")), cols=6, cell=256, title="The Final Take - %s" % cat)


if __name__ == "__main__":
    main()
