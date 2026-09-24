"""Cycles preview renders (thumbnails, contact sheets, in-layout comparisons)."""
import math
import os

import bpy
from mathutils import Vector


def _world(color=(0.36, 0.55, 0.78), strength=0.9):
    w = bpy.data.worlds.get("ft_preview") or bpy.data.worlds.new("ft_preview")
    w.use_nodes = True
    nt = w.node_tree
    bg = nt.nodes.get("Background")
    bg.inputs["Color"].default_value = (*color, 1.0)
    bg.inputs["Strength"].default_value = strength
    bpy.context.scene.world = w


def setup(samples=24, res=(512, 512), view="Standard"):
    s = bpy.context.scene
    s.render.engine = 'CYCLES'
    s.cycles.device = 'CPU'
    s.cycles.samples = samples
    s.cycles.use_denoising = True
    try:
        s.cycles.denoiser = 'OPENIMAGEDENOISE'
    except TypeError:
        pass
    s.cycles.max_bounces = 4
    s.render.resolution_x, s.render.resolution_y = res
    s.render.film_transparent = False
    s.view_settings.view_transform = view
    s.view_settings.look = 'None'
    s.view_settings.exposure = 0.0
    _world()
    col = bpy.data.collections.get("ft_preview_rig")
    if not col:
        col = bpy.data.collections.new("ft_preview_rig")
        s.collection.children.link(col)
        sun = bpy.data.lights.new("ft_key", 'SUN')
        sun.energy = 3.2
        sun.angle = math.radians(8)
        key = bpy.data.objects.new("ft_key", sun)
        key.rotation_euler = (math.radians(50), 0, math.radians(35))
        col.objects.link(key)
        fill = bpy.data.lights.new("ft_fill", 'SUN')
        fill.energy = 0.9
        fill.color = (0.75, 0.85, 1.0)
        fo = bpy.data.objects.new("ft_fill", fill)
        fo.rotation_euler = (math.radians(65), 0, math.radians(-140))
        col.objects.link(fo)
        cam = bpy.data.cameras.new("ft_cam")
        cam.lens = 60
        cam.clip_start = 1
        cam.clip_end = 200000
        co = bpy.data.objects.new("ft_cam", cam)
        col.objects.link(co)
        s.camera = co
        ground = bpy.data.meshes.new("ft_ground")
        ground.from_pydata([(-1, -1, 0), (1, -1, 0), (1, 1, 0), (-1, 1, 0)], [], [(0, 1, 2, 3)])
        g = bpy.data.objects.new("ft_ground", ground)
        mat = bpy.data.materials.new("ft_ground")
        mat.use_nodes = True
        mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.42, 0.62, 0.86, 1)
        mat.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 1.0
        ground.materials.append(mat)
        col.objects.link(g)
    return s


def frame(objs, direction=(1.0, -0.85, 0.62), lens=60, margin=1.12):
    """Aim the preview camera at the bounds of objs from a 3/4 front view (front = +X)."""
    s = bpy.context.scene
    cam = s.camera
    cam.data.lens = lens
    bpy.context.view_layer.update()
    pts = []
    for o in objs:
        pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
    mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (mn + mx) / 2
    radius = max((mx - mn).length / 2, 1.0)
    d = Vector(direction).normalized()
    fov = 2 * math.atan(18.0 / lens)
    dist = radius * margin / math.sin(fov / 2)
    cam.location = center + d * dist
    cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
    ground = bpy.data.objects.get("ft_ground")
    if ground:
        below = d.z < 0
        ground.location = (center.x, center.y, (mn.z - 0.2) if not below else (mx.z + radius * 50))
        ground.scale = (radius * 30, radius * 30, 1)
    return center, radius


def render(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    s = bpy.context.scene
    s.render.filepath = path
    bpy.ops.render.render(write_still=True)


def only_visible(objs):
    keep = set(objs) | set(bpy.data.collections["ft_preview_rig"].objects)
    for o in bpy.context.scene.objects:
        o.hide_render = o not in keep


def contact_sheet(items, path, cols=6, cell=256, title=None):
    """items: [(png_path, label)] -> one PNG grid (needs Pillow)."""
    from PIL import Image, ImageDraw, ImageFont
    rows = (len(items) + cols - 1) // cols
    head = 44 if title else 0
    sheet = Image.new("RGB", (cols * cell, rows * (cell + 22) + head), (27, 33, 64))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype(os.path.join(os.path.dirname(os.path.dirname(__file__)), "fonts", "LilitaOne-Regular.ttf"), 15)
        tfont = ImageFont.truetype(os.path.join(os.path.dirname(os.path.dirname(__file__)), "fonts", "LilitaOne-Regular.ttf"), 28)
    except OSError:
        font = tfont = ImageFont.load_default()
    if title:
        draw.text((12, 8), title, fill=(255, 214, 90), font=tfont)
    for i, (png, label) in enumerate(items):
        r, c = divmod(i, cols)
        x, y = c * cell, head + r * (cell + 22)
        try:
            im = Image.open(png).convert("RGB").resize((cell, cell))
            sheet.paste(im, (x, y))
        except OSError:
            pass
        draw.text((x + 6, y + cell + 2), label[:34], fill=(246, 231, 200), font=font)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    sheet.save(path)
