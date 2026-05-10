"""Buzzer bottom shell (v4).
Internal cavity 100x120x40mm. Wall 2.5. Outer 105x125x42.5.

Improvements over v3:
- Wider footprint (100x120 vs 80x100) for tip-resistance when slammed
- Front-firing speaker (was side-firing into desk)
- Battery bay 36x31 pocket with two 3mm zip-tie slots through floor
- Component rails for TP4056, ESP32, two MT3608 boards
- Compression rib under arcade-button slam zone
- Rectangular cutout on front-top for separate angled OLED bezel
- 4 rubber-foot dimples on underside (3M Bumpon spec)
- Cable-management ridges around OLED area
"""

import sys, os
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

# ---- Outer/cavity ----
W, D, H = 100, 120, 40
WALL = 2.5
R_OUT = 5
R_IN = R_OUT - WALL / 2
OUT_W = W + 2 * WALL
OUT_D = D + 2 * WALL

# ---- Components ----
SPEAKER_DIA = 40
SPEAKER_HOLE = 5

USBC_W, USBC_H = 10, 4.5

SWITCH_DIA = 6.2

# Bezel cutout (front-top of bottom shell — bezel inserts here from outside)
BEZEL_CUT_W = 28
BEZEL_CUT_H = 18
BEZEL_CUT_Z = H - BEZEL_CUT_H + 2  # near top of cavity

# Posts
POST_OD = 6
POST_HOLE = 2.5
POST_H = 7
POST_INSET = 9

# Rubber foot dimples (3M Bumpon SJ5018: ~6mm dia, 1mm tall)
FOOT_DIA = 6
FOOT_DEPTH = 1
FOOT_INSET = 12

# Battery bay
BAT_W, BAT_D = 36, 31
BAT_DEPTH = 6  # how deep the pocket sinks the battery
ZIPTIE_SLOT_W, ZIPTIE_SLOT_L = 3, 8

# TP4056 26x17, mounted at back wall
TP_LEN, TP_WID = 26, 17

# ESP32 25.4 x 51.8, mounted along right wall
ESP_W, ESP_LEN = 25.4, 51.8

# MT3608 37x17x14 - lying flat
MT_LEN, MT_WID = 37, 17

OUT_DIR = LIB
NAME = "buzzer_bottom"


def build():
    L.reset_scene()

    # Outer shell
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, H + WALL, R_OUT)
    # Inner cavity (cuts through top, leaves WALL-thick floor)
    L.make_rounded_extrusion("cav", W, D, H + 2, R_IN, loc=(WALL, WALL, WALL))
    L.boolean("shell", "cav")

    cuts = []

    # --- Front wall: speaker grille + bezel cutout ---
    spk_cx = OUT_W / 2
    spk_cy = -1  # cylinder loc.y for axis=Y
    spk_cz = WALL + 14  # speaker centered ~14mm above floor (lower part of front)
    # Use grille hole helper (axis Y)
    spk_holes = L.make_speaker_grille_holes(
        "c_spk",
        center=(spk_cx, WALL, spk_cz),
        normal='Y',
        dia=SPEAKER_DIA,
        hole_dia=SPEAKER_HOLE,
        wall_thickness=WALL,
    )
    cuts.extend(spk_holes)

    # Bezel rectangular cutout in front wall, near top
    L.make_box("c_bezel", BEZEL_CUT_W, WALL + 2, BEZEL_CUT_H,
               loc=(spk_cx - BEZEL_CUT_W / 2, -1, WALL + BEZEL_CUT_Z))
    cuts.append("c_bezel")

    # --- Right wall: ESP32 USB-C (debug) + toggle switch ---
    # ESP32 sits along right wall with USB-C exiting to right
    esp_y = WALL + (D - ESP_LEN) / 2
    esp_x = OUT_W - WALL - ESP_W - 2  # 2mm clearance from right wall
    # USB-C cutout aligned with ESP32 USB-C port (at the +Y end of ESP32)
    usb_dbg_y = esp_y + ESP_LEN - 8  # USB-C is on one short end
    L.make_box("c_usb_dbg", WALL + 2, USBC_W, USBC_H,
               loc=(OUT_W - WALL - 1,
                    usb_dbg_y - USBC_W / 2,
                    WALL + 6))
    cuts.append("c_usb_dbg")

    # Toggle switch on right wall
    L.make_cylinder("c_switch", SWITCH_DIA / 2, WALL + 2,
                    loc=(OUT_W - WALL - 1, WALL + 15, WALL + 25), axis='X')
    cuts.append("c_switch")

    # --- Back wall: TP4056 USB-C charge port ---
    tp_x = WALL + (W - TP_LEN) / 2
    L.make_box("c_charge", USBC_W, WALL + 2, USBC_H,
               loc=(tp_x + (TP_LEN - USBC_W) / 2,
                    OUT_D - WALL - 1,
                    WALL + 6))
    cuts.append("c_charge")

    # --- Apply each cut individually (joining many tools confuses EXACT solver) ---
    for c in cuts:
        L.boolean("shell", c)

    # --- Battery zip-tie slots through floor ---
    BAT_X = WALL + 6
    BAT_Y = WALL + 6
    for i, x_offset in enumerate([-3, BAT_W + 1]):
        n = f"c_zip_{i}"
        L.make_box(n, ZIPTIE_SLOT_W, ZIPTIE_SLOT_L, WALL + 2,
                   loc=(BAT_X + x_offset,
                        BAT_Y + (BAT_D - ZIPTIE_SLOT_L) / 2,
                        -1))
        L.boolean("shell", n)

    # --- Rubber-foot dimples on underside ---
    for i, (fx, fy) in enumerate([
        (FOOT_INSET, FOOT_INSET),
        (OUT_W - FOOT_INSET, FOOT_INSET),
        (FOOT_INSET, OUT_D - FOOT_INSET),
        (OUT_W - FOOT_INSET, OUT_D - FOOT_INSET),
    ]):
        n = f"c_foot_{i}"
        L.make_cylinder(n, FOOT_DIA / 2, FOOT_DEPTH + 0.05, loc=(fx, fy, -0.05))
        L.boolean("shell", n)

    # --- Internal mounting features (added after cuts) ---
    additions = []

    # Corner posts (4× M3 self-tap)
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        po = f"post_o_{i}"
        ph = f"post_h_{i}"
        L.make_cylinder(po, POST_OD / 2, POST_H, loc=(px, py, WALL))
        L.make_cylinder(ph, POST_HOLE / 2, POST_H + 0.1,
                        loc=(px, py, WALL - 0.05))
        L.boolean(po, ph)
        additions.append(po)

    # Compression rib: a ring on the floor under where the lid's collar lands.
    # Lid collar will be at the lid's center (arcade button center). When
    # assembled, the collar's bottom face is at z = WALL + (cavity_h - collar_h).
    # The compression rib transfers force through the cavity floor.
    # Center of arcade button = center of bottom shell footprint
    rib_cx = OUT_W / 2
    rib_cy = OUT_D / 2
    rib_outer_d = 38
    rib_inner_d = 30
    rib_h = 4
    rib_o = "rib_o"
    rib_i = "rib_i"
    L.make_cylinder(rib_o, rib_outer_d / 2, rib_h, loc=(rib_cx, rib_cy, WALL))
    L.make_cylinder(rib_i, rib_inner_d / 2, rib_h + 0.1,
                    loc=(rib_cx, rib_cy, WALL - 0.05))
    L.boolean(rib_o, rib_i)
    additions.append(rib_o)

    # ESP32 mounting rails along right wall
    rail_h = 4
    rail_w = 2
    L.make_box("esp_r1", rail_w, ESP_LEN, rail_h, loc=(esp_x - rail_w, esp_y, WALL))
    L.make_box("esp_r2", rail_w, ESP_LEN, rail_h,
               loc=(esp_x + ESP_W, esp_y, WALL))
    additions += ["esp_r1", "esp_r2"]

    # TP4056 rails near back wall
    tp_y = OUT_D - WALL - TP_WID - 2
    L.make_box("tp_r1", rail_w, TP_WID + 2, 3, loc=(tp_x - rail_w, tp_y, WALL))
    L.make_box("tp_r2", rail_w, TP_WID + 2, 3, loc=(tp_x + TP_LEN, tp_y, WALL))
    additions += ["tp_r1", "tp_r2"]

    # MT3608 #1 (5V to ESP32) — lying flat on left side, behind battery
    mt1_x = WALL + 4
    mt1_y = WALL + BAT_D + 8
    L.make_box("mt1_r1", MT_LEN + 2, rail_w, 3,
               loc=(mt1_x - 1, mt1_y - rail_w, WALL))
    L.make_box("mt1_r2", MT_LEN + 2, rail_w, 3,
               loc=(mt1_x - 1, mt1_y + MT_WID, WALL))
    additions += ["mt1_r1", "mt1_r2"]

    # MT3608 #2 (12V LED) — lying flat on left side, behind MT1
    mt2_y = mt1_y + MT_WID + 6
    if mt2_y + MT_WID < OUT_D - WALL - 5:
        L.make_box("mt2_r1", MT_LEN + 2, rail_w, 3,
                   loc=(mt1_x - 1, mt2_y - rail_w, WALL))
        L.make_box("mt2_r2", MT_LEN + 2, rail_w, 3,
                   loc=(mt1_x - 1, mt2_y + MT_WID, WALL))
        additions += ["mt2_r1", "mt2_r2"]

    if additions:
        L.join_into("shell", additions)

    bpy.context.active_object.name = NAME
    return NAME


# Build
name = build()

# Stats
o = bpy.data.objects[name]
verts = o.data.vertices
bbox_min = [round(min(v.co[i] for v in verts), 2) for i in range(3)]
bbox_max = [round(max(v.co[i] for v in verts), 2) for i in range(3)]

# Export STL
stl_path = os.path.join(OUT_DIR, "buzzer-bottom.stl")
L.export_stl(name, stl_path)

# Render PNG
png_path = os.path.join(OUT_DIR, "buzzer-bottom.png")
R.render_part(name, color=(0.20, 0.45, 0.75), out_path=png_path)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": [bbox_min, bbox_max],
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
