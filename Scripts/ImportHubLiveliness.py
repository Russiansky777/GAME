"""Import the project-authored perched parrot kit into /Game/Generated/HubLiveliness."""
import json
import os
import re

import unreal


ROOT = unreal.Paths.project_dir()
SOURCE = os.path.join(ROOT, "SourceAssets", "HubLiveliness")
DEST = "/Game/Generated/HubLiveliness"
EXPECTED = (
    "SM_LT_Parrot_Perch", "SM_LT_Parrot_Body", "SM_LT_Parrot_Head",
    "SM_LT_Parrot_Wing_L", "SM_LT_Parrot_Wing_R", "SM_LT_Parrot_Tail",
    "SM_LT_Lantern_Rope", "SM_LT_Hanging_Lantern",
)


def fail(message):
    unreal.log_error("[ImportHubLiveliness] " + message)
    raise RuntimeError(message)


def mtl_palette(path):
    palette = {}
    current = None
    with open(path, "r", encoding="utf-8") as source:
        for raw in source:
            line = raw.strip()
            if line.startswith("newmtl "):
                current = line.split(None, 1)[1]
            elif current and line.startswith("Kd "):
                palette[current] = [float(value) for value in line.split()[1:4]]
    return palette


def material(name, color):
    safe = re.sub(r"[^A-Za-z0-9_]", "_", name)
    asset_name = "MI_LT_HubLiveliness_" + safe
    path = DEST + "/" + asset_name
    result = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not result:
        result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, DEST, unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew())
    parent = unreal.EditorAssetLibrary.load_asset("/Game/Generated/M1/M_LT_StylizedOpaque")
    if not parent:
        fail("Missing /Game/Generated/M1/M_LT_StylizedOpaque")
    result.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        result, "BaseColor", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(result, "Roughness", 0.8)
    unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result


def main():
    manifest_path = os.path.join(SOURCE, "manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as source:
        manifest = json.load(source)
    names = tuple(entry.get("asset") for entry in manifest.get("components", []))
    if tuple(sorted(names)) != tuple(sorted(EXPECTED)):
        fail("Manifest does not match the eight-piece liveliness contract: {}".format(names))
    unreal.EditorAssetLibrary.make_directory(DEST)
    all_colors = {}
    for name in EXPECTED:
        obj_path = os.path.join(SOURCE, name + ".obj")
        mtl_path = os.path.join(SOURCE, name + ".mtl")
        if not os.path.isfile(obj_path) or not os.path.isfile(mtl_path):
            fail("Missing generated OBJ/MTL for " + name)
        all_colors.update(mtl_palette(mtl_path))
    materials = {name: material(name, color) for name, color in all_colors.items()}

    for name in EXPECTED:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(SOURCE, name + ".obj"))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("import_as_skeletal", False)
        options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
        task.set_editor_property("options", options)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        mesh = unreal.EditorAssetLibrary.load_asset(DEST + "/" + name)
        if not mesh or not isinstance(mesh, unreal.StaticMesh):
            fail("Import did not produce " + name)
        slots = mesh.get_editor_property("static_materials")
        for index, slot in enumerate(slots):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            if slot_name not in materials:
                fail("{} has unexpected material slot {}".format(name, slot_name))
            mesh.set_material(index, materials[slot_name])
        bounds = mesh.get_bounding_box()
        extent = bounds.max - bounds.min
        if max(extent.x, extent.y, extent.z) < 1.0 or max(extent.x, extent.y, extent.z) > 150.0:
            fail("{} imported at implausible size {}".format(name, extent))
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        unreal.log("[ImportHubLiveliness] {} extent {} slots {}".format(name, extent, len(slots)))
    unreal.log("[ImportHubLiveliness] SUCCESS: imported authored parrot/perch/lantern kit")


if __name__ == "__main__":
    main()
