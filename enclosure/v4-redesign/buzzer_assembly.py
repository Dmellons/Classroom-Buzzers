"""Render buzzer exploded assembly preview.
Imports the 3 buzzer STLs (bottom + lid + bezel), places ghost component
placeholders inside the bottom shell, and renders an exploded 3D view
with the lid lifted above and bezel pulled forward."""

import sys, os, math
LIB = "/home/david/Classroom-Buzzers/enclosure/v4-redesign"
for m in list(sys.modules):
    if 'build_lib' in m or 'render_helper' in m:
        del sys.modules[m]
sys.path = [p for p in sys.path if 'v3-blender' not in p]
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import bpy, bmesh
from mathutils import Vector

# Reset
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
for m_ in list(bpy.data.meshes):
    bpy.data.meshes.remove(m_, do_unlink=True)
for mat in list(bpy.data.materials):
    bpy.data.materials.remove(mat, do_unlink=True)
for lt in list(bpy.data.lights):
    bpy.data.lights.remove(lt, do_unlink=True)
for cam in list(bpy.data.cameras):
    bpy.data.cameras.remove(cam, do_unlink=True)


def make_mat(name, color, alpha=1.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.45
    if alpha < 1.0:
        bsdf.inputs["Alpha"].default_value = alpha
        mat.blend_method = 'BLEND'
    return mat


def import_stl(path, name):
    bpy.ops.wm.stl_import(filepath=path)
    obj = bpy.context.selected_objects[-1]
    obj.name = name
    return obj


# Match buzzer_bottom.py constants
W, D, H = 100, 120, 40
WALL = 2.5
OUT_W = W + 2 * WALL    # 105
OUT_D = D + 2 * WALL    # 125

# --- Import the three case parts ---
bottom = import_stl(os.path.join(LIB, "buzzer-bottom.stl"), "case_bottom")
bottom.data.materials.clear()
bottom.data.materials.append(make_mat("mat_bottom", (0.20, 0.45, 0.75)))

lid = import_stl(os.path.join(LIB, "buzzer-lid.stl"), "case_lid")
lid.data.materials.clear()
lid.data.materials.append(make_mat("mat_lid", (0.90, 0.30, 0.25)))
# Lift lid up
LIFT = 80
lid.location = (0, 0, H + WALL + LIFT)

bezel = import_stl(os.path.join(LIB, "buzzer-bezel.stl"), "case_bezel")
bezel.data.materials.clear()
bezel.data.materials.append(make_mat("mat_bezel", (0.95, 0.85, 0.35)))
# Bezel sits in front of bottom shell front-top cutout, pulled forward
BEZEL_FORWARD = 50
bezel.location = (
    OUT_W / 2 - 35 / 2,           # X centered on bottom shell
    -BEZEL_FORWARD,                # pulled forward in -Y
    WALL + 24                      # at the height of the bezel cutout
)

# --- Component placeholders (matching buzzer_bottom.py positions) ---
# All boxes/cylinders use: name, dims, location_min_corner, color
BAT_X = WALL + 6
BAT_Y = WALL + 6


def make_box_at(name, w, d, h, loc, color):
    bpy.ops.mesh.primitive_cube_add(size=1)
    o = bpy.context.active_object
    o.name = name
    o.scale = (w, d, h)
    bpy.ops.object.transform_apply(scale=True)
    o.location = (loc[0] + w / 2, loc[1] + d / 2, loc[2] + h / 2)
    bpy.ops.object.transform_apply(location=True)
    mat = make_mat(name + "_mat", color)
    o.data.materials.append(mat)
    return o


def make_cyl_at(name, r, h, loc, color, axis='Z'):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=h, vertices=48)
    o = bpy.context.active_object
    o.name = name
    if axis == 'Y':
        o.rotation_euler = (math.pi / 2, 0, 0)
        bpy.ops.object.transform_apply(rotation=True)
        o.location = (loc[0], loc[1] + h / 2, loc[2])
    elif axis == 'X':
        o.rotation_euler = (0, math.pi / 2, 0)
        bpy.ops.object.transform_apply(rotation=True)
        o.location = (loc[0] + h / 2, loc[1], loc[2])
    else:
        o.location = (loc[0], loc[1], loc[2] + h / 2)
    bpy.ops.object.transform_apply(location=True)
    mat = make_mat(name + "_mat", color)
    o.data.materials.append(mat)
    return o


# Battery (yellow/orange flat pouch)
make_box_at("battery", 35, 30, 5, (BAT_X, BAT_Y, WALL),
            (0.95, 0.55, 0.20))

# TP4056 (green PCB)
TP_LEN, TP_WID = 26, 17
tp_x = WALL + (W - TP_LEN) / 2
tp_y = OUT_D - WALL - TP_WID - 2
make_box_at("tp4056", TP_LEN, TP_WID, 4,
            (tp_x, tp_y, WALL + 0.5), (0.10, 0.55, 0.25))

# ESP32-C6 (green PCB, larger)
ESP_W, ESP_LEN = 25.4, 51.8
esp_y = WALL + (D - ESP_LEN) / 2
esp_x = OUT_W - WALL - ESP_W - 2
make_box_at("esp32", ESP_W, ESP_LEN, 10,
            (esp_x, esp_y, WALL + 0.5), (0.05, 0.40, 0.15))

# MT3608 #1 (5V, blue PCB)
MT_LEN, MT_WID = 37, 17
mt1_x = WALL + 4
mt1_y = WALL + 30 + 8
make_box_at("mt3608_1", MT_LEN, MT_WID, 14,
            (mt1_x, mt1_y, WALL + 0.5), (0.30, 0.40, 0.85))

# MT3608 #2 (12V LED option, dimmer blue)
mt2_y = mt1_y + MT_WID + 6
make_box_at("mt3608_2", MT_LEN, MT_WID, 14,
            (mt1_x, mt2_y, WALL + 0.5), (0.45, 0.55, 0.85))

# Speaker (black cylinder along Y axis, against front wall)
make_cyl_at("speaker", 20, 20,
            (OUT_W / 2, WALL + 0.5, WALL + 14), (0.10, 0.10, 0.10),
            axis='Y')

# Toggle switch (small dark cylinder, on right wall)
make_cyl_at("switch", 3, 8,
            (OUT_W - WALL - 0.5, WALL + 15, WALL + 25),
            (0.15, 0.15, 0.15), axis='X')

# Arcade button cap (red dome above the lid, where it pokes through)
arcade_cx = OUT_W / 2
arcade_cy = OUT_D / 2 + 18
# Button body (extending down through lid)
make_cyl_at("arcade_body", 14, 30,
            (arcade_cx, arcade_cy, H + WALL + LIFT - 30),
            (0.85, 0.20, 0.15))
# Dome on top
bpy.ops.mesh.primitive_uv_sphere_add(radius=18, location=(arcade_cx, arcade_cy,
                                                            H + WALL + LIFT + 5))
dome = bpy.context.active_object
dome.name = "arcade_dome"
# Crush bottom half
import bmesh as _bm
bm_d = _bm.new()
bm_d.from_mesh(dome.data)
bm_d.verts.ensure_lookup_table()
for v in bm_d.verts:
    if v.co.z < 0:
        v.co.z = 0
bm_d.to_mesh(dome.data)
bm_d.free()
dome.data.materials.append(make_mat("dome_mat", (0.90, 0.20, 0.15)))

# OLED on bezel (small black rectangle)
# bezel angled 30° (atan2(14, 22) ≈ 32.5°). Place an OLED-sized box on
# the angled face of the bezel.
bezel_bx = OUT_W / 2  # X-center of bezel
bezel_by = -BEZEL_FORWARD + 3
bezel_bz = WALL + 24 + 11  # mid-height of bezel
make_box_at("oled", 23, 4, 12,
            (bezel_bx - 11.5, bezel_by - 2, bezel_bz - 6),
            (0.05, 0.05, 0.05))

# --- Render setup ---
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = 64
scene.cycles.use_denoising = True
try:
    scene.cycles.device = 'GPU'
except Exception:
    pass
scene.render.resolution_x = 1600
scene.render.resolution_y = 1200
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
out_png = os.path.join(LIB, "buzzer-assembly.png")
scene.render.filepath = out_png

# World background
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

# Floor
bpy.ops.mesh.primitive_plane_add(size=600, location=(50, 50, -2))
floor = bpy.context.active_object
floor.name = "floor"
fmat = bpy.data.materials.new("floor_mat")
fmat.use_nodes = True
fmat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.55, 0.57, 0.60, 1.0)
fmat.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.5
floor.data.materials.append(fmat)

# Camera — wide angle to capture exploded layout
cam_loc = (-150, -200, 200)
bpy.ops.object.camera_add(location=cam_loc)
cam = bpy.context.active_object
target = Vector((OUT_W / 2, OUT_D / 2, 60))
direction = target - cam.location
cam.rotation_mode = 'QUATERNION'
cam.rotation_quaternion = direction.to_track_quat('-Z', 'Y')
cam.data.lens = 35
scene.camera = cam

# Lights
def add_light(name, loc, energy, size=80):
    bpy.ops.object.light_add(type='AREA', location=loc)
    light = bpy.context.active_object
    light.name = name
    light.data.energy = energy
    light.data.size = size
    d = target - light.location
    light.rotation_mode = 'QUATERNION'
    light.rotation_quaternion = d.to_track_quat('-Z', 'Y')
    return light

add_light("key", (200, -180, 350), 200000, size=120)
add_light("fill", (-250, -100, 200), 80000, size=150)
add_light("rim", (50, 250, 250), 100000, size=100)

# Render
bpy.ops.render.render(write_still=True)

result = {
    "png": out_png,
    "png_kb": round(os.path.getsize(out_png) / 1024, 1) if os.path.exists(out_png) else None,
}
