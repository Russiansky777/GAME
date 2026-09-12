"""Import the hub quality slice and its local-origin job board into Unreal.

Run inside an Unreal Editor Python session:
    py Scripts/ImportHubSlice.py

Only SourceAssets/HubSlice and /Game/Generated/HubSlice are used. Imported meshes
remain decorative; runtime traversal and collision continue to come from CoastalScene.
"""
import json
import os
import re

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "HubSlice")
MANIFEST_PATH = os.path.join(SOURCE_DIR, "manifest.json")
DESTINATION_PATH = "/Game/Generated/HubSlice"
NATURE_SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "QuaterniusNature", "FBX")
NATURE_TEXTURE_SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "QuaterniusNature", "Textures")
NATURE_DESTINATION_PATH = "/Game/Generated/HubNature"
PARENT_MATERIAL = "/Game/Generated/M1/M_LT_StylizedOpaque"
EXPECTED_ASSETS = (
    "SM_LT_HubSlice_Apron",
    "SM_LT_HubSlice_WorkCluster",
    "SM_LT_HubSlice_GoodsCluster",
    "SM_LT_HubSlice_MaritimeDetails",
    "SM_LT_HubSlice_GroundDressing",
    "SM_LT_HubSlice_JobBoard",
)
NATURE_ASSETS = (
    "Bush_Common_Flowers", "Fern_1", "Grass_Common_Tall", "Grass_Wispy_Tall", "Plant_1_Big",
    "Rock_Medium_1", "Rock_Medium_2", "Rock_Medium_3", "RockPath_Round_Wide", "TwistedTree_1",
)
NATURE_TEXTURES = {
    "Leaves_NormalTree": "Leaves_NormalTree_C.png", "Flowers": "Flowers.png",
    "Leaves": "Leaves.png", "Grass": "Grass.png", "Bark_TwistedTree": "Bark_TwistedTree.png",
    "Leaves_TwistedTree": "Leaves_TwistedTree_C.png", "Rocks": "Rocks_Diffuse.png",
    "PathRocks": "PathRocks_Diffuse.png",
}


def log(message):
    unreal.log("[ImportHubSlice] " + message)


def fail(message):
    unreal.log_error("[ImportHubSlice] " + message)
    raise RuntimeError(message)


def connect_expression(source, source_output, destination, destination_input):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(
            source, source_output, destination, destination_input):
        fail("Could not connect material expression output '{}' to input '{}'".format(
            source_output, destination_input))


def connect_property(source, source_output, material_property):
    if not unreal.MaterialEditingLibrary.connect_material_property(
            source, source_output, material_property):
        fail("Could not connect material expression output '{}' to property '{}'".format(
            source_output, material_property))


def safe_name(value):
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def read_manifest():
    if not os.path.isfile(MANIFEST_PATH):
        fail("Missing hub-slice manifest: " + MANIFEST_PATH)
    with open(MANIFEST_PATH, "r", encoding="utf-8") as source:
        manifest = json.load(source)
    entries = manifest.get("meshes", manifest.get("components"))
    if not isinstance(entries, list) or len(entries) != len(EXPECTED_ASSETS):
        fail("manifest.json must contain exactly six mesh entries")
    normalized = []
    for raw_entry in entries:
        entry = dict(raw_entry)
        asset = entry.get("asset", entry.get("name"))
        if isinstance(asset, str):
            asset = asset.rsplit("/", 1)[-1]
        entry["asset"] = asset
        entry["source"] = entry.get("source", entry.get("obj", asset + ".obj" if asset else None))
        normalized.append(entry)
    assets = tuple(entry.get("asset") for entry in normalized)
    if tuple(sorted(assets)) != tuple(sorted(EXPECTED_ASSETS)):
        fail("Manifest assets do not match the approved five-piece contract: {}".format(assets))

    contract = manifest.get("coordinate_contract", manifest.get("coordinateContract", {}))
    if contract.get("worldOrigin", manifest.get("worldOrigin")) is not True:
        fail("Hub slice must declare worldOrigin=true")
    units = str(contract.get("units", manifest.get("units", ""))).lower()
    if units not in ("cm", "centimeters", "centimetres"):
        fail("Hub slice must declare centimetre units")
    ground_top = contract.get("groundTopZ", manifest.get("groundTopZ"))
    if ground_top is None or abs(float(ground_top) - 126.0) > 0.01:
        fail("Hub slice must declare groundTopZ=126.0")

    for entry in normalized:
        source_name = entry.get("source")
        materials = entry.get("materials")
        if not isinstance(source_name, str) or not source_name.lower().endswith(".obj"):
            fail("Each hub mesh requires an OBJ source: {}".format(entry))
        if not os.path.isfile(os.path.join(SOURCE_DIR, source_name)):
            fail("Missing hub OBJ: " + os.path.join(SOURCE_DIR, source_name))
        if entry.get("collision", False) is not False:
            fail("Hub slice meshes must declare collision=false: " + entry["asset"])
        if not isinstance(materials, list) or not materials:
            fail("Each hub mesh requires an ordered materials list: " + entry["asset"])
        bounds = entry.get("boundsUnrealImported", entry.get("bounds", entry.get("boundsCm")))
        if not isinstance(bounds, dict) or not all(
                isinstance(bounds.get(key), list) and len(bounds[key]) == 3 for key in ("min", "max")):
            fail("Each hub mesh requires imported min/max bounds: " + entry["asset"])
    manifest["meshes"] = normalized
    return manifest


def read_palette(manifest):
    palette = dict(manifest.get("palette", {}))
    for filename in os.listdir(SOURCE_DIR):
        if not filename.lower().endswith(".mtl"):
            continue
        current = None
        with open(os.path.join(SOURCE_DIR, filename), "r", encoding="utf-8") as source:
            for raw_line in source:
                line = raw_line.strip()
                if line.startswith("newmtl "):
                    current = line.split(None, 1)[1]
                elif current and line.startswith("Kd "):
                    values = line.split()[1:4]
                    if len(values) == 3:
                        palette.setdefault(current, [float(value) for value in values])
    required = {name for entry in manifest["meshes"] for name in entry["materials"]}
    missing = sorted(required.difference(palette))
    if missing:
        fail("Palette/MTL has no color for: " + ", ".join(missing))
    if len(required) > 9:
        fail("Hub slice exceeds the approved nine-slot palette")
    return palette


def create_material(name, color):
    asset_name = "MI_LT_HubSlice_" + safe_name(name)
    asset_path = DESTINATION_PATH + "/" + asset_name
    material = unreal.EditorAssetLibrary.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, DESTINATION_PATH, unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
    parent = unreal.EditorAssetLibrary.load_asset(PARENT_MATERIAL)
    if not parent:
        fail("Missing shared opaque parent material: " + PARENT_MATERIAL)
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
    options = unreal.FbxImportUI()
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_as_skeletal", False)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        fail("OBJ import did not create " + asset_path)
    slots = mesh.get_editor_property("static_materials")
    if len(slots) != len(entry["materials"]):
        fail("{} imported {} slots; manifest declares {}".format(
            entry["asset"], len(slots), len(entry["materials"])))
    imported_names = [str(slot.get_editor_property("material_slot_name")) for slot in slots]
    if imported_names != entry["materials"]:
        fail("{} imported slots {}; manifest declares {}".format(
            entry["asset"], imported_names, entry["materials"]))
    bounds = mesh.get_bounding_box()
    expected = entry.get("boundsUnrealImported", entry.get("bounds", entry.get("boundsCm")))
    for label, actual in (("min", bounds.min), ("max", bounds.max)):
        values = (actual.x, actual.y, actual.z)
        if any(abs(value - float(expected[label][axis])) > 0.1 for axis, value in enumerate(values)):
            fail("{} {} bounds {} differ from manifest {}".format(
                entry["asset"], label, values, expected[label]))
    for index, material_name in enumerate(entry["materials"]):
        mesh.set_material(index, materials[material_name])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("Imported {} with {} material slots".format(asset_path, len(slots)))


def create_nature_materials():
    texture_path = NATURE_DESTINATION_PATH + "/Textures"
    material_path = NATURE_DESTINATION_PATH + "/Materials"
    unreal.EditorAssetLibrary.make_directory(texture_path)
    unreal.EditorAssetLibrary.make_directory(material_path)
    results = {}
    for material_name, filename in NATURE_TEXTURES.items():
        source_path = os.path.join(NATURE_TEXTURE_SOURCE_DIR, filename)
        if not os.path.isfile(source_path):
            fail("Missing curated CC0 texture: " + source_path)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", texture_path)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.EditorAssetLibrary.load_asset(texture_path + "/" + os.path.splitext(filename)[0])
        if not texture or not isinstance(texture, unreal.Texture2D):
            fail("Texture import failed: " + filename)
        asset_name = "M_LT_Nature_" + material_name
        full_path = material_path + "/" + asset_name
        if unreal.EditorAssetLibrary.does_asset_exist(full_path):
            unreal.EditorAssetLibrary.delete_asset(full_path)
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, material_path, unreal.Material, unreal.MaterialFactoryNew())
        is_card = material_name in ("Leaves_NormalTree", "Flowers", "Leaves", "Grass", "Leaves_TwistedTree")
        material.set_editor_property("used_with_instanced_static_meshes", True)
        material.set_editor_property("two_sided", is_card)
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED if is_card else unreal.BlendMode.BLEND_OPAQUE)
        if is_card:
            material.set_editor_property("opacity_mask_clip_value", 0.2)
        sample = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureSample, -300, 0)
        sample.set_editor_property("texture", texture)
        base_color_expression = sample
        base_color_output = "RGB"
        if material_name == "Leaves_TwistedTree":
            luminance = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionDotProduct, -100, -40)
            luminance_weights = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionConstant3Vector, -300, 100)
            luminance_weights.set_editor_property(
                "constant", unreal.LinearColor(0.299, 0.587, 0.114, 1.0))
            connect_expression(sample, "RGB", luminance, "A")
            connect_expression(luminance_weights, "", luminance, "B")
            tint = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionConstant3Vector, -100, 70)
            tint.set_editor_property("constant", unreal.LinearColor(0.42, 1.0, 0.34, 1.0))
            multiply = unreal.MaterialEditingLibrary.create_material_expression(
                material, unreal.MaterialExpressionMultiply, 100, 0)
            connect_expression(luminance, "", multiply, "A")
            connect_expression(tint, "", multiply, "B")
            base_color_expression = multiply
            base_color_output = ""
        connect_property(base_color_expression, base_color_output, unreal.MaterialProperty.MP_BASE_COLOR)
        if is_card:
            connect_property(sample, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -100, 180)
        roughness.set_editor_property("r", 0.86)
        connect_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        results[material_name.lower()] = material
    return results


def nature_material_for_slot(slot_name, materials):
    compact = slot_name.lower().replace("_", "")
    ordered_matches = (
        ("leavesnormaltree", "leaves_normaltree"), ("leavestwistedtree", "leaves_twistedtree"),
        ("barktwistedtree", "bark_twistedtree"), ("pathrocks", "pathrocks"),
        ("flowers", "flowers"), ("grass", "grass"), ("leaves", "leaves"), ("rocks", "rocks"),
    )
    for token, key in ordered_matches:
        if token in compact:
            return materials[key]
    return None


def main():
    manifest = read_manifest()
    palette = read_palette(manifest)
    unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
    materials = {name: create_material(name, color) for name, color in palette.items()}
    for entry in manifest["meshes"]:
        import_mesh(entry, materials)
    unreal.EditorAssetLibrary.make_directory(NATURE_DESTINATION_PATH)
    nature_materials = create_nature_materials()
    for asset_name in NATURE_ASSETS:
        source_path = os.path.join(NATURE_SOURCE_DIR, asset_name + ".fbx")
        if not os.path.isfile(source_path):
            fail("Missing curated CC0 nature source: " + source_path)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", NATURE_DESTINATION_PATH)
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
        mesh_path = NATURE_DESTINATION_PATH + "/" + asset_name
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            fail("Nature import did not create " + mesh_path)
        bounds = mesh.get_bounding_box()
        size = bounds.max - bounds.min
        if min(size.x, size.y, size.z) <= 0.1 or max(size.x, size.y, size.z) > 2000.0:
            fail("Implausible imported bounds for {}: {}. Check FBX units/axes.".format(asset_name, size))
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            material = nature_material_for_slot(slot_name, nature_materials)
            if not material:
                fail("No verified CC0 texture mapping for {} material slot {}".format(asset_name, slot_name))
            mesh.set_material(index, material)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        log("Imported CC0 nature {} with bounds {}".format(asset_name, size))
    obsolete_auto_materials = (
        "Bark_TwistedTree", "Flowers", "Grass", "Leaves_NormalTree",
        "Leaves_TwistedTree", "Leaves", "PathRocks", "Rocks",
    )
    for asset_name in obsolete_auto_materials:
        asset_path = NATURE_DESTINATION_PATH + "/" + asset_name
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            continue
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset or not isinstance(asset, unreal.MaterialInterface):
            fail("Refusing to delete non-Material obsolete asset: " + asset_path)
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            fail("Could not delete obsolete auto-imported material: " + asset_path)
    log("SUCCESS: imported five world-origin dressing meshes and one local-origin job board.")


if __name__ == "__main__":
    if "-HubNatureMaterialsOnly" in unreal.SystemLibrary.get_command_line():
        create_nature_materials()
        log("SUCCESS: rebuilt hub nature materials only.")
    elif "-HubApronOnly" in unreal.SystemLibrary.get_command_line():
        apron_entry = next(entry for entry in read_manifest()["meshes"]
                           if entry["asset"] == "SM_LT_HubSlice_Apron")
        apron_materials = {}
        for material_name in apron_entry["materials"]:
            material_path = DESTINATION_PATH + "/MI_LT_HubSlice_" + safe_name(material_name)
            material = unreal.EditorAssetLibrary.load_asset(material_path)
            if not material:
                fail("Missing existing hub material for apron-only import: " + material_path)
            apron_materials[material_name] = material
        import_mesh(apron_entry, apron_materials)
        log("SUCCESS: reimported hub apron only.")
    else:
        main()
