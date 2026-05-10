"""Desktop teacher/host base — wedge form factor (v4).
Single-piece wedge that prints flat on its open back face.
Front edge 25mm tall, back edge 45mm tall, depth 110mm, width 130mm.
Top surface tilts ~10deg toward user, naturally angles the OLED display.

Cutouts:
  - OLED window 23 x 13mm through the angled top, perpendicular to surface
  - 2x 12mm momentary buttons through angled top (with shape indicators)
  - USB-C 12 x 8mm on rear (vertical) wall
  - microSD 30 x 5mm on right wall
  - 4 corner posts (M3 self-tap into 2.5mm pilot)
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
from mathutils import Vector

W = 130          # X — width
D = 110          # Y — depth front-to-back
H_FRONT = 25     # Z at Y=0
H_BACK = 45      # Z at Y=D
WALL = 2.5
R_OUT = 5

OLED_W, OLED_H = 23, 13
USBC_W, USBC_H = 12, 8
USBC_Z = 8
SD_SLOT_W, SD_SLOT_H = 30, 5
SD_SLOT_Z = 12

BTN_DIA = 12
BTN_SPACING = 32         # X distance between the two button centers
BTN_Y_FROM_FRONT = 22    # Y distance from front edge to button row (along top)

# Indicator deboss shapes
TRIANGLE_SIZE = 6        # Start indicator
SQUARE_SIZE = 5          # Reset indicator
INDICATOR_DEPTH = 0.6
INDICATOR_OFFSET = 10    # X offset from button center to indicator center

POST_OD = 6
POST_HOLE = 2.5
POST_INSET = 9
POST_H_FRONT = H_FRONT - 8   # post height at front (shorter)
POST_H_BACK = H_BACK - 8     # post height at back (taller)

NAME = "base_desktop_wedge"


def angled_top_z(y):
    """Z coordinate of the angled top surface at given Y."""
    return H_FRONT + (H_BACK - H_FRONT) * (y / D)


def build():
    L.reset_scene()

    # Build the hollow wedge shell
    L.make_wedge_shell("shell", w=W, d=D, h_front=H_FRONT, h_back=H_BACK,
                       r=R_OUT, wall=WALL)

    slope_angle = math.atan2(H_BACK - H_FRONT, D)  # ~10deg

    # --- Cutouts ---

    # USB-C on back wall (vertical wall at Y=D)
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(W / 2 - USBC_W / 2, D - WALL - 1, USBC_Z))
    L.boolean("shell", "c_usbc")

    # microSD on right wall (X=W)
    L.make_box("c_sd", WALL + 2, SD_SLOT_W, SD_SLOT_H,
               loc=(W - WALL - 1, D / 2 - SD_SLOT_W / 2, SD_SLOT_Z))
    L.boolean("shell", "c_sd")

    # OLED window — through angled top surface, perpendicular to surface
    # Position: centered along X, around Y = D/2 + 10 (slightly back of center)
    oled_y = D / 2 + 10
    oled_top_z = angled_top_z(oled_y)
    # Build cut box and rotate around X axis to align with slope
    cut_d = WALL + 4
    L.make_box("c_oled", OLED_W, OLED_H, cut_d,
               loc=(W / 2 - OLED_W / 2, -OLED_H / 2, -cut_d / 2))
    co = bpy.data.objects["c_oled"]
    co.rotation_mode = 'XYZ'
    co.rotation_euler = (slope_angle, 0, 0)
    co.location = (W / 2, oled_y, oled_top_z)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_oled")

    # Buttons through angled top
    btn_y = BTN_Y_FROM_FRONT
    btn_top_z = angled_top_z(btn_y)
    btn_xs = [W / 2 - BTN_SPACING / 2, W / 2 + BTN_SPACING / 2]
    for i, bx in enumerate(btn_xs):
        n = f"c_btn_{i}"
        L.make_cylinder(n, BTN_DIA / 2, H_BACK + 5,
                        loc=(bx, btn_y, -2), axis='Z')
        # Rotate cylinder to align with surface normal
        co = bpy.data.objects[n]
        co.rotation_mode = 'XYZ'
        co.rotation_euler = (slope_angle, 0, 0)
        co.location = (bx, btn_y, btn_top_z - (H_BACK + 5) / 2 + 1)
        # Adjust so cylinder spans through the wall and well into the cavity
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
        L.boolean("shell", n)

    # Indicator deboss: triangle for left button (Start), square for right (Reset)
    # These sit on the angled top surface beside each button hole.
    # Left button: triangle indicator to its left (-X side)
    # We approximate triangle with a thin triangular prism (3 verts -> face)
    import bmesh
    me_t = bpy.data.meshes.new("indicator_tri_mesh")
    bm = bmesh.new()
    s = TRIANGLE_SIZE
    v1 = bm.verts.new((-s/2, -s/2*0.866, 0))
    v2 = bm.verts.new((s/2, -s/2*0.866, 0))
    v3 = bm.verts.new((0, s/2*0.866, 0))
    bm.faces.new([v1, v2, v3])
    bm.normal_update()
    geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((0, 0, INDICATOR_DEPTH + 0.5)), verts=ev)
    bm.normal_update()
    bm.to_mesh(me_t)
    bm.free()
    tri_obj = bpy.data.objects.new("c_tri", me_t)
    bpy.context.collection.objects.link(tri_obj)
    tri_obj.rotation_mode = 'XYZ'
    tri_obj.rotation_euler = (slope_angle, 0, 0)
    tri_obj.location = (btn_xs[0] - INDICATOR_OFFSET, btn_y,
                        btn_top_z - INDICATOR_DEPTH / 2)
    bpy.context.view_layer.objects.active = tri_obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_tri")

    # Right button: square indicator
    L.make_box("c_sq", SQUARE_SIZE, SQUARE_SIZE, INDICATOR_DEPTH + 0.5,
               loc=(-SQUARE_SIZE / 2, -SQUARE_SIZE / 2,
                    -(INDICATOR_DEPTH + 0.5) / 2))
    sq = bpy.data.objects["c_sq"]
    sq.rotation_mode = 'XYZ'
    sq.rotation_euler = (slope_angle, 0, 0)
    sq.location = (btn_xs[1] + INDICATOR_OFFSET, btn_y,
                   btn_top_z - INDICATOR_DEPTH / 2)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("shell", "c_sq")

    # --- Posts (M3 self-tap, varying height per corner since wedge floor is flat) ---
    additions = []
    post_xs = [WALL + POST_INSET, W - WALL - POST_INSET]
    post_corners = [
        (post_xs[0], WALL + POST_INSET, H_FRONT - 8),
        (post_xs[1], WALL + POST_INSET, H_FRONT - 8),
        (post_xs[0], D - WALL - POST_INSET, H_BACK - 8),
        (post_xs[1], D - WALL - POST_INSET, H_BACK - 8),
    ]
    for i, (px, py, ph) in enumerate(post_corners):
        po = f"post_o_{i}"
        phole = f"post_h_{i}"
        L.make_cylinder(po, POST_OD / 2, ph, loc=(px, py, 0))  # post extends from floor
        # Wait the wedge is OPEN-TOP so floor is the back face which is at Z=0?
        # Actually wedge is built with rectangular footprint Z=0..H_BACK with
        # outer back face at Z=H_BACK. There's no closed floor — the opening
        # is at Y=D (the back wall). Hmm.
        # Re-read: make_wedge_shell builds outer prism Z=0..h_back, slices off
        # top with sloped plane, hollows interior. Open face is the back (Y=d).
        # So the "floor" when assembled (resting on desk) is the bottom face
        # at Z=0. Posts should go up FROM the bottom interior (Z=WALL) to
        # the top interior surface (angled, at Z=angled_top_z(py) - WALL/cos).
        # Recompute:
        pass

    # Redo posts: anchored to bottom interior floor (Z=WALL), height = ceiling - WALL
    for i, (px, py, _) in enumerate(post_corners):
        po = f"post_o_{i}"
        phole = f"post_h_{i}"
        # Remove already-created post if present
        if po in bpy.data.objects:
            bpy.data.objects.remove(bpy.data.objects[po], do_unlink=True)
        ceiling_z = angled_top_z(py)
        ph_actual = max(ceiling_z - WALL - WALL - 1, 5)  # leave 1mm gap below ceiling
        L.make_cylinder(po, POST_OD / 2, ph_actual, loc=(px, py, WALL))
        L.make_cylinder(phole, POST_HOLE / 2, ph_actual + 0.1,
                        loc=(px, py, WALL - 0.05))
        L.boolean(po, phole)
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

stl_path = os.path.join(LIB, "base-desktop-wedge.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-desktop-wedge.png")
R.render_part(name, color=(0.30, 0.30, 0.35), out_path=png_path)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
