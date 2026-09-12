"""Generate LOW TIDE's authored stylized coastal salvage trader hub.

Run with Blender 5.2 in background mode:
  blender.exe --background --python Scripts/GenerateTraderHub.py

The script creates seven common-origin OBJ components in SourceAssets/TraderHub,
an import manifest, an editable .blend source in Intermediate, and two lightweight
Eevee review renders in Artifacts. Geometry and materials are project-authored.
"""

import json
import math
import os
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


PROJECT_DIR = Path(__file__).resolve().parents[1]
EXPORT_DIR = PROJECT_DIR / "SourceAssets" / "TraderHub"
PREVIEW_DIR = PROJECT_DIR / "Artifacts" / "TraderHubPreview"
BLEND_DIR = PROJECT_DIR / "Intermediate" / "TraderHub"

COMPONENTS = (
    "SM_LT_TraderHub_Structure",
    "SM_LT_TraderHub_Awning",
    "SM_LT_TraderHub_Counter",
    "SM_LT_TraderHub_Storage",
    "SM_LT_TraderHub_Nautical",
    "SM_LT_TraderHub_Workbench",
    "SM_LT_TraderHub_Sign",
)

PALETTE = {
    "WeatheredCream": (0.72, 0.62, 0.46, 1.0),
    "TealPaint": (0.055, 0.30, 0.31, 1.0),
    "RustCanvas": (0.66, 0.20, 0.095, 1.0),
    "CreamCanvas": (0.91, 0.72, 0.45, 1.0),
    "DarkWood": (0.18, 0.095, 0.055, 1.0),
    "Brass": (0.66, 0.42, 0.12, 1.0),
    "Rope": (0.55, 0.39, 0.20, 1.0),
    "MapPaper": (0.78, 0.66, 0.43, 1.0),
}

objects_by_component = {name: [] for name in COMPONENTS}
materials = {}


def configure_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "CENTIMETERS"
    scene.unit_settings.scale_length = 0.01
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 960
    scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.world = bpy.data.worlds.new("LOW TIDE Preview World")
    scene.world.color = (0.055, 0.075, 0.085)
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.085, 0.13, 0.15, 1.0)
    background.inputs["Strength"].default_value = 0.42


def make_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = 0.72
    bsdf.inputs["Metallic"].default_value = 0.62 if name == "Brass" else 0.0
    if name == "Brass":
        bsdf.inputs["Roughness"].default_value = 0.34
    return mat


def setup_materials():
    for name, color in PALETTE.items():
        materials[name] = make_material(name, color)


def assign_material(obj, material_name):
    obj.data.materials.append(materials[material_name])


def tag(obj, component, material_name=None):
    if material_name:
        assign_material(obj, material_name)
    obj["LT_Component"] = component
    objects_by_component[component].append(obj)
    return obj


def apply_bevel(obj, width=2.0, segments=2):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bevel = obj.modifiers.new("Handmade edge roll", "BEVEL")
    bevel.width = width
    bevel.segments = segments
    bevel.limit_method = "ANGLE"
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    obj.select_set(False)


def box(name, loc, dims, mat, component, rotation=(0.0, 0.0, 0.0), bevel=2.0):
    bpy.ops.mesh.primitive_cube_add(location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dims
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        apply_bevel(obj, min(bevel, min(dims) * 0.28), 1)
    return tag(obj, component, mat)


def cylinder(name, loc, radius, depth, mat, component, vertices=16,
             rotation=(0.0, 0.0, 0.0), bevel=1.5):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
                                       location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    if bevel:
        apply_bevel(obj, bevel, 1)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return tag(obj, component, mat)


def torus(name, loc, major_radius, minor_radius, mat, component,
          rotation=(0.0, 0.0, 0.0), major_segments=24, minor_segments=6):
    bpy.ops.mesh.primitive_torus_add(major_radius=major_radius, minor_radius=minor_radius,
                                    major_segments=major_segments, minor_segments=minor_segments,
                                    location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return tag(obj, component, mat)


def curve_tube(name, points, radius, mat, component, cyclic=False, resolution=1):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = resolution
    curve.bevel_depth = radius
    curve.bevel_resolution = 1
    curve.resolution_u = 2
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for bp, co in zip(spline.bezier_points, points):
        bp.co = co
        bp.handle_left_type = "AUTO"
        bp.handle_right_type = "AUTO"
    spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    tag(obj, component, mat)
    return obj


def custom_prism(name, front_points_yz, x0, x1, mat, component, bevel=1.2):
    """Extrude a YZ polygon between x0/x1."""
    count = len(front_points_yz)
    verts = [(x0, y, z) for y, z in front_points_yz] + [(x1, y, z) for y, z in front_points_yz]
    faces = [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    for i in range(count):
        j = (i + 1) % count
        faces.append((i, j, count + j, count + i))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    if bevel:
        apply_bevel(obj, bevel, 2)
    return tag(obj, component, mat)


def custom_xz_prism(name, points_xz, y0, y1, mat, component, bevel=1.2):
    """Extrude an XZ polygon between y0/y1."""
    count = len(points_xz)
    verts = [(x, y0, z) for x, z in points_xz] + [(x, y1, z) for x, z in points_xz]
    faces = [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    for i in range(count):
        j = (i + 1) % count
        faces.append((i, j, count + j, count + i))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    if bevel:
        apply_bevel(obj, bevel, 2)
    return tag(obj, component, mat)


def make_fan_shell(name, center, width, height, depth, mat, component):
    """Closed bulged clam shell facing local -X, with raised radial ribs."""
    cx, cy, base_z = center
    rings, segments = 3, 10
    verts = [(cx - depth, cy, base_z + height * 0.22)]
    for ring in range(1, rings + 1):
        r = ring / rings
        for j in range(segments + 1):
            angle = math.pi * j / segments
            verts.append((cx - depth * (1.0 - r * r),
                          cy + math.cos(angle) * width * r,
                          base_z + math.sin(angle) * height * r))
    back_center = len(verts)
    verts.append((cx + 1.5, cy, base_z + height * 0.28))
    faces = [(0, 1 + j, 2 + j) for j in range(segments)]
    for ring in range(1, rings):
        a0 = 1 + (ring - 1) * (segments + 1)
        b0 = 1 + ring * (segments + 1)
        for j in range(segments):
            faces.append((a0 + j, b0 + j, b0 + j + 1, a0 + j + 1))
    outer = 1 + (rings - 1) * (segments + 1)
    for j in range(segments):
        faces.append((back_center, outer + j + 1, outer + j))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    apply_bevel(obj, 0.8, 1)
    tag(obj, component, mat)
    for j in range(1, segments, 2):
        angle = math.pi * j / segments
        points = []
        for step in range(5):
            r = step / 4
            points.append((cx - depth * (1.0 - r * r) - 1.2,
                           cy + math.cos(angle) * width * r,
                           base_z + height * 0.22 * (1 - r) + math.sin(angle) * height * r))
        curve_tube(name + "Rib", points, 1.15, "Rope", component)
    return obj


def sloped_beam(name, start, end, width, depth, mat, component):
    start_v = Vector(start)
    end_v = Vector(end)
    midpoint = (start_v + end_v) * 0.5
    direction = end_v - start_v
    length = direction.length
    obj = box(name, midpoint, (length, width, depth), mat, component, bevel=1.6)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = direction.to_track_quat("X", "Z")
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    obj.select_set(False)
    return obj


def build_structure():
    comp = COMPONENTS[0]
    # Four grounded, subtly tapered-looking post assemblies and doubled sill blocks.
    for x in (-55.0, 445.0):
        for y in (-312.0, 312.0):
            box("MainPost", (x, y, 257), (30, 30, 514), "DarkWood", comp, bevel=4.5)
            box("PostFoot", (x, y, 18), (48, 48, 36), "Brass", comp, bevel=5.0)
    # Front posts carry readable hand-work: inset grain strips, paint scars and forged nails.
    for y in (-312.0, 312.0):
        for z, span in ((105, 58), (205, 72), (330, 64)):
            box("FrontPostGrain", (-71.0, y + (z % 3 - 1) * 3, z), (1.5, 3.5, span),
                "Rope" if z == 205 else "DarkWood", comp,
                rotation=(0, math.radians((z % 7) - 3), 0), bevel=0.35)
        for z in (62, 158, 265, 410):
            cylinder("FrontPostNail", (-72.5, y, z), 3.5, 3.0, "Brass", comp,
                     vertices=10, rotation=(0, math.pi / 2, 0), bevel=0.7)
    # Rear wall: individually varied cream planks, teal horizontal binding rails.
    plank_w = 54.0
    for i in range(11):
        y = -275 + i * 55
        z = 265 + (i % 3 - 1) * 2.5
        rot = (0, math.radians((i % 2) * 0.5 - 0.25), math.radians((i % 3 - 1) * 0.35))
        box("RearWallPlank", (449, y, z), (20, plank_w - 3, 430),
            "WeatheredCream" if i not in (2, 8) else "TealPaint", comp, rot, 2.4)
    for z in (82, 285, 472):
        box("RearRail", (430, 0, z), (26, 610, 20), "TealPaint", comp, bevel=3.0)
    # Side half-walls preserve the front/open shop read.
    for side in (-1, 1):
        y = side * 314
        for i in range(7):
            x = 82 + i * 54
            z = 190 + (i % 2) * 2
            box("SideWallPlank", (x, y, z), (51, 18, 315),
                "WeatheredCream" if i % 4 else "TealPaint", comp,
                rotation=(0, math.radians((i % 3 - 1) * 0.3), 0), bevel=2.2)
        box("SideTopRail", (225, y - side * 3, 360), (355, 24, 24), "TealPaint", comp, bevel=3)
        box("SideBottomRail", (225, y - side * 3, 54), (355, 24, 24), "DarkWood", comp, bevel=3)
        sloped_beam("SideBrace", (-20, y - side * 5, 72), (170, y - side * 5, 340),
                    20, 18, "DarkWood", comp)
        sloped_beam("RearBrace", (425, y - side * 5, 75), (275, y - side * 5, 350),
                    20, 18, "DarkWood", comp)
    # Front lintel, roof seat and small boat-builder pegs.
    box("FrontLintel", (-55, 0, 445), (34, 660, 34), "DarkWood", comp, bevel=4)
    box("RearLintel", (445, 0, 492), (34, 660, 34), "DarkWood", comp, bevel=4)
    for y in (-250, -125, 125, 250):
        cylinder("ToolPeg", (428, y, 300), 5, 25, "Brass", comp,
                 vertices=12, rotation=(0, math.pi / 2, 0), bevel=0.8)


def roof_height(y):
    t = min(1.0, abs(y) / 370.0)
    return 515.0 + 145.0 * (1.0 - t ** 1.55) + 10.0 * t ** 4


def build_roof():
    # Roof is baked into Structure so the building imports as one coherent shell.
    comp = COMPONENTS[0]
    # Twelve individually bevelled roof planks form a warm, faceted boat-hull arch.
    edges = [-370 + i * (740 / 14) for i in range(15)]
    for i, (y0, y1) in enumerate(zip(edges[:-1], edges[1:])):
        z0, z1 = roof_height(y0), roof_height(y1)
        verts = [
            (-135, y0 + 1.3, z0), (495, y0 + 1.3, z0 + 8),
            (495, y1 - 1.3, z1 + 8), (-135, y1 - 1.3, z1),
            (-135, y0 + 1.3, z0 - 15), (495, y0 + 1.3, z0 - 7),
            (495, y1 - 1.3, z1 - 7), (-135, y1 - 1.3, z1 - 15),
        ]
        faces = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 4, 5, 1),
                 (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
        mesh = bpy.data.meshes.new("RoofPlankMesh")
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        obj = bpy.data.objects.new("CurvedRoofPlank", mesh)
        bpy.context.collection.objects.link(obj)
        apply_bevel(obj, 2.6, 2)
        tag(obj, comp, "TealPaint" if i in (0, 13) else "DarkWood")
    # Curved structural ribs make the roof silhouette intentional from every angle.
    for x in (-112, 70, 250, 472):
        points = [(x, y, roof_height(y) + (4 if x == 472 else 0)) for y in range(-370, 371, 46)]
        curve_tube("RoofRib", points, 5.5, "WeatheredCream", comp)
    # Pronounced front and rear fascia with a centered high cap.
    for x in (-140, 500):
        points = [(x, y, roof_height(y) - 8) for y in range(-370, 371, 40)]
        curve_tube("CurvedEave", points, 8.5, "WeatheredCream", comp)
    # Short contrasting scars break the fascia's machine-perfect edge without visual noise.
    for y, z in ((-275, roof_height(-275)), (-155, roof_height(-155)),
                 (115, roof_height(115)), (255, roof_height(255))):
        box("FasciaWear", (-150.2, y, z - 8), (2.5, 42, 5), "DarkWood", comp,
            rotation=(0, 0, math.radians((y % 5) - 2)), bevel=1.0)
    box("RidgeCap", (180, 0, 663), (660, 25, 25), "Brass", comp, bevel=5)
    # End-grain pegs along the front eave.
    for y in (-315, -210, -105, 105, 210, 315):
        cylinder("EavePeg", (-151, y, roof_height(y) - 7), 7.0, 10.0, "Brass", comp,
                 vertices=12, rotation=(0, math.pi / 2, 0), bevel=1.0)


def cloth_patch(name, y0, y1, mat, comp):
    x_steps = 7
    y_steps = 2
    verts = []
    for yi in range(y_steps + 1):
        v = yi / y_steps
        y = y0 + (y1 - y0) * v
        for xi in range(x_steps + 1):
            u = xi / x_steps
            x = -225 + 166 * u
            base_z = 286 + 78 * u
            sag = 23 * math.sin(math.pi * u) * (0.82 + 0.18 * math.cos(y / 100.0))
            ripple = 2.0 * math.sin(v * math.pi) * math.sin(u * math.pi * 2)
            verts.append((x, y, base_z - sag + ripple))
    faces = []
    row = x_steps + 1
    for yi in range(y_steps):
        for xi in range(x_steps):
            a = yi * row + xi
            faces.append((a, a + 1, a + row + 1, a + row))
    mesh = bpy.data.meshes.new(name + "Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, mat)
    solidify = obj.modifiers.new("Canvas thickness", "SOLIDIFY")
    solidify.thickness = 2.0
    bevel = obj.modifiers.new("Soft stitched edge", "BEVEL")
    bevel.width = 1.4
    bevel.segments = 1
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=solidify.name)
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    obj.select_set(False)
    return tag(obj, comp)


def build_canopy():
    comp = COMPONENTS[1]
    stripe_count = 10
    y_min, y_max = -300.0, 300.0
    for i in range(stripe_count):
        y0 = y_min + i * (y_max - y_min) / stripe_count
        y1 = y_min + (i + 1) * (y_max - y_min) / stripe_count
        cloth_patch("CanvasStripe", y0 + 0.8, y1 - 0.8,
                    "RustCanvas" if i % 2 == 0 else "CreamCanvas", comp)
        # Scalloped vertical hem at the player-facing edge.
        center = (y0 + y1) * 0.5
        half = (y1 - y0) * 0.5 - 1
        poly = [
            (center - half, 286), (center + half, 286),
            (center + half, 254), (center + half * 0.65, 245),
            (center, 237), (center - half * 0.65, 245), (center - half, 254),
        ]
        custom_prism("ScallopedValance", poly, -231, -221,
                     "RustCanvas" if i % 2 == 0 else "CreamCanvas", comp, bevel=1.4)
    box("AwningBackRail", (-55, 0, 365), (18, 625, 18), "Brass", comp, bevel=3)
    box("AwningFrontRail", (-226, 0, 286), (16, 625, 16), "DarkWood", comp, bevel=3)
    for y in (-300, 300):
        curve_tube("AwningStay", [(-229, y, 287), (-145, y, 332), (-58, y, 365)],
                   3.0, "Rope", comp)


def build_counter():
    comp = COMPONENTS[2]
    # Left/right counter bays retain a clear 138 cm opening directly in front of Mara.
    for side in (-1, 1):
        y = side * 178
        box("CounterTop", (-92, y, 118), (112, 205, 18), "DarkWood", comp, bevel=5)
        # Pale grain scratches, worn paint edge, and nailheads read clearly from the approach.
        for yy in (-54, 0, 54):
            box("CounterGrain", (-95, y + yy, 127.6), (72, 2.0, 1.4),
                "WeatheredCream", comp, rotation=(0, 0, math.radians(side * 4)), bevel=0.3)
        box("CounterEdgeWear", (-150, y, 117), (2.5, 146, 7), "RustCanvas", comp, bevel=1.0)
        for yy in (-78, 78):
            cylinder("CounterNail", (-120, y + yy, 129), 3.0, 2.2, "Brass", comp,
                     vertices=10, bevel=0.6)
        box("CounterFascia", (-92, y, 70), (20, 190, 80), "TealPaint", comp, bevel=3.5)
        box("CounterOuterPost", (-92, side * 279, 60), (28, 28, 120), "DarkWood", comp, bevel=4)
        box("CounterInnerPost", (-92, side * 70, 60), (24, 24, 120), "WeatheredCream", comp, bevel=3)
        # Side return gives the trader work zone a believable U shape.
        box("CounterReturn", (5, side * 282, 118), (190, 65, 18), "DarkWood", comp, bevel=5)
        sloped_beam("CounterBrace", (-102, side * 250, 25), (-102, side * 145, 108),
                    13, 12, "Brass", comp)
    # A deliberately low threshold reads as an opening and never hides the NPC.
    box("OpeningSill", (-92, 0, 17), (34, 132, 34), "Brass", comp, bevel=5)
    # Shelf and a few readable valuable silhouettes behind the merchant.
    box("BackShelf", (420, 0, 335), (35, 390, 20), "DarkWood", comp, bevel=4)
    for y, z, mat in ((-135, 365, "Brass"), (-45, 360, "TealPaint"), (65, 363, "Brass"), (145, 361, "RustCanvas")):
        cylinder("ShelfSalvage", (400, y, z), 19, 30, mat, comp, vertices=12,
                 rotation=(0, math.pi / 2, 0), bevel=2)
    # Merchandise is grouped on the outer bays; the central Y +/-66 interaction opening stays clear.
    for side in (-1, 1):
        by = side * 190
        # Green sea-glass bottles with brass necks.
        for j in range(3):
            yy = by + side * (j - 1) * 28
            cylinder("CounterBottle", (-118, yy, 149 + (j % 2) * 5), 10, 40,
                     "TealPaint", comp, vertices=12, bevel=2)
            cylinder("BottleNeck", (-118, yy, 174 + (j % 2) * 5), 5, 15,
                     "Brass", comp, vertices=10, bevel=1)
        # Smooth nested shells and a tied bundle of salvage tools.
        for j in range(3):
            yy = side * (125 + j * 38)
            make_fan_shell("CounterClam", (-143, yy, 130), 13 + j * 2,
                           18 + j * 2, 7, "CreamCanvas", comp)
        for j in range(3):
            box("BundledTool", (-112 + j * 8, side * 260, 151), (58, 6, 6),
                "Brass" if j == 1 else "WeatheredCream", comp,
                rotation=(0, math.radians(-8 + j * 8), math.radians(side * 8)), bevel=2)
        torus("ToolBinding", (-112, side * 260, 151), 11, 3, "Rope", comp,
              rotation=(math.pi / 2, 0, 0), major_segments=14, minor_segments=5)


def build_sign():
    comp = COMPONENTS[6]
    # Rope-hung arched board above the canopy.
    sign_outline = [(-142, 455), (142, 455), (142, 515), (112, 539),
                    (0, 553), (-112, 539), (-142, 515)]
    custom_prism("SalvageSignBoard", sign_outline, -125, -105,
                 "TealPaint", comp, bevel=5.0)
    # Cream inset creates a carved-board read behind the raised lettering.
    inset_outline = [(-125, 469), (125, 469), (125, 510), (100, 526),
                     (0, 539), (-100, 526), (-125, 510)]
    custom_prism("SignInset", inset_outline, -131, -125,
                 "WeatheredCream", comp, bevel=2.5)
    for y in (-102, 102):
        curve_tube("SignRope", [(-115, y, 539), (-92, y, 578), (-72, y, 605)],
                   3.8, "Rope", comp)
        torus("SignRing", (-72, y, 608), 11, 3.5, "Brass", comp,
              rotation=(0, math.pi / 2, 0), major_segments=16, minor_segments=6)

    # Bundled Inter (SIL OFL 1.1; see Docs/INTER_FONT_LICENSE.md), exported as mesh.
    bpy.ops.object.text_add(location=(0, 0, 0))
    text_obj = bpy.context.object
    text_obj.name = "SALVAGE_Letters"
    text_obj.data.body = "SALVAGE"
    inter_font_path = Path(bpy.app.binary_path).parent / "5.2" / "datafiles" / "fonts" / "Inter.woff2"
    if not inter_font_path.exists():
        raise FileNotFoundError(f"Required bundled Inter font missing: {inter_font_path}")
    text_obj.data.font = bpy.data.fonts.load(str(inter_font_path))
    text_obj.data.align_x = "CENTER"
    text_obj.data.align_y = "CENTER"
    text_obj.data.size = 42
    text_obj.data.resolution_u = 2
    text_obj.data.extrude = 2.4
    text_obj.data.bevel_depth = 0.8
    text_obj.data.bevel_resolution = 2
    # Text local X -> world -Y, local Y -> world +Z, extrusion -> world -X.
    # The export path compensates this submesh for UE's observed text handedness.
    text_obj.matrix_world = Matrix(((0, 0, -1, -136),
                                    (-1, 0, 0, 0),
                                    (0, 1, 0, 500),
                                    (0, 0, 0, 1)))
    assign_material(text_obj, "RustCanvas")
    tag(text_obj, comp)
    # Small anchor emblem centered above the word.
    cylinder("SignAnchorStem", (-139, 0, 541), 4.5, 34, "Brass", comp,
             vertices=12, rotation=(0, math.pi / 2, 0), bevel=1)
    torus("SignAnchorRing", (-145, 0, 559), 8, 2.8, "Brass", comp,
          rotation=(0, math.pi / 2, 0), major_segments=16, minor_segments=6)


def make_crate(center, size, comp, accent=False):
    x, y, z0 = center
    sx, sy, sz = size
    for side in (-1, 1):
        for i in range(4):
            z = z0 + sz * (0.2 + i * 0.2)
            box("CrateSlat", (x + side * sx / 2, y, z), (8, sy, sz * 0.16),
                "WeatheredCream", comp, bevel=1.8)
        for yy in (-sy * 0.38, sy * 0.38):
            box("CrateRail", (x + side * (sx / 2 + 2), y + yy, z0 + sz * 0.5),
                (11, 12, sz), "TealPaint" if accent else "DarkWood", comp, bevel=2)
    for side in (-1, 1):
        for i in range(4):
            z = z0 + sz * (0.2 + i * 0.2)
            box("CrateSlat", (x, y + side * sy / 2, z), (sx, 8, sz * 0.16),
                "WeatheredCream", comp, bevel=1.8)


def make_barrel(center, radius, height, comp):
    x, y, z0 = center
    stave_count = 14
    for i in range(stave_count):
        a = 2 * math.pi * i / stave_count
        r = radius - 4 + 5 * math.sin(math.pi * 0.5)
        px, py = x + math.cos(a) * r, y + math.sin(a) * r
        stave = box("BarrelStave", (px, py, z0 + height / 2),
                    (11, 15, height - 7), "DarkWood" if i % 3 else "WeatheredCream",
                    comp, rotation=(0, 0, a), bevel=2.2)
    for z in (z0 + 13, z0 + height * 0.5, z0 + height - 13):
        torus("BarrelHoop", (x, y, z), radius + 1.5, 3.2, "Brass", comp,
              major_segments=18, minor_segments=5)
    cylinder("BarrelTop", (x, y, z0 + height - 2), radius - 6, 5,
             "DarkWood", comp, vertices=16, bevel=1.2)


def build_dressing():
    storage_comp = COMPONENTS[3]
    nautical_comp = COMPONENTS[4]
    make_crate((315, -235, 0), (95, 90, 84), storage_comp, accent=True)
    make_crate((365, -210, 84), (82, 72, 68), storage_comp, accent=False)
    make_barrel((330, 225, 0), 45, 105, storage_comp)
    make_barrel((410, 258, 0), 35, 78, storage_comp)
    # Readable rope coils lie near storage, rather than scattered clutter.
    for i, radius in enumerate((34, 28, 22)):
        torus("RopeCoil", (240, 235, 5 + i * 5), radius, 4.2, "Rope", nautical_comp,
              major_segments=22, minor_segments=5)
    curve_tube("LooseRopeTail", [(240, 205, 9), (205, 176, 8), (168, 193, 7), (146, 170, 6)],
               4.2, "Rope", nautical_comp)
    # A restrained draped net occupies one side wall with real sag and knots.
    net_x = 245.0
    y0 = 305.0
    for j in range(6):
        x = 70 + j * 48
        points = []
        for k in range(6):
            z = 92 + k * 45
            sag = 13 * math.sin(k / 5 * math.pi) + (j % 2) * 5
            points.append((x, y0 + 7 + sag, z))
        curve_tube("NetVertical", points, 1.45, "Rope", nautical_comp)
    for k in range(6):
        z = 92 + k * 45
        points = []
        for j in range(6):
            x = 70 + j * 48
            sag = 8 * math.sin(j / 5 * math.pi)
            points.append((x, y0 + 10 + sag + (j % 2) * 5, z))
        curve_tube("NetHorizontal", points, 1.35, "Rope", nautical_comp)
    for j in range(6):
        for k in range(6):
            if (j + k) % 2 == 0:
                cylinder("NetKnot", (70 + j * 48, y0 + 18 + (j % 2) * 5, 92 + k * 45),
                         2.4, 4, "Rope", nautical_comp, vertices=8,
                         rotation=(math.pi / 2, 0, 0), bevel=0.0)
    # A second, looser net layer hangs across the outer front-right bay, visible on approach.
    # It stops well above the counter and never crosses the center opening.
    for j in range(5):
        y = 112 + j * 40
        curve_tube("FrontNetDrop", [(-73, y, 432), (-88, y + 8, 390),
                                     (-80, y - 5, 345), (-72, y, 302)],
                   1.7, "Rope", nautical_comp)
    for k in range(4):
        z = 318 + k * 32
        curve_tube("FrontNetCross", [(-77, 108, z), (-91, 190, z - 10),
                                      (-78, 274, z + 2)],
                   1.6, "Rope", nautical_comp)
    for y, z in ((132, 360), (205, 328), (265, 392)):
        torus("NetSalvageRing", (-94, y, z), 12, 3.5, "Brass", nautical_comp,
              rotation=(0, math.pi / 2, 0), major_segments=16, minor_segments=6)
        curve_tube("NetSalvageTag", [(-95, y, z - 12), (-97, y + 4, z - 31)],
                   2.2, "RustCanvas", nautical_comp)


def build_anchor(comp):
    # Large stylized hero anchor leans against the left side; broad shapes read at distance.
    x, y, z = 250, -360, 95
    cylinder("AnchorShank", (x, y, z + 62), 8, 132, "Brass", comp,
             vertices=12, rotation=(0, 0, math.radians(-10)), bevel=2)
    torus("AnchorEye", (x - 11, y, z + 138), 18, 5.5, "Brass", comp,
          rotation=(math.pi / 2, 0, 0), major_segments=20, minor_segments=7)
    box("AnchorStock", (x - 6, y, z + 104), (100, 13, 13), "Brass", comp,
        rotation=(0, math.radians(-8), math.radians(-10)), bevel=4)
    # Crown and flukes made from tapered prisms.
    sloped_beam("AnchorArmL", (x - 5, y, z), (x - 70, y, z + 32), 16, 16, "Brass", comp)
    sloped_beam("AnchorArmR", (x - 5, y, z), (x + 60, y, z + 32), 16, 16, "Brass", comp)
    custom_xz_prism("AnchorFlukeL", [(x - 82, z), (x - 42, z + 5), (x - 55, z + 35)],
                    y - 8, y + 8, "Brass", comp, bevel=2.5)
    custom_xz_prism("AnchorFlukeR", [(x + 42, z + 5), (x + 82, z), (x + 55, z + 35)],
                    y - 8, y + 8, "Brass", comp, bevel=2.5)


def build_hero_prop():
    comp = COMPONENTS[5]
    # Mapmaker/work table in the rear-left, aligned so it does not block Mara.
    box("WorkbenchTop", (305, -120, 106), (185, 155, 18), "DarkWood", comp, bevel=5)
    for x in (235, 375):
        for y in (-175, -65):
            box("WorkbenchLeg", (x, y, 51), (18, 18, 102), "WeatheredCream", comp,
                rotation=(0, math.radians(2 if x < 300 else -2), 0), bevel=3)
    # Slightly curled map uses a shallow subdivided surface.
    verts, faces = [], []
    xs = [225 + i * 28 for i in range(6)]
    ys = [-175 + j * 24 for j in range(5)]
    for j, yy in enumerate(ys):
        for i, xx in enumerate(xs):
            curl = 5.5 * ((i / 5 - 0.5) ** 4) * 16 + 1.5 * math.sin(j * 1.3)
            verts.append((xx, yy, 117 + curl))
    for j in range(4):
        for i in range(5):
            a = j * 6 + i
            faces.append((a, a + 1, a + 7, a + 6))
    mesh = bpy.data.meshes.new("MapMesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    map_obj = bpy.data.objects.new("PinnedExpeditionMap", mesh)
    bpy.context.collection.objects.link(map_obj)
    tag(map_obj, comp, "MapPaper")
    solidify = map_obj.modifiers.new("Paper thickness", "SOLIDIFY")
    solidify.thickness = 1.2
    bevel = map_obj.modifiers.new("Worn map edge", "BEVEL")
    bevel.width = 1.0
    bevel.segments = 1
    bpy.context.view_layer.objects.active = map_obj
    map_obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=solidify.name)
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    map_obj.select_set(False)
    # Map route lines and brass pins.
    curve_tube("MapRoute", [(240, -150, 122), (270, -135, 124), (305, -148, 122),
                             (340, -110, 122), (360, -92, 124)], 2.0,
               "TealPaint", comp)
    for px, py in ((240, -150), (305, -148), (360, -92)):
        cylinder("MapPin", (px, py, 126), 4, 5, "Brass", comp, vertices=10, bevel=0.8)
    # Chunky stylized lantern: brass cage, cream glowing-looking inner volume.
    lantern_x, lantern_y = 235, -55
    cylinder("LanternBase", (lantern_x, lantern_y, 133), 20, 10, "Brass", comp,
             vertices=12, bevel=2)
    cylinder("LanternGlow", (lantern_x, lantern_y, 160), 15, 46, "CreamCanvas", comp,
             vertices=12, bevel=3)
    cylinder("LanternCap", (lantern_x, lantern_y, 189), 20, 12, "Brass", comp,
             vertices=12, bevel=2)
    for a in range(0, 360, 90):
        rad = math.radians(a)
        box("LanternCage", (lantern_x + math.cos(rad) * 17, lantern_y + math.sin(rad) * 17, 161),
            (4, 4, 56), "Brass", comp, bevel=1)
    curve_tube("LanternHandle", [(lantern_x - 18, lantern_y, 190),
                                  (lantern_x - 10, lantern_y, 214),
                                  (lantern_x + 10, lantern_y, 214),
                                  (lantern_x + 18, lantern_y, 190)],
               3.0, "Brass", comp)
    # A single large anchor provides the memorable boatbuilder/salvage motif.
    build_anchor(COMPONENTS[4])


def convert_all_to_mesh():
    for component, objs in objects_by_component.items():
        for obj in list(objs):
            if obj.type != "MESH":
                bpy.context.view_layer.objects.active = obj
                obj.select_set(True)
                bpy.ops.object.convert(target="MESH")
                obj.select_set(False)


def join_component(component):
    objs = [obj for obj in objects_by_component[component] if obj and obj.name in bpy.data.objects]
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objs:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = component
    # Remove unused slots after joins so each imported mesh has only needed draw calls.
    bpy.ops.object.material_slot_remove_unused()
    # Bake the active object's origin offset while preserving every world-space vertex.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    joined["LT_CommonOrigin"] = True
    joined["LT_ExportPreReflectedY"] = True
    objects_by_component[component] = [joined]
    return joined


def mesh_stats(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = obj.evaluated_get(depsgraph)
    mesh = eval_obj.to_mesh()
    mesh.calc_loop_triangles()
    vertices = len(mesh.vertices)
    triangles = len(mesh.loop_triangles)
    eval_obj.to_mesh_clear()
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    min_v = [min(v[i] for v in corners) for i in range(3)]
    max_v = [max(v[i] for v in corners) for i in range(3)]
    return vertices, triangles, min_v, max_v


def add_preview_stage():
    # Preview-only sandy apron and sea-colored background planes.
    bpy.ops.mesh.primitive_plane_add(size=1800, location=(120, 0, -1))
    ground = bpy.context.object
    ground.name = "PREVIEW_Ground"
    ground_mat = make_material("PREVIEW_Sand", (0.34, 0.25, 0.15, 1))
    ground.data.materials.append(ground_mat)
    bevel = None

    bpy.ops.object.light_add(type="SUN", location=(-520, -360, 720))
    key = bpy.context.object
    key.name = "PREVIEW_WarmSun"
    key.data.energy = 3.2
    key.data.angle = math.radians(18)
    key.data.color = (1.0, 0.72, 0.46)
    point_camera(key, (130, 0, 230))
    bpy.ops.object.light_add(type="AREA", location=(260, 500, 470))
    fill = bpy.context.object
    fill.name = "PREVIEW_SkyFill"
    fill.data.energy = 320000
    fill.data.size = 450
    fill.data.color = (0.35, 0.62, 0.75)
    point_camera(fill, (130, 0, 260))
    bpy.ops.object.light_add(type="POINT", location=(220, -70, 190))
    lantern = bpy.context.object
    lantern.name = "PREVIEW_LanternGlow"
    lantern.data.energy = 95000
    lantern.data.color = (1.0, 0.35, 0.08)
    lantern.data.shadow_soft_size = 85
    return [ground, key, fill, lantern]


def point_camera(camera, target):
    direction = Vector(target) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def render_previews(preview_objects):
    scene = bpy.context.scene
    bpy.ops.object.camera_add(location=(-840, -760, 185))
    camera = bpy.context.object
    camera.name = "PREVIEW_Camera"
    camera.data.lens = 42
    camera.data.sensor_width = 36
    camera.data.clip_start = 1.0
    camera.data.clip_end = 10000.0
    point_camera(camera, (130, 0, 270))
    scene.camera = camera
    scene.render.filepath = str(PREVIEW_DIR / "TraderHub_FirstPerson.png")
    bpy.ops.render.render(write_still=True)

    camera.location = (-1080, 900, 690)
    camera.data.lens = 52
    point_camera(camera, (145, 0, 280))
    scene.render.filepath = str(PREVIEW_DIR / "TraderHub_Overview.png")
    bpy.ops.render.render(write_still=True)
    return camera


def remove_preview_stage(preview_objects, camera):
    for obj in preview_objects + [camera]:
        bpy.data.objects.remove(obj, do_unlink=True)


def export_component_obj(component):
    source = objects_by_component[component][0]
    duplicate = source.copy()
    duplicate.data = source.data.copy()
    bpy.context.collection.objects.link(duplicate)
    duplicate.name = component + "_EXPORT"
    # UE's legacy OBJ importer mirrors Y. Bake the opposite reflection here and
    # reverse each polygon so imported winding/normals remain outward-facing.
    # Inter letter outlines arrive in UE with an additional horizontal reflection
    # relative to the rest of the sign. Counter-reflect only that disconnected
    # RustCanvas submesh, preserving every other sign vertex and component.
    compensated_polygons = []
    if component == "SM_LT_TraderHub_Sign":
        letter_slot = next((index for index, slot in enumerate(duplicate.material_slots)
                            if slot.material and slot.material.name == "RustCanvas"), None)
        if letter_slot is None:
            raise RuntimeError("Sign lettering material slot RustCanvas is missing")
        compensated_polygons = [polygon for polygon in duplicate.data.polygons
                                if polygon.material_index == letter_slot]
        letter_vertices = {vertex_index for polygon in compensated_polygons
                           for vertex_index in polygon.vertices}
        for vertex_index in letter_vertices:
            duplicate.data.vertices[vertex_index].co.y *= -1.0
    for vertex in duplicate.data.vertices:
        vertex.co.y *= -1.0
    for polygon in duplicate.data.polygons:
        polygon.flip()
    for polygon in compensated_polygons:
        polygon.flip()
    duplicate.data.update()
    bpy.ops.object.select_all(action="DESELECT")
    duplicate.select_set(True)
    bpy.context.view_layer.objects.active = duplicate
    output = EXPORT_DIR / f"{component}.obj"
    bpy.ops.wm.obj_export(filepath=str(output), export_selected_objects=True,
                          export_materials=True, export_triangulated_mesh=True,
                          # Preserve the project's Z-up centimetre contract in OBJ.
                          # Unreal handles its own OBJ coordinate conversion.
                          forward_axis="Y", up_axis="Z", global_scale=1.0)
    bpy.data.objects.remove(duplicate, do_unlink=True)
    return output


def write_manifest(joined_objects):
    component_data = []
    total_triangles = 0
    for component, obj in zip(COMPONENTS, joined_objects):
        vertices, triangles, min_v, max_v = mesh_stats(obj)
        total_triangles += triangles
        ordered_materials = [slot.material.name for slot in obj.material_slots if slot.material]
        bounds = {"min": [round(x, 2) for x in min_v],
                  "max": [round(x, 2) for x in max_v]}
        obj_bounds = {"min": [round(min_v[0], 2), round(-max_v[1], 2), round(min_v[2], 2)],
                      "max": [round(max_v[0], 2), round(-min_v[1], 2), round(max_v[2], 2)]}
        component_data.append({
            "name": component,
            "source": f"{component}.obj",
            "asset": f"/Game/Generated/TraderHub/{component}",
            "materials": ordered_materials,
            "sourceFile": f"{component}.obj",
            "intendedUnrealAsset": f"/Game/Generated/TraderHub/{component}",
            "commonOriginCm": [0.0, 0.0, 0.0],
            "bounds": bounds,
            "boundsUnrealImported": bounds,
            "boundsObjPreReflected": obj_bounds,
            "boundsCmAuthored": bounds,
            "vertices": vertices,
            "triangles": triangles,
            "materialSlots": ordered_materials,
            "collision": "none; import as non-colliding dressing",
        })
    manifest = {
        "assetFamily": "LOW TIDE Stylized Coastal Trader Hub",
        "authorship": "Project-authored procedural mesh; no external assets or textures.",
        "generator": "Scripts/GenerateTraderHub.py",
        "blenderVersion": bpy.app.version_string,
        "units": "centimeters",
        "orientationAuthored": {"up": "+Z", "rearAwayFromPlayer": "+X", "right": "+Y",
                                "frontTowardPlayer": "-X"},
        "unrealPlacement": {"worldLocationCm": [900.0, -1350.0, 125.0], "yawDegrees": -38.0},
        "maraReference": {"localXYCm": [0.0, 0.0], "localFootZCm": 90.0,
                          "worldReference": [900.0, -1350.0, 215.0]},
        "objImport": {"preReflectedY": True,
                      "reason": "Unreal OBJ import reflects Y; export reflection restores authored +Y right.",
                      "importUniformScale": 1.0, "combineMeshes": True,
                      "generateCollision": False, "importMaterials": False},
        "palette": {name: list(color[:3]) for name, color in PALETTE.items()},
        "materials": [{"slot": name, "baseColorLinearRGBA": list(color),
                       "finish": "stylized clean rough" if name != "Brass" else "worn stylized metal"}
                      for name, color in PALETTE.items()],
        "components": component_data,
        "totalTriangles": total_triangles,
        "triangleBudget": {"target": 40000, "withinBudget": total_triangles < 40000},
        "collision_proxies": [
            {"name": "RearWall", "center": [442.0, 0.0, 260.0], "extent": [22.0, 306.0, 225.0]},
            {"name": "LeftSideWall", "center": [225.0, -312.0, 190.0], "extent": [178.0, 18.0, 170.0]},
            {"name": "RightSideWall", "center": [225.0, 312.0, 190.0], "extent": [178.0, 18.0, 170.0]},
        ],
        "interactionClearance": {
            "frontApproach": "open from local -X",
            "counterCenterOpeningCm": {"yMin": -66.0, "yMax": 66.0, "topClearFromZ": 34.0},
            "noFloorElevation": True,
        },
        "reviewImages": ["Artifacts/TraderHubPreview/TraderHub_FirstPerson.png",
                         "Artifacts/TraderHubPreview/TraderHub_Overview.png"],
    }
    # Keep collisionProxyBoxesCm as a descriptive alias for human readers.
    manifest["collisionProxyBoxesCm"] = manifest["collision_proxies"]
    with open(EXPORT_DIR / "manifest.json", "w", encoding="utf-8", newline="\n") as handle:
        json.dump(manifest, handle, indent=2)
        handle.write("\n")
    return manifest


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    BLEND_DIR.mkdir(parents=True, exist_ok=True)
    configure_scene()
    setup_materials()
    build_structure()
    build_roof()
    build_canopy()
    build_counter()
    build_sign()
    build_dressing()
    build_hero_prop()
    convert_all_to_mesh()
    joined_objects = [join_component(component) for component in COMPONENTS]

    sign_only = "--sign-only" in sys.argv
    if not sign_only:
        preview_stage = add_preview_stage()
        camera = render_previews(preview_stage)
        remove_preview_stage(preview_stage, camera)
        bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_DIR / "LT_TraderHub_Source.blend"))

    export_components = ("SM_LT_TraderHub_Sign",) if sign_only else COMPONENTS
    for component in export_components:
        export_component_obj(component)
    manifest = write_manifest(joined_objects)
    if not manifest["triangleBudget"]["withinBudget"]:
        raise RuntimeError(f"Trader hub exceeds triangle budget: {manifest['totalTriangles']}")
    print(f"[GenerateTraderHub] Exported {len(export_components)} component(s); "
          f"{manifest['totalTriangles']} triangles total")
    print(f"[GenerateTraderHub] Manifest: {EXPORT_DIR / 'manifest.json'}")
    print(f"[GenerateTraderHub] Preview: {PREVIEW_DIR / 'TraderHub_FirstPerson.png'}")


if __name__ == "__main__":
    main()
