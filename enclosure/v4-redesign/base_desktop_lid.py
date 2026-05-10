"""Desktop base lid (v4) — back cover for the wedge.
The wedge is printed with its open bottom (Z=0) up, so the lid is what
covers that face after components are installed. Flat plate matching
the wedge footprint with a 2mm registration lip and 4 corner M3 holes.
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

W = 140        # match new wedge footprint (was 130 in v4)
D = 110
WALL = 2.5
LID_THICK = 3
LID_LIP = 2
LID_TOL = 0.4

POST_INSET = 9
SCREW_DIA = 3.2
CBORE_DIA = 6
CBORE_DEPTH = 2

NAME = "base_desktop_lid"


def build():
    L.reset_scene()
    # Base flat plate
    L.make_box("plate", W, D, LID_THICK, loc=(0, 0, 0))

    # Registration lip (extends UP from plate top, fits inside wedge cavity)
    lip_w = W - 2 * (WALL + LID_TOL)
    lip_d = D - 2 * (WALL + LID_TOL)
    L.make_box("lip_o", lip_w, lip_d, LID_LIP,
               loc=(WALL + LID_TOL, WALL + LID_TOL, LID_THICK))
    L.make_box("lip_i", lip_w - 2 * 1.25, lip_d - 2 * 1.25, LID_LIP + 0.5,
               loc=(WALL + LID_TOL + 1.25, WALL + LID_TOL + 1.25,
                    LID_THICK - 0.1))
    L.boolean("lip_o", "lip_i")
    L.join_into("plate", ["lip_o"])

    # Corner screw holes through plate, with counterbore on bottom face
    post_xs = [WALL + POST_INSET, W - WALL - POST_INSET]
    post_ys = [WALL + POST_INSET, D - WALL - POST_INSET]
    for i, (px, py) in enumerate([(x, y) for x in post_xs for y in post_ys]):
        nh = f"c_screw_{i}"
        nb = f"c_cbore_{i}"
        L.make_cylinder(nh, SCREW_DIA / 2, LID_THICK + LID_LIP + 4,
                        loc=(px, py, -1))
        L.boolean("plate", nh)
        # Counterbore on the underside (Z=0 face) so screw heads sit flush
        L.make_cylinder(nb, CBORE_DIA / 2, CBORE_DEPTH + 0.05,
                        loc=(px, py, -0.05))
        L.boolean("plate", nb)

    bpy.context.active_object.name = NAME
    return NAME


name = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]
stl_path = os.path.join(LIB, "base-desktop-lid.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-desktop-lid.png")
R.render_part(name, color=(0.55, 0.55, 0.60), out_path=png_path, cam_z_mul=2.5)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
