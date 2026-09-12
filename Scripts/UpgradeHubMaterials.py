"""Build cheap reusable hub surface-detail masters and reparent existing MICs.

Run inside Unreal Editor after the current hub assets exist:
    py Scripts/UpgradeHubMaterials.py

The generated shaders use sparse world-space arithmetic so authored OBJ meshes do
not require usable UVs. Existing instance colors, roughness values, and mesh slot
bindings are preserved. The treatment is restrained: long wood grain for deck and
structure colors, a finer crossed weave for cloth, and irregular fibre for rope.
"""
import os

import unreal


DESTINATION = "/Game/Generated/HubMaterials"
SOURCE_DIR = os.path.join(unreal.Paths.project_dir(), "SourceAssets", "HubMaterials")
INSTANCE_GROUPS = {
    "wood": (
        "/Game/Generated/HubSlice/MI_LT_HubSlice_WarmWood",
        "/Game/Generated/HubSlice/MI_LT_HubSlice_DarkWood",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_DarkWood",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_WeatheredCream",
        "/Game/Generated/HubSlice/MI_LT_HubSlice_TealPaint",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_TealPaint",
    ),
    "cloth": (
        "/Game/Generated/TraderHub/MI_LT_TraderHub_CreamCanvas",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_RustCanvas",
    ),
    "fibre": (
        "/Game/Generated/HubSlice/MI_LT_HubSlice_Rope",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_Rope",
        "/Game/Generated/TraderHub/MI_LT_TraderHub_MapPaper",
    ),
}

COLOR_REFINEMENTS = {
    "/Game/Generated/HubSlice/MI_LT_HubSlice_DarkWood": (0.32, 0.16, 0.070),
    "/Game/Generated/TraderHub/MI_LT_TraderHub_DarkWood": (0.32, 0.16, 0.070),
    "/Game/Generated/TraderHub/MI_LT_TraderHub_WeatheredCream": (0.48, 0.31, 0.16),
}


def log(message):
    unreal.log("[UpgradeHubMaterials] " + message)


def fail(message):
    unreal.log_error("[UpgradeHubMaterials] " + message)
    raise RuntimeError(message)


def expression(material, kind, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, kind, x, y)


def connect(source, output, destination, input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, output, destination, input_name):
        fail("Could not connect {} to {}".format(type(source).__name__, input_name))


def connect_property(source, output, prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(source, output, prop):
        fail("Could not connect material property {}".format(prop))


def scalar(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def multiply(material, a, b, x, y):
    node = expression(material, unreal.MaterialExpressionMultiply, x, y)
    connect(a, "", node, "A")
    connect(b, "", node, "B")
    return node


def add(material, a, b, x, y):
    node = expression(material, unreal.MaterialExpressionAdd, x, y)
    connect(a, "", node, "A")
    connect(b, "", node, "B")
    return node


def sine(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionSine, x, y)
    connect(value, "", node, "")
    return node


def make_master(family):
    names = {"wood": "M_LT_HubWoodDetailV2", "cloth": "M_LT_HubClothDetailV2", "fibre": "M_LT_HubFibreDetail"}
    path = DESTINATION + "/" + names[family]
    material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    # UE 5.8 can assert when DeleteAllMaterialExpressions removes rooted nodes from
    # a master already referenced by loaded MICs. Generated masters are immutable:
    # reuse a complete existing asset, and create it only when absent.
    if material:
        return material
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        names[family], DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("used_with_instanced_static_meshes", True)

    base = expression(material, unreal.MaterialExpressionVectorParameter, -900, -220)
    base.set_editor_property("parameter_name", "BaseColor")
    base.set_editor_property("default_value", unreal.LinearColor(0.32, 0.18, 0.08, 1.0))
    roughness = expression(material, unreal.MaterialExpressionScalarParameter, -900, 420)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.82)

    world = expression(material, unreal.MaterialExpressionWorldPosition, -900, 0)
    x_mask = expression(material, unreal.MaterialExpressionComponentMask, -720, -20)
    x_mask.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
    connect(world, "XYZ", x_mask, "")
    y_mask = expression(material, unreal.MaterialExpressionComponentMask, -720, 120)
    y_mask.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
    connect(world, "XYZ", y_mask, "")

    if family == "wood":
        texture = unreal.EditorAssetLibrary.load_asset(DESTINATION + "/Textures/T_LT_HubWoodGrain")
        if not texture:
            fail("Missing imported project-authored wood grain texture")
        z_mask = expression(material, unreal.MaterialExpressionComponentMask, -720, 260)
        z_mask.set_editor_properties({"r": False, "g": False, "b": True, "a": False})
        connect(world, "XYZ", z_mask, "")
        scale = scalar(material, 0.004, -520, 350)
        # Rotate the horizontal projection 30 degrees to follow the authored deck.
        top_u = add(material, multiply(material, x_mask, scalar(material, 0.866, -620, -120), -470, -130),
                    multiply(material, y_mask, scalar(material, 0.5, -620, 0), -470, -10), -320, -80)
        top_v = add(material, multiply(material, x_mask, scalar(material, -0.5, -620, 90), -470, 80),
                    multiply(material, y_mask, scalar(material, 0.866, -620, 200), -470, 190), -320, 130)
        uv_top = expression(material, unreal.MaterialExpressionAppendVector, -130, -40)
        connect(multiply(material, top_u, scale, -260, -90), "", uv_top, "A")
        connect(multiply(material, top_v, scale, -260, 30), "", uv_top, "B")
        uv_side = expression(material, unreal.MaterialExpressionAppendVector, -330, 170)
        connect(multiply(material, x_mask, scale, -520, 150), "", uv_side, "A")
        connect(multiply(material, z_mask, scale, -520, 270), "", uv_side, "B")
        top_sample = expression(material, unreal.MaterialExpressionTextureSample, -120, -80)
        side_sample = expression(material, unreal.MaterialExpressionTextureSample, -120, 160)
        top_sample.set_editor_property("texture", texture)
        side_sample.set_editor_property("texture", texture)
        connect(uv_top, "", top_sample, "UVs")
        connect(uv_side, "", side_sample, "UVs")
        normal = expression(material, unreal.MaterialExpressionPixelNormalWS, -330, 380)
        normal_z = expression(material, unreal.MaterialExpressionComponentMask, -120, 370)
        normal_z.set_editor_properties({"r": False, "g": False, "b": True, "a": False})
        connect(normal, "", normal_z, "")
        absolute_z = expression(material, unreal.MaterialExpressionAbs, 60, 360)
        connect(normal_z, "", absolute_z, "")
        projected = expression(material, unreal.MaterialExpressionLinearInterpolate, 170, 60)
        connect(side_sample, "R", projected, "A")
        connect(top_sample, "R", projected, "B")
        connect(absolute_z, "", projected, "Alpha")
        # Remap the authored 0.45..0.92 grain to restrained 0.84..1.12 colour gain.
        detail = add(material, multiply(material, projected, scalar(material, 0.61, 180, 250), 360, 150),
                     scalar(material, 0.56, 350, 290), 540, 150)
    elif family == "cloth":
        weave_x = sine(material, multiply(material, x_mask, scalar(material, 0.12, -700, -150), -520, -100), -330, -100)
        weave_y = sine(material, multiply(material, y_mask, scalar(material, 0.115, -520, 240), -340, 180), -160, 180)
        detail = multiply(material, add(material, weave_x, weave_y, 10, 20), scalar(material, 0.018, 0, 230), 210, 40)

        # One-centimetre breeze on canvas only. Approximate local vertical position
        # from ObjectPositionWS, saturating the upper half so the lower attachment
        # edge stays visually pinned instead of making the whole canopy rubbery.
        world_z = expression(material, unreal.MaterialExpressionComponentMask, -700, 520)
        world_z.set_editor_properties({"r": False, "g": False, "b": True, "a": False})
        connect(world, "XYZ", world_z, "")
        object_position = expression(material, unreal.MaterialExpressionObjectPositionWS, -700, 650)
        object_z = expression(material, unreal.MaterialExpressionComponentMask, -520, 650)
        object_z.set_editor_properties({"r": False, "g": False, "b": True, "a": False})
        connect(object_position, "", object_z, "")
        height_delta = expression(material, unreal.MaterialExpressionSubtract, -340, 570)
        connect(world_z, "", height_delta, "A")
        connect(object_z, "", height_delta, "B")
        raised = add(material, height_delta, scalar(material, 150.0, -340, 700), -160, 620)
        normalized = expression(material, unreal.MaterialExpressionDivide, 20, 620)
        connect(raised, "", normalized, "A")
        connect(scalar(material, 300.0, -150, 760), "", normalized, "B")
        pin_weight = expression(material, unreal.MaterialExpressionSaturate, 190, 620)
        connect(normalized, "", pin_weight, "")

        time = expression(material, unreal.MaterialExpressionTime, -520, 830)
        phase = add(material, multiply(material, time, scalar(material, 0.26, -520, 940), -340, 850),
                    multiply(material, x_mask, scalar(material, 0.012, -340, 1030), -160, 900), 20, 850)
        breeze = sine(material, phase, 190, 850)
        weighted_breeze = multiply(material, breeze, pin_weight, 370, 760)
        direction = expression(material, unreal.MaterialExpressionConstant3Vector, 370, 920)
        direction.set_editor_property("constant", unreal.LinearColor(0.36, 0.93, 0.0, 1.0))
        offset = multiply(material, weighted_breeze, direction, 560, 820)
        connect_property(offset, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    else:
        diagonal = add(material, x_mask, multiply(material, y_mask, scalar(material, 0.55, -520, 220), -340, 170), -160, 80)
        detail = multiply(material, sine(material, multiply(material, diagonal, scalar(material, 0.09, -180, 250), 10, 150), 190, 100),
                          scalar(material, 0.028, 180, 260), 370, 120)

    one = scalar(material, 1.0, 390, -120)
    tint = detail if family == "wood" else add(material, one, detail, 590, -40)
    color = multiply(material, base, tint, 780, -100)
    rough_strength = 0.04 if family == "wood" else 0.24
    rough_detail = add(material, roughness, multiply(material, detail, scalar(material, rough_strength, 420, 320), 600, 280), 800, 230)
    connect_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(rough_detail, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def main():
    unreal.EditorAssetLibrary.make_directory(DESTINATION)
    texture_destination = DESTINATION + "/Textures"
    unreal.EditorAssetLibrary.make_directory(texture_destination)
    texture_file = os.path.join(SOURCE_DIR, "T_LT_HubWoodGrain.png")
    if not os.path.isfile(texture_file):
        fail("Generate the authored wood grain first: " + texture_file)
    task = unreal.AssetImportTask()
    task.set_editor_properties({"filename": texture_file, "destination_path": texture_destination,
                                "destination_name": "T_LT_HubWoodGrain", "automated": True,
                                "replace_existing": True, "save": True})
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    masters = {family: make_master(family) for family in INSTANCE_GROUPS}
    changed = 0
    for family, paths in INSTANCE_GROUPS.items():
        for path in paths:
            instance = unreal.EditorAssetLibrary.load_asset(path)
            if not instance or not isinstance(instance, unreal.MaterialInstanceConstant):
                fail("Missing expected hub material instance: " + path)
            instance.set_editor_property("parent", masters[family])
            if path in COLOR_REFINEMENTS:
                color = COLOR_REFINEMENTS[path]
                unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
                    instance, "BaseColor", unreal.LinearColor(color[0], color[1], color[2], 1.0))
            unreal.MaterialEditingLibrary.update_material_instance(instance)
            unreal.EditorAssetLibrary.save_loaded_asset(instance)
            changed += 1
    log("SUCCESS: created 3 reusable detail masters and preserved/reparented {} hub instances.".format(changed))


if __name__ == "__main__":
    main()
