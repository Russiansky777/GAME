"""Import the inspected Meshy hut and parrot with UE 5.8 Interchange glTF.

Run Scripts/InspectMeshyHub.py in Blender first. Then run this file through an
Unreal Editor Python session while no other process has the assets loaded.
"""

import os
import unreal


PROJECT = unreal.Paths.project_dir()
ROOT = os.path.join(PROJECT, "Artifacts", "MeshyHubReview")
DEST = "/Game/Generated/MeshyHub"
SPECS = {
    "Budka": {
        "source": os.path.join(ROOT, "Budka", "SM_MeshyHub_Budka_import.glb"),
        "destination": DEST + "/Budka",
        "folder": DEST + "/Budka/SM_MeshyHub_Budka_import",
        "mesh": DEST + "/Budka/SM_MeshyHub_Budka_import/StaticMeshes/SM_MeshyHub_Budka_import",
        "material": DEST + "/Materials/M_MeshyHub_Budka_V1",
        "expected_size": (500.0, 375.241, 344.390),
    },
    "Papug": {
        "source": os.path.join(ROOT, "Papug", "SM_MeshyHub_Papug_import.glb"),
        "destination": DEST + "/Papug",
        "folder": DEST + "/Papug/SM_MeshyHub_Papug_import",
        "mesh": DEST + "/Papug/SM_MeshyHub_Papug_import/StaticMeshes/SM_MeshyHub_Papug_import",
        "material": DEST + "/Materials/M_MeshyHub_Papug_V1",
        "expected_size": (61.395, 67.030, 140.0),
    },
}


def fail(message):
    unreal.log_error("[ImportMeshyHub] " + message)
    raise RuntimeError(message)


def log(message):
    unreal.log("[ImportMeshyHub] " + message)


def imported_textures(folder):
    textures = []
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Texture2D):
            textures.append(asset)
    by_name = {texture.get_name().lower(): texture for texture in textures}
    selected = []
    for wanted in ("image_0", "image_1", "image_2"):
        matches = [texture for name, texture in by_name.items() if name == wanted or name.endswith("_" + wanted)]
        if len(matches) != 1:
            fail("Expected exactly one {} below {}; found {}".format(
                wanted, folder, [texture.get_path_name() for texture in textures]))
        selected.append(matches[0])
    return selected


def expression(material, cls, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, cls, x, y)


def rebuild_material(path, base_color, packed_mr, normal):
    directory, name = path.rsplit("/", 1)
    unreal.EditorAssetLibrary.make_directory(directory)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        material = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(material, unreal.Material):
            fail(path + " exists but is not a Material")
        samples = [node for node in unreal.MaterialEditingLibrary.get_material_expressions(material)
                   if isinstance(node, unreal.MaterialExpressionTextureSample)]
        if len(samples) != 3:
            fail("Existing versioned graph has unexpected structure; create a new version instead of deleting graph roots: " + path)
        samples.sort(key=lambda node: node.get_editor_property("material_expression_editor_y"))
        base, packed, normal_sample = samples
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, directory, unreal.Material, unreal.MaterialFactoryNew())
        base = expression(material, unreal.MaterialExpressionTextureSample, -620, -100)
        packed = expression(material, unreal.MaterialExpressionTextureSample, -620, 140)
        normal_sample = expression(material, unreal.MaterialExpressionTextureSample, -620, 380)
    # Update the actual nodes on repeat runs. UE 5.8 can root expressions in a
    # way that makes delete-all unsafe, so structural changes require a new
    # versioned master rather than deleting an existing graph.
    material.set_editor_property("two_sided", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    base.set_editor_property("texture", base_color)
    base.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    packed.set_editor_property("texture", packed_mr)
    packed.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    normal_sample.set_editor_property("texture", normal)
    normal_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    links = (
        (base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR),
        (packed, "G", unreal.MaterialProperty.MP_ROUGHNESS),
        (packed, "B", unreal.MaterialProperty.MP_METALLIC),
        (normal_sample, "RGB", unreal.MaterialProperty.MP_NORMAL),
    )
    for node, output, prop in links:
        if not unreal.MaterialEditingLibrary.connect_material_property(node, output, prop):
            fail("Could not connect {} to {} on {}".format(output, prop, path))
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def validate_texture_settings(base, packed, normal):
    base.set_editor_property("srgb", True)
    packed.set_editor_property("srgb", False)
    packed.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    normal.set_editor_property("srgb", False)
    normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    # UE 5.8 Interchange converts glTF's normal convention by setting this once;
    # keep and assert it rather than flipping pixels or the material a second time.
    if not normal.get_editor_property("flip_green_channel"):
        fail("Interchange normal texture did not retain required flip_green_channel: " + normal.get_path_name())
    for texture in (base, packed, normal):
        unreal.EditorAssetLibrary.save_loaded_asset(texture)


def import_one(label, spec):
    if not os.path.isfile(spec["source"]):
        fail("Missing normalized GLB; run Scripts/InspectMeshyHub.py: " + spec["source"])
    unreal.EditorAssetLibrary.make_directory(spec["destination"])
    if not unreal.EditorAssetLibrary.does_asset_exist(spec["mesh"]):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", spec["source"])
        task.set_editor_property("destination_path", spec["destination"])
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        # Interchange glTF owns its factory settings; legacy FBX options are invalid.
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(spec["mesh"])
    if not isinstance(mesh, unreal.StaticMesh):
        fail("Interchange did not create stable mesh path " + spec["mesh"])
    base, packed, normal = imported_textures(spec["folder"])
    validate_texture_settings(base, packed, normal)
    material = rebuild_material(spec["material"], base, packed, normal)
    mesh.set_material(0, material)
    # StaticMeshEditorSubsystem is unavailable in commandlet context in UE 5.8.
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
    mesh.set_editor_property("customized_collision", True)
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    actual = (size.x, size.y, size.z)
    if any(abs(value - expected) > 1.0 for value, expected in zip(actual, spec["expected_size"])):
        fail("{} unexpected UE bounds size {}; expected {} cm".format(label, actual, spec["expected_size"]))
    if abs(bounds.min.z) > 0.5:
        fail("{} expected ground pivot min Z=0; got {}".format(label, bounds.min.z))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    imported_material = spec["folder"] + "/Materials/Material_0"
    if unreal.EditorAssetLibrary.does_asset_exist(imported_material):
        if not unreal.EditorAssetLibrary.delete_asset(imported_material):
            fail("Could not remove unused Interchange material " + imported_material)
    log("{} SUCCESS mesh={} material={} bounds min={} max={} cm; native front +Y".format(
        label, spec["mesh"], spec["material"], bounds.min, bounds.max))


def main():
    for label, spec in SPECS.items():
        import_one(label, spec)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("SUCCESS: imported and validated Budka and Papug")


if __name__ == "__main__":
    main()
