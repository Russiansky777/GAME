"""Inspect the generated hero job-board GLB with Blender 5.2.

Usage:
  blender.exe --background --factory-startup --python Scripts/InspectHeroJobBoard.py -- \
      SourceAssets/Generated/JobBoard/SM_JobBoard_Hero_v01.glb \
      Artifacts/HeroJobBoardReview/blender
"""

import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


def argv():
    args = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    root = Path(__file__).resolve().parents[1]
    source = Path(args[0]).resolve() if args else root / "SourceAssets/Generated/JobBoard/SM_JobBoard_Hero_v01.glb"
    output = Path(args[1]).resolve() if len(args) > 1 else root / "Artifacts/HeroJobBoardReview/blender"
    return source, output


def image_info(node):
    if not node or node.type != "TEX_IMAGE" or not node.image:
        return None
    image = node.image
    return {
        "name": image.name,
        "size": list(image.size),
        "colorspace": image.colorspace_settings.name,
        "source": image.source,
    }


def material_info(mat):
    data = {"name": mat.name, "blend_method": getattr(mat, "surface_render_method", None), "textures": []}
    if not mat.use_nodes:
        return data
    for node in mat.node_tree.nodes:
        info = image_info(node)
        if info:
            sockets = []
            for output in node.outputs:
                for link in output.links:
                    sockets.append(f"{link.to_node.name}.{link.to_socket.name}")
            info["destinations"] = sockets
            data["textures"].append(info)
    return data


def mesh_info(obj):
    mesh = obj.data
    world_corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    mins = [min(v[i] for v in world_corners) for i in range(3)]
    maxs = [max(v[i] for v in world_corners) for i in range(3)]
    loops_with_custom_normals = sum(1 for loop in mesh.loops if loop.normal.length_squared > 0.0)
    adjacency = [set() for _ in mesh.vertices]
    for edge in mesh.edges:
        a, b = edge.vertices
        adjacency[a].add(b)
        adjacency[b].add(a)
    unseen = set(range(len(mesh.vertices)))
    components = []
    while unseen:
        seed = unseen.pop()
        stack = [seed]
        indices = [seed]
        while stack:
            for other in adjacency[stack.pop()]:
                if other in unseen:
                    unseen.remove(other)
                    stack.append(other)
                    indices.append(other)
        points = [obj.matrix_world @ mesh.vertices[i].co for i in indices]
        cmin = [min(v[i] for v in points) for i in range(3)]
        cmax = [max(v[i] for v in points) for i in range(3)]
        components.append({
            "vertices": len(indices),
            "bounds_world_m": {"min": cmin, "max": cmax, "size": [cmax[i] - cmin[i] for i in range(3)]},
        })
    components.sort(key=lambda item: item["vertices"], reverse=True)
    # Repeat connectivity after welding coincident POSITION values for analysis.
    # This does not mutate source vertices, UVs, normals or the exported mesh.
    position_keys = [tuple(round(value, 5) for value in vertex.co) for vertex in mesh.vertices]
    welded_points = {}
    for index, key in enumerate(position_keys):
        welded_points.setdefault(key, []).append(index)
    welded_adjacency = {key: set() for key in welded_points}
    for edge in mesh.edges:
        a = position_keys[edge.vertices[0]]
        b = position_keys[edge.vertices[1]]
        welded_adjacency[a].add(b)
        welded_adjacency[b].add(a)
    welded_unseen = set(welded_points)
    welded_components = []
    while welded_unseen:
        seed = welded_unseen.pop()
        stack = [seed]
        keys = [seed]
        while stack:
            for other in welded_adjacency[stack.pop()]:
                if other in welded_unseen:
                    welded_unseen.remove(other)
                    stack.append(other)
                    keys.append(other)
        points = [obj.matrix_world @ Vector(key) for key in keys]
        cmin = [min(v[i] for v in points) for i in range(3)]
        cmax = [max(v[i] for v in points) for i in range(3)]
        welded_components.append({
            "unique_positions": len(keys),
            "source_vertices": sum(len(welded_points[key]) for key in keys),
            "bounds_world_m": {"min": cmin, "max": cmax, "size": [cmax[i] - cmin[i] for i in range(3)]},
        })
    welded_components.sort(key=lambda item: item["source_vertices"], reverse=True)
    return {
        "name": obj.name,
        "mesh": mesh.name,
        "parent": obj.parent.name if obj.parent else None,
        "location_m": list(obj.location),
        "rotation_euler_rad": list(obj.rotation_euler),
        "scale": list(obj.scale),
        "vertices": len(mesh.vertices),
        "edges": len(mesh.edges),
        "polygons": len(mesh.polygons),
        "triangles": sum(max(0, len(poly.vertices) - 2) for poly in mesh.polygons),
        "uv_layers": [layer.name for layer in mesh.uv_layers],
        "color_attributes": [layer.name for layer in mesh.color_attributes],
        "material_slots": [slot.material.name if slot.material else None for slot in obj.material_slots],
        "normals_present": loops_with_custom_normals == len(mesh.loops) and len(mesh.loops) > 0,
        "connected_components": components,
        "position_welded_components": welded_components,
        "bounds_world_m": {"min": mins, "max": maxs, "size": [maxs[i] - mins[i] for i in range(3)]},
    }


def main():
    source, output = argv()
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source), import_pack_images=True, merge_vertices=False)

    meshes = [mesh_info(obj) for obj in bpy.context.scene.objects if obj.type == "MESH"]
    materials = [material_info(mat) for mat in bpy.data.materials]
    report = {
        "source": str(source),
        "blender_version": bpy.app.version_string,
        "objects": [
            {
                "name": obj.name,
                "type": obj.type,
                "parent": obj.parent.name if obj.parent else None,
                "children": [child.name for child in obj.children],
            }
            for obj in bpy.context.scene.objects
        ],
        "meshes": meshes,
        "materials": materials,
        "totals": {
            "objects": len(bpy.context.scene.objects),
            "meshes": len(meshes),
            "vertices": sum(item["vertices"] for item in meshes),
            "triangles": sum(item["triangles"] for item in meshes),
            "materials": len(materials),
            "images": len(bpy.data.images),
        },
    }
    if meshes:
        report["combined_bounds_world_m"] = {
            "min": [min(item["bounds_world_m"]["min"][i] for item in meshes) for i in range(3)],
            "max": [max(item["bounds_world_m"]["max"][i] for item in meshes) for i in range(3)],
        }
        report["combined_bounds_world_m"]["size"] = [
            report["combined_bounds_world_m"]["max"][i] - report["combined_bounds_world_m"]["min"][i]
            for i in range(3)
        ]

    (output / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    bpy.ops.wm.save_as_mainfile(filepath=str(output / "SM_JobBoard_Hero_v01_inspection.blend"))

    # Produce the deterministic import intermediate. Keep source depth natural,
    # normalize width/height, center plan axes, and bake a bottom-centre pivot.
    if len(meshes) != 1:
        raise RuntimeError("Expected the approved source to contain exactly one mesh")
    source_obj = next(obj for obj in bpy.context.scene.objects if obj.type == "MESH")
    size = report["combined_bounds_world_m"]["size"]
    source_obj.scale.x *= 1.8 / size[0]
    source_obj.scale.z *= 2.1 / size[2]
    bpy.context.view_layer.objects.active = source_obj
    source_obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    corners = [source_obj.matrix_world @ Vector(corner) for corner in source_obj.bound_box]
    mins = [min(v[i] for v in corners) for i in range(3)]
    maxs = [max(v[i] for v in corners) for i in range(3)]
    source_obj.location += Vector((-(mins[0] + maxs[0]) * 0.5, -(mins[1] + maxs[1]) * 0.5, -mins[2]))
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    # UE keeps glTF plan axes. Rotate -90 degrees so depth/front becomes X
    # (front -X) and the 180 cm span becomes width Y.
    source_obj.data.transform(Matrix.Rotation(-math.pi * 0.5, 4, "Z"))
    source_obj.name = "SM_JobBoard_Hero"
    source_obj.data.name = "SM_JobBoard_Hero"
    normalized = Path(bpy.path.abspath("//")) / "SM_JobBoard_Hero_import.glb"
    bpy.ops.export_scene.gltf(
        filepath=str(normalized), export_format="GLB", use_selection=True,
        export_apply=False, export_yup=True, export_materials="EXPORT")

    if "--no-render" in sys.argv:
        return

    # Orthographic three-quarter evidence render; dimensions are derived from the source.
    bounds = report["combined_bounds_world_m"]
    center = Vector([(bounds["min"][i] + bounds["max"][i]) * 0.5 for i in range(3)])
    size = max(bounds["size"])
    bpy.ops.object.camera_add(location=center + Vector((size * 2.3, -size * 2.3, size * 1.5)))
    camera = bpy.context.object
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = size * 1.35
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera
    bpy.ops.object.light_add(type="AREA", location=center + Vector((size, -size, size * 1.8)))
    bpy.context.object.data.energy = 900
    bpy.context.object.data.shape = "DISK"
    bpy.context.object.data.size = size * 1.5
    bpy.ops.object.light_add(type="AREA", location=center + Vector((-size, size, size)))
    bpy.context.object.data.energy = 450
    bpy.context.object.data.size = size
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(output / "preview.png")
    if scene.world is None:
        scene.world = bpy.data.worlds.new("InspectionWorld")
    scene.world.color = (0.035, 0.045, 0.06)
    bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main()
