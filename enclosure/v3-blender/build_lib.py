"""Reusable bmesh helpers for printable enclosure construction.
Designed to be exec'd inside Blender via the MCP socket.
All dimensions in mm; corners rounded only on vertical edges (flat top/bottom)."""

import bpy
import bmesh
import math
from mathutils import Vector


def reset_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m, do_unlink=True)


def select_only(name):
    bpy.ops.object.select_all(action='DESELECT')
    o = bpy.data.objects[name]
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    return o


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
    select_only(target)
    for n in others:
        bpy.data.objects[n].select_set(True)
    bpy.ops.object.join()


def export_stl(obj_name, path):
    select_only(obj_name)
    try:
        bpy.ops.wm.stl_export(filepath=path, export_selected_objects=True,
                              apply_modifiers=True)
    except Exception:
        bpy.ops.export_mesh.stl(filepath=path, use_selection=True)
