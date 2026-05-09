"""Teacher/Host base station — handheld remote-control form factor.
Outer: 160 x 60 x 28mm (rounded rectangle). Two halves split front/back.
Front half (visible side):
  - OLED window 23x13 near top
  - 2x 12mm momentary buttons below OLED, side by side (Start/Stop, Reset)
Back half (case body):
  - USB-C 12x8 on bottom edge (short side near user's hand)
  - microSD 30x5 on right edge
ESP32 mounts to back-half floor; OLED mounts behind front window.
Halves join with M3 screws at 4 corners + 2 mid-side bosses.
"""

import sys
LIB = "/home/david/Classroom-Buzzers/enclosure/v3-blender"
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import build_lib as L
import bpy, os

# Outer dims
W = 60       # width (X) - across hand
D = 160      # length (Y) - tip to base
WALL = 2.5
R_OUT = 8    # generous corner radius for ergonomics

# Each half: 14mm tall = 28mm total external
HALF_H = 14
LID_LIP = 2
LID_TOL = 0.4

# Internal cavity (combined when assembled)
INNER_W = W - 2 * WALL
INNER_D = D - 2 * WALL
INNER_H_HALF = HALF_H - WALL  # interior height per half
R_IN = R_OUT - WALL / 2

# Components
OLED_W, OLED_H = 23, 13
OLED_Y = D - 35              # OLED center Y
BTN_DIA = 12
BTN_Y = D - 75               # button row Y
BTN_DX = 22                  # spacing between buttons (each side from center)

USBC_W, USBC_H = 12, 8       # bottom edge
USBC_OFFSET = 3              # from internal floor

SD_W, SD_H = 30, 5

POST_OD = 5.5
POST_HOLE = 2.5              # M3 self-tap in front half post
POST_HOLE_CLEAR = 3.2        # M3 clearance in back half
POST_INSET_X = 6
POST_INSET_Y = 12

OUT_DIR = "/home/david/Classroom-Buzzers/enclosure/v3-blender"


def build_back():
    """Back half = bottom shell that holds components (open top)."""
    L.reset_scene()
    L.make_rounded_extrusion("shell", W, D, HALF_H, R_OUT)
    L.make_rounded_extrusion("cav", INNER_W, INNER_D, HALF_H + 1, R_IN,
                             loc=(WALL, WALL, WALL))
    L.boolean("shell", "cav")

    cuts = []

    # USB-C on bottom edge (Y=0, the end nearest user when holding)
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(W / 2 - USBC_W / 2, -1, WALL + USBC_OFFSET))
    cuts.append("c_usbc")

    # microSD on right edge (X=W)
    L.make_box("c_sd", WALL + 2, SD_W, SD_H,
               loc=(W - WALL - 1, D - 50, WALL + 4))
    cuts.append("c_sd")

    L.join_into(cuts[0], cuts[1:])
    L.boolean("shell", cuts[0])

    # Mounting posts: 4 corners + 2 mid-sides (long body needs middle stiffeners)
    posts = []
    post_pts = [
        (POST_INSET_X + WALL, POST_INSET_Y + WALL),
        (W - POST_INSET_X - WALL, POST_INSET_Y + WALL),
        (POST_INSET_X + WALL, D - POST_INSET_Y - WALL),
        (W - POST_INSET_X - WALL, D - POST_INSET_Y - WALL),
        (POST_INSET_X + WALL, D / 2),
        (W - POST_INSET_X - WALL, D / 2),
    ]
    for i, (px, py) in enumerate(post_pts):
        po = f"post_o_{i}"
        ph = f"post_h_{i}"
        L.make_cylinder(po, POST_OD / 2, INNER_H_HALF, loc=(px, py, WALL))
        # Through hole for screw passing through back to thread into front
        L.make_cylinder(ph, POST_HOLE_CLEAR / 2, INNER_H_HALF + WALL + 1,
                        loc=(px, py, -0.5))
        L.boolean(po, ph)
        posts.append(po)

        # Counterbore from outside back so screw head sits flush
        # (counterbore the bottom face of shell)
        nb = f"cb_{i}"
        L.make_cylinder(nb, 3.2, 2.0, loc=(px, py, -0.05))
        # Subtract from shell directly
        L.boolean("shell", nb)

    # ESP32 mounting rails along back half floor (51.8x25.4)
    esp_x = WALL + (INNER_W - 25.4) / 2
    esp_y = 30
    L.make_box("esp_rl", 25.4 + 2, 2, 4, loc=(esp_x - 1, esp_y, WALL))
    L.make_box("esp_rr", 25.4 + 2, 2, 4, loc=(esp_x - 1, esp_y + 51.8 - 1, WALL))
    posts += ["esp_rl", "esp_rr"]

    L.join_into("shell", posts)
    bpy.context.active_object.name = "base_handheld_back"
    return "base_handheld_back"


def build_front():
    """Front half = closed top with display and buttons."""
    L.reset_scene()
    L.make_rounded_extrusion("shell", W, D, HALF_H, R_OUT)
    L.make_rounded_extrusion("cav", INNER_W, INNER_D, HALF_H - WALL + 1, R_IN,
                             loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # Registration lip (drops into back half)
    lip_w = INNER_W - 2 * LID_TOL
    lip_d = INNER_D - 2 * LID_TOL
    L.make_rounded_extrusion("lip_o", lip_w, lip_d, LID_LIP,
                             max(R_IN - LID_TOL, 1),
                             loc=(WALL + LID_TOL, WALL + LID_TOL, -LID_LIP))
    L.make_rounded_extrusion("lip_i", lip_w - 2.5, lip_d - 2.5, LID_LIP + 0.5,
                             max(R_IN - LID_TOL - 1.25, 0.5),
                             loc=(WALL + LID_TOL + 1.25, WALL + LID_TOL + 1.25,
                                  -LID_LIP - 0.1))
    L.boolean("lip_o", "lip_i")
    L.join_into("shell", ["lip_o"])

    cuts = []

    # OLED window (visible face = top z)
    L.make_box("c_oled", OLED_W, OLED_H, WALL + 2,
               loc=(W / 2 - OLED_W / 2, OLED_Y - OLED_H / 2, HALF_H - WALL - 1))
    cuts.append("c_oled")

    # Two buttons below OLED
    L.make_cylinder("c_btn_l", BTN_DIA / 2, HALF_H + 4,
                    loc=(W / 2 - BTN_DX / 2, BTN_Y, -2))
    L.make_cylinder("c_btn_r", BTN_DIA / 2, HALF_H + 4,
                    loc=(W / 2 + BTN_DX / 2, BTN_Y, -2))
    cuts += ["c_btn_l", "c_btn_r"]

    # Screw bosses with self-tap holes (M3 thread bites into 2.5mm hole)
    post_pts = [
        (POST_INSET_X + WALL, POST_INSET_Y + WALL),
        (W - POST_INSET_X - WALL, POST_INSET_Y + WALL),
        (POST_INSET_X + WALL, D - POST_INSET_Y - WALL),
        (W - POST_INSET_X - WALL, D - POST_INSET_Y - WALL),
        (POST_INSET_X + WALL, D / 2),
        (W - POST_INSET_X - WALL, D / 2),
    ]
    bosses = []
    for i, (px, py) in enumerate(post_pts):
        po = f"boss_{i}"
        ph = f"bhole_{i}"
        # Boss hangs from inside ceiling INTO cavity (positive Z, fills cavity)
        boss_h = HALF_H - WALL  # 11.5mm — meets closed top of shell
        L.make_cylinder(po, POST_OD / 2 + 1, boss_h, loc=(px, py, 0))
        L.make_cylinder(ph, POST_HOLE / 2, boss_h - 0.5,
                        loc=(px, py, 0 - 0.1))
        L.boolean(po, ph)
        bosses.append(po)

    # OLED pocket rails (frames the display from inside, attached to closed top)
    rail_z = HALF_H - WALL - 5
    L.make_box("o_rt", OLED_W + 6, 3, 5,
               loc=(W / 2 - OLED_W / 2 - 3, OLED_Y - OLED_H / 2 - 3, rail_z))
    L.make_box("o_rb", OLED_W + 6, 3, 5,
               loc=(W / 2 - OLED_W / 2 - 3, OLED_Y + OLED_H / 2, rail_z))
    bosses += ["o_rt", "o_rb"]

    L.join_into(cuts[0], cuts[1:])
    L.boolean("shell", cuts[0])

    L.join_into("shell", bosses)
    bpy.context.active_object.name = "base_handheld_front"
    return "base_handheld_front"


bn = build_back()
out_b = os.path.join(OUT_DIR, "base-handheld-back.stl")
L.export_stl(bn, out_b)
ob = bpy.data.objects[bn]
b_info = {"verts": len(ob.data.vertices), "faces": len(ob.data.polygons),
          "stl_kb": round(os.path.getsize(out_b) / 1024, 1)}

fn = build_front()
out_f = os.path.join(OUT_DIR, "base-handheld-front.stl")
L.export_stl(fn, out_f)
of = bpy.data.objects[fn]
f_info = {"verts": len(of.data.vertices), "faces": len(of.data.polygons),
          "stl_kb": round(os.path.getsize(out_f) / 1024, 1)}

result = {"back": b_info, "front": f_info}
