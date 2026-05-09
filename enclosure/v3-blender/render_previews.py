"""Render preview PNGs for each STL in this directory.
3/4 isometric-ish camera, 3-point lighting, plastic-like material.
Output: <name>.png next to each .stl file.
"""

import bpy
import math
import os
from mathutils import Vector

OUT_DIR = "/home/david/Classroom-Buzzers/enclosure/v3-blender"

PARTS = [
    "buzzer-bottom",
    "buzzer-top",
    "base-desktop-bottom",
    "base-desktop-top",
    "base-handheld-back",
    "base-handheld-front",
]

W, H = 1280, 960  # render resolution


def reset():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m, do_unlink=True)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat, do_unlink=True)
    for lt in list(bpy.data.lights):
        bpy.data.lights.remove(lt, do_unlink=True)
    for cam in list(bpy.data.cameras):
        bpy.data.cameras.remove(cam, do_unlink=True)


def make_plastic_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.45
    # Slight metallic-ish for visual interest (matte plastic look)
    if "Metallic" in bsdf.inputs:
        bsdf.inputs["Metallic"].default_value = 0.05
    return mat


def add_three_point_light(target_loc, distance):
    # Key (much brighter — area lights at long distance need lots of energy)
    bpy.ops.object.light_add(type='AREA', location=(
        target_loc[0] + distance * 0.8,
        target_loc[1] - distance * 0.8,
        target_loc[2] + distance * 1.2))
    key = bpy.context.active_object
    key.data.energy = 8000
    key.data.size = distance * 0.6
    # Fill
    bpy.ops.object.light_add(type='AREA', location=(
        target_loc[0] - distance * 1.0,
        target_loc[1] - distance * 0.4,
        target_loc[2] + distance * 0.6))
    fill = bpy.context.active_object
    fill.data.energy = 3000
    fill.data.size = distance * 0.8
    # Rim/back
    bpy.ops.object.light_add(type='AREA', location=(
        target_loc[0] + distance * 0.2,
        target_loc[1] + distance * 1.0,
        target_loc[2] + distance * 0.9))
    rim = bpy.context.active_object
    rim.data.energy = 5000
    rim.data.size = distance * 0.5

    # Point all lights at target
    for light in [key, fill, rim]:
        constraint = light.constraints.new('TRACK_TO')
        # We need a target empty
    # Simpler: rotate them manually by computing direction
    for light in [key, fill, rim]:
        direction = Vector(target_loc) - light.location
        light.rotation_mode = 'QUATERNION'
        light.rotation_quaternion = direction.to_track_quat('-Z', 'Y')


def add_floor(z, size):
    bpy.ops.mesh.primitive_plane_add(size=size, location=(size / 4, size / 4, z))
    floor = bpy.context.active_object
    floor.scale = (1, 1, 1)
    mat = bpy.data.materials.new("floor_mat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    # Darker floor so part pops; subtle gradient via roughness
    bsdf.inputs["Base Color"].default_value = (0.55, 0.57, 0.60, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.5
    floor.data.materials.append(mat)


def frame_camera(obj, view='iso'):
    # Compute object center and largest dim
    bbox_corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    xs = [v.x for v in bbox_corners]
    ys = [v.y for v in bbox_corners]
    zs = [v.z for v in bbox_corners]
    center = Vector((sum(xs) / 8, sum(ys) / 8, sum(zs) / 8))
    size = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
    distance = size * 1.8

    # High 3/4 angle (~55deg elevation) so the open top + side cutouts show
    cam_offset = Vector((distance * 0.6, -distance * 0.85, distance * 1.5))
    cam_loc = center + cam_offset

    bpy.ops.object.camera_add(location=cam_loc)
    cam = bpy.context.active_object
    # Aim at a point slightly above the part center so it sits in lower 2/3
    aim = center + Vector((0, 0, size * 0.05))
    direction = aim - cam.location
    cam.rotation_mode = 'QUATERNION'
    cam.rotation_quaternion = direction.to_track_quat('-Z', 'Y')
    cam.data.lens = 40   # slightly wider for better part visibility
    bpy.context.scene.camera = cam
    return center, size


def setup_render(out_path):
    scene = bpy.context.scene
    # Cycles for better quality; samples kept low (64) for speed
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
    # Use GPU if available, otherwise CPU
    try:
        scene.cycles.device = 'GPU'
    except Exception:
        pass
    scene.render.resolution_x = W
    scene.render.resolution_y = H
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.filepath = out_path
    # Mid-grey background so light-colored parts show contrast
    world = scene.world
    if world is None:
        world = bpy.data.worlds.new("World")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.32, 0.34, 0.38, 1.0)
        bg.inputs["Strength"].default_value = 1.0
    # Color management: standard view transform with high contrast
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'Medium High Contrast'


def render_part(name, color):
    reset()
    stl_path = os.path.join(OUT_DIR, name + ".stl")
    bpy.ops.wm.stl_import(filepath=stl_path)
    obj = bpy.context.selected_objects[-1]

    # Center the object on origin for easier framing
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    bcenter = Vector((
        sum(v.x for v in bbox) / 8,
        sum(v.y for v in bbox) / 8,
        sum(v.z for v in bbox) / 8,
    ))
    bmin_z = min(v.z for v in bbox)
    obj.location = obj.location - bcenter
    obj.location.z -= bmin_z - bcenter.z  # rest on z=0
    bpy.context.view_layer.update()

    # Smooth-shade only faces above 30 degrees angle (auto-smooth equivalent in 5.x)
    try:
        bpy.ops.object.shade_auto_smooth(angle=math.radians(30))
    except Exception:
        bpy.ops.object.shade_flat()

    # Material
    mat = make_plastic_material(name + "_mat", color)
    obj.data.materials.clear()
    obj.data.materials.append(mat)

    # Frame
    center, size = frame_camera(obj)

    # Floor under part
    add_floor(0 - 0.01, max(size * 4, 200))

    # Lights
    add_three_point_light((0, 0, size / 3), size * 1.5)

    out_png = os.path.join(OUT_DIR, name + ".png")
    setup_render(out_png)
    bpy.ops.render.render(write_still=True)
    return out_png, os.path.getsize(out_png)


# Color palette per part
COLORS = {
    "buzzer-bottom": (0.20, 0.45, 0.75),     # blue
    "buzzer-top": (0.90, 0.30, 0.25),        # red
    "base-desktop-bottom": (0.30, 0.30, 0.35),  # dark grey
    "base-desktop-top": (0.55, 0.55, 0.60),  # light grey
    "base-handheld-back": (0.25, 0.55, 0.35),  # green
    "base-handheld-front": (0.85, 0.75, 0.30),  # yellow
}

results = {}
for p in PARTS:
    out_png, sz = render_part(p, COLORS[p])
    results[p] = {"path": out_png, "kb": round(sz / 1024, 1)}

result = results
