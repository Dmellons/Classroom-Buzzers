"""Render a single named object to PNG, then clean up render-only objects.
Designed to be called after a part script has built its mesh.
Background: mid-grey for contrast; lighting: 3-point area lights;
engine: Cycles 64 samples; resolution: 1280x960."""

import bpy
import math
import os
from mathutils import Vector

W, H = 1280, 960


def _make_plastic_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.45
    if "Metallic" in bsdf.inputs:
        bsdf.inputs["Metallic"].default_value = 0.05
    return mat


def _add_three_point(target, distance):
    bpy.ops.object.light_add(type='AREA', location=(
        target[0] + distance * 0.8,
        target[1] - distance * 0.8,
        target[2] + distance * 1.2))
    key = bpy.context.active_object
    key.name = "_render_key"
    key.data.energy = 8000
    key.data.size = distance * 0.6

    bpy.ops.object.light_add(type='AREA', location=(
        target[0] - distance * 1.0,
        target[1] - distance * 0.4,
        target[2] + distance * 0.6))
    fill = bpy.context.active_object
    fill.name = "_render_fill"
    fill.data.energy = 3000
    fill.data.size = distance * 0.8

    bpy.ops.object.light_add(type='AREA', location=(
        target[0] + distance * 0.2,
        target[1] + distance * 1.0,
        target[2] + distance * 0.9))
    rim = bpy.context.active_object
    rim.name = "_render_rim"
    rim.data.energy = 5000
    rim.data.size = distance * 0.5

    for light in [key, fill, rim]:
        d = Vector(target) - light.location
        light.rotation_mode = 'QUATERNION'
        light.rotation_quaternion = d.to_track_quat('-Z', 'Y')


def _add_floor(z, size):
    bpy.ops.mesh.primitive_plane_add(size=size, location=(size / 4, size / 4, z))
    floor = bpy.context.active_object
    floor.name = "_render_floor"
    mat = bpy.data.materials.new("_render_floor_mat")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.55, 0.57, 0.60, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.5
    floor.data.materials.append(mat)


def _setup_render_settings(out_path):
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 64
    scene.cycles.use_denoising = True
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
    world = scene.world
    if world is None:
        world = bpy.data.worlds.new("World")
        scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.32, 0.34, 0.38, 1.0)
        bg.inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'Medium High Contrast'


def render_part(name, color, out_path, lift_to_floor=True, cam_z_mul=1.5):
    """Render the existing object `name` to `out_path`. Adds floor + lights
    + camera, then removes them after render so the source mesh remains
    available for further export.

    cam_z_mul controls camera elevation (higher = more top-down).
    """
    obj = bpy.data.objects[name]

    # Compute bbox in world space
    bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    bcenter = Vector((
        sum(v.x for v in bbox) / 8,
        sum(v.y for v in bbox) / 8,
        sum(v.z for v in bbox) / 8,
    ))
    size = max(
        max(v.x for v in bbox) - min(v.x for v in bbox),
        max(v.y for v in bbox) - min(v.y for v in bbox),
        max(v.z for v in bbox) - min(v.z for v in bbox),
    )

    # Optionally translate object so its lowest Z is at 0
    pre_loc = obj.location.copy()
    if lift_to_floor:
        bmin_z = min(v.z for v in bbox)
        obj.location.z -= bmin_z
        bpy.context.view_layer.update()
        bbox = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
        bcenter = Vector((
            sum(v.x for v in bbox) / 8,
            sum(v.y for v in bbox) / 8,
            sum(v.z for v in bbox) / 8,
        ))

    # Smooth shading
    select_only(name)
    try:
        bpy.ops.object.shade_auto_smooth(angle=math.radians(30))
    except Exception:
        bpy.ops.object.shade_flat()

    # Material
    mat = _make_plastic_material(name + "_render_mat", color)
    obj.data.materials.clear()
    obj.data.materials.append(mat)

    # Camera
    distance = size * 1.8
    cam_offset = Vector((distance * 0.6, -distance * 0.85, distance * cam_z_mul))
    aim = bcenter + Vector((0, 0, size * 0.05))
    bpy.ops.object.camera_add(location=bcenter + cam_offset)
    cam = bpy.context.active_object
    cam.name = "_render_cam"
    d = aim - cam.location
    cam.rotation_mode = 'QUATERNION'
    cam.rotation_quaternion = d.to_track_quat('-Z', 'Y')
    cam.data.lens = 40
    bpy.context.scene.camera = cam

    # Floor + lights
    _add_floor(0 - 0.01, max(size * 4, 200))
    _add_three_point((bcenter.x, bcenter.y, size / 3), size * 1.5)

    _setup_render_settings(out_path)
    bpy.ops.render.render(write_still=True)

    # Cleanup transient render objects
    for n in ["_render_floor", "_render_cam", "_render_key",
              "_render_fill", "_render_rim"]:
        if n in bpy.data.objects:
            bpy.data.objects.remove(bpy.data.objects[n], do_unlink=True)
    # Restore object position for any subsequent operations
    if lift_to_floor:
        obj.location = pre_loc
        bpy.context.view_layer.update()

    return out_path, os.path.getsize(out_path) if os.path.exists(out_path) else 0


def select_only(name):
    """Internal helper: same as build_lib.select_only but local to avoid imports."""
    bpy.ops.object.select_all(action='DESELECT')
    o = bpy.data.objects[name]
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    return o
