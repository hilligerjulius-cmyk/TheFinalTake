"""Scene setup, FBX export for Unreal and a re-import check."""
import os
import struct

import bpy
from mathutils import Matrix

# Unreal: 1 unit = 1 cm, Z up, X forward. The Blender scene is set to 1 BU = 1 cm; the FBX
# exporter writes Y-up/-Z-forward with UnitScaleFactor 1 (cm) and Unreal's importer ("Convert
# Scene", the default) maps it back so Blender (x, y, z) arrives as Unreal (x, -y, z) - exactly the
# inverse of the conversion Asset.build() applied. Objects are exported from the origin with
# identity transforms, so the pivot is the asset origin whether or not "Transform Vertex to
# Absolute" is ticked on import.
FBX_SETTINGS = dict(
    use_selection=True,
    object_types={'MESH'},
    use_mesh_modifiers=False,
    mesh_smooth_type='FACE',
    use_tspace=False,
    colors_type='SRGB',
    prioritize_active_color=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Z',
    axis_up='Y',
    bake_space_transform=False,
    add_leaf_bones=False,
    bake_anim=False,
    use_custom_props=False,
    path_mode='AUTO',
    embed_textures=False,
)


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.unit_settings.system = 'METRIC'
    s.unit_settings.scale_length = 0.01
    s.unit_settings.length_unit = 'CENTIMETERS'
    return s


def export_fbx(obj, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    saved = obj.matrix_world.copy()
    obj.matrix_world = Matrix.Identity(4)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=path, **FBX_SETTINGS)
    obj.select_set(False)
    obj.matrix_world = saved


def reimport_check(path):
    """Import the FBX back and report bounds (Blender space, cm), tris, colours and slots."""
    objs_before = set(bpy.data.objects)
    mats_before = set(bpy.data.materials)
    meshes_before = set(bpy.data.meshes)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in objs_before]
    out = []
    for o in new:
        if o.type != 'MESH':
            continue
        me = o.data
        pts = [o.matrix_world @ v.co for v in me.vertices]
        out.append({
            "min": [min(p[i] for p in pts) for i in range(3)],
            "max": [max(p[i] for p in pts) for i in range(3)],
            "tris": sum(len(p.vertices) - 2 for p in me.polygons),
            "colors": len(me.color_attributes),
            "materials": [m.name.split(".")[0] for m in me.materials if m],
            "scale": list(o.matrix_world.to_scale()),
        })
    for o in new:
        bpy.data.objects.remove(o)
    for m in set(bpy.data.meshes) - meshes_before:
        bpy.data.meshes.remove(m)
    for m in set(bpy.data.materials) - mats_before:
        bpy.data.materials.remove(m)
    return out


def fbx_header(path):
    """UnitScaleFactor and axis settings straight from the binary FBX (no FBX SDK needed)."""
    with open(path, "rb") as f:
        data = f.read()
    info = {}
    for key in (b"UnitScaleFactor", b"UpAxis", b"UpAxisSign", b"FrontAxis", b"FrontAxisSign", b"CoordAxis", b"CoordAxisSign"):
        i = data.find(b"S" + struct.pack("<I", len(key)) + key)
        if i < 0:
            continue
        j = i + 5 + len(key)
        for _ in range(3):  # type, label, flags
            if data[j:j + 1] != b"S":
                break
            ln = struct.unpack("<I", data[j + 1:j + 5])[0]
            j += 5 + ln
        t = data[j:j + 1]
        if t == b"D":
            info[key.decode()] = struct.unpack("<d", data[j + 1:j + 9])[0]
        elif t == b"I":
            info[key.decode()] = struct.unpack("<i", data[j + 1:j + 5])[0]
    return info
