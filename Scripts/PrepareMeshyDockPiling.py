"""Derive one reusable deep-water piling from the normalized Meshy DockStraight.

The normalized source and its packed PBR material remain untouched.  This script
crops the positive-X/negative-Y corner post, preserves its source mesh attributes,
and lengthens only the post to a grounded four-metre support.
"""
import hashlib
import json
import sys
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Artifacts/MeshyHubReview/DockStraight/SM_Meshy_DockStraight_import.glb"
OUTPUT = ROOT / "Artifacts/MeshyHubReview/DockPiling"
TARGET_HEIGHT_M = 4.0
TARGET_DIAMETER_M = 0.30


def bounds(obj):
    points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    lo = [min(point[i] for point in points) for i in range(3)]
    hi = [max(point[i] for point in points) for i in range(3)]
    return {"min": lo, "max": hi, "size": [hi[i] - lo[i] for i in range(3)]}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def render_preview(obj, output):
    size = bounds(obj)["size"]
    bpy.ops.mesh.primitive_plane_add(size=2.4, location=(0, 0, 0))
    plane = bpy.context.object
    material = bpy.data.materials.new("PreviewGround")
    material.diffuse_color = (0.08, 0.11, 0.13, 1)
    plane.data.materials.append(material)
    bpy.ops.object.camera_add(location=(2.8, -3.4, 2.4))
    camera = bpy.context.object
    camera.rotation_euler = (Vector((0, 0, size[2] * 0.48)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.lens = 58
    bpy.context.scene.camera = camera
    bpy.ops.object.light_add(type="AREA", location=(2.0, -2.0, 4.8))
    bpy.context.object.data.energy = 850
    bpy.context.object.data.size = 3.0
    bpy.ops.object.light_add(type="AREA", location=(-2.0, 1.5, 2.5))
    bpy.context.object.data.energy = 350
    bpy.context.object.data.size = 2.0
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 700
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(output)
    if scene.world is None:
        scene.world = bpy.data.worlds.new("DockPilingPreviewWorld")
    scene.world.color = (0.025, 0.035, 0.05)
    bpy.ops.render.render(write_still=True)


def main():
    if not SOURCE.is_file():
        raise FileNotFoundError(SOURCE)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(SOURCE), import_pack_images=True, merge_vertices=False)
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if len(meshes) != 1:
        raise RuntimeError(f"Expected one normalized DockStraight mesh, found {len(meshes)}")
    post = meshes[0]
    world_transform = post.matrix_world.copy()
    post.parent = None
    post.matrix_world = world_transform
    bpy.context.view_layer.objects.active = post
    post.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    # The source's southeast corner piling is centred close to (+X,-Y). Keep only
    # its clean lower wood shaft below the first dock attachment. Requiring every
    # face vertex to stay inside both limits excludes brackets, braces and deck.
    centre = Vector((1.84, -0.66))
    crop_radius = 0.18
    clean_shaft_top_z = 0.12
    keep = []
    for polygon in post.data.polygons:
        points = [post.matrix_world @ post.data.vertices[index].co for index in polygon.vertices]
        if all((Vector((point.x, point.y)) - centre).length <= crop_radius
               and point.z <= clean_shaft_top_z for point in points):
            keep.append(polygon.index)
    if not keep:
        raise RuntimeError("Corner piling crop selected no faces")
    bpy.context.view_layer.objects.active = post
    post.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    bpy.ops.object.mode_set(mode="OBJECT")
    for index in keep:
        post.data.polygons[index].select = True
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="INVERT")
    bpy.ops.mesh.delete(type="FACE")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.delete_loose()
    bpy.ops.object.mode_set(mode="OBJECT")
    bm = bmesh.new()
    bm.from_mesh(post.data)
    remaining = set(bm.verts)
    islands = []
    while remaining:
        seed = remaining.pop()
        island = {seed}
        pending = [seed]
        while pending:
            vertex = pending.pop()
            for edge in vertex.link_edges:
                neighbour = edge.other_vert(vertex)
                if neighbour in remaining:
                    remaining.remove(neighbour)
                    island.add(neighbour)
                    pending.append(neighbour)
        islands.append(island)
    largest = max(islands, key=len)
    bmesh.ops.delete(bm, geom=[vertex for island in islands if island is not largest for vertex in island],
                     context="VERTS")
    bm.to_mesh(post.data)
    bm.free()
    post.name = "SM_Meshy_DockPiling_import"

    cropped = bounds(post)
    if cropped["size"][2] < 0.08 or max(cropped["size"][:2]) > 0.65:
        raise RuntimeError(f"Implausible corner piling crop bounds: {cropped}")
    diameter_scale = TARGET_DIAMETER_M / max(cropped["size"][:2])
    height_scale = TARGET_HEIGHT_M / cropped["size"][2]
    post.scale.x *= diameter_scale
    post.scale.y *= diameter_scale
    post.scale.z *= height_scale
    bpy.context.view_layer.update()
    grounded = bounds(post)
    post.location += Vector((-(grounded["min"][0] + grounded["max"][0]) * 0.5,
                             -(grounded["min"][1] + grounded["max"][1]) * 0.5,
                             -grounded["min"][2]))
    bpy.context.view_layer.update()
    final_bounds = bounds(post)
    post.data.calc_loop_triangles()

    bpy.ops.object.select_all(action="DESELECT")
    post.select_set(True)
    output_glb = OUTPUT / "SM_Meshy_DockPiling_import.glb"
    bpy.ops.export_scene.gltf(filepath=str(output_glb), export_format="GLB", use_selection=True,
                              export_apply=False, export_yup=True, export_materials="EXPORT",
                              export_skins=False, export_animations=False)
    textures = sorted({node.image.name for material in post.data.materials if material and material.use_nodes
                       for node in material.node_tree.nodes if node.type == "TEX_IMAGE" and node.image})
    report = {
        "derivative_source": str(SOURCE),
        "source_sha256": sha256(SOURCE),
        "output": str(output_glb),
        "output_sha256": sha256(output_glb),
        "method": "clean lower wood shaft cropped below all southeast corner attachments; source UVs, normals and packed PBR material preserved; shaft-only vertical extension",
        "crop_center_xy_m": list(centre),
        "crop_radius_m": crop_radius,
        "clean_shaft_top_z_m": clean_shaft_top_z,
        "cropped_bounds_m": cropped,
        "target_height_m": TARGET_HEIGHT_M,
        "target_diameter_m": TARGET_DIAMETER_M,
        "xy_scale": diameter_scale,
        "z_scale": height_scale,
        "final_bounds_m": final_bounds,
        "pivot": "ground centre; bottom Z=0, top Z=4 m",
        "vertices": len(post.data.vertices),
        "triangles": len(post.data.loop_triangles),
        "material_slots": [slot.material.name if slot.material else None for slot in post.material_slots],
        "textures": textures,
        "reuse_import_materials_from": "/Game/Generated/MeshyModules/Materials/M_Meshy_DockStraight_V1 (do not duplicate textures/materials)",
    }
    (OUTPUT / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if "--no-render" not in sys.argv:
        render_preview(post, OUTPUT / "preview.png")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
