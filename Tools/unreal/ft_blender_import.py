"""Import the Blender asset library into Unreal (Editor Python).  UNTESTED - written without an Unreal session.

Run inside the Unreal Editor (Python plugin enabled), e.g. from the Output Log's Python console:

    import sys; sys.path.append(r"<project>/Tools/unreal")
    import ft_blender_import as ftb
    ftb.create_materials()            # M_FT_Vertex / VertexGlow / VertexGlass / CarBody / CarTrim / CrewPaint / BeamGlow
    ftb.import_all()                  # every FBX listed in SourceArt/Blender/asset_manifest.json
    ftb.import_all(only="Vehicles")   # ... or a subset (substring of name or folder)
    ftb.build_preview_level()         # optional: new level with every mesh at its manifest placement

Nothing here touches gameplay code or the existing map: the preview level is a separate asset
(/Game/TheFinalTake/Maps/L_BlenderAssetPreview) for comparing the new meshes with the code-built layout.
"""
import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MANIFEST = os.path.join(PROJECT, "SourceArt", "Blender", "asset_manifest.json")
MAT_DIR = "/Game/TheFinalTake/Materials/Blender"
PREVIEW_LEVEL = "/Game/TheFinalTake/Maps/L_BlenderAssetPreview"

MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def manifest():
    with open(MANIFEST, encoding="utf-8") as f:
        return json.load(f)["assets"]


# ------------------------------------------------------------------------------------------- materials

def _new_material(name):
    path = "%s/%s" % (MAT_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path), False
    mat = TOOLS.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    return mat, True


def _vertex_colour(mat):
    """Vertex colour node; RGB optionally linearised (the FBX stores sRGB bytes)."""
    vc = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -900, 0)
    pw = MEL.create_material_expression(mat, unreal.MaterialExpressionPower, -650, -60)
    exp = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 160)
    exp.set_editor_property("parameter_name", "VertexColourGamma")
    exp.set_editor_property("default_value", 2.2)  # set to 1.0 if the colours look too dark
    MEL.connect_material_expressions(vc, "", pw, "Base")
    MEL.connect_material_expressions(exp, "", pw, "Exp")
    return vc, pw


def create_materials():
    """Vertex-colour masters. Colour = vertex RGB; roughness = vertex alpha (opaque slots);
    glow slot: emissive = RGB * 20 * alpha^2 * EmissiveScale (same scale as the code's CPD emissive)."""
    made = []
    # opaque
    mat, new = _new_material("M_FT_Vertex")
    if new:
        vc, rgb = _vertex_colour(mat)
        MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(vc, "A", unreal.MaterialProperty.MP_ROUGHNESS)
        made.append(mat)
    # glow
    mat, new = _new_material("M_FT_VertexGlow")
    if new:
        vc, rgb = _vertex_colour(mat)
        sq = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -650, 120)
        MEL.connect_material_expressions(vc, "A", sq, "A")
        MEL.connect_material_expressions(vc, "A", sq, "B")
        scale = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -650, 260)
        scale.set_editor_property("parameter_name", "EmissiveScale")
        scale.set_editor_property("default_value", 20.0)
        k = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, 160)
        MEL.connect_material_expressions(sq, "", k, "A")
        MEL.connect_material_expressions(scale, "", k, "B")
        em = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 60)
        MEL.connect_material_expressions(rgb, "", em, "A")
        MEL.connect_material_expressions(k, "", em, "B")
        MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        made.append(mat)
    # glass
    mat, new = _new_material("M_FT_VertexGlass")
    if new:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        vc, rgb = _vertex_colour(mat)
        op = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -650, 200)
        op.set_editor_property("parameter_name", "Opacity")
        op.set_editor_property("default_value", 0.35)
        MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
        made.append(mat)
    # vehicle paint + trim: vertex colour, optionally replaced by a paint parameter
    for name, param in (("M_FT_CarBody", "BodyColor"), ("M_FT_CarTrim", "TrimColor")):
        mat, new = _new_material(name)
        if not new:
            continue
        vc, rgb = _vertex_colour(mat)
        paint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 200)
        paint.set_editor_property("parameter_name", param)
        amount = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -650, 340)
        amount.set_editor_property("parameter_name", "UseParameterColor")
        amount.set_editor_property("default_value", 0.0)
        lerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -350, 100)
        MEL.connect_material_expressions(rgb, "", lerp, "A")
        MEL.connect_material_expressions(paint, "", lerp, "B")
        MEL.connect_material_expressions(amount, "", lerp, "Alpha")
        MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(vc, "A", unreal.MaterialProperty.MP_ROUGHNESS)
        made.append(mat)
    # crew paint: vertex shading x the colour the code paints (FTVis::Paint -> custom primitive data 0-2)
    mat, new = _new_material("M_FT_CrewPaint")
    if new:
        vc, rgb = _vertex_colour(mat)
        paint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 200)
        paint.set_editor_property("parameter_name", "PaintColor")
        paint.set_editor_property("use_custom_primitive_data", True)
        paint.set_editor_property("primitive_data_index", 0)
        paint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 100)
        MEL.connect_material_expressions(rgb, "", mul, "A")
        MEL.connect_material_expressions(paint, "", mul, "B")
        MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
        MEL.connect_material_property(vc, "A", unreal.MaterialProperty.MP_ROUGHNESS)
        made.append(mat)
    # light beams: unlit, additive, two-sided; brightness = vertex alpha x painted colour x BeamScale
    mat, new = _new_material("M_FT_BeamGlow")
    if new:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        mat.set_editor_property("two_sided", True)
        vc, rgb = _vertex_colour(mat)
        paint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -650, 200)
        paint.set_editor_property("parameter_name", "BeamColor")
        paint.set_editor_property("use_custom_primitive_data", True)
        paint.set_editor_property("primitive_data_index", 0)
        scale = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -650, 340)
        scale.set_editor_property("parameter_name", "BeamScale")
        scale.set_editor_property("default_value", 3.0)
        a = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, 120)
        MEL.connect_material_expressions(rgb, "", a, "A")
        MEL.connect_material_expressions(paint, "", a, "B")
        b = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 180)
        MEL.connect_material_expressions(vc, "A", b, "A")
        MEL.connect_material_expressions(scale, "", b, "B")
        em = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 120)
        MEL.connect_material_expressions(a, "", em, "A")
        MEL.connect_material_expressions(b, "", em, "B")
        MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        made.append(mat)
    for m in made:
        MEL.recompile_material(m)
        unreal.EditorAssetLibrary.save_loaded_asset(m)
    unreal.log("[ftb] materials ready (%d new)" % len(made))


# ------------------------------------------------------------------------------------------- meshes

def _fbx_options():
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = ui.static_mesh_import_data
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", False)   # collision stays with the code's solid parts
    sm.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    sm.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    sm.set_editor_property("generate_lightmap_u_vs", True)
    sm.set_editor_property("import_uniform_scale", 1.0)
    sm.set_editor_property("convert_scene", True)
    sm.set_editor_property("force_front_x_axis", False)
    return ui


def _assign_materials(mesh):
    mats = mesh.get_editor_property("static_materials")
    for i, sm in enumerate(mats):
        slot = str(sm.get_editor_property("material_slot_name"))
        path = "%s/%s" % (MAT_DIR, slot)
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            mesh.set_material(i, unreal.EditorAssetLibrary.load_asset(path))


def _add_sockets(mesh, sockets):
    for name, s in (sockets or {}).items():
        sock = unreal.StaticMeshSocket(mesh)
        sock.set_editor_property("socket_name", name)
        loc, rot = s["location"], s["rotation"]
        sock.set_editor_property("relative_location", unreal.Vector(loc[0], loc[1], loc[2]))
        sock.set_editor_property("relative_rotation", unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
        mesh.add_socket(sock)


def import_all(only=None):
    tasks, entries = [], []
    for e in manifest():
        if only and only not in e["name"] and only not in e["folder"]:
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(PROJECT, e["fbx"]))
        t.set_editor_property("destination_path", "/Game/TheFinalTake/Meshes/" + e["folder"])
        t.set_editor_property("destination_name", e["name"])
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        t.set_editor_property("options", _fbx_options())
        tasks.append(t)
        entries.append(e)
    TOOLS.import_asset_tasks(tasks)
    for e in entries:
        mesh = unreal.EditorAssetLibrary.load_asset(e["unreal_path"])
        if not mesh:
            unreal.log_warning("[ftb] import failed: " + e["name"])
            continue
        _assign_materials(mesh)
        _add_sockets(mesh, e.get("sockets"))
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log("[ftb] imported %d meshes" % len(entries))


# ------------------------------------------------------------------------------------------- preview level

def build_preview_level(only=None):
    """Spawn every mesh at its manifest placements in a separate level (no gameplay actors)."""
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not unreal.EditorAssetLibrary.does_asset_exist(PREVIEW_LEVEL):
        les.new_level(PREVIEW_LEVEL)
    else:
        les.load_level(PREVIEW_LEVEL)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    n = 0
    for e in manifest():
        if only and only not in e["name"] and only not in e["folder"]:
            continue
        mesh = unreal.EditorAssetLibrary.load_asset(e["unreal_path"])
        if not mesh:
            continue
        for i, p in enumerate(e.get("placements") or []):
            loc, rot, scale = p["loc"], p["rot"], p["scale"]
            a = eas.spawn_actor_from_object(mesh, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
            a.set_actor_scale3d(unreal.Vector(*scale))
            a.set_actor_label("%s_%d" % (e["name"], i))
            a.set_folder_path(e["folder"])
            n += 1
    les.save_current_level()
    unreal.log("[ftb] preview level: %d mesh actors" % n)
