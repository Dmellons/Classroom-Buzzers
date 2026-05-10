"""Reusable bmesh helpers for printable enclosure construction (v4).
Extends v3 with: wedge shell, dot-grid texture, dish indent, collar.
All dimensions in mm. Vertical-only corner rounding (flat top/bottom)
unless specified otherwise."""

import bpy
import bmesh
import math
from mathutils import Vector


# ---------------------------------------------------------------------------
# Scene management
# ---------------------------------------------------------------------------

def reset_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m, do_unlink=True)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat, do_unlink=True)
    for lt in list(bpy.data.lights):
        bpy.data.lights.remove(lt, do_unlink=True)
    for cam in list(bpy.data.cameras):
        bpy.data.cameras.remove(cam, do_unlink=True)


def select_only(name):
    bpy.ops.object.select_all(action='DESELECT')
    o = bpy.data.objects[name]
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    return o


# ---------------------------------------------------------------------------
# Primitive shapes
# ---------------------------------------------------------------------------

def make_rounded_extrusion(name, w, d, h, r, loc=(0, 0, 0), arc_segs=10):
    """Flat top/bottom box, rounded vertical corners. BBox min at loc."""
    me = bpy.data.meshes.new(name + "_mesh")
    bm = bmesh.new()
    centers = [(w - r, r), (w - r, d - r), (r, d - r), (r, r)]
    starts = [-math.pi / 2, 0, math.pi / 2, math.pi]
    pts = []
    for (cx, cy), s in zip(centers, starts):
        for i in range(arc_segs + 1):
            a = s + (math.pi / 2) * (i / arc_segs)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a), 0.0))
    seen = []
    for p in pts:
        if not seen or (abs(p[0] - seen[-1][0]) > 1e-6 or abs(p[1] - seen[-1][1]) > 1e-6):
            seen.append(p)
    if seen and abs(seen[0][0] - seen[-1][0]) < 1e-6 and abs(seen[0][1] - seen[-1][1]) < 1e-6:
        seen.pop()
    vs = [bm.verts.new(p) for p in seen]
    bm.faces.new(vs)
    bm.normal_update()
    geom = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    ev = [v for v in geom["geom"] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=Vector((0, 0, h)), verts=ev)
    bm.normal_update()
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(obj)
    obj.location = loc
    bpy.context.view_layer.objects.active = obj
    return name


def make_box(name, w, d, h, loc=(0, 0, 0)):
    """Plain axis-aligned box. BBox min at loc."""
    bpy.ops.mesh.primitive_cube_add(size=1)
    o = bpy.context.active_object
    o.name = name
    o.scale = (w, d, h)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.location = (loc[0] + w / 2, loc[1] + d / 2, loc[2] + h / 2)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    return name


def make_cylinder(name, r, h, loc=(0, 0, 0), axis='Z', segs=48):
    """Cylinder centered at loc; h along given axis, base at loc."""
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=h, vertices=segs)
    o = bpy.context.active_object
    o.name = name
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if axis == 'Z':
        o.location = (loc[0], loc[1], loc[2] + h / 2)
    elif axis == 'X':
        o.rotation_euler = (0, math.pi / 2, 0)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
        o.location = (loc[0] + h / 2, loc[1], loc[2])
    elif axis == 'Y':
        o.rotation_euler = (math.pi / 2, 0, 0)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
        o.location = (loc[0], loc[1] + h / 2, loc[2])
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    return name


def make_sphere(name, r, loc=(0, 0, 0), segs=32, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, segments=segs, ring_count=rings,
                                          location=loc)
    o = bpy.context.active_object
    o.name = name
    return name


# ---------------------------------------------------------------------------
# Boolean operations
# ---------------------------------------------------------------------------

def boolean(target, tool, op='DIFFERENCE', delete_tool=True):
    t = select_only(target)
    mod = t.modifiers.new("Bool", 'BOOLEAN')
    mod.operation = op
    mod.object = bpy.data.objects[tool]
    mod.solver = 'EXACT'
    bpy.ops.object.modifier_apply(modifier="Bool")
    if delete_tool:
        bpy.data.objects.remove(bpy.data.objects[tool], do_unlink=True)


def boolean_many(target, tools, op='DIFFERENCE', delete_tools=True):
    for t in tools:
        boolean(target, t, op=op, delete_tool=delete_tools)


def join_into(target, others):
    if not others:
        return
    select_only(target)
    for n in others:
        bpy.data.objects[n].select_set(True)
    bpy.ops.object.join()


# ---------------------------------------------------------------------------
# Higher-level shape builders (new in v4)
# ---------------------------------------------------------------------------

def make_collar(name, inner_d, outer_d, height, loc=(0, 0, 0)):
    """Ring/collar (cylinder minus inner cylinder) for arcade-button reinforcement."""
    o_name = name + "_o"
    i_name = name + "_i"
    make_cylinder(o_name, outer_d / 2, height, loc=loc)
    make_cylinder(i_name, inner_d / 2, height + 0.1,
                  loc=(loc[0], loc[1], loc[2] - 0.05))
    boolean(o_name, i_name)
    bpy.data.objects[o_name].name = name
    return name


def make_dot_grid(name, area_w, area_h, dot_dia, dot_depth,
                  spacing, base_loc=(0, 0, 0)):
    """Hex array of small cylinders covering area_w x area_h, returned as
    a single joined object centered at base_loc with bbox min at base_loc.
    Use this as a boolean DIFFERENCE tool to deboss a grip pattern."""
    dots = []
    rows = int(area_h / (spacing * math.sqrt(3) / 2)) + 1
    for row in range(rows + 1):
        y = row * spacing * math.sqrt(3) / 2
        if y > area_h:
            break
        x_start = (spacing / 2) if (row % 2) else 0
        cols = int((area_w - x_start) / spacing) + 1
        for col in range(cols + 1):
            x = x_start + col * spacing
            if x > area_w:
                break
            n = f"{name}_{row}_{col}"
            make_cylinder(n, dot_dia / 2, dot_depth,
                          loc=(base_loc[0] + x, base_loc[1] + y, base_loc[2]))
            dots.append(n)
    if not dots:
        return None
    join_into(dots[0], dots[1:])
    bpy.data.objects[dots[0]].name = name
    return name


def make_dish_indent(name, diameter, depth, loc=(0, 0, 0)):
    """Sphere-section indent. Subtract from a face to create a shallow dish.
    Ball is positioned so its lowest point dips `depth` below loc.z."""
    # For shallow dish (depth << diameter/2) we use a large sphere
    # whose chord at the cut plane has the requested diameter.
    # chord^2/4 + (R-depth)^2 = R^2  =>  R = (chord^2/4 + depth^2) / (2*depth)
    chord = diameter
    if depth <= 0:
        depth = 0.01
    R = (chord * chord / 4 + depth * depth) / (2 * depth)
    # Place sphere center above loc so bottom of sphere touches z=loc.z-depth
    center_z = loc[2] + R - depth
    make_sphere(name, R, loc=(loc[0], loc[1], center_z), segs=64, rings=32)
    return name


def make_wedge_shell(name, w, d, h_front, h_back, r, wall, arc_segs=10):
    """Hollow wedge shell built directly via bmesh (no booleans for the
    outer slope — much more reliable than rotated-cube subtraction).

    Outer geometry: rectangular footprint with vertical-rounded corners,
    flat bottom (Z=0), sloped top going from Z=h_front at Y=0 to Z=h_back
    at Y=d, vertical front/left/right/back walls. Open back-bottom face
    (the cavity opens out the back-and-bottom for printing flat on back).

    For simplicity we make sharp corners on the wedge (not rounded) since
    rounding the corners of a sloped-top prism is geometrically complex.
    `r` is accepted for API parity but ignored for this version; the
    chamfered look is achieved by adding small bevel modifier later if
    needed (or accept sharp corners — it's a desktop case, not a hand-held).
    """
    me = bpy.data.meshes.new(name + "_outer_mesh")
    bm = bmesh.new()

    # Outer 8 vertices
    v_fbl = bm.verts.new((0, 0, 0))            # front-bottom-left
    v_fbr = bm.verts.new((w, 0, 0))            # front-bottom-right
    v_bbl = bm.verts.new((0, d, 0))            # back-bottom-left
    v_bbr = bm.verts.new((w, d, 0))            # back-bottom-right
    v_ftl = bm.verts.new((0, 0, h_front))      # front-top-left
    v_ftr = bm.verts.new((w, 0, h_front))      # front-top-right
    v_btl = bm.verts.new((0, d, h_back))       # back-top-left
    v_btr = bm.verts.new((w, d, h_back))       # back-top-right

    # Outer faces (CCW outward normals)
    bm.faces.new([v_fbl, v_fbr, v_ftr, v_ftl])      # front (Y=0)
    bm.faces.new([v_bbr, v_bbl, v_btl, v_btr])      # back (Y=d)
    bm.faces.new([v_fbl, v_ftl, v_btl, v_bbl])      # left (X=0)
    bm.faces.new([v_fbr, v_bbr, v_btr, v_ftr])      # right (X=w)
    bm.faces.new([v_fbl, v_bbl, v_bbr, v_fbr])      # bottom (Z=0)
    bm.faces.new([v_ftl, v_ftr, v_btr, v_btl])      # sloped top

    # Inner cavity: offset by wall on all sides EXCEPT bottom (the bottom
    # opens out for printing). Inner sloped top sits `wall / cos(angle)`
    # below the outer slope (perpendicular wall thickness).
    angle = math.atan2(h_back - h_front, d)
    perp_inset = wall / math.cos(angle)
    ih_front = h_front - perp_inset
    ih_back = h_back - perp_inset

    # Inner cavity 8 verts (note: open at Z=0 means we just don't make
    # bottom face, but we still need a "floor" of the cavity — actually
    # for a hollow shell printed open-bottom-up, the bottom is the print
    # bed face and the case has NO floor on its bottom. Lid is the bottom.)
    iv_fbl = bm.verts.new((wall, wall, 0 - 0.5))
    iv_fbr = bm.verts.new((w - wall, wall, 0 - 0.5))
    iv_bbl = bm.verts.new((wall, d - wall, 0 - 0.5))
    iv_bbr = bm.verts.new((w - wall, d - wall, 0 - 0.5))
    iv_ftl = bm.verts.new((wall, wall, ih_front))
    iv_ftr = bm.verts.new((w - wall, wall, ih_front))
    iv_btl = bm.verts.new((wall, d - wall, ih_back))
    iv_btr = bm.verts.new((w - wall, d - wall, ih_back))

    # Inner faces (CCW with INWARD normals = reversed from outer convention)
    bm.faces.new([iv_fbl, iv_ftl, iv_ftr, iv_fbr])     # front
    bm.faces.new([iv_bbr, iv_btr, iv_btl, iv_bbl])     # back
    bm.faces.new([iv_fbl, iv_bbl, iv_btl, iv_ftl])     # left
    bm.faces.new([iv_fbr, iv_ftr, iv_btr, iv_bbr])     # right
    bm.faces.new([iv_ftl, iv_btl, iv_btr, iv_ftr])     # sloped top (inner)
    bm.faces.new([iv_fbl, iv_fbr, iv_bbr, iv_bbl])     # cavity bottom (closes the cavity)

    # Note: inner faces' winding above is "outward" from cavity perspective.
    # For a manifold solid, inner shell's normals should point INTO the
    # cavity (since they're the inside of the case). Bmesh auto-corrects
    # via recalc_face_normals, but we'll do it explicitly:
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])

    # The way to make this a proper shell (solid wall): merge outer + inner
    # via a bridge. Easier path: build the shell as a single solid by
    # subtracting inner from outer using boolean. We'll stop the bmesh
    # construction here, finalize as separate objects, and use boolean.
    bm.to_mesh(me)
    bm.free()
    obj_outer = bpy.data.objects.new(name + "_outer", me)
    bpy.context.collection.objects.link(obj_outer)
    bpy.context.view_layer.objects.active = obj_outer

    # Actually simpler: build outer and inner as separate solids, then bool diff.
    return _build_wedge_via_boolean(name, w, d, h_front, h_back, wall, angle)


def _build_wedge_via_boolean(name, w, d, h_front, h_back, wall, angle):
    """Build outer wedge and inner wedge as separate solids, boolean diff."""
    # Clear the partially-built mesh from make_wedge_shell
    if name + "_outer" in bpy.data.objects:
        bpy.data.objects.remove(bpy.data.objects[name + "_outer"], do_unlink=True)

    def _wedge_solid(obj_name, x_min, y_min, x_max, y_max,
                     z_floor, z_front, z_back):
        """Create a solid wedge prism via 8-vertex bmesh."""
        me = bpy.data.meshes.new(obj_name + "_mesh")
        bm = bmesh.new()
        v = [
            bm.verts.new((x_min, y_min, z_floor)),     # 0 fbl
            bm.verts.new((x_max, y_min, z_floor)),     # 1 fbr
            bm.verts.new((x_min, y_max, z_floor)),     # 2 bbl
            bm.verts.new((x_max, y_max, z_floor)),     # 3 bbr
            bm.verts.new((x_min, y_min, z_front)),     # 4 ftl
            bm.verts.new((x_max, y_min, z_front)),     # 5 ftr
            bm.verts.new((x_min, y_max, z_back)),      # 6 btl
            bm.verts.new((x_max, y_max, z_back)),      # 7 btr
        ]
        # All 6 faces, outward winding
        bm.faces.new([v[0], v[1], v[5], v[4]])         # front (Y=y_min)
        bm.faces.new([v[3], v[2], v[6], v[7]])         # back  (Y=y_max)
        bm.faces.new([v[0], v[4], v[6], v[2]])         # left  (X=x_min)
        bm.faces.new([v[1], v[3], v[7], v[5]])         # right (X=x_max)
        bm.faces.new([v[0], v[2], v[3], v[1]])         # bottom (Z=z_floor)
        bm.faces.new([v[4], v[5], v[7], v[6]])         # sloped top
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        bm.to_mesh(me)
        bm.free()
        ob = bpy.data.objects.new(obj_name, me)
        bpy.context.collection.objects.link(ob)
        bpy.context.view_layer.objects.active = ob
        return obj_name

    outer = name + "_outer"
    inner = name + "_inner"
    _wedge_solid(outer, 0, 0, w, d, 0, h_front, h_back)
    perp_inset = wall / math.cos(angle)
    _wedge_solid(inner, wall, wall, w - wall, d - wall,
                 -1, h_front - perp_inset, h_back - perp_inset)
    boolean(outer, inner)
    bpy.data.objects[outer].name = name
    return name


# ---------------------------------------------------------------------------
# Speaker grille (reused from v3)
# ---------------------------------------------------------------------------

def make_speaker_grille_holes(name_prefix, center, normal, dia, hole_dia,
                              wall_thickness):
    """Generate a list of cylinder names that, when subtracted, form a hex
    grille of `hole_dia` holes in a circle of `dia`. Cylinder axis along
    `normal` ('X','Y','Z'). center = (cx, cy, cz) of grille center.
    Wall thickness = how thick the wall to cut through."""
    names = []
    spacing = hole_dia + 2
    grille_r = (dia - hole_dia) / 2
    rows = int(grille_r * 2 / (spacing * math.sqrt(3) / 2)) + 1
    idx = 0
    cx, cy, cz = center
    for row in range(-rows, rows + 1):
        offset_a = row * spacing * math.sqrt(3) / 2
        x_off_start = (spacing / 2) if (row % 2) else 0
        cols = int((grille_r * 2 - x_off_start) / spacing) + 1
        for col in range(-cols, cols + 1):
            offset_b = col * spacing + x_off_start
            if math.hypot(offset_b, offset_a) <= grille_r:
                n = f"{name_prefix}_{idx}"
                if normal == 'X':
                    loc = (cx - 1, cy + offset_b, cz + offset_a)
                    make_cylinder(n, hole_dia / 2, wall_thickness + 2,
                                  loc=loc, axis='X')
                elif normal == 'Y':
                    loc = (cx + offset_b, cy - 1, cz + offset_a)
                    make_cylinder(n, hole_dia / 2, wall_thickness + 2,
                                  loc=loc, axis='Y')
                else:  # Z
                    loc = (cx + offset_b, cy + offset_a, cz - 1)
                    make_cylinder(n, hole_dia / 2, wall_thickness + 2,
                                  loc=loc, axis='Z')
                names.append(n)
                idx += 1
    return names


# ---------------------------------------------------------------------------
# Export
# ---------------------------------------------------------------------------

def export_stl(obj_name, path):
    select_only(obj_name)
    try:
        bpy.ops.wm.stl_export(filepath=path, export_selected_objects=True,
                              apply_modifiers=True)
    except Exception:
        bpy.ops.export_mesh.stl(filepath=path, use_selection=True)
