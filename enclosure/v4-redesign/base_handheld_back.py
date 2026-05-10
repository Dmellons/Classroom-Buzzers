"""Handheld base — back half (v5).
Pistol/wand grip outline.

Outer profile (Y axis: 0 = grip tip, increasing toward head):
  HEAD:   75 wide x 90 long, contains TFT + buttons (in front half)
  TAPER:  12mm transition
  GRIP:   50 wide x 80 long, narrower handle
  Total length: 182mm. Thickness: 11mm (this half)

Improvements over v4:
- Pistol-grip outline (no longer rectangular)
- microSD slot relocated, screw bosses repositioned to NEVER intersect it
- Lanyard cross-bore at grip tip
- Dot-grid texture only on grip section side walls
- USB-C on bottom edge (grip tip)
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

# Pistol outline
HEAD_W = 75
HEAD_D = 90
TAPER_LEN = 12
GRIP_W = 50
GRIP_D = 80
TOTAL_D = HEAD_D + TAPER_LEN + GRIP_D     # 182
H = 11
WALL = 2.5
CORNER_R = 8
TAPER_R = 4

# Y boundaries
GRIP_BASE_Y = 0
GRIP_TOP_Y = GRIP_D
TAPER_TOP_Y = GRIP_D + TAPER_LEN
HEAD_TOP_Y = TAPER_TOP_Y + HEAD_D

# microSD slot — placed in head section, on RIGHT wall
SD_W = WALL + 2
SD_LEN = 30
SD_HEIGHT = 5
SD_Y_CENTER = TAPER_TOP_Y + HEAD_D / 2 + 5  # roughly mid-head, slightly back of center
SD_Z = 4
# Slot Y range:
SD_Y_MIN = SD_Y_CENTER - SD_LEN / 2
SD_Y_MAX = SD_Y_CENTER + SD_LEN / 2

# USB-C on bottom edge (at Y=0, X=W/2 of grip section)
GRIP_INSET = (HEAD_W - GRIP_W) / 2     # 12.5
USBC_W = 12
USBC_H = 8
USBC_Z = 2

# Lanyard cross-bore at grip tip
LANYARD_DIA = 5
LANYARD_Y = 8
LANYARD_Z = H / 2

POST_OD = 5.5
POST_HOLE_CLEAR = 3.2
CBORE_DIA = 6
CBORE_DEPTH = 2

NAME = "base_handheld_back"


def boss_outside_sd(px, py, sd_y_min, sd_y_max, head_right_x):
    """Returns True if a boss at (px, py) does NOT intersect the SD slot zone.
    SD slot is on the right wall of the head section (X near HEAD_W),
    extending from sd_y_min to sd_y_max in Y.
    A boss intersects if it's near the right wall AND its Y is within
    the slot's Y range (with margin)."""
    margin = POST_OD / 2 + 2
    if abs(px - head_right_x) < margin:  # boss is near the right wall
        if (sd_y_min - margin) <= py <= (sd_y_max + margin):
            return False
    return True


def build():
    L.reset_scene()

    # Outer pistol outline extruded to half height
    L.make_pistol_outline("shell", head_w=HEAD_W, head_d=HEAD_D,
                           taper_len=TAPER_LEN, grip_w=GRIP_W, grip_d=GRIP_D,
                           height=H, corner_r=CORNER_R, taper_r=TAPER_R)

    # Inner cavity: smaller pistol outline offset by WALL on all sides,
    # extruded from Z=WALL up to Z=H+1 (cuts through top) so we get a
    # WALL-thick floor plus walls.
    L.make_pistol_outline("cav", head_w=HEAD_W - 2 * WALL,
                           head_d=HEAD_D - 2 * WALL,
                           taper_len=TAPER_LEN, grip_w=GRIP_W - 2 * WALL,
                           grip_d=GRIP_D - 2 * WALL,
                           height=H + 1,
                           corner_r=max(CORNER_R - WALL, 1),
                           taper_r=max(TAPER_R - WALL, 0.5),
                           loc=(WALL, WALL, WALL))
    L.boolean("shell", "cav")

    # USB-C on bottom edge (Y=0 face, in grip section)
    # Grip section bottom edge spans X=GRIP_INSET to X=GRIP_INSET+GRIP_W
    # Center on grip center: X = HEAD_W/2 (since pistol is symmetric about HEAD_W/2)
    L.make_box("c_usbc", USBC_W, WALL + 2, USBC_H,
               loc=(HEAD_W / 2 - USBC_W / 2, -1, WALL + USBC_Z))
    L.boolean("shell", "c_usbc")

    # microSD slot on right wall of head section
    L.make_box("c_sd", SD_W, SD_LEN, SD_HEIGHT,
               loc=(HEAD_W - WALL - 1, SD_Y_MIN, WALL + 2))
    L.boolean("shell", "c_sd")

    # Lanyard cross-bore at grip tip
    L.make_cylinder("c_lan", LANYARD_DIA / 2, GRIP_W + 2,
                    loc=(GRIP_INSET - 1, LANYARD_Y, LANYARD_Z), axis='X')
    L.boolean("shell", "c_lan")

    # ----- Mounting bosses (6 points), positioned to NEVER intersect SD slot -----
    # Layout strategy:
    #  - Head: 4 points around the head (corners), but the right-side ones
    #    must avoid the SD-slot Y zone.
    #  - Grip: 2 points near grip corners.
    head_right_x = HEAD_W - WALL - 6  # boss inset from right wall
    head_left_x = WALL + 6
    grip_right_x = GRIP_INSET + GRIP_W - WALL - 6
    grip_left_x = GRIP_INSET + WALL + 6

    # Head bosses: 2 left side (top + middle for stiffness), 2 right side
    # AVOIDING the SD slot Y range
    head_y_top = HEAD_TOP_Y - 12
    head_y_above_sd = SD_Y_MAX + POST_OD / 2 + 4   # above SD slot
    head_y_below_sd = SD_Y_MIN - POST_OD / 2 - 4   # below SD slot
    # Make sure below-SD position is still in head section (above taper)
    if head_y_below_sd < TAPER_TOP_Y + 6:
        head_y_below_sd = TAPER_TOP_Y + 6
    head_y_left_mid = (head_y_top + TAPER_TOP_Y + 6) / 2

    raw_post_pts = [
        (head_left_x, head_y_top, "head-TL"),
        (head_right_x, head_y_top, "head-TR"),
        (head_left_x, head_y_left_mid, "head-LM"),
        (head_right_x, head_y_below_sd, "head-RB"),  # below SD slot
        (grip_left_x, GRIP_BASE_Y + 14, "grip-BL"),
        (grip_right_x, GRIP_BASE_Y + 14, "grip-BR"),
    ]

    # Verify NO boss intersects the SD slot zone
    sd_check_passed = True
    for (px, py, label) in raw_post_pts:
        if not boss_outside_sd(px, py, SD_Y_MIN, SD_Y_MAX, head_right_x):
            print(f"FAIL: boss {label} at ({px},{py}) intersects SD slot zone "
                  f"Y={SD_Y_MIN}..{SD_Y_MAX}")
            sd_check_passed = False
    assert sd_check_passed, "Boss positions intersect microSD slot — fix layout!"

    additions = []
    for i, (px, py, label) in enumerate(raw_post_pts):
        po = f"post_{i}"
        ph = f"hole_{i}"
        cbore = f"cb_{i}"
        # Boss extends from interior floor to top
        L.make_cylinder(po, POST_OD / 2, H - WALL, loc=(px, py, WALL))
        # Through-hole through entire half + boss
        L.make_cylinder(ph, POST_HOLE_CLEAR / 2, H + 2, loc=(px, py, -1))
        L.boolean(po, ph)
        # Counterbore on back (Z=0) face
        L.make_cylinder(cbore, CBORE_DIA / 2, CBORE_DEPTH + 0.05,
                        loc=(px, py, -0.05))
        L.boolean("shell", cbore)
        additions.append(po)

    if additions:
        L.join_into("shell", additions)

    # Dot-grid grip texture on back face exterior — ONLY on grip section,
    # not the head section
    DOT_DIA = 1.8
    DOT_DEPTH = 0.7
    DOT_SPACING = 4.5
    GRIP_PATCH_W = 36   # narrower than grip width to leave margin
    GRIP_PATCH_H = 50
    # Single patch centered on grip section (on the back face = Z=0)
    grip_cx = HEAD_W / 2 - GRIP_PATCH_W / 2
    grip_cy = GRIP_BASE_Y + (GRIP_D - GRIP_PATCH_H) / 2 + 5
    grid_name = "dotgrid"
    if L.make_dot_grid(grid_name, area_w=GRIP_PATCH_W, area_h=GRIP_PATCH_H,
                        dot_dia=DOT_DIA, dot_depth=DOT_DEPTH,
                        spacing=DOT_SPACING,
                        base_loc=(grip_cx, grip_cy, -DOT_DEPTH / 2)):
        L.boolean("shell", grid_name)

    bpy.context.active_object.name = NAME
    return NAME, raw_post_pts


name, post_pts = build()
o = bpy.data.objects[name]
verts = o.data.vertices
bbox = [
    [round(min(v.co[i] for v in verts), 2) for i in range(3)],
    [round(max(v.co[i] for v in verts), 2) for i in range(3)],
]

stl_path = os.path.join(LIB, "base-handheld-back.stl")
L.export_stl(name, stl_path)
png_path = os.path.join(LIB, "base-handheld-back.png")
R.render_part(name, color=(0.25, 0.55, 0.35), out_path=png_path, cam_z_mul=2.0)

result = {
    "name": name,
    "verts": len(verts),
    "faces": len(o.data.polygons),
    "bbox": bbox,
    "stl_kb": round(os.path.getsize(stl_path) / 1024, 1),
    "png_kb": round(os.path.getsize(png_path) / 1024, 1),
    "boss_positions": [(round(p[0], 1), round(p[1], 1), p[2]) for p in post_pts],
    "sd_y_range": [SD_Y_MIN, SD_Y_MAX],
}
