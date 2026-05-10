"""Buzzer display bezel (v4) — angled 30deg from vertical.
Right-triangular prism with the angled face holding the 0.91" OLED.
Glues/screws onto the front-top edge of the buzzer bottom shell;
the bottom-shell cutout passes the OLED PCB through.

Print orientation: angled face DOWN on bed (= cleanest finish on
the visible surface, zero supports). The triangular sides + back
face print up from the bed without any overhang.

Outer footprint (back face): 35 wide x 22 tall.
Depth (back-to-front along bottom): 14mm.
Angle: atan(14/22) = ~32.5 deg from vertical. (Close enough to 30.)
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
import bmesh
from mathutils import Vector

# Outer dimensions
W_BACK = 35    # X — width across the back face
H_BACK = 22    # Z — height of back face
DEPTH = 14     # Y — distance from back face to bottom front edge
THICK = 3      # thickness of the bezel (skin on angled face)

# OLED window
OLED_W = 23
OLED_H = 12

# Power LED
LED_DIA = 3

# Mounting screw holes (through back face for retention into bottom shell)
SCREW_DIA = 3.2
SCREW_INSET_X = 5
SCREW_Z = 4   # near bottom for stability

NAME = "buzzer_bezel"


def build():
    L.reset_scene()

    # Build the wedge as a triangular prism via bmesh: extrude a triangle
    # face along X to give the prism width.
    me = bpy.data.meshes.new(NAME + "_mesh")
    bm = bmesh.new()
    # Triangle in YZ plane: (0,0), (DEPTH,0), (0,H_BACK)
    # Vertices for the back triangle face (X=0)
    v00 = bm.verts.new((0, 0, 0))
    vd0 = bm.verts.new((0, DEPTH, 0))
    v0h = bm.verts.new((0, 0, H_BACK))
    bm.faces.new([v00, vd0, v0h])
    bm.normal_update()
    geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((W_BACK, 0, 0)), verts=ev)
    bm.normal_update()
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new("wedge", me)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj

    # The angled face goes from (X, 0, H_BACK) to (X, DEPTH, 0) — i.e., from
    # the top-back edge to the front-bottom edge of the prism.
    # Length of angled face = sqrt(DEPTH^2 + H_BACK^2)
    angled_len = math.sqrt(DEPTH * DEPTH + H_BACK * H_BACK)
    angle = math.atan2(H_BACK, DEPTH)  # angle from horizontal at front-bottom

    # OLED window: cut through the angled face. We model the cut as a
    # rectangular box positioned in the bezel coordinate system, oriented
    # along the angled face's normal.
    # Center of angled face at (X=W_BACK/2, Y=DEPTH/2, Z=H_BACK/2)
    cx = W_BACK / 2
    cy = DEPTH / 2
    cz = H_BACK / 2

    # Build cut box (axis-aligned), then rotate around X axis so its
    # length-axis aligns with the angled face direction.
    # Aligned-cut: length=OLED_H along the angled direction, width=OLED_W along X,
    # depth = THICK + 4 to ensure clean cut.
    cut_w = OLED_W
    cut_l_along = OLED_H
    cut_d = THICK + 4
    L.make_box("c_oled", cut_w, cut_l_along, cut_d,
               loc=(cx - cut_w / 2, -cut_l_along / 2, -cut_d / 2))
    co = bpy.data.objects["c_oled"]
    # Rotate around X axis to align with angled face
    # Angled face normal points outward in +Y' direction (rotated)
    # Standard angle from horizontal at the front-bottom is `angle`.
    # The angled face direction (from front-bottom to back-top) makes angle
    # `pi/2 - angle` with the +Y axis... wait let me redo this:
    # Front-bottom corner: (any X, DEPTH, 0). Back-top corner: (any X, 0, H_BACK).
    # Vector from front-bottom to back-top: (0, -DEPTH, H_BACK).
    # Length = angled_len.
    # The angle this vector makes with +Z axis = atan(DEPTH / H_BACK).
    # The angle with +Y axis (in YZ plane) = atan(H_BACK / DEPTH) = angle (defined above).
    # We want to rotate the cut box so its "depth" direction (originally Z)
    # aligns with the angled-face NORMAL (which points outward away from the prism).
    # The outward normal of the angled face in the YZ plane is
    # (DEPTH, H_BACK) normalized — points up-and-out.
    # Rotation: rotate around X axis by angle (counterclockwise when looking along +X)
    co.rotation_mode = 'XYZ'
    co.rotation_euler = (math.atan2(DEPTH, H_BACK), 0, 0)
    # Place at center of angled face
    co.location = (cx, cy, cz)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("wedge", "c_oled")

    # Power LED hole — small cylinder, axis along the angled-face normal
    led_offset_along = OLED_H / 2 + 5  # 5mm above OLED window along the face
    # Position along the angled face: starting from face center, move by
    # led_offset_along in the +face-direction (toward back-top).
    # Face direction unit vector in YZ: (-DEPTH/L, H_BACK/L)
    face_dir_y = -DEPTH / angled_len
    face_dir_z = H_BACK / angled_len
    led_y = cy + face_dir_y * led_offset_along
    led_z = cz + face_dir_z * led_offset_along
    L.make_cylinder("c_led", LED_DIA / 2, THICK + 4,
                    loc=(cx, led_y - (THICK + 4) / 2, led_z), axis='Y')
    led_obj = bpy.data.objects["c_led"]
    led_obj.rotation_mode = 'XYZ'
    led_obj.rotation_euler = (-math.atan2(DEPTH, H_BACK), 0, 0)
    led_obj.location = (cx, led_y, led_z)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("wedge", "c_led")

    # Two retention screw holes through the back face (Y=0 plane), going
    # from back to front through the prism, in the bottom region.
    for i, sx in enumerate([SCREW_INSET_X, W_BACK - SCREW_INSET_X]):
        n = f"c_screw_{i}"
        L.make_cylinder(n, SCREW_DIA / 2, DEPTH + 4,
                        loc=(sx, -2, SCREW_Z), axis='Y')
        L.boolean("wedge", n)

    # Hollow the back side a bit (small interior pocket) so the OLED PCB
    # has clearance to sit recessed behind the angled face.
    pocket_w = OLED_W + 4
    pocket_l_along = OLED_H + 4
    pocket_d = 2.5  # depth into the wedge (along inward normal)
    L.make_box("c_pocket", pocket_w, pocket_l_along, pocket_d + 0.5,
               loc=(cx - pocket_w / 2, -pocket_l_along / 2, -pocket_d / 2))
    po = bpy.data.objects["c_pocket"]
    po.rotation_mode = 'XYZ'
    po.rotation_euler = (math.atan2(DEPTH, H_BACK), 0, 0)
    po.location = (cx, cy + face_dir_y * (-pocket_d / 2),
                   cz + face_dir_z * (-pocket_d / 2))
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=False)
    L.boolean("wedge", "c_pocket")

    bpy.data.objects["wedge"].name = NAME
    return NAME


name = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]

stl_path = os.path.join(LIB, "buzzer-bezel.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "buzzer-bezel.png")
R.render_part(name, color=(0.95, 0.85, 0.35), out_path=png_path)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
}
