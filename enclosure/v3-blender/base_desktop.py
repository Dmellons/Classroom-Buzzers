"""Teacher/Host base station — desktop form factor.
Internal cavity 110x80x32mm. Wall 2.5mm. Outer 115x85mm footprint.
Cutouts:
  - Front wall: 0.96" OLED window 23x13mm (PCB 27x27mm) at center, Z offset 14mm
  - Top lid: 2x 12mm momentary buttons (Start/Stop, Reset)
  - Back wall: USB-C 12x8mm (powers ESP32)
  - Right wall: microSD slot 30x5mm (slide-in access)
  - Vent slots both sides for cooling
Two parts: bottom + top lid. Lid drops in via 2mm lip, M3 corner screws.
"""

import sys
LIB = "/home/david/Classroom-Buzzers/enclosure/v3-blender"
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import build_lib as L
import bpy, os

W, D, H = 110, 80, 32
WALL = 2.5
R_OUT = 5
R_IN = R_OUT - WALL / 2
OUT_W = W + 2 * WALL
OUT_D = D + 2 * WALL

LID_H = 15
LID_LIP = 2
LID_TOL = 0.4

OLED_W, OLED_H = 23, 13       # 0.96" 128x64 active 22x12 + tol
OLED_Z = 12                   # offset from floor (interior)
USBC_W, USBC_H = 12, 8        # USB-C cutout
USBC_Z = 8
SD_W, SD_H = 30, 5            # microSD slot
SD_Z = 16

BTN_DIA = 12                  # momentary push button hole
POST_OD = 6
POST_HOLE = 2.5
POST_H = 6
POST_INSET = 8
POST_HOLE_CLEAR = 3.2

OUT_DIR = "/home/david/Classroom-Buzzers/enclosure/v3-blender"


def build_bottom():
    L.reset_scene()
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, H + WALL, R_OUT)
    L.make_rounded_extrusion("cav", W, D, H + 2, R_IN, loc=(WALL, WALL, WALL))
    L.boolean("shell", "cav")

    cuts = []

    # OLED window (front, Y=0)
    cx = OUT_W / 2
    L.make_box("c_oled", OLED_W, WALL + 2, OLED_H,
               loc=(cx - OLED_W / 2, -1, WALL + OLED_Z))
    cuts.append("c_oled")

    # USB-C (back, Y=OUT_D)
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(cx - USBC_W / 2, OUT_D - WALL - 1, WALL + USBC_Z))
    cuts.append("c_usbc")

    # microSD slot (right side)
    L.make_box("c_sd", WALL + 2, SD_W, SD_H,
               loc=(OUT_W - WALL - 1, OUT_D / 2 - SD_W / 2, WALL + SD_Z))
    cuts.append("c_sd")

    # Vent slots (both sides) — 4 slots each, 12mm long, 2mm wide
    for side, x_pos in [("L", -1), ("R", OUT_W - WALL - 1)]:
        for i in range(4):
            n = f"c_vent_{side}_{i}"
            L.make_box(n, WALL + 2, 2, 12,
                       loc=(x_pos, 15 + i * 8, WALL + 2))
            cuts.append(n)

    L.join_into(cuts[0], cuts[1:])
    L.boolean("shell", cuts[0])

    # Mounting posts
    posts = []
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        po = f"post_o_{i}"
        ph = f"post_h_{i}"
        L.make_cylinder(po, POST_OD / 2, POST_H, loc=(px, py, WALL))
        L.make_cylinder(ph, POST_HOLE / 2, POST_H + 0.1, loc=(px, py, WALL - 0.05))
        L.boolean(po, ph)
        posts.append(po)

    # ESP32 mounting rails (51.8x25.4mm) - center back
    esp_x = WALL + (W - 51.8) / 2
    esp_y = WALL + 8
    L.make_box("esp_rl", 2, 51.8, 4, loc=(esp_x - 1, esp_y, WALL))
    L.make_box("esp_rr", 2, 51.8, 4, loc=(esp_x + 25.4 - 1, esp_y, WALL))
    posts += ["esp_rl", "esp_rr"]

    # OLED pocket rails behind front window
    L.make_box("oled_rl", 3, 4, OLED_H + 4,
               loc=(cx - OLED_W / 2 - 3 - 0.5, WALL, WALL + OLED_Z - 2))
    L.make_box("oled_rr", 3, 4, OLED_H + 4,
               loc=(cx + OLED_W / 2 + 0.5, WALL, WALL + OLED_Z - 2))
    posts += ["oled_rl", "oled_rr"]

    L.join_into("shell", posts)
    bpy.context.active_object.name = "base_desktop_bottom"
    return "base_desktop_bottom"


def build_top():
    L.reset_scene()
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, LID_H, R_OUT)
    L.make_rounded_extrusion("cav", W, D, LID_H - WALL + 1, R_IN,
                             loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # Registration lip
    lip_w = W - 2 * LID_TOL
    lip_d = D - 2 * LID_TOL
    L.make_rounded_extrusion("lip_o", lip_w, lip_d, LID_LIP, R_IN - LID_TOL,
                             loc=(WALL + LID_TOL, WALL + LID_TOL, -LID_LIP))
    L.make_rounded_extrusion("lip_i", lip_w - 2.5, lip_d - 2.5, LID_LIP + 0.5,
                             max(R_IN - LID_TOL - 1.25, 1),
                             loc=(WALL + LID_TOL + 1.25, WALL + LID_TOL + 1.25,
                                  -LID_LIP - 0.1))
    L.boolean("lip_o", "lip_i")
    L.join_into("shell", ["lip_o"])

    cuts = []

    # Two 12mm button holes on top (offset toward back)
    btn_y_back = OUT_D - 22
    btn_x_l = OUT_W / 2 - 18
    btn_x_r = OUT_W / 2 + 18
    L.make_cylinder("c_btn_l", BTN_DIA / 2, LID_H + 4, loc=(btn_x_l, btn_y_back, -2))
    L.make_cylinder("c_btn_r", BTN_DIA / 2, LID_H + 4, loc=(btn_x_r, btn_y_back, -2))
    cuts += ["c_btn_l", "c_btn_r"]

    # Corner screw clearance + counterbore
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        n = f"c_screw_{i}"
        nc = f"c_cbore_{i}"
        L.make_cylinder(n, POST_HOLE_CLEAR / 2, LID_H + 4, loc=(px, py, -LID_LIP - 1))
        L.make_cylinder(nc, 3.0, 2.0, loc=(px, py, LID_H - 2.0 + 0.01))
        cuts += [n, nc]

    L.join_into(cuts[0], cuts[1:])
    L.boolean("shell", cuts[0])

    bpy.context.active_object.name = "base_desktop_top"
    return "base_desktop_top"


bn = build_bottom()
out_b = os.path.join(OUT_DIR, "base-desktop-bottom.stl")
L.export_stl(bn, out_b)
ob = bpy.data.objects[bn]
b_info = {
    "verts": len(ob.data.vertices),
    "faces": len(ob.data.polygons),
    "stl_kb": round(os.path.getsize(out_b) / 1024, 1),
}

tn = build_top()
out_t = os.path.join(OUT_DIR, "base-desktop-top.stl")
L.export_stl(tn, out_t)
ot = bpy.data.objects[tn]
t_info = {
    "verts": len(ot.data.vertices),
    "faces": len(ot.data.polygons),
    "stl_kb": round(os.path.getsize(out_t) / 1024, 1),
}

result = {"bottom": b_info, "top": t_info}
