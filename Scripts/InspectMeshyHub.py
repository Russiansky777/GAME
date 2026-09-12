"""Inspect and prepare the Director-provided Meshy hut and parrot GLBs.

Run with Blender 5.2:
  blender.exe --background --factory-startup --python Scripts/InspectMeshyHub.py

The source GLBs are read-only. Reports, previews, inspection blends and normalized
Interchange inputs are written under ignored Artifacts/MeshyHubReview.
"""

import hashlib
import json
import math
import struct
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Vector


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "Artifacts/MeshyHubReview"
SOURCES = {
    "Budka": {"path": ROOT / "SourceAssets/Generated/Budka/Budka.glb", "target_plan_m": 5.0,
              "native_front": "UE +Y (source storefront is Blender -Y)"},
    "Papug": {"path": ROOT / "SourceAssets/Generated/Papug/Papug.glb", "target_height_m": 1.40,
              "native_front": "UE +Y (source bird face is Blender -Y)"},
    "Lodka": {"path": ROOT / "SourceAssets/Generated/Lodka/Lodka.glb", "target_plan_m": 4.0,
              "native_front": "bow UE -X (source bow is Blender -X)"},
    "Verstak": {"path": ROOT / "SourceAssets/Generated/Verstak/Verstak.glb", "target_plan_m": 2.0,
                "native_front": "working side UE +Y (source working side is Blender -Y)"},
    "DeckCorner": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-corner-lowpoly.glb", "target_plan_m": 2.0, "native_front": "plan axes UE X, reflected UE Y"},
    "DeckLong": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-long-lowpoly.glb", "target_plan_m": 2.0, "native_front": "long axis UE X"},
    "DeckOuterEdge": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-outer-edge-lowpoly.glb", "target_plan_m": 2.0, "native_front": "long axis UE X; source sides reflected on UE Y"},
    "DeckRepaired": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-repaired-lowpoly.glb", "target_plan_m": 2.0, "native_front": "long axis UE X"},
    "DeckStandard": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-standard-lowpoly.glb", "target_plan_m": 2.0, "native_front": "long axis UE X"},
    "DeckStepRamp": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-step-ramp-lowpoly.glb", "target_plan_m": 2.0, "native_front": "step direction UE Y after reflection"},
    "DeckTransition": {"path": ROOT / "SourceAssets/Generated/Deck/deck-module-transition-lowpoly.glb", "target_plan_m": 2.0, "native_front": "long axis UE Y after reflection"},
    "DockCornerPlatform": {"path": ROOT / "SourceAssets/Generated/Dock/Dock Corner Platform.glb", "target_plan_m": 4.0, "native_front": "long axis UE Y after reflection"},
    "DockEndBerth": {"path": ROOT / "SourceAssets/Generated/Dock/Dock End Berth Section.glb", "target_plan_m": 4.0, "native_front": "long axis UE X"},
    "DockLadderAccess": {"path": ROOT / "SourceAssets/Generated/Dock/Dock Ladder Access Section.glb", "target_plan_m": 4.0, "native_front": "long axis UE X; ladder side reflected on UE Y"},
    "DockRepaired": {"path": ROOT / "SourceAssets/Generated/Dock/Dock Repaired Weathered Variant.glb", "target_plan_m": 4.0, "native_front": "long axis UE X; repair side reflected on UE Y"},
    "DockStraight": {"path": ROOT / "SourceAssets/Generated/Dock/Dock Straight Section.glb", "target_plan_m": 4.0, "native_front": "long axis UE X"},
    "Crane": {"path": ROOT / "SourceAssets/Generated/Crane/Meshy_AI_coastal_salvage_crane_0912114729_texture.glb", "target_height_m": 3.0, "native_front": "boom axis requires UE import/profile confirmation"},
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def glb_json(path):
    with path.open("rb") as stream:
        magic, version, length = struct.unpack("<4sII", stream.read(12))
        if magic != b"glTF" or version != 2 or length != path.stat().st_size:
            raise RuntimeError(f"Invalid GLB 2 header: {path}")
        chunk_length, chunk_type = struct.unpack("<II", stream.read(8))
        if chunk_type != 0x4E4F534A:
            raise RuntimeError(f"First GLB chunk is not JSON: {path}")
        return json.loads(stream.read(chunk_length).decode("utf-8"))


def bounds_for(obj):
    corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo = [min(p[i] for p in corners) for i in range(3)]
    hi = [max(p[i] for p in corners) for i in range(3)]
    return {"min": lo, "max": hi, "size": [hi[i] - lo[i] for i in range(3)]}


def image_record(image):
    return {
        "name": image.name,
        "size": list(image.size),
        "colorspace": image.colorspace_settings.name,
        "source": image.source,
        "packed": image.packed_file is not None,
    }


def material_record(material):
    record = {
        "name": material.name,
        "surface_render_method": getattr(material, "surface_render_method", None),
        "use_nodes": material.use_nodes,
        "textures": [],
    }
    if not material.use_nodes:
        return record
    for node in material.node_tree.nodes:
        if node.type != "TEX_IMAGE" or not node.image:
            continue
        image = image_record(node.image)
        image["destinations"] = [
            f"{link.to_node.name}.{link.to_socket.name}"
            for socket in node.outputs for link in socket.links
        ]
        record["textures"].append(image)
    return record


def mesh_record(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    non_manifold = 0
    edge_faces = {edge.index: 0 for edge in mesh.edges}
    for polygon in mesh.polygons:
        for edge_index in polygon.edge_keys:
            pass
    # Count boundary and over-shared edges from loop triangles without mutating data.
    edge_use = {}
    for triangle in mesh.loop_triangles:
        verts = triangle.vertices
        for a, b in ((verts[0], verts[1]), (verts[1], verts[2]), (verts[2], verts[0])):
            key = tuple(sorted((a, b)))
            edge_use[key] = edge_use.get(key, 0) + 1
    non_manifold = sum(1 for uses in edge_use.values() if uses != 2)
    upward_bins = {}
    normal_matrix = obj.matrix_world.to_3x3().inverted().transposed()
    for polygon in mesh.polygons:
        world_normal = (normal_matrix @ polygon.normal).normalized()
        if world_normal.z < 0.8:
            continue
        world_center = obj.matrix_world @ polygon.center
        z_key = round(world_center.z, 3)
        entry = upward_bins.setdefault(z_key, {"area": 0.0, "points": []})
        entry["area"] += polygon.area
        entry["points"].extend(obj.matrix_world @ mesh.vertices[index].co for index in polygon.vertices)
    upward_peaks = [
        {
            "z_m": z,
            "source_mesh_area_m2": entry["area"],
            "bounds_xy_m": {
                "min": [min(point[i] for point in entry["points"]) for i in range(2)],
                "max": [max(point[i] for point in entry["points"]) for i in range(2)],
            },
        }
        for z, entry in sorted(upward_bins.items(), key=lambda item: item[1]["area"], reverse=True)[:12]
    ]
    return {
        "name": obj.name,
        "mesh": mesh.name,
        "parent": obj.parent.name if obj.parent else None,
        "transform": {
            "location": list(obj.location),
            "rotation_euler_rad": list(obj.rotation_euler),
            "scale": list(obj.scale),
        },
        "vertices": len(mesh.vertices),
        "edges": len(mesh.edges),
        "polygons": len(mesh.polygons),
        "triangles": len(mesh.loop_triangles),
        "loops": len(mesh.loops),
        "uv_layers": [layer.name for layer in mesh.uv_layers],
        "color_attributes": [layer.name for layer in mesh.color_attributes],
        "material_slots": [slot.material.name if slot.material else None for slot in obj.material_slots],
        # Blender may report zero loop normals before evaluated-mesh calculation;
        # authoritative source-normal presence is glb.primitive_attributes.NORMAL.
        "blender_loop_normals_all_nonzero": bool(mesh.loops) and all(loop.normal.length_squared > 0 for loop in mesh.loops),
        "non_manifold_or_boundary_edges": non_manifold,
        "dominant_upward_surface_z": upward_peaks,
        "bounds_world_m": bounds_for(obj),
    }


def combined_bounds(objects):
    records = [bounds_for(obj) for obj in objects]
    lo = [min(record["min"][i] for record in records) for i in range(3)]
    hi = [max(record["max"][i] for record in records) for i in range(3)]
    return {"min": lo, "max": hi, "size": [hi[i] - lo[i] for i in range(3)]}


def render_preview(output, bounds):
    center = Vector([(bounds["min"][i] + bounds["max"][i]) * 0.5 for i in range(3)])
    extent = max(bounds["size"])
    bpy.ops.object.camera_add(location=center + Vector((extent * 2.1, -extent * 2.1, extent * 1.35)))
    camera = bpy.context.object
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = extent * 1.32
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = camera
    bpy.ops.object.light_add(type="AREA", location=center + Vector((extent, -extent, extent * 1.8)))
    bpy.context.object.data.energy = 1000
    bpy.context.object.data.shape = "DISK"
    bpy.context.object.data.size = extent * 1.5
    bpy.ops.object.light_add(type="AREA", location=center + Vector((-extent, extent, extent)))
    bpy.context.object.data.energy = 500
    bpy.context.object.data.size = extent
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1000
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(output)
    if scene.world is None:
        scene.world = bpy.data.worlds.new("MeshyInspectionWorld")
    scene.world.color = (0.025, 0.035, 0.05)
    bpy.ops.render.render(write_still=True)


def inspect(label, spec, output_root, no_render):
    source = spec["path"]
    if not source.is_file():
        raise FileNotFoundError(source)
    output = output_root / label
    output.mkdir(parents=True, exist_ok=True)
    raw = glb_json(source)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source), import_pack_images=True, merge_vertices=False)
    objects = list(bpy.context.scene.objects)
    meshes = [obj for obj in objects if obj.type == "MESH"]
    bounds = combined_bounds(meshes)
    report = {
        "source": str(source),
        "source_bytes": source.stat().st_size,
        "source_sha256": sha256(source),
        "blender_version": bpy.app.version_string,
        "glb": {
            "asset": raw.get("asset", {}),
            "scene_count": len(raw.get("scenes", [])),
            "node_count": len(raw.get("nodes", [])),
            "mesh_count": len(raw.get("meshes", [])),
            "material_count": len(raw.get("materials", [])),
            "texture_count": len(raw.get("textures", [])),
            "image_count": len(raw.get("images", [])),
            "skin_count": len(raw.get("skins", [])),
            "animation_count": len(raw.get("animations", [])),
            "materials": raw.get("materials", []),
            "skins": raw.get("skins", []),
            "animations": raw.get("animations", []),
            "primitive_attributes": [
                primitive.get("attributes", {})
                for mesh in raw.get("meshes", []) for primitive in mesh.get("primitives", [])
            ],
        },
        "objects": [{
            "name": obj.name, "type": obj.type,
            "parent": obj.parent.name if obj.parent else None,
            "children": [child.name for child in obj.children],
        } for obj in objects],
        "meshes": [mesh_record(obj) for obj in meshes],
        "materials": [material_record(mat) for mat in bpy.data.materials],
        "armatures": [{
            "name": obj.name,
            "bones": [bone.name for bone in obj.data.bones],
            "action": obj.animation_data.action.name if obj.animation_data and obj.animation_data.action else None,
        } for obj in objects if obj.type == "ARMATURE"],
        "actions": [{"name": action.name, "frame_range": list(action.frame_range), "fcurves": len(action.fcurves)} for action in bpy.data.actions],
        "combined_bounds_world_m": bounds,
        "totals": {
            "objects": len(objects), "meshes": len(meshes),
            "vertices": sum(len(obj.data.vertices) for obj in meshes),
            "triangles": sum(len(obj.data.loop_triangles) for obj in meshes),
            "materials": len(bpy.data.materials), "images": len(bpy.data.images),
        },
    }
    (output / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    bpy.ops.wm.save_as_mainfile(filepath=str(output / f"{label}_inspection.blend"))

    if not spec.get("target_plan_m") and not spec.get("target_height_m"):
        if not no_render:
            render_preview(output / "preview.png", bounds)
        return

    # Meshy sources are Y-up glTF. Preserve every mesh/material and apply one
    # uniform scale to the common hierarchy, then bake a ground-centred pivot.
    target_plan = spec.get("target_plan_m")
    target_height = spec.get("target_height_m")
    scale = target_plan / max(bounds["size"][0], bounds["size"][1]) if target_plan else target_height / bounds["size"][2]
    roots = [obj for obj in objects if obj.parent is None]
    for obj in roots:
        obj.scale *= scale
    bpy.context.view_layer.update()
    normalized_bounds = combined_bounds(meshes)
    offset = Vector((
        -(normalized_bounds["min"][0] + normalized_bounds["max"][0]) * 0.5,
        -(normalized_bounds["min"][1] + normalized_bounds["max"][1]) * 0.5,
        -normalized_bounds["min"][2],
    ))
    for obj in roots:
        obj.location += offset
    bpy.context.view_layer.update()
    for obj in objects:
        obj.select_set(obj.type in {"MESH", "ARMATURE", "EMPTY"})
    normalized = output / f"SM_Meshy_{label}_import.glb"
    bpy.ops.export_scene.gltf(
        filepath=str(normalized), export_format="GLB", use_selection=True,
        export_apply=False, export_yup=True, export_materials="EXPORT",
        export_skins=True, export_animations=True,
    )
    report["normalization"] = {
        "uniform_scale": scale,
        "target": (f"{target_plan:.2f} m maximum plan dimension" if target_plan else
                   "1.40 m overall grounded height (~0.65-0.75 m bird body+tail)"),
        "normalized_bounds_world_m": combined_bounds(meshes),
        "pivot": "ground centre",
        "unreal_axes": "Blender +X becomes UE +X; Blender +Y becomes UE -Y; Blender +Z becomes UE +Z",
        "native_front": spec["native_front"],
    }
    (output / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if not no_render:
        # Restore source-scale inspection scene for honest source previews.
        bpy.ops.wm.open_mainfile(filepath=str(output / f"{label}_inspection.blend"))
        render_preview(output / "preview.png", bounds)


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    output = Path(args[0]).resolve() if args and not args[0].startswith("--") else DEFAULT_OUTPUT
    no_render = "--no-render" in args
    requested = next((arg.split("=", 1)[1] for arg in args if arg.startswith("--only=")), None)
    modules_only = "--modules-only" in args
    for label, spec in SOURCES.items():
        if requested and label.lower() != requested.lower():
            continue
        if modules_only and not (label.startswith("Deck") or label.startswith("Dock") or label == "Crane"):
            continue
        inspect(label, spec, output, no_render)


if __name__ == "__main__":
    main()
