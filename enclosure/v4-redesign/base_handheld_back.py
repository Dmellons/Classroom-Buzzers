"""Handheld base — back half (v4).
65 x 165 x 11mm (one half of 22mm total assembled height).

Improvements over v3:
- Side-edge chamfers (4x4mm) — feels hand-shaped without un-printable curves
- Lanyard cross-bore (5mm) through bottom-corner thickness
- Dot-grid grip texture debossed on outer back face
- USB-C on bottom edge, microSD on right
- 6 mounting bosses with through-screw counterbores on outer face
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
import math

# Outer dims
W = 65       # X
D = 165      # Y (length, tip to base)
H = 11       # Z (this half's height)

WALL = 2.5
R_OUT = 6
R_IN = R_OUT - WALL / 2

CHAMFER = 4   # 4x4mm side-edge chamfer

# Cutouts
USBC_W, USBC_H = 12, 8
USBC_OFFSET_Z = 2     # from interior floor

SD_SLOT_W, SD_SLOT_H = 30, 5
SD_Y_FROM_BACK = 60   # from base end (Y=0)

# Lanyard
LANYARD_DIA = 5
LANYARD_INSET_X = 8       # from outer side
LANYARD_INSET_Y = 5       # from end
LANYARD_Z = H / 2

# Mounting bosses (6 points: 4 corners + 2 mid-sides)
POST_OD = 5.5
POST_HOLE_CLEAR = 3.2     # M3 clearance through this half
POST_INSET_X = 6
POST_INSET_Y = 12
CBORE_DIA = 6
CBORE_DEPTH = 2

# Dot-grid texture (back face exterior, 2 patches)
DOT_DIA = 1.8
DOT_DEPTH = 0.7
DOT_SPACING = 4
GRIP_PATCH_W = 50
GRIP_PATCH_H = 12

NAME = "base_handheld_back"


def build():
    L.reset_scene()

    # Outer rounded extrusion for the half
    L.make_rounded_extrusion("shell", W, D, H, R_OUT)
    # Inner cavity (cuts through top, leaves WALL-thick floor on bottom face)
    L.make_rounded_extrusion("cav", W - 2 * WALL, D - 2 * WALL,
                             H - WALL + 1, R_IN,
                             loc=(WALL, WALL, WALL))
    L.boolean("shell", "cav")

    # Side-edge chamfers: subtract two long triangular prisms running
    # along Y on left and right outer edges (top corner)
    # Left side chamfer at X=0 top edge
    import bmesh
    from mathutils import Vector
    for side, sign, x_anchor in [("L", 1, 0), ("R", -1, W)]:
        me = bpy.data.meshes.new(f"chamfer_{side}_mesh")
        bm = bmesh.new()
        # Triangle in XZ plane: at corner (x_anchor, 0, H), going +sign*CHAMFER in X and -CHAMFER in Z
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

    # USB-C on bottom edge (Y=0)
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(W / 2 - USBC_W / 2, -1, WALL + USBC_OFFSET_Z))
    L.boolean("shell", "c_usbc")

    # microSD on right edge (X=W)
    L.make_box("c_sd", WALL + 2, SD_SLOT_W, SD_SLOT_H,
               loc=(W - WALL - 1, SD_Y_FROM_BACK, WALL + 2))
    L.boolean("shell", "c_sd")

    # Lanyard cross-bore: through left bottom corner thickness, X-axis
    L.make_cylinder("c_lan", LANYARD_DIA / 2, W + 2,
                    loc=(-1, LANYARD_INSET_Y, LANYARD_Z), axis='X')
    # Position so it goes through one corner only (clip via second box)
    # Actually simpler: just put it through the corner thickness near the
    # bottom-left so the user can run a strap. Use a smaller length.
    bpy.data.objects["c_lan"].location.x = -1
    # Replace with a properly-sized cylinder through just the corner
    bpy.data.objects.remove(bpy.data.objects["c_lan"], do_unlink=True)
    L.make_cylinder("c_lan", LANYARD_DIA / 2, LANYARD_INSET_X * 2 + 4,
                    loc=(-2, LANYARD_INSET_Y, LANYARD_Z), axis='X')
    L.boolean("shell", "c_lan")

    # Mounting bosses with through-screw + counterbore on back
    post_pts = [
        (POST_INSET_X + WALL, POST_INSET_Y + WALL),
        (W - POST_INSET_X - WALL, POST_INSET_Y + WALL),
        (POST_INSET_X + WALL, D - POST_INSET_Y - WALL),
        (W - POST_INSET_X - WALL, D - POST_INSET_Y - WALL),
        (POST_INSET_X + WALL, D / 2),
        (W - POST_INSET_X - WALL, D / 2),
    ]
    additions = []
    for i, (px, py) in enumerate(post_pts):
        po = f"post_o_{i}"
        ph_through = f"post_t_{i}"
        cbore = f"post_cb_{i}"
        # Boss: 5.5mm OD cylinder from interior floor to top (full half height)
        L.make_cylinder(po, POST_OD / 2, H - WALL, loc=(px, py, WALL))
        # Through-hole through entire half + boss
        L.make_cylinder(ph_through, POST_HOLE_CLEAR / 2, H + 2,
                        loc=(px, py, -1))
        L.boolean(po, ph_through)
        # Counterbore on back (Z=0) face for screw head
        L.make_cylinder(cbore, CBORE_DIA / 2, CBORE_DEPTH + 0.05,
                        loc=(px, py, -0.05))
        L.boolean("shell", cbore)
        additions.append(po)

    if additions:
        L.join_into("shell", additions)

    # Dot-grid grip texture on back face exterior (Z=0)
    # Two small patches near the upper portion (where palm grips most)
    for side, side_x in [("L", 6), ("R", W - 6 - GRIP_PATCH_W)]:
        # Skip the right-side grip if it would overlap with the SD slot;
        # offset Y so patches are in upper grip zone
        patch_y = D / 2 + 30
        grid_name = f"dotgrid_{side}"
        if L.make_dot_grid(grid_name, area_w=GRIP_PATCH_W, area_h=GRIP_PATCH_H,
                            dot_dia=DOT_DIA, dot_depth=DOT_DEPTH,
                            spacing=DOT_SPACING,
                            base_loc=(side_x, patch_y, -DOT_DEPTH / 2)):
            L.boolean("shell", grid_name)

    bpy.context.active_object.name = NAME
    return NAME


name = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]
stl_path = os.path.join(LIB, "base-handheld-back.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-handheld-back.png")
R.render_part(name, color=(0.25, 0.55, 0.35), out_path=png_path, cam_z_mul=1.8)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
