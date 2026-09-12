"""Import the source-derived DockStraight piling and reuse its validated master."""

import os
import json
import hashlib
import unreal

PROJECT = unreal.Paths.project_dir()
SOURCE = os.path.join(PROJECT, "Artifacts", "MeshyHubReview", "DockPiling", "SM_Meshy_DockPiling_import.glb")
DEST = "/Game/Generated/MeshyModules/Dock/DockPiling"
FOLDER = DEST + "/SM_Meshy_DockPiling_import"
MESH_PATH = FOLDER + "/StaticMeshes/SM_Meshy_DockPiling_import"
MATERIAL_PATH = "/Game/Generated/MeshyModules/Materials/M_Meshy_DockStraight_V1"
REPORT = os.path.join(PROJECT, "Artifacts", "MeshyHubReview", "DockPiling", "report.json")


def fail(message):
    unreal.log_error("[ImportMeshyDockPiling] " + message)
    raise RuntimeError(message)


def main():
    if not os.path.isfile(SOURCE):
        fail("Missing prepared piling GLB: " + SOURCE)
    force_reimport = "-MeshyPilingReimport" in unreal.SystemLibrary.get_command_line()
    if force_reimport or not unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", SOURCE)
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
    if not isinstance(mesh, unreal.StaticMesh) or not isinstance(material, unreal.Material):
        fail("Missing stable mesh or DockStraight material")
    material.set_editor_property("used_with_instanced_static_meshes", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    mesh.set_material(0, material)
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
    mesh.set_editor_property("customized_collision", True)
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    if not os.path.isfile(REPORT):
        fail("Missing derivative report: " + REPORT)
    with open(REPORT, "r", encoding="utf-8") as handle:
        report = json.load(handle)
    expected_hash = report.get("output_sha256")
    if not expected_hash:
        fail("Derivative report must record output_sha256 before import")
    with open(SOURCE, "rb") as handle:
        actual_hash = hashlib.sha256(handle.read()).hexdigest().upper()
    if actual_hash != expected_hash.upper():
        fail("Derivative GLB hash does not match locked report: {} != {}".format(actual_hash, expected_hash))
    expected = tuple(value * 100.0 for value in report["final_bounds_m"]["size"])
    if abs(expected[2] - report["target_height_m"] * 100.0) > 0.5:
        fail("Derivative report final height does not meet target height: {}".format(expected[2]))
    if abs(max(expected[0], expected[1]) - report["target_diameter_m"] * 100.0) > 0.5:
        fail("Derivative report final diameter does not meet target diameter: {}".format(expected[:2]))
    actual = (size.x, size.y, size.z)
    if any(abs(value - target) > 1.0 for value, target in zip(actual, expected)) or abs(bounds.min.z) > 0.5:
        fail("Unexpected bounds min={} max={} size={}".format(bounds.min, bounds.max, actual))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    # The derivative keeps DockStraight UVs, so its temporary imported texture and
    # material assets are redundant after binding the established DockStraight master.
    for path in unreal.EditorAssetLibrary.list_assets(FOLDER, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if path != MESH_PATH and isinstance(asset, (unreal.Texture2D, unreal.MaterialInstance, unreal.Material)):
            if not unreal.EditorAssetLibrary.delete_asset(path):
                fail("Could not remove redundant imported asset " + path)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("[ImportMeshyDockPiling] SUCCESS mesh={} material={} bounds min={} max={} cm".format(
        MESH_PATH, MATERIAL_PATH, bounds.min, bounds.max))


if __name__ == "__main__":
    main()
