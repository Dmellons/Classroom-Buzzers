"""Buzzer (button unit) top half / lid.
Internal cavity 80x100x15mm. Lid drops over bottom via 2mm overlap lip.
Top face has 28mm hole for 60mm arcade button (centered).
Corner posts have M3 clearance holes that align with the bottom posts."""

import sys
LIB = "/home/david/Classroom-Buzzers/enclosure/v3-blender"
if LIB not in sys.path:
    sys.path.insert(0, LIB)
import build_lib as L

# Match buzzer_bottom.py
W, D = 80, 100
WALL = 2.5
R_OUT = 5
R_IN = R_OUT - WALL / 2
OUT_W = W + 2 * WALL
OUT_D = D + 2 * WALL
LID_H = 15
LID_LIP = 2
LID_TOL = 0.4

ARCADE_HOLE_DIA = 28          # 60mm arcade button mounting hole
POST_INSET = 8
POST_HOLE_CLEAR = 3.2         # M3 clearance hole

def build():
    L.reset_scene()

    # Outer lid shell, flat closed top
    L.make_rounded_extrusion("shell", OUT_W, OUT_D, LID_H, R_OUT)
    # Hollow inside, leaves WALL closed top
    L.make_rounded_extrusion("cav", W, D, LID_H - WALL + 1, R_IN,
                             loc=(WALL, WALL, -1))
    L.boolean("shell", "cav")

    # Inner lip that drops into bottom shell (registration)
    # Lip outer dim = box internal - tolerance, inner dim = ... we just make a thin wall lip
    lip_w = W - 2 * LID_TOL
    lip_d = D - 2 * LID_TOL
    lip_inner_w = lip_w - 2.5
    lip_inner_d = lip_d - 2.5
    L.make_rounded_extrusion("lip_o", lip_w, lip_d, LID_LIP, R_IN - LID_TOL,
                             loc=(WALL + LID_TOL, WALL + LID_TOL, -LID_LIP))
    L.make_rounded_extrusion("lip_i", lip_inner_w, lip_inner_d, LID_LIP + 0.5,
                             max(R_IN - LID_TOL - 1.25, 1),
                             loc=(WALL + LID_TOL + 1.25, WALL + LID_TOL + 1.25, -LID_LIP - 0.1))
    L.boolean("lip_o", "lip_i")
    L.join_into("shell", ["lip_o"])

    cuts = []

    # Arcade button hole through closed top (center)
    L.make_cylinder("c_arcade", ARCADE_HOLE_DIA / 2, LID_H + 4,
                    loc=(OUT_W / 2, OUT_D / 2, -2))
    cuts.append("c_arcade")

    # Corner screw clearance holes (M3 clearance, aligned with bottom posts)
    post_xs = [WALL + POST_INSET, OUT_W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, OUT_D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        n = f"c_screw_{i}"
        L.make_cylinder(n, POST_HOLE_CLEAR / 2, LID_H + 4, loc=(px, py, -LID_LIP - 1))
        cuts.append(n)
        # Counterbore on top so screw head sits flush
        nc = f"c_cbore_{i}"
        L.make_cylinder(nc, 3.0, 2.0, loc=(px, py, LID_H - 2.0 + 0.01))
        cuts.append(nc)

    L.join_into(cuts[0], cuts[1:])
    L.boolean("shell", cuts[0])

    L.select_only("shell")
    import bpy
    bpy.context.active_object.name = "buzzer_top"
    return "buzzer_top"


name = build()
import os, bpy
out_stl = "/home/david/Classroom-Buzzers/enclosure/v3-blender/buzzer-top.stl"
L.export_stl(name, out_stl)
o = bpy.data.objects[name]
result = {
    "name": name,
    "verts": len(o.data.vertices),
    "faces": len(o.data.polygons),
    "bbox": [
        [round(min(v.co[i] for v in o.data.vertices), 3) for i in range(3)],
        [round(max(v.co[i] for v in o.data.vertices), 3) for i in range(3)],
    ],
    "stl_kb": round(os.path.getsize(out_stl) / 1024, 1) if os.path.exists(out_stl) else None,
}
