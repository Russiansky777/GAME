"""Generate the project-authored LOW TIDE perched parrot kit and review render.

Run with Blender 5.2:
  blender.exe --background --python Scripts/GenerateHubLiveliness.py

Six independently pivoted OBJ parts are written to SourceAssets/HubLiveliness.
The bird faces local +X, uses centimeters, and is designed for component animation.
"""

import json
import math
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceAssets" / "HubLiveliness"
PREVIEW = ROOT / "Artifacts" / "HubLivelinessPreview"
BLEND = ROOT / "Intermediate" / "HubLiveliness" / "HubLiveliness.blend"

PARTS = {
    "SM_LT_Parrot_Perch": (0, 0, 0),
    "SM_LT_Parrot_Body": (0, 0, 0),
    "SM_LT_Parrot_Head": (17, 0, 54),
    "SM_LT_Parrot_Wing_L": (1, -8, 41),
    "SM_LT_Parrot_Wing_R": (1, 8, 41),
    "SM_LT_Parrot_Tail": (-13, 0, 29),
    "SM_LT_Lantern_Rope": (0, 0, 0),
    "SM_LT_Hanging_Lantern": (0, 0, 0),
}
parts = {name: [] for name in PARTS}

COLORS = {
    "CoralRed": (0.58, 0.045, 0.025, 1),
    "DeepRed": (0.29, 0.018, 0.012, 1),
    "Golden": (0.95, 0.48, 0.045, 1),
    "WingBlue": (0.025, 0.24, 0.42, 1),
    "DarkBlue": (0.012, 0.075, 0.14, 1),
    "Cream": (0.94, 0.72, 0.42, 1),
    "Beak": (0.12, 0.09, 0.075, 1),
    "Eye": (0.006, 0.004, 0.003, 1),
    "Claw": (0.23, 0.18, 0.13, 1),
    "Wood": (0.19, 0.075, 0.025, 1),
    "Rope": (0.47, 0.29, 0.12, 1),
}
mats = {}


def setup():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "CENTIMETERS"
    scene.unit_settings.scale_length = 0.01
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.look = "AgX - Medium High Contrast"
    world = bpy.data.worlds.new("Warm coast")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.055, 0.085, 0.10, 1)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.45
    for name, color in COLORS.items():
        mat = bpy.data.materials.new(name)
        mat.diffuse_color = color
        mat.use_nodes = True
        shader = mat.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = color
        shader.inputs["Roughness"].default_value = 0.78
        mats[name] = mat


def tag(obj, part, material):
    obj.data.materials.append(mats[material])
    parts[part].append(obj)
    for face in obj.data.polygons:
        face.use_smooth = True
    return obj


def ellipsoid(name, part, loc, scale, material, segments=20, rings=12, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return tag(obj, part, material)


def cone(name, part, loc, radii, depth, material, rotation=(0, 0, 0), vertices=24):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radii[0], radius2=radii[1], depth=depth,
                                    location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    return tag(obj, part, material)


def cylinder(name, part, loc, radius, depth, material, rotation=(0, 0, 0), vertices=20):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
                                        location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    bevel = obj.modifiers.new("soft edge", "BEVEL")
    bevel.width = min(radius * 0.22, 1.1)
    bevel.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return tag(obj, part, material)


def make_perch():
    p = "SM_LT_Parrot_Perch"
    cylinder("weathered branch", p, (0, 0, 7), 4.4, 82, "Wood", rotation=(0, math.pi / 2, 0), vertices=24)
    cylinder("branch end", p, (-25, 0, 14), 2.5, 20, "Wood", rotation=(0, -0.75, 0), vertices=16)
    for x in (-9, 9):
        bpy.ops.mesh.primitive_torus_add(major_radius=4.75, minor_radius=0.75, major_segments=24,
                                         minor_segments=8, location=(x, 0, 7), rotation=(0, math.pi / 2, 0))
        tag(bpy.context.object, p, "Rope")


def make_body_and_feet():
    p = "SM_LT_Parrot_Body"
    ellipsoid("pear shaped body", p, (0, 0, 36), (17, 13, 23), "CoralRed", rotation=(0, 0.10, 0))
    ellipsoid("gold breast", p, (11.0, 0, 34), (10.5, 12.1, 15.5), "Golden", segments=18, rings=10)
    ellipsoid("shoulder mantle", p, (-4, 0, 47), (12, 13.4, 8), "DeepRed", segments=18, rings=10)
    for side in (-1, 1):
        y = side * 5.0
        cylinder("ankle", p, (4, y, 17), 1.35, 8, "Claw", vertices=14)
        for toe, yaw in ((0, -0.34), (1, 0.06), (2, 0.42)):
            cylinder("gripping toe", p, (8 + toe * 0.5, y + side * (toe - 1) * 1.0, 11), 0.75, 9,
                     "Claw", rotation=(0, math.pi / 2 + yaw, 0), vertices=12)


def make_head():
    p = "SM_LT_Parrot_Head"
    pivot = Vector(PARTS[p])
    ellipsoid("parrot head", p, pivot + Vector((1, 0, 6)), (12.5, 11, 13), "CoralRed")
    ellipsoid("cream cheek L", p, pivot + Vector((5, -8.6, 4)), (7, 2.4, 7.8), "Cream", segments=16, rings=10)
    ellipsoid("cream cheek R", p, pivot + Vector((5, 8.6, 4)), (7, 2.4, 7.8), "Cream", segments=16, rings=10)
    # Two overlapping tapered cones create the hooked macaw profile along +X.
    cone("upper curved beak", p, pivot + Vector((13.5, 0, 4)), (7.2, 2.0), 15, "Beak",
         rotation=(0, math.pi / 2, 0), vertices=28)
    cone("downturned beak", p, pivot + Vector((18.0, 0, 0)), (4.8, 0.6), 10, "Beak",
         rotation=(0, 2.15, 0), vertices=24)
    for side in (-1, 1):
        ellipsoid("bright eye", p, pivot + Vector((7.0, side * 10.0, 9.0)), (1.9, 1.1, 1.9), "Eye", 12, 8)
    for i in range(3):
        cone("crest feather", p, pivot + Vector((-5.0 - i * 2.2, (i - 1) * 2.2, 15.0 + i)),
             (2.5, 0.25), 11 + i * 1.5, "DeepRed", rotation=(0, -0.35, 0), vertices=16)


def make_wing(side):
    suffix = "L" if side < 0 else "R"
    p = "SM_LT_Parrot_Wing_" + suffix
    pivot = Vector(PARTS[p])
    ellipsoid("wing shoulder", p, pivot + Vector((-3, side * 1.2, -2)), (11, 3.2, 16), "WingBlue", 18, 10,
              rotation=(0, -0.25, side * 0.07))
    # Layered long feathers make a designed folded wing silhouette.
    for i in range(8):
        z = -6.0 - i * 2.1
        x = -2.0 - i * 1.15
        color = "DarkBlue" if i >= 5 else "WingBlue"
        ellipsoid("layered primary", p, pivot + Vector((x, side * 2.2, z)),
                  (4.2, 1.6, 10.5 - i * 0.25), color, 12, 8, rotation=(0, -0.30 - i * 0.025, 0))
    for i in range(4):
        ellipsoid("gold covert", p, pivot + Vector((3 - i * 2.5, side * 3.1, 1 - i * 2.4)),
                  (5.0, 1.1, 4.8), "Golden", 12, 8)


def make_tail():
    p = "SM_LT_Parrot_Tail"
    pivot = Vector(PARTS[p])
    for i, (side, color) in enumerate(((-1, "WingBlue"), (0, "DeepRed"), (1, "WingBlue"))):
        cone("long tail feather", p, pivot + Vector((-12 - i * 2, side * 3.6, -9 - i * 1.5)),
             (4.6, 0.45), 34 - i * 2, color, rotation=(0, -math.pi / 2 - 0.16, 0), vertices=20)


def make_lantern():
    rope = "SM_LT_Lantern_Rope"
    lantern = "SM_LT_Hanging_Lantern"
    cylinder("hanging rope", rope, (0, 0, -25), 0.9, 50, "Rope", vertices=12)
    cylinder("lantern cap", lantern, (0, 0, -51), 7.5, 3.5, "Beak", vertices=16)
    cylinder("warm glass", lantern, (0, 0, -62), 6.0, 19, "Golden", vertices=20)
    cylinder("lantern base", lantern, (0, 0, -73), 8.0, 4.0, "Beak", vertices=16)
    for x, y in ((-6, 0), (6, 0), (0, -6), (0, 6)):
        cylinder("lantern cage", lantern, (x, y, -62), 0.65, 21, "Beak", vertices=10)


def join_and_export():
    OUT.mkdir(parents=True, exist_ok=True)
    result = []
    for name, objects in parts.items():
        bpy.ops.object.select_all(action="DESELECT")
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objects[0]
        bpy.ops.object.join()
        joined = bpy.context.object
        joined.name = name
        # Bake the joined object's transform, then express vertices around the
        # explicit anatomical hinge used by AHubLivelinessActor.
        joined.data.transform(joined.matrix_world)
        joined.matrix_world = Matrix.Identity(4)
        pivot = Vector(PARTS[name])
        for vertex in joined.data.vertices:
            vertex.co -= pivot
        # Compensate the legacy UE OBJ importer's Y reflection and preserve winding.
        for vertex in joined.data.vertices:
            vertex.co.y *= -1.0
        for polygon in joined.data.polygons:
            polygon.flip()
        joined.data.update()
        bpy.ops.wm.obj_export(filepath=str(OUT / (name + ".obj")), export_selected_objects=True,
                              export_materials=True, export_triangulated_mesh=True,
                              forward_axis="Y", up_axis="Z")
        result.append({"asset": name, "source": name + ".obj", "pivot_cm": list(PARTS[name])})
    (OUT / "manifest.json").write_text(json.dumps({
        "license": "Project-authored; LOW TIDE repository",
        "orientation": "+X forward, +Z up, centimeters",
        "components": result,
    }, indent=2) + "\n", encoding="utf-8")


def render_preview():
    PREVIEW.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.camera_add(location=(125, -145, 88))
    camera = bpy.context.object
    bpy.context.scene.camera = camera
    direction = Vector((0, 0, 35)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    camera.data.lens = 58
    bpy.ops.object.light_add(type="AREA", location=(75, -85, 135))
    bpy.context.object.data.energy = 110000
    bpy.context.object.data.shape = "DISK"
    bpy.context.object.data.size = 95
    bpy.ops.object.light_add(type="AREA", location=(-50, 75, 80))
    bpy.context.object.data.energy = 45000
    bpy.context.object.data.color = (0.25, 0.55, 0.9)
    bpy.context.object.data.size = 70
    bpy.context.scene.render.filepath = str(PREVIEW / "parrot_authored.png")
    bpy.ops.render.render(write_still=True)


setup()
make_perch()
make_body_and_feet()
make_head()
make_wing(-1)
make_wing(1)
make_tail()
make_lantern()
render_preview()
BLEND.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
join_and_export()
