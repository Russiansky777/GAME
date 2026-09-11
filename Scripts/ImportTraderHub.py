"""Import the approved authored trader-hub OBJ kit into its cookable UE asset folder.

Run in an Unreal Editor Python session after visual approval:
    py Scripts/ImportTraderHub.py

The script is intentionally limited to SourceAssets/TraderHub and
/Game/Generated/TraderHub. It does not regenerate terrain or touch the route assets.
"""
import json
import os
import re

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "TraderHub")
MANIFEST_PATH = os.path.join(SOURCE_DIR, "manifest.json")
DESTINATION_PATH = "/Game/Generated/TraderHub"
EXPECTED_ASSETS = (
    "SM_LT_TraderHub_Structure",
    "SM_LT_TraderHub_Awning",
    "SM_LT_TraderHub_Counter",
    "SM_LT_TraderHub_Storage",
    "SM_LT_TraderHub_Nautical",
    "SM_LT_TraderHub_Workbench",
    "SM_LT_TraderHub_Sign",
)


def log(message):
    unreal.log("[ImportTraderHub] " + message)


def fail(message):
    unreal.log_error("[ImportTraderHub] " + message)
    raise RuntimeError(message)


def read_manifest():
    if not os.path.isfile(MANIFEST_PATH):
        fail("Missing approved kit manifest: " + MANIFEST_PATH)
    with open(MANIFEST_PATH, "r", encoding="utf-8") as source:
        manifest = json.load(source)
    # `components` is the generator's canonical name; `meshes` remains accepted for
    # a hand-authored manifest with the same contract.
    meshes = manifest.get("meshes", manifest.get("components"))
    if not isinstance(meshes, list) or len(meshes) != len(EXPECTED_ASSETS):
        fail("manifest.json must contain exactly seven mesh entries")
    normalized_meshes = []
    for entry in meshes:
        normalized = dict(entry)
        asset = normalized.get("asset")
        if isinstance(asset, str):
            normalized["asset"] = asset.rsplit("/", 1)[-1]
        normalized_meshes.append(normalized)
    assets = tuple(entry.get("asset") for entry in normalized_meshes)
    if tuple(sorted(assets)) != tuple(sorted(EXPECTED_ASSETS)):
        fail("Manifest assets do not match the approved seven-piece trader-hub contract: {}".format(assets))
    for entry in normalized_meshes:
        source_name = entry.get("source")
        materials = entry.get("materials")
        if not isinstance(source_name, str) or not source_name.lower().endswith(".obj"):
            fail("Each mesh needs an OBJ `source`: {}".format(entry))
        if not os.path.isfile(os.path.join(SOURCE_DIR, source_name)):
            fail("Missing approved OBJ: " + os.path.join(SOURCE_DIR, source_name))
        if not isinstance(materials, list) or not materials:
            fail("Each mesh needs its ordered non-empty `materials` list: {}".format(entry.get("asset")))
        bounds = entry.get("boundsUnrealImported", entry.get("bounds"))
        if not isinstance(bounds, dict) or not all(isinstance(bounds.get(key), list) and len(bounds[key]) == 3
                                               for key in ("min", "max")):
            fail("Each mesh needs expected UE-imported `bounds` with three-value min/max: {}".format(entry.get("asset")))
    manifest["meshes"] = normalized_meshes
    return manifest


def parse_mtl_colors(manifest):
    palette = dict(manifest.get("palette", {}))
    mtl_name = manifest.get("mtl", "SM_LT_TraderHub.mtl")
    mtl_path = os.path.join(SOURCE_DIR, mtl_name)
    if os.path.isfile(mtl_path):
        current = None
        with open(mtl_path, "r", encoding="utf-8") as source:
            for raw_line in source:
                line = raw_line.strip()
                if line.startswith("newmtl "):
                    current = line.split(None, 1)[1]
                elif current and line.startswith("Kd "):
                    values = line.split()[1:4]
                    if len(values) == 3:
                        palette.setdefault(current, [float(value) for value in values])
    required = {name for entry in manifest["meshes"] for name in entry["materials"]}
    missing = sorted(name for name in required if name not in palette)
    if missing:
        fail("Palette/MTL has no Kd color for: " + ", ".join(missing))
    return palette


def safe_name(name):
    return re.sub(r"[^A-Za-z0-9_]", "_", name)


def create_palette_material(name, color):
    asset_name = "MI_LT_TraderHub_" + safe_name(name)
    asset_path = DESTINATION_PATH + "/" + asset_name
    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, DESTINATION_PATH, unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
    parent = unreal.EditorAssetLibrary.load_asset("/Game/Generated/M1/M_LT_StylizedOpaque")
    if not parent:
        fail("Missing shared parent material /Game/Generated/M1/M_LT_StylizedOpaque")
    material.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material, "BaseColor", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material, "Roughness", 0.82)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def import_mesh(entry, materials):
    asset_path = DESTINATION_PATH + "/" + entry["asset"]
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE_DIR, entry["source"]))
    task.set_editor_property("destination_path", DESTINATION_PATH)
    task.set_editor_property("destination_name", entry["asset"])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    # Keep the authored eight-color palette as MICs of the existing project master.
    # The OBJ's MTL is read only as a manifest cross-check, never imported as masters.
    options = unreal.FbxImportUI()
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_as_skeletal", False)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        fail("OBJ import did not create static mesh " + asset_path)
    slots = mesh.get_editor_property("static_materials")
    slot_count = len(slots)
    if slot_count != len(entry["materials"]):
        fail("{} imported {} material slots; manifest declares {}".format(
            entry["asset"], slot_count, len(entry["materials"])))
    imported_names = [str(slot.get_editor_property("material_slot_name")) for slot in slots]
    if imported_names != entry["materials"]:
        fail("{} imported material slots {}; manifest declares {}".format(
            entry["asset"], imported_names, entry["materials"]))
    imported_bounds = mesh.get_bounding_box()
    actual_bounds = (imported_bounds.min, imported_bounds.max)
    expected = entry.get("boundsUnrealImported", entry["bounds"])
    expected_bounds = (expected["min"], expected["max"])
    for label, actual, expected in zip(("min", "max"), actual_bounds, expected_bounds):
        actual_values = (actual.x, actual.y, actual.z)
        if any(abs(value - float(expected[axis])) > 0.1 for axis, value in enumerate(actual_values)):
            fail("{} imported {} bounds {} but manifest expects {}. Check OBJ Y reflection/winding.".format(
                entry["asset"], label, actual_values, expected))
    for index, material_name in enumerate(entry["materials"]):
        mesh.set_material(index, materials[material_name])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("Imported {} with {} palette slots".format(asset_path, slot_count))


def main():
    manifest = read_manifest()
    palette = parse_mtl_colors(manifest)
    coordinate = manifest.get("coordinate_contract", manifest.get("orientationAuthored", {}))
    front = coordinate.get("front", coordinate.get("frontTowardPlayer"))
    rear = coordinate.get("rear", coordinate.get("rearAwayFromPlayer"))
    if front not in (None, "-X") or rear not in (None, "+X"):
        fail("Kit coordinate contract must be front -X and rear +X")
    unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
    materials = {name: create_palette_material(name, color) for name, color in palette.items()}
    for entry in manifest["meshes"]:
        import_mesh(entry, materials)
    log("SUCCESS: imported seven mesh trader hub. Runtime anchor is (900,-1350,125), yaw -38 degrees.")


if __name__ == "__main__":
    main()
