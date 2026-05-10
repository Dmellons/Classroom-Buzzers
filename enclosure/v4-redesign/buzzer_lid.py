"""Buzzer lid (v4) — flat top with reinforced arcade-button collar.
Outer: 105 x 125 x 17mm.
- 6mm thick top deck (was 2.5mm in v3) for slam resistance
- 10mm-tall reinforcement collar on underside ringing the 28mm hole
- Arcade button hole offset toward back so front-top is clear for the
  bezel mounted on the bottom shell's front wall
- 2mm registration lip
- 4 corner M3 clearance + counterbores
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

# Match buzzer_bottom dims
W, D = 100, 120
WALL = 2.5
R_OUT = 5
R_IN = R_OUT - WALL / 2
OUT_W = W + 2 * WALL
OUT_D = D + 2 * WALL

DECK_THICK = 6                # thicker top deck for slam resistance
INTERIOR_H = 11               # cavity inside lid above bottom shell rim
LID_H = DECK_THICK + INTERIOR_H  # 17mm total lid height
LID_LIP = 2
LID_TOL = 0.4

ARCADE_HOLE_DIA = 28
ARCADE_BTN_OFFSET_Y = 18      # arcade button offset toward back (positive = back)
                              # 0 = centered; positive moves toward Y=OUT_D

COLLAR_OD = 32
COLLAR_ID = 28                # matches arcade hole
COLLAR_H = 10

POST_INSET = 9
POST_HOLE_CLEAR = 3.2
POST_CBORE_DIA = 6
POST_CBORE_DEPTH = 2

NAME = "buzzer_lid"


def build():
    L.reset_scene()

    # Outer shell + interior cavity (cuts through bottom open face)
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, LID_H, R_OUT)
    L.make_rounded_extrusion("cav", W, D, INTERIOR_H + 1, R_IN,
                             loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # --- Reinforced collar on underside ---
    # Hangs from interior face of deck (Z=INTERIOR_H) downward into cavity.
    arcade_cx = OUT_W / 2
    arcade_cy = OUT_D / 2 + ARCADE_BTN_OFFSET_Y
    L.make_collar("collar", inner_d=COLLAR_ID, outer_d=COLLAR_OD,
                  height=COLLAR_H,
                  loc=(arcade_cx, arcade_cy, INTERIOR_H - COLLAR_H))
    L.join_into("shell", ["collar"])

    # --- Registration lip ---
    lip_w = W - 2 * LID_TOL
    lip_d = D - 2 * LID_TOL
    L.make_rounded_extrusion("lip_o", lip_w, lip_d, LID_LIP,
                             max(R_IN - LID_TOL, 1),
                             loc=(WALL + LID_TOL, WALL + LID_TOL, -LID_LIP))
    # Hollow the lip
    L.make_rounded_extrusion("lip_i", lip_w - 2.5, lip_d - 2.5, LID_LIP + 0.5,
                             max(R_IN - LID_TOL - 1.25, 0.5),
                             loc=(WALL + LID_TOL + 1.25,
                                  WALL + LID_TOL + 1.25,
                                  -LID_LIP - 0.1))
    L.boolean("lip_o", "lip_i")
    L.join_into("shell", ["lip_o"])

    # --- Cuts (apply individually to avoid EXACT solver issues) ---

    # Arcade button hole (through entire lid)
    L.make_cylinder("c_arcade", ARCADE_HOLE_DIA / 2, LID_H + 4,
                    loc=(arcade_cx, arcade_cy, -2))
    L.boolean("shell", "c_arcade")

    # Corner screw clearance + counterbore (on top face)
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        nh = f"c_screw_{i}"
        nb = f"c_cbore_{i}"
        L.make_cylinder(nh, POST_HOLE_CLEAR / 2, LID_H + 4,
                        loc=(px, py, -LID_LIP - 1))
        L.boolean("shell", nh)
        L.make_cylinder(nb, POST_CBORE_DIA / 2, POST_CBORE_DEPTH + 0.05,
                        loc=(px, py, LID_H - POST_CBORE_DEPTH))
        L.boolean("shell", nb)

    bpy.context.active_object.name = NAME
    return NAME


name = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]

stl_path = os.path.join(LIB, "buzzer-lid.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "buzzer-lid.png")
R.render_part(name, color=(0.90, 0.30, 0.25), out_path=png_path)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
