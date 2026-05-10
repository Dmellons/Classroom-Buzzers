"""Handheld base — front half (v5).
Pistol-grip outline matching back half.

Improvements:
- Pistol-grip outline (matches back half exactly)
- GMT020-02-8p TFT in portrait orientation, in HEAD section
- 2x 12mm momentary-button holes below TFT
- Triangle/square indicators
- Recessed inset around TFT for scratch protection
- Internal screw bosses match the back half's 6 through-screw positions
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

# Pistol outline (must match back half)
HEAD_W = 75
HEAD_D = 90
TAPER_LEN = 12
GRIP_W = 50
GRIP_D = 80
TOTAL_D = HEAD_D + TAPER_LEN + GRIP_D
H = 11
WALL = 2.5
CORNER_R = 8
TAPER_R = 4

GRIP_INSET = (HEAD_W - GRIP_W) / 2

# Y boundaries
GRIP_BASE_Y = 0
GRIP_TOP_Y = GRIP_D
TAPER_TOP_Y = GRIP_D + TAPER_LEN
HEAD_TOP_Y = TAPER_TOP_Y + HEAD_D

# TFT (portrait orientation): long axis along Y (slope direction)
TFT_WINDOW_W = 31.0      # active 30.6 + tol (short axis along X)
TFT_WINDOW_H = 41.2      # active 40.8 + tol (long axis along Y)
TFT_PCB_W = 38.0
TFT_PCB_H = 63.0
TFT_INSET_DEPTH = 2
TFT_Y_CENTER = TAPER_TOP_Y + HEAD_D / 2 + 8   # head section, slightly toward top
TFT_X_CENTER = HEAD_W / 2

# Buttons
BTN_DIA = 12
BTN_DX = 22
BTN_Y = TAPER_TOP_Y + 16  # below TFT, near bottom of head section

INDICATOR_OFFSET = 10
INDICATOR_DEPTH = 0.6
TRIANGLE_SIZE = 6
SQUARE_SIZE = 5

# Internal screw bosses (matching back half through-screw positions)
POST_OD = 5.5
POST_HOLE = 2.5  # M3 self-tap

# Boss positions must match back half exactly
SD_Y_MIN = 127.0
SD_Y_MAX = 157.0
HEAD_Y_TOP = HEAD_TOP_Y - 12
HEAD_Y_BELOW_SD = SD_Y_MIN - POST_OD / 2 - 4
HEAD_Y_LEFT_MID = (HEAD_Y_TOP + TAPER_TOP_Y + 6) / 2

POST_PTS = [
    (WALL + 6, HEAD_Y_TOP),                       # head-TL
    (HEAD_W - WALL - 6, HEAD_Y_TOP),              # head-TR
    (WALL + 6, HEAD_Y_LEFT_MID),                  # head-LM
    (HEAD_W - WALL - 6, HEAD_Y_BELOW_SD),         # head-RB
    (GRIP_INSET + WALL + 6, GRIP_BASE_Y + 14),    # grip-BL
    (GRIP_INSET + GRIP_W - WALL - 6, GRIP_BASE_Y + 14),  # grip-BR
]

NAME = "base_handheld_front"


def build():
    L.reset_scene()

    L.make_pistol_outline("shell", head_w=HEAD_W, head_d=HEAD_D,
                           taper_len=TAPER_LEN, grip_w=GRIP_W, grip_d=GRIP_D,
                           height=H, corner_r=CORNER_R, taper_r=TAPER_R)
    # Inner cavity: cuts from BELOW (Z=-1) up to (H - WALL + 1) so the
    # bottom face is open and the front face (Z=H) is closed
    L.make_pistol_outline("cav", head_w=HEAD_W - 2 * WALL,
                           head_d=HEAD_D - 2 * WALL,
                           taper_len=TAPER_LEN, grip_w=GRIP_W - 2 * WALL,
                           grip_d=GRIP_D - 2 * WALL,
                           height=H - WALL + 1,
                           corner_r=max(CORNER_R - WALL, 1),
                           taper_r=max(TAPER_R - WALL, 0.5),
                           loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # TFT recessed inset on outer front face
    INSET_W = TFT_WINDOW_W + 7
    INSET_H = TFT_WINDOW_H + 7
    L.make_box("c_tft_inset", INSET_W, INSET_H, TFT_INSET_DEPTH + 0.05,
               loc=(TFT_X_CENTER - INSET_W / 2,
                    TFT_Y_CENTER - INSET_H / 2,
                    H - TFT_INSET_DEPTH))
    L.boolean("shell", "c_tft_inset")

    # TFT window cutout (through entire front wall)
    L.make_box("c_tft", TFT_WINDOW_W, TFT_WINDOW_H, WALL + 4,
               loc=(TFT_X_CENTER - TFT_WINDOW_W / 2,
                    TFT_Y_CENTER - TFT_WINDOW_H / 2,
                    H - WALL - 1))
    L.boolean("shell", "c_tft")

    # Buttons (centered horizontally, below TFT)
    btn_xs = [HEAD_W / 2 - BTN_DX / 2, HEAD_W / 2 + BTN_DX / 2]
    for i, bx in enumerate(btn_xs):
        n = f"c_btn_{i}"
        L.make_cylinder(n, BTN_DIA / 2, H + 4, loc=(bx, BTN_Y, -2))
        L.boolean("shell", n)

    # Indicator: triangle next to left button (Start)
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
    bmesh.ops.translate(bm, vec=Vector((0, 0, INDICATOR_DEPTH + 0.5)),
                        verts=ev)
    bm.normal_update()
    bm.to_mesh(me_t)
    bm.free()
    tri = bpy.data.objects.new("c_tri", me_t)
    bpy.context.collection.objects.link(tri)
    tri.location = (btn_xs[0] - INDICATOR_OFFSET, BTN_Y, H)
    bpy.context.view_layer.objects.active = tri
    bpy.ops.object.transform_apply(location=True)
    L.boolean("shell", "c_tri")

    # Square next to right button (Reset)
    L.make_box("c_sq", SQUARE_SIZE, SQUARE_SIZE, INDICATOR_DEPTH + 0.5,
               loc=(btn_xs[1] + INDICATOR_OFFSET - SQUARE_SIZE / 2,
                    BTN_Y - SQUARE_SIZE / 2,
                    H - INDICATOR_DEPTH))
    L.boolean("shell", "c_sq")

    # Internal screw bosses with M3 self-tap pilot
    boss_h = H - WALL  # 8.5mm — fills cavity top to ceiling
    additions = []
    for i, (px, py) in enumerate(POST_PTS):
        po = f"boss_{i}"
        ph = f"bhole_{i}"
        L.make_cylinder(po, POST_OD / 2 + 1, boss_h, loc=(px, py, 0))
        L.make_cylinder(ph, POST_HOLE / 2, boss_h - 0.5,
                        loc=(px, py, -0.05))
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
R.render_part(name, color=(0.85, 0.75, 0.30), out_path=png_path, cam_z_mul=2.0)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
