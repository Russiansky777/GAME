"""Import the inspected hero job board through UE 5.8 Interchange glTF.

Run InspectHeroJobBoard.py first, then in an Unreal Editor Python session:
    py Scripts/ImportHeroJobBoard.py
"""
import os

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
INTERMEDIATE = os.path.join(PROJECT_DIR, "Artifacts", "HeroJobBoardReview", "blender", "SM_JobBoard_Hero_import.glb")
MOTION_INTERMEDIATE = os.path.join(PROJECT_DIR, "Artifacts", "HeroJobBoardReview", "blender", "SM_JobBoard_Hero_motion.glb")
DEST = "/Game/Generated/HeroJobBoard"
MESH_PATH = DEST + "/SM_JobBoard_Hero"
MATERIAL_DIR = DEST + "/Materials"
MATERIAL_PATH = MATERIAL_DIR + "/M_JobBoard_Hero_V2"


def log(message):
    unreal.log("[ImportHeroJobBoard] " + message)


def fail(message):
    unreal.log_error("[ImportHeroJobBoard] " + message)
    raise RuntimeError(message)


def texture_by_fragment(fragment):
    matches = []
    for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Texture2D) and fragment.lower() in asset.get_name().lower():
            matches.append(asset)
    if len(matches) != 1:
        fail("Expected one texture containing {!r}, found {}".format(fragment, [x.get_path_name() for x in matches]))
    return matches[0]


def sample(material, texture, x, y, sampler_type=None):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", texture)
    if sampler_type is not None:
        node.set_editor_property("sampler_type", sampler_type)
    return node


def create_material(base_color, orm, emissive):
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if not isinstance(material, unreal.Material):
            fail(MATERIAL_PATH + " exists but is not a Material")
        # TC_MASKS requires the Masks sampler on SM5, including existing graphs.
        for node in unreal.MaterialEditingLibrary.get_material_expressions(material):
            if isinstance(node, unreal.MaterialExpressionTextureSample):
                texture = node.get_editor_property("texture")
                if texture and "metallic_roughness" in texture.get_name():
                    node.set_editor_property("texture", orm)
                    node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        return material
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_JobBoard_Hero_V2", MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())
    # Source glTF explicitly declares doubleSided=true.
    material.set_editor_property("two_sided", True)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    base = sample(material, base_color, -620, -80, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    packed = sample(material, orm, -620, 180, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    emit = sample(material, emissive, -620, 420, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    connections = (
        (base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR),
        (packed, "G", unreal.MaterialProperty.MP_ROUGHNESS),
        (packed, "B", unreal.MaterialProperty.MP_METALLIC),
        (emit, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR),
    )
    for node, output, prop in connections:
        if not unreal.MaterialEditingLibrary.connect_material_property(node, output, prop):
            fail("Could not connect {} to {}".format(output, prop))
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def create_paper_material():
    path = MATERIAL_DIR + "/M_JobBoard_PaperEdge"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_JobBoard_PaperEdge", MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property("constant", unreal.LinearColor(0.55, 0.43, 0.29, 1.0))
    roughness = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 150)
    roughness.set_editor_property("r", 0.9)
    assert unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    assert unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material

def main():
    if not os.path.isfile(INTERMEDIATE):
        fail("Missing normalized intermediate; run Scripts/InspectHeroJobBoard.py first: " + INTERMEDIATE)
    unreal.EditorAssetLibrary.make_directory(DEST)
    # Interchange reimport preserves the original source-node transform in 5.8;
    # recreating this one scoped mesh makes axis/pivot normalization reproducible.
    if unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH):
        if not unreal.EditorAssetLibrary.delete_asset(MESH_PATH):
            fail("Could not replace existing static mesh " + MESH_PATH)
    interchange_folder = DEST + "/SM_JobBoard_Hero_import"
    if unreal.EditorAssetLibrary.does_directory_exist(interchange_folder):
        # UE may retain the now-empty Content Browser folder on disk; only assets
        # matter for deterministic replacement.
        unreal.EditorAssetLibrary.delete_directory(interchange_folder)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", INTERMEDIATE)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "SM_JobBoard_Hero")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    # GLB is handled by UE 5.8's Interchange glTF translator. Deliberately leave
    # task.options unset: legacy FBX options do not apply to Interchange glTF.
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH) if unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH) else None
    if not isinstance(mesh, unreal.StaticMesh):
        source_path = interchange_folder + "/StaticMeshes/SM_JobBoard_Hero"
        if not unreal.EditorAssetLibrary.does_asset_exist(source_path):
            fail("Interchange did not create expected static mesh " + source_path)
        if not unreal.EditorAssetLibrary.rename_asset(source_path, MESH_PATH):
            fail("Could not normalize imported mesh name from " + source_path)
        mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)

    # Exact-name filtering avoids selecting the base texture for either suffix.
    textures = [unreal.EditorAssetLibrary.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(interchange_folder, True, False)]
    textures = [t for t in textures if isinstance(t, unreal.Texture2D)]
    orm_matches = [t for t in textures if "metallic_roughness" in t.get_name().lower()]
    emit_matches = [t for t in textures if "emit" in t.get_name().lower()]
    base_matches = [t for t in textures if "emit" not in t.get_name().lower() and "metallic_roughness" not in t.get_name().lower()]
    if len(orm_matches) != 1 or len(emit_matches) != 1 or len(base_matches) != 1:
        fail("Expected base/emissive/ORM texture triplet; got {}".format([t.get_name() for t in textures]))
    base_color, orm, emissive = base_matches[0], orm_matches[0], emit_matches[0]
    base_color.set_editor_property("srgb", True)
    emissive.set_editor_property("srgb", True)
    orm.set_editor_property("srgb", False)
    orm.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    for texture in (base_color, orm, emissive):
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
    material = create_material(base_color, orm, emissive)
    mesh.set_material(0, material)
    # Runtime owns a simple structural proxy; keep visual mesh collision-free.
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
    mesh.set_editor_property("customized_collision", True)
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    expected = (41.56, 180.0, 210.0)
    actual = (size.x, size.y, size.z)
    if any(abs(a - e) > 1.0 for a, e in zip(actual, expected)):
        fail("Unexpected UE bounds {} cm; expected approximately {} (front-X,widthY,upZ)".format(actual, expected))
    if abs(bounds.min.z) > 0.5:
        fail("Expected bottom-origin Z=0, imported min Z={}".format(bounds.min.z))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("SUCCESS: {} bounds min={} max={} cm; front -X, width Y, up Z, bottom origin.".format(MESH_PATH, bounds.min, bounds.max))

    # If the deterministic motion preparation has been run, replace the merged
    # visual with its lossless main/lantern split. Paper overlays are intentionally
    # imported only after their authored review is complete.
    if os.path.isfile(MOTION_INTERMEDIATE):
        motion_folder = DEST + "/SM_JobBoard_Hero_motion"
        if unreal.EditorAssetLibrary.does_directory_exist(motion_folder):
            unreal.EditorAssetLibrary.delete_directory(motion_folder)
        motion_task = unreal.AssetImportTask()
        motion_task.set_editor_property("filename", MOTION_INTERMEDIATE)
        motion_task.set_editor_property("destination_path", DEST)
        motion_task.set_editor_property("automated", True)
        motion_task.set_editor_property("replace_existing", True)
        motion_task.set_editor_property("save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([motion_task])
        for source_name, target_name in (
                ("SM_JobBoard_Hero_Main", "SM_JobBoard_Hero"),
                ("SM_JobBoard_Hero_Lantern", "SM_JobBoard_Hero_Lantern"),
                ("SM_JobBoard_Hero_Paper_A", "SM_JobBoard_Hero_Paper_A"),
                ("SM_JobBoard_Hero_Paper_B", "SM_JobBoard_Hero_Paper_B")):
            source_path = motion_folder + "/StaticMeshes/" + source_name
            if not unreal.EditorAssetLibrary.does_asset_exist(source_path):
                fail("Motion Interchange import missing " + source_path)
            target_path = source_path
            part = unreal.EditorAssetLibrary.load_asset(source_path)
            if "Paper" not in target_name:
                part.set_material(0, material)
            else:
                part.set_material(0, create_paper_material())
            unreal.EditorStaticMeshLibrary.remove_collisions(part)
            part.set_editor_property("customized_collision", True)
            unreal.EditorAssetLibrary.save_loaded_asset(part)
            part_bounds = part.get_bounding_box()
            log("Motion part {} bounds min={} max={}".format(target_path, part_bounds.min, part_bounds.max))


if __name__ == "__main__":
    main()
