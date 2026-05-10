"""Desktop teacher/host base — wedge form factor (v5).
Single-piece wedge that prints flat on its open back face.

CHANGED IN v5:
- Upgraded to GMT020-02-8p 2.0" TFT (240x320). Window 31x41mm,
  PCB pocket 38x63mm internal, 12.5mm clearance behind face.
- Layout: landscape TFT on left, 2 buttons stacked vertically to
  the right of the display (Start on top, Reset below).
- Footprint widened to 140 x 110 to accommodate stacked buttons.
- Bug fix: per-button cylinders are now created fresh per iteration
  with no rotation state leak; both buttons render reliably.
"""

import sys, os, math
LIB = "/home/david/Classroom-Buzzers/enclosure/v4-redesign"
for m in list(sys.modules):
    if 'build_lib' in m or 'render_helper' in m:
        del sys.modules[m]
sys.path = [p for p in sys.path if 'v3-blender' not in p]
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import build_lib as L
import render_helper as R
import bpy, bmesh
from mathutils import Vector

# Outer
W = 140                 # X — width (was 130 in v4)
D = 110                 # Y — depth front to back
H_FRONT = 25
H_BACK = 45
WALL = 2.5
R_OUT = 5

# GMT020-02-8p TFT (landscape orientation here)
TFT_WINDOW_W = 41.2     # active 40.8 + tol (long axis along X)
TFT_WINDOW_H = 31.0     # active 30.6 + tol (short axis along Y/slope)
TFT_PCB_W = 63.0        # PCB outline + tol (long axis along X)
TFT_PCB_H = 38.0        # PCB outline + tol (short axis along Y)
TFT_DEPTH = 13.0        # internal clearance behind face
TFT_X_CENTER_OFFSET = -25  # offset display LEFT of wedge X center

# Buttons (stacked column to RIGHT of display)
BTN_DIA = 12
BTN_X_OFFSET = 35       # X offset from wedge center to button column
BTN_SPACING = 26        # Y distance between the two buttons
BTN_ROW_Y = D / 2       # Y center of the button column

# Indicator deboss shapes (next to each button)
TRIANGLE_SIZE = 6
SQUARE_SIZE = 5
INDICATOR_DEPTH = 0.6
INDICATOR_OFFSET_X = 10  # shift indicator inward (toward display)

# Other cutouts
USBC_W, USBC_H = 12, 8
USBC_Z = 8
SD_SLOT_W, SD_SLOT_H = 30, 5
SD_SLOT_Z = 12

POST_OD = 6
POST_HOLE = 2.5
POST_INSET = 9

NAME = "base_desktop_wedge"


def angled_top_z(y):
    return H_FRONT + (H_BACK - H_FRONT) * (y / D)


def build():
    L.reset_scene()

    # Hollow wedge shell
    L.make_wedge_shell("shell", w=W, d=D, h_front=H_FRONT, h_back=H_BACK,
                       r=R_OUT, wall=WALL)

    slope_angle = math.atan2(H_BACK - H_FRONT, D)

    # USB-C on rear (vertical) wall
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(W / 2 - USBC_W / 2, D - WALL - 1, USBC_Z))
    L.boolean("shell", "c_usbc")

    # microSD on right wall
    L.make_box("c_sd", WALL + 2, SD_SLOT_W, SD_SLOT_H,
               loc=(W - WALL - 1, D / 2 - SD_SLOT_W / 2, SD_SLOT_Z))
    L.boolean("shell", "c_sd")

    # ----- TFT window through angled top, perpendicular to surface -----
    tft_x = W / 2 + TFT_X_CENTER_OFFSET
    tft_y = D / 2 + 5
    tft_z = angled_top_z(tft_y)
    cut_d = WALL + 4
    L.make_box("c_tft", TFT_WINDOW_W, TFT_WINDOW_H, cut_d,
               loc=(-TFT_WINDOW_W / 2, -TFT_WINDOW_H / 2, -cut_d / 2))
    co = bpy.data.objects["c_tft"]
    co.rotation_mode = 'XYZ'
    co.rotation_euler = (slope_angle, 0, 0)
    co.location = (tft_x, tft_y, tft_z)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_tft")

    # ----- TWO BUTTONS, stacked to RIGHT of display -----
    btn_x = W / 2 + BTN_X_OFFSET
    button_centers_y = [BTN_ROW_Y + BTN_SPACING / 2,   # Start (top, toward back)
                        BTN_ROW_Y - BTN_SPACING / 2]    # Reset (bottom, toward front)
    button_positions = []
    for i, by in enumerate(button_centers_y):
        n = f"c_btn_{i}"
        # Build cylinder fresh, with axis Z, no prior rotation state.
        # Then rotate around its own center to match the slope, and translate
        # to place its midpoint at the angled top surface.
        cyl_h = H_BACK + 6
        L.make_cylinder(n, BTN_DIA / 2, cyl_h,
                        loc=(0, 0, -cyl_h / 2), axis='Z')
        co = bpy.data.objects[n]
        co.rotation_mode = 'XYZ'
        co.rotation_euler = (slope_angle, 0, 0)
        bz = angled_top_z(by)
        co.location = (btn_x, by, bz)
        bpy.ops.object.transform_apply(location=True, rotation=True,
                                        scale=False)
        L.boolean("shell", n)
        button_positions.append((btn_x, by, bz))

    # ----- Indicator shapes next to each button -----
    # Triangle next to Start (button 0, top)
    me_t = bpy.data.meshes.new("tri_mesh")
    bm = bmesh.new()
    s = TRIANGLE_SIZE
    bm.faces.new([
        bm.verts.new((-s/2, -s/2*0.866, 0)),
        bm.verts.new((s/2, -s/2*0.866, 0)),
        bm.verts.new((0, s/2*0.866, 0)),
    ])
    geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((0, 0, INDICATOR_DEPTH + 0.5)),
                        verts=ev)
    bm.normal_update()
    bm.to_mesh(me_t)
    bm.free()
    tri = bpy.data.objects.new("c_tri", me_t)
    bpy.context.collection.objects.link(tri)
    tri.rotation_mode = 'XYZ'
    tri.rotation_euler = (slope_angle, 0, 0)
    tx, ty, tz = button_positions[0]
    tri.location = (tx - INDICATOR_OFFSET_X, ty, tz - INDICATOR_DEPTH / 2)
    bpy.context.view_layer.objects.active = tri
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_tri")

    # Square next to Reset (button 1, bottom)
    L.make_box("c_sq", SQUARE_SIZE, SQUARE_SIZE, INDICATOR_DEPTH + 0.5,
               loc=(-SQUARE_SIZE / 2, -SQUARE_SIZE / 2,
                    -(INDICATOR_DEPTH + 0.5) / 2))
    sq = bpy.data.objects["c_sq"]
    sq.rotation_mode = 'XYZ'
    sq.rotation_euler = (slope_angle, 0, 0)
    sx, sy, sz = button_positions[1]
    sq.location = (sx - INDICATOR_OFFSET_X, sy, sz - INDICATOR_DEPTH / 2)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_sq")

    # ----- Mounting posts (4 corners) -----
    additions = []
    post_xs = [WALL + POST_INSET, W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        po = f"post_o_{i}"
        ph = f"post_h_{i}"
        ceiling = angled_top_z(py)
        post_h = max(ceiling - WALL - WALL - 1, 5)
        L.make_cylinder(po, POST_OD / 2, post_h, loc=(px, py, WALL))
        L.make_cylinder(ph, POST_HOLE / 2, post_h + 0.1,
                        loc=(px, py, WALL - 0.05))
        L.boolean(po, ph)
        additions.append(po)

    if additions:
        L.join_into("shell", additions)

    bpy.context.active_object.name = NAME
    return NAME


name = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]

stl_path = os.path.join(LIB, "base-desktop-wedge.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-desktop-wedge.png")
R.render_part(name, color=(0.30, 0.30, 0.35), out_path=png_path, cam_z_mul=2.5)

# Top-down verification render to confirm both buttons + display visible
import bpy as _bpy
_bpy.ops.object.select_all(action='DESELECT')
o2 = _bpy.data.objects[name]
o2.select_set(True)
_bpy.context.view_layer.objects.active = o2
# Add a top-down camera, render
for cam_obj in [c for c in _bpy.data.objects if c.type == 'CAMERA']:
    _bpy.data.objects.remove(cam_obj, do_unlink=True)
_bpy.ops.object.camera_add(location=(W / 2, D / 2, 250))
top_cam = _bpy.context.active_object
top_cam.rotation_mode = 'XYZ'
top_cam.rotation_euler = (0, 0, 0)
top_cam.data.lens = 50
_bpy.context.scene.camera = top_cam
top_png = os.path.join(LIB, "base-desktop-wedge-topdown.png")
_bpy.context.scene.render.filepath = top_png
_bpy.context.scene.render.resolution_x = 1024
_bpy.context.scene.render.resolution_y = 800
_bpy.ops.render.render(write_still=True)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
    "topdown_png_kb": round(os.path.getsize(top_png) / 1024, 1),
}
