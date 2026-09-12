"""Prepare independently animated pieces from the normalized Meshy job board.

Usage:
  blender.exe --background --factory-startup --python Scripts/PrepareHeroJobBoardMotion.py -- \
    Artifacts/HeroJobBoardReview/blender/SM_JobBoard_Hero_import.glb \
    Artifacts/HeroJobBoardReview/motion

The source uses Blender metres with X depth (front -X), Y width and Z up.
"""

import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def args():
    values = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    root = Path(__file__).resolve().parents[1]
    source = Path(values[0]).resolve() if values else root / "Artifacts/HeroJobBoardReview/blender/SM_JobBoard_Hero_import.glb"
    output = Path(values[1]).resolve() if len(values) > 1 else root / "Artifacts/HeroJobBoardReview/motion"
    return source, output


def bounds(obj):
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    lo = Vector(min(p[i] for p in points) for i in range(3))
    hi = Vector(max(p[i] for p in points) for i in range(3))
    return lo, hi


def set_origin_world(obj, point):
    cursor = bpy.context.scene.cursor
    cursor.location = point
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR", center="MEDIAN")
    obj.select_set(False)


def paper_material():
    mat = bpy.data.materials.new("M_JobBoard_PaperMotion")
    mat.diffuse_color = (0.74, 0.64, 0.45, 1.0)
    mat.use_nodes = True
    principled = mat.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = (0.74, 0.64, 0.45, 1.0)
    principled.inputs["Roughness"].default_value = 0.88
    return mat


def add_paper(name, front_x, center_y, top_z, width, height, mat):
    # Four horizontal strips give runtime bending a little silhouette detail while
    # remaining only eight triangles. The pivot is the pinned top-edge midpoint.
    verts = []
    rows = 4
    for row in range(rows + 1):
        z = top_z - height * row / rows
        # Slightly lift the loose lower edge away from the source notices.
        x = front_x - 0.0025 * (row / rows) ** 2
        verts.extend([(x, center_y - width * 0.5, z), (x, center_y + width * 0.5, z)])
    faces = []
    for row in range(rows):
        a = row * 2
        faces.extend([(a, a + 2, a + 3), (a, a + 3, a + 1)])
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.materials.append(mat)
    uv = mesh.uv_layers.new(name="UVMap")
    for poly in mesh.polygons:
        for loop_index in poly.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv.data[loop_index].uv = ((vertex.y - (center_y - width * 0.5)) / width,
                                      (vertex.z - (top_z - height)) / height)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    set_origin_world(obj, Vector((front_x, center_y, top_z)))
    return obj


def render_review(output, objects):
    bpy.ops.object.camera_add(location=(-3.4, -3.2, 2.45))
    camera = bpy.context.object
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 2.55
    target = Vector((0.0, 0.0, 1.08))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera
    bpy.ops.object.light_add(type="AREA", location=(-2.2, -2.0, 3.4))
    bpy.context.object.data.energy = 900
    bpy.context.object.data.shape = "DISK"
    bpy.context.object.data.size = 2.5
    bpy.ops.object.light_add(type="AREA", location=(1.4, 1.6, 2.2))
    bpy.context.object.data.energy = 350
    bpy.context.object.data.size = 1.8
    scene = bpy.context.scene
    # Workbench keeps this deterministic and avoids compiling three embedded 4K
    # texture graphs merely to review silhouette continuity and cut placement.
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(output / "preview.png")
    if scene.world is None:
        scene.world = bpy.data.worlds.new("MotionReviewWorld")
    scene.world.color = (0.035, 0.045, 0.06)
    bpy.ops.render.render(write_still=True)


def main():
    source, output = args()
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source), import_pack_images=True, merge_vertices=False)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if len(meshes) != 1:
        raise RuntimeError(f"Expected one normalized mesh, found {len(meshes)}")
    board = meshes[0]
    board.name = "SM_JobBoard_Hero_Main"
    board.data.name = board.name
    lo, hi = bounds(board)

    # The lantern is the isolated forward protrusion on the viewer's right post.
    # Select by polygon centre in normalized board-relative space; the depth gate
    # excludes the timber behind it and the volume includes the thin top hanger.
    bpy.context.view_layer.objects.active = board
    board.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    bpy.ops.object.mode_set(mode="OBJECT")
    selected = 0
    for poly in board.data.polygons:
        c = board.matrix_world @ poly.center
        hit = (c.x < lo.x + 0.14 and
               lo.y + 0.06 * (hi.y - lo.y) < c.y < lo.y + 0.33 * (hi.y - lo.y) and
               lo.z + 0.43 * (hi.z - lo.z) < c.z < lo.z + 0.91 * (hi.z - lo.z))
        poly.select = hit
        selected += int(hit)
    if selected < 80:
        raise RuntimeError(f"Lantern spatial cut selected only {selected} polygons; refusing unsafe export")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.separate(type="SELECTED")
    bpy.ops.object.mode_set(mode="OBJECT")
    parts = [obj for obj in bpy.context.selected_objects if obj != board and obj.type == "MESH"]
    if len(parts) != 1:
        raise RuntimeError(f"Expected one separated lantern object, found {len(parts)}")
    lantern = parts[0]
    lantern.name = "SM_JobBoard_Hero_Lantern"
    lantern.data.name = lantern.name
    lantern_lo, lantern_hi = bounds(lantern)
    # Pivot where the narrow hanger meets the board, centred across lantern width.
    lantern_pivot = Vector((lantern_hi.x, (lantern_lo.y + lantern_hi.y) * 0.5, lantern_hi.z))
    set_origin_world(lantern, lantern_pivot)
    set_origin_world(board, Vector((0.0, 0.0, 0.0)))

    mat = paper_material()
    # Small lower-edge accents sit just ahead of existing notices on the panel;
    # they add flutter without covering the generated atlas artwork.
    paper_a = add_paper("SM_JobBoard_Hero_Paper_A", -0.092, -0.12, 1.34, 0.08, 0.11, mat)
    paper_b = add_paper("SM_JobBoard_Hero_Paper_B", -0.094, 0.20, 1.10, 0.07, 0.09, mat)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in (board, lantern, paper_a, paper_b):
        obj.select_set(True)
    bpy.context.view_layer.objects.active = board
    export = output.parent / "blender" / "SM_JobBoard_Hero_motion.glb"
    export.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=str(export), export_format="GLB", use_selection=True,
                              export_apply=False, export_yup=True, export_materials="EXPORT")
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "SM_JobBoard_Hero_motion.blend"))
    render_review(output, (board, lantern, paper_a, paper_b))

    report = {
        "source": str(source), "export": str(export), "lantern_polygons": selected,
        "objects": [{"name": obj.name, "vertices": len(obj.data.vertices),
                     "triangles": sum(max(0, len(p.vertices) - 2) for p in obj.data.polygons),
                     "origin_world_m": list(obj.matrix_world.translation),
                     "bounds_world_m": {"min": list(bounds(obj)[0]), "max": list(bounds(obj)[1])}}
                    for obj in (board, lantern, paper_a, paper_b)],
        "runtime_motion": {"lantern": "local Y rotation, +/-2.5 degrees, 2.8 s period",
                           "papers": "local Y rotation, +/-1.0 and +/-0.7 degrees, 1.7/2.1 s periods"},
    }
    (output / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
