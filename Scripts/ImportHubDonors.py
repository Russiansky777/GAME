"""Import the curated CC0 Kenney props used by the M1 hub dressing actor.

Run in an Unreal Editor Python session:
    py Scripts/ImportHubDonors.py

The source is the retained Kenney Pirate Kit 2.1 subset documented in
SourceAssets/HubDonors/README.md. All imported static meshes are decorative and
have collision removed; gameplay collision remains owned by CoastalScene.
"""
import os

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "HubDonors", "KenneyPirateKitCC0")
FBX_DIR = os.path.join(SOURCE_DIR, "FBX")
TEXTURE_FILE = os.path.join(SOURCE_DIR, "Textures", "colormap.png")
DESTINATION = "/Game/Generated/HubDonors/KenneyPirateKitCC0"
TEXTURE_DESTINATION = DESTINATION + "/Textures"
MATERIAL_DESTINATION = DESTINATION + "/Materials"

# Source dimensions were inspected in Blender 5.2 before import. Unreal's FBX
# importer converts the source metres to centimetres, so these broad limits catch
# silent unit/axis failures without coupling the script to engine-version rounding.
MESHES = {
    "barrel": "SM_LT_Kenney_Barrel",
    "boat-row-small": "SM_LT_Kenney_Rowboat",
    "chest": "SM_LT_Kenney_Chest",
    "crate-bottles": "SM_LT_Kenney_BottleCrate",
    "crate": "SM_LT_Kenney_Crate",
    "mast-ropes": "SM_LT_Kenney_MastRopes",
    "ship-wreck": "SM_LT_Kenney_ShipWreck",
    "tool-paddle": "SM_LT_Kenney_Paddle",
}


def log(message):
    unreal.log("[ImportHubDonors] " + message)


def fail(message):
    unreal.log_error("[ImportHubDonors] " + message)
    raise RuntimeError(message)


def import_texture():
    if not os.path.isfile(TEXTURE_FILE):
        fail("Missing retained CC0 atlas: " + TEXTURE_FILE)
    unreal.EditorAssetLibrary.make_directory(TEXTURE_DESTINATION)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", TEXTURE_FILE)
    task.set_editor_property("destination_path", TEXTURE_DESTINATION)
    task.set_editor_property("destination_name", "T_LT_KenneyPirate_Colormap")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(TEXTURE_DESTINATION + "/T_LT_KenneyPirate_Colormap")
    if not texture or not isinstance(texture, unreal.Texture2D):
        fail("CC0 atlas import failed")
    return texture


def create_atlas_material(texture):
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DESTINATION)
    path = MATERIAL_DESTINATION + "/M_LT_KenneyPirate_Atlas"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LT_KenneyPirate_Atlas", MATERIAL_DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("used_with_instanced_static_meshes", True)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -260, 0)
    sample.set_editor_property("texture", texture)
    if not unreal.MaterialEditingLibrary.connect_material_property(
            sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR):
        fail("Could not connect donor atlas Base Color")
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -80, 160)
    roughness.set_editor_property("r", 0.78)
    if not unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS):
        fail("Could not connect donor roughness")
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def remove_collision(mesh):
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
    mesh.set_editor_property("customized_collision", True)
    mesh.set_editor_property("light_map_coordinate_index", 1)


def import_mesh(source_stem, asset_name, material):
    filename = os.path.join(FBX_DIR, source_stem + ".fbx")
    if not os.path.isfile(filename):
        fail("Missing selected CC0 FBX: " + filename)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(DESTINATION + "/" + asset_name)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        fail("FBX import did not create " + asset_name)
    size = mesh.get_bounding_box().max - mesh.get_bounding_box().min
    largest = max(size.x, size.y, size.z)
    if min(size.x, size.y, size.z) < 5.0 or largest < 55.0 or largest > 1400.0:
        fail("Implausible centimetre bounds for {}: {}".format(asset_name, size))
    for slot_index in range(len(mesh.get_editor_property("static_materials"))):
        mesh.set_material(slot_index, material)
    remove_collision(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("Imported {} from {} with bounds {} cm and no collision".format(asset_name, source_stem, size))


def main():
    unreal.EditorAssetLibrary.make_directory(DESTINATION)
    material = create_atlas_material(import_texture())
    for source_stem, asset_name in MESHES.items():
        import_mesh(source_stem, asset_name, material)
    log("SUCCESS: imported eight curated Kenney Pirate Kit 2.1 CC0 donor meshes.")


if __name__ == "__main__":
    main()
