"""Handheld base — front half (v4).
65 x 165 x 11mm.

Improvements:
- Side-edge chamfers (4x4mm) matching back half
- Thumb dish (35mm dia x 1.5mm deep) above buttons
- Recessed OLED window (2mm inset around 23x13 window, scratch protection)
- 2x 12mm momentary-button holes with shape indicators (triangle / square)
- 6 internal screw bosses (M3 self-tap into 2.5mm pilot)
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
import bpy
import bmesh
from mathutils import Vector

W = 65
D = 165
H = 11
WALL = 2.5
R_OUT = 6
R_IN = R_OUT - WALL / 2
CHAMFER = 4

OLED_W = 23
OLED_H = 13
OLED_INSET_W = OLED_W + 8
OLED_INSET_H = OLED_H + 6
OLED_INSET_DEPTH = 2
OLED_Y = D - 35   # near top end (Y=D)

BTN_DIA = 12
BTN_Y = D - 75
BTN_DX = 22
INDICATOR_OFFSET = 10
INDICATOR_DEPTH = 0.6
TRIANGLE_SIZE = 6
SQUARE_SIZE = 5

THUMB_DISH_DIA = 35
THUMB_DISH_DEPTH = 1.5
THUMB_DISH_Y = D - 110  # below buttons (lower-on-thumb-when-held)

POST_OD = 5.5
POST_HOLE = 2.5
POST_INSET_X = 6
POST_INSET_Y = 12

NAME = "base_handheld_front"


def build():
    L.reset_scene()

    L.make_rounded_extrusion("shell", W, D, H, R_OUT)
    # Hollow inside (cuts through OPEN BOTTOM at Z=0; top closed)
    L.make_rounded_extrusion("cav", W - 2 * WALL, D - 2 * WALL,
                             H - WALL + 1, R_IN,
                             loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # Side-edge chamfers (top edges = closed front face when assembled)
    for side, sign, x_anchor in [("L", 1, 0), ("R", -1, W)]:
        me = bpy.data.meshes.new(f"chamfer_{side}_mesh")
        bm = bmesh.new()
        v1 = bm.verts.new((x_anchor, -1, H + 1))
        v2 = bm.verts.new((x_anchor + sign * CHAMFER, -1, H + 1))
        v3 = bm.verts.new((x_anchor, -1, H - CHAMFER))
        bm.faces.new([v1, v2, v3])
        bm.normal_update()
        geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
        ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
        bmesh.ops.translate(bm, vec=Vector((0, D + 2, 0)), verts=ev)
        bm.normal_update()
        bm.to_mesh(me)
        bm.free()
        co = bpy.data.objects.new(f"chamfer_{side}", me)
        bpy.context.collection.objects.link(co)
        bpy.context.view_layer.objects.active = co
        L.boolean("shell", f"chamfer_{side}")

    # Thumb dish on outer front face (Z=H)
    L.make_dish_indent("c_thumb", diameter=THUMB_DISH_DIA, depth=THUMB_DISH_DEPTH,
                       loc=(W / 2, THUMB_DISH_Y, H))
    L.boolean("shell", "c_thumb")

    # Recessed OLED inset on outer face
    L.make_box("c_oled_inset", OLED_INSET_W, OLED_INSET_H, OLED_INSET_DEPTH + 0.05,
               loc=(W / 2 - OLED_INSET_W / 2,
                    OLED_Y - OLED_INSET_H / 2,
                    H - OLED_INSET_DEPTH))
    L.boolean("shell", "c_oled_inset")

    # OLED window (through entire front wall)
    L.make_box("c_oled", OLED_W, OLED_H, WALL + 4,
               loc=(W / 2 - OLED_W / 2,
                    OLED_Y - OLED_H / 2,
                    H - WALL - 1))
    L.boolean("shell", "c_oled")

    # Two 12mm button holes
    btn_xs = [W / 2 - BTN_DX / 2, W / 2 + BTN_DX / 2]
    for i, bx in enumerate(btn_xs):
        n = f"c_btn_{i}"
        L.make_cylinder(n, BTN_DIA / 2, H + 4, loc=(bx, BTN_Y, -2))
        L.boolean("shell", n)

    # Triangle indicator (Start) — left of left button
    me_t = bpy.data.meshes.new("tri_mesh")
    bm = bmesh.new()
    s = TRIANGLE_SIZE
    bm.faces.new([
        bm.verts.new((-s/2, -s/2*0.866, -INDICATOR_DEPTH)),
        bm.verts.new((s/2, -s/2*0.866, -INDICATOR_DEPTH)),
        bm.verts.new((0, s/2*0.866, -INDICATOR_DEPTH)),
    ])
    geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((0, 0, INDICATOR_DEPTH + 0.5)), verts=ev)
    bm.normal_update()
    bm.to_mesh(me_t)
    bm.free()
    tri = bpy.data.objects.new("c_tri", me_t)
    bpy.context.collection.objects.link(tri)
    tri.location = (btn_xs[0] - INDICATOR_OFFSET, BTN_Y, H)
    bpy.ops.object.transform_apply(location=True)
    L.boolean("shell", "c_tri")

    # Square indicator (Reset) — right of right button
    L.make_box("c_sq", SQUARE_SIZE, SQUARE_SIZE, INDICATOR_DEPTH + 0.5,
               loc=(btn_xs[1] + INDICATOR_OFFSET - SQUARE_SIZE / 2,
                    BTN_Y - SQUARE_SIZE / 2,
                    H - INDICATOR_DEPTH))
    L.boolean("shell", "c_sq")

    # Internal screw bosses with M3 self-tap pilot — 6 points
    post_pts = [
        (POST_INSET_X + WALL, POST_INSET_Y + WALL),
        (W - POST_INSET_X - WALL, POST_INSET_Y + WALL),
        (POST_INSET_X + WALL, D - POST_INSET_Y - WALL),
        (W - POST_INSET_X - WALL, D - POST_INSET_Y - WALL),
        (POST_INSET_X + WALL, D / 2),
        (W - POST_INSET_X - WALL, D / 2),
    ]
    additions = []
    boss_h = H - WALL
    for i, (px, py) in enumerate(post_pts):
        po = f"boss_o_{i}"
        ph = f"boss_h_{i}"
        # Boss: extends from ceiling (Z=H-WALL = top of cavity) downward into cavity
        # In open-bottom-up print orientation, this means the boss hangs from
        # the closed top into the cavity.
        L.make_cylinder(po, POST_OD / 2 + 1, boss_h, loc=(px, py, 0))
        L.make_cylinder(ph, POST_HOLE / 2, boss_h - 0.5,
                        loc=(px, py, 0 - 0.05))
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
stl_path = os.path.join(LIB, "base-handheld-front.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-handheld-front.png")
R.render_part(name, color=(0.85, 0.75, 0.30), out_path=png_path, cam_z_mul=1.8)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
