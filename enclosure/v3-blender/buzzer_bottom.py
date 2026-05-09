"""Buzzer (button unit) bottom half.
Internal cavity 80x100x30mm. Wall 2.5mm. Outer 85x105x32.5mm.
Cutouts:
  - Front wall (Y=0): OLED window 23x12mm, centered, Z=10..22mm above floor
  - Right wall (X=outer_w): 40mm speaker grille (hex pattern of 5mm holes)
  - Back wall (Y=outer_d): USB-C 10x4.5mm, aligned with TP4056 mount
  - Left wall (X=0): 6.2mm toggle switch hole, centered
Internal:
  - 4 corner mounting posts (6mm OD, 2.5mm M3 hole, 6mm tall) inset 8mm
  - OLED pocket rails behind window
  - TP4056 rails near back, aligned with USB-C
"""

import math
import os
import sys

# Ensure lib is importable
LIB_DIR = "/home/david/Classroom-Buzzers/enclosure/v3-blender"
if LIB_DIR not in sys.path:
    sys.path.insert(0, LIB_DIR)

import build_lib as L

# ---- Parameters ----
W = 80          # internal width (X)
D = 100         # internal depth (Y)
H = 30          # internal height (Z) of bottom shell
WALL = 2.5
R_OUT = 5       # outer corner radius
R_IN = R_OUT - WALL / 2  # inner corner radius

OUT_W = W + 2 * WALL
OUT_D = D + 2 * WALL

LID_LIP = 2
LID_TOL = 0.4

# Component dims (verified)
OLED_W = 23
OLED_H = 12
OLED_Z = 10           # offset from floor (interior)

SPEAKER_DIA = 40
SPEAKER_HOLE_DIA = 5  # printable grille hole

USBC_W = 10
USBC_H = 4.5
USBC_Z = 5            # offset from floor

SWITCH_DIA = 6.2
SWITCH_Z = 18

POST_OD = 6
POST_HOLE = 2.5       # M3 self-tap
POST_H = 6
POST_INSET = 8        # from outer corner (so post is well inside cavity)

TP_LEN = 26           # TP4056 length
TP_WID = 17

OUT_DIR = "/home/david/Classroom-Buzzers/enclosure/v3-blender"


def build():
    L.reset_scene()

    # ---- Outer shell (hollow, open top) ----
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, H + WALL, R_OUT)
    # Inner cavity cuts through top (open top); leaves WALL floor
    L.make_rounded_extrusion("cavity", W, D, H + 1, R_IN,
                             loc=(WALL, WALL, WALL))
    L.boolean("shell", "cavity")

    # ---- Cutouts ----
    cuts = []

    # OLED window (front, Y=0)
    cx = OUT_W / 2
    L.make_box("c_oled", OLED_W, WALL + 2, OLED_H,
               loc=(cx - OLED_W / 2, -1, WALL + OLED_Z))
    cuts.append("c_oled")

    # USB-C (back, Y=OUT_D)
    # TP4056 mount: x = WALL + (W - TP_LEN)/2 ... center it horizontally
    tp_x = WALL + 8
    usbc_cx = tp_x + TP_LEN / 2
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(usbc_cx - USBC_W / 2, OUT_D - WALL - 1, WALL + USBC_Z))
    cuts.append("c_usbc")

    # Toggle switch (left, X=0)
    L.make_cylinder("c_sw", SWITCH_DIA / 2, WALL + 2,
                    loc=(-1, OUT_D / 2, WALL + SWITCH_Z), axis='X')
    cuts.append("c_sw")

    # Speaker grille (right, X=OUT_W) — hex pattern of 5mm holes within 35mm circle
    spk_cy = OUT_D / 2
    spk_cz = WALL + H / 2 + 2
    spacing = SPEAKER_HOLE_DIA + 2
    grille_r = (SPEAKER_DIA - SPEAKER_HOLE_DIA) / 2
    rows = int(grille_r * 2 / (spacing * math.sqrt(3) / 2)) + 1
    idx = 0
    for row in range(-rows, rows + 1):
        zoff = row * spacing * math.sqrt(3) / 2
        xoff_start = (spacing / 2) if (row % 2) else 0
        cols = int((grille_r * 2 - xoff_start) / spacing) + 1
        for col in range(-cols, cols + 1):
            yoff = col * spacing + xoff_start
            if math.hypot(yoff, zoff) <= grille_r:
                name = f"c_spk_{idx}"
                L.make_cylinder(name, SPEAKER_HOLE_DIA / 2, WALL + 2,
                                loc=(OUT_W - WALL - 1, spk_cy + yoff, spk_cz + zoff),
                                axis='X')
                cuts.append(name)
                idx += 1

    # Apply all cutouts in one go (faster) by joining tools first
    if cuts:
        # Join all cut tools into one, then subtract once
        L.join_into(cuts[0], cuts[1:])
        L.boolean("shell", cuts[0])

    # ---- Mounting posts (added after cuts) ----
    posts = []
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        p_outer = f"post_o_{i}"
        p_hole = f"post_h_{i}"
        L.make_cylinder(p_outer, POST_OD / 2, POST_H, loc=(px, py, WALL))
        L.make_cylinder(p_hole, POST_HOLE / 2, POST_H + 0.1, loc=(px, py, WALL - 0.05))
        L.boolean(p_outer, p_hole)
        posts.append(p_outer)

    # OLED pocket rails (behind front wall, hold display in place)
    pocket_depth = 5
    pocket_tol = 0.5
    rail_x_l = OUT_W / 2 - OLED_W / 2 - 3 - pocket_tol
    rail_x_r = OUT_W / 2 + OLED_W / 2 + pocket_tol
    L.make_box("rail_l", 3, pocket_depth, OLED_H + 4,
               loc=(rail_x_l, WALL, WALL + OLED_Z - 2))
    L.make_box("rail_r", 3, pocket_depth, OLED_H + 4,
               loc=(rail_x_r, WALL, WALL + OLED_Z - 2))
    L.make_box("rail_b", OLED_W + 6 + pocket_tol * 2, pocket_depth, 2,
               loc=(rail_x_l, WALL, WALL + OLED_Z - 2))
    posts += ["rail_l", "rail_r", "rail_b"]

    # TP4056 rails (hold module so USB-C aligns with back-wall hole)
    tp_y = OUT_D - WALL - TP_WID - 1
    L.make_box("tp_l", 2, TP_WID + 2, 4, loc=(tp_x - 1, tp_y - 1, WALL))
    L.make_box("tp_r", 2, TP_WID + 2, 4, loc=(tp_x + TP_LEN - 1, tp_y - 1, WALL))
    posts += ["tp_l", "tp_r"]

    # Join posts into shell
    L.join_into("shell", posts)
    L.select_only("shell")
    bpy_obj = __import__("bpy")
    bpy_obj.context.active_object.name = "buzzer_bottom"

    return "buzzer_bottom"


name = build()
import bpy
o = bpy.data.objects[name]
result = {
    "name": name,
    "verts": len(o.data.vertices),
    "faces": len(o.data.polygons),
    "bbox": [
        [round(min(v.co[i] for v in o.data.vertices), 3) for i in range(3)],
        [round(max(v.co[i] for v in o.data.vertices), 3) for i in range(3)],
    ],
}
