"""Create LOW TIDE's inexpensive animated coastal-water material.

Run in an Unreal Editor Python session. The script authors deterministic tileable
TGA inputs under SourceAssets/Generated/Water and creates only assets below
/Game/Generated/Water. It never changes water geometry, collision, or tide logic.
"""
import math
import os
import random
import struct

import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "Generated", "Water")
DESTINATION = "/Game/Generated/Water"
MATERIAL_PATH = DESTINATION + "/M_LT_CoastalWater"
SIZE = 512


def log(message):
    unreal.log("[PolishWater] " + message)


def fail(message):
    unreal.log_error("[PolishWater] " + message)
    raise RuntimeError(message)


def write_tga(path, pixels):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, SIZE, SIZE, 32, 0x28)
    with open(path, "wb") as target:
        target.write(header)
        for red, green, blue, alpha in pixels:
            target.write(bytes((blue, green, red, alpha)))


def tileable_height(seed, octave_count=22):
    rng = random.Random(seed)
    waves = []
    for index in range(octave_count):
        band = 1 + index // 5
        limit = 2 + band * 2
        # Signed lattice vectors keep the texture exactly tileable while
        # distributing ridges across both diagonal families and varied angles.
        while True:
            kx = rng.randint(-limit, limit)
            ky = rng.randint(-limit, limit)
            if kx != 0 and ky != 0:
                break
        phase = rng.random() * math.tau
        amplitude = rng.uniform(0.55, 1.0) / (1.0 + 0.36 * (kx * kx + ky * ky))
        waves.append((kx, ky, phase, amplitude))
    values = []
    for y in range(SIZE):
        v = y / SIZE
        for x in range(SIZE):
            u = x / SIZE
            value = sum(amplitude * math.sin(math.tau * (kx * u + ky * v) + phase)
                        for kx, ky, phase, amplitude in waves)
            values.append(value)
    low, high = min(values), max(values)
    return [(value - low) / (high - low) for value in values]


def normal_pixels(height, strength):
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            left = height[y * SIZE + (x - 1) % SIZE]
            right = height[y * SIZE + (x + 1) % SIZE]
            down = height[((y - 1) % SIZE) * SIZE + x]
            up = height[((y + 1) % SIZE) * SIZE + x]
            nx, ny, nz = -(right - left) * strength, -(up - down) * strength, 1.0
            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            nx, ny, nz = nx / length, ny / length, nz / length
            pixels.append((round((nx * 0.5 + 0.5) * 255), round((ny * 0.5 + 0.5) * 255),
                           round((nz * 0.5 + 0.5) * 255), 255))
    return pixels


def foam_pixels(height_a, height_b):
    pixels = []
    for first, second in zip(height_a, height_b):
        broken = max(0.0, min(1.0, (0.68 * first + 0.32 * second - 0.34) * 1.75))
        value = round(broken * broken * (3.0 - 2.0 * broken) * 255)
        pixels.append((value, value, value, 255))
    return pixels


def author_textures():
    height_a = tileable_height(17031)
    height_b = tileable_height(48157, 27)
    paths = {
        "T_LT_WaterNormal_A": os.path.join(SOURCE_DIR, "T_LT_WaterNormal_A.tga"),
        "T_LT_WaterNormal_B": os.path.join(SOURCE_DIR, "T_LT_WaterNormal_B.tga"),
        "T_LT_WaterFoamBreakup": os.path.join(SOURCE_DIR, "T_LT_WaterFoamBreakup.tga"),
    }
    write_tga(paths["T_LT_WaterNormal_A"], normal_pixels(height_a, 18.0))
    write_tga(paths["T_LT_WaterNormal_B"], normal_pixels(height_b, 11.0))
    write_tga(paths["T_LT_WaterFoamBreakup"], foam_pixels(height_a, height_b))
    return paths


def import_texture(asset_name, filename, normal=False):
    asset_path = DESTINATION + "/" + asset_name
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not texture or not isinstance(texture, unreal.Texture2D):
        fail("Texture import failed: " + asset_path)
    texture.set_editor_property("srgb", False)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if normal
                                else unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def expression(material, expression_class, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
    if not node:
        fail("Could not create " + expression_class.__name__)
    return node


def connect(source, output, destination, input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, output, destination, input_name):
        fail("Could not connect {}.{} to {}.{}".format(source.get_name(), output, destination.get_name(), input_name))


def connect_property(source, output, prop):
    if not unreal.MaterialEditingLibrary.connect_material_property(source, output, prop):
        fail("Could not connect {} to {}".format(source.get_name(), prop))


def scalar(material, name, default, x, y):
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def vector(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*value, 1.0))
    return node


def binary(material, cls, a, b, x, y):
    node = expression(material, cls, x, y)
    connect(a, "", node, "A")
    connect(b, "", node, "B")
    return node


def world_uv(material, scale, speed, direction, x, y):
    world = expression(material, unreal.MaterialExpressionWorldPosition, x, y)
    mask = expression(material, unreal.MaterialExpressionComponentMask, x + 160, y)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mask.set_editor_property("b", False)
    mask.set_editor_property("a", False)
    connect(world, "XYZ", mask, "")
    scaled = binary(material, unreal.MaterialExpressionMultiply, mask, scale, x + 320, y)
    time = expression(material, unreal.MaterialExpressionTime, x, y + 100)
    timed = binary(material, unreal.MaterialExpressionMultiply, time, speed, x + 160, y + 100)
    direction_x = expression(material, unreal.MaterialExpressionConstant, x + 300, y + 100)
    direction_x.set_editor_property("r", direction[0])
    direction_y = expression(material, unreal.MaterialExpressionConstant, x + 300, y + 150)
    direction_y.set_editor_property("r", direction[1])
    direction_node = expression(material, unreal.MaterialExpressionAppendVector, x + 430, y + 120)
    connect(direction_x, "", direction_node, "A")
    connect(direction_y, "", direction_node, "B")
    offset = binary(material, unreal.MaterialExpressionMultiply, timed, direction_node, x + 580, y + 100)
    return binary(material, unreal.MaterialExpressionAdd, scaled, offset, x + 740, y)


def texture_sample(material, texture, coordinates, sampler_type, x, y):
    node = expression(material, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", texture)
    node.set_editor_property("sampler_type", sampler_type)
    # UE 5.8 exposes the texture-coordinate expression input to Python as
    # "UVs" (matching the project's other material-authoring scripts).
    connect(coordinates, "", node, "UVs")
    return node


def create_material(textures):
    unreal.EditorAssetLibrary.make_directory(DESTINATION)
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        if not isinstance(material, unreal.Material):
            fail(MATERIAL_PATH + " exists but is not a Material")
        expressions = list(unreal.MaterialEditingLibrary.get_material_expressions(material))
        parameters = {
            str(node.get_editor_property("parameter_name")): node
            for node in expressions
            if isinstance(node, unreal.MaterialExpressionScalarParameter)
        }
        expected = {
            "DeepOpacity": 0.98,
            "ShallowOpacity": 0.78,
            "Roughness": 0.28,
            "Specular": 0.45,
        }
        missing = [name for name in expected if name not in parameters]
        if missing:
            fail("Existing water graph is missing parameters: " + ", ".join(missing))
        for name, value in expected.items():
            parameters[name].set_editor_property("default_value", value)

        samples = [node for node in expressions if isinstance(node, unreal.MaterialExpressionTextureSample)]
        by_texture_name = {node.get_editor_property("texture").get_name(): node for node in samples
                           if node.get_editor_property("texture")}
        for texture_name, texture in textures.items():
            if texture_name not in by_texture_name:
                fail("Existing water graph is missing texture sample " + texture_name)
            by_texture_name[texture_name].set_editor_property("texture", texture)

        foam_roughness = parameters.get("FoamRoughness")
        if not foam_roughness:
            foam_roughness = scalar(material, "FoamRoughness", 0.65, 260, 260)
        else:
            foam_roughness.set_editor_property("default_value", 0.65)
        roughness_lerps = [node for node in expressions
                           if isinstance(node, unreal.MaterialExpressionLinearInterpolate)
                           and node.get_editor_property("material_expression_editor_x") == 470
                           and node.get_editor_property("material_expression_editor_y") == 260]
        roughness_lerp = roughness_lerps[0] if roughness_lerps else expression(
            material, unreal.MaterialExpressionLinearInterpolate, 470, 260)
        foam_masks = [node for node in expressions
                      if isinstance(node, unreal.MaterialExpressionMultiply)
                      and node.get_editor_property("material_expression_editor_x") == 60
                      and node.get_editor_property("material_expression_editor_y") == 450]
        if len(foam_masks) != 1:
            fail("Existing water graph foam mask structure is unexpected")
        connect(parameters["Roughness"], "", roughness_lerp, "A")
        connect(foam_roughness, "", roughness_lerp, "B")
        connect(foam_masks[0], "", roughness_lerp, "Alpha")
        connect_property(roughness_lerp, "", unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        return material
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LT_CoastalWater", DESTINATION, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        fail("Could not create " + MATERIAL_PATH)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", False)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    lighting_enum = getattr(unreal, "TranslucencyLightingMode", None)
    if not lighting_enum or not hasattr(lighting_enum, "TLM_SURFACE_PER_PIXEL_LIGHTING"):
        fail("UE build does not expose TLM_SURFACE_PER_PIXEL_LIGHTING")
    material.set_editor_property("translucency_lighting_mode", lighting_enum.TLM_SURFACE_PER_PIXEL_LIGHTING)

    shallow = vector(material, "ShallowColor", (0.045, 0.42, 0.48), -1100, -520)
    deep = vector(material, "DeepColor", (0.008, 0.095, 0.16), -1100, -450)
    foam_color = vector(material, "FoamColor", (0.68, 0.86, 0.82), -1100, -380)
    depth_distance = scalar(material, "DepthDistance", 520.0, -1100, -300)
    foam_distance = scalar(material, "FoamDistance", 115.0, -1100, -240)
    foam_strength = scalar(material, "FoamStrength", 0.72, -1100, -180)
    normal_strength_b = scalar(material, "SecondaryNormalStrength", 0.45, -1100, 120)
    roughness = scalar(material, "Roughness", 0.28, -1100, 260)
    foam_roughness = scalar(material, "FoamRoughness", 0.65, 260, 260)
    specular = scalar(material, "Specular", 0.45, -1100, 320)
    opacity_shallow = scalar(material, "ShallowOpacity", 0.78, -1100, 390)
    opacity_deep = scalar(material, "DeepOpacity", 0.98, -1100, 450)

    depth_fade = expression(material, unreal.MaterialExpressionDepthFade, -760, -430)
    connect(depth_distance, "", depth_fade, "FadeDistance")
    depth_color = expression(material, unreal.MaterialExpressionLinearInterpolate, -520, -430)
    connect(shallow, "", depth_color, "A")
    connect(deep, "", depth_color, "B")
    connect(depth_fade, "", depth_color, "Alpha")

    scale_a = scalar(material, "NormalScaleA", 0.0018, -1100, -40)
    speed_a = scalar(material, "NormalSpeedA", 0.035, -1100, 20)
    uv_a = world_uv(material, scale_a, speed_a, (0.78, 0.32), -920, -20)
    normal_a = texture_sample(material, textures["T_LT_WaterNormal_A"], uv_a,
                              unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -80, -20)
    scale_b = scalar(material, "NormalScaleB", 0.0036, -1100, 80)
    speed_b = scalar(material, "NormalSpeedB", 0.021, -1100, 140)
    uv_b = world_uv(material, scale_b, speed_b, (-0.24, 0.91), -920, 170)
    normal_b = texture_sample(material, textures["T_LT_WaterNormal_B"], uv_b,
                              unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -80, 170)
    weighted_b = binary(material, unreal.MaterialExpressionMultiply, normal_b, normal_strength_b, 100, 150)
    summed_normal = binary(material, unreal.MaterialExpressionAdd, normal_a, weighted_b, 280, 60)
    normalized_normal = expression(material, unreal.MaterialExpressionNormalize, 460, 60)
    connect(summed_normal, "", normalized_normal, "VectorInput")

    foam_scale = scalar(material, "FoamNoiseScale", 0.0027, -1100, 560)
    foam_speed = scalar(material, "FoamDriftSpeed", 0.014, -1100, 620)
    foam_uv = world_uv(material, foam_scale, foam_speed, (0.34, -0.63), -920, 520)
    foam_noise = texture_sample(material, textures["T_LT_WaterFoamBreakup"], foam_uv,
                                unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, -80, 520)
    shore_depth = expression(material, unreal.MaterialExpressionDepthFade, -520, 450)
    connect(foam_distance, "", shore_depth, "FadeDistance")
    shore = expression(material, unreal.MaterialExpressionOneMinus, -330, 450)
    # Unary expression inputs are exposed as the unnamed pin by the UE Python
    # material graph API even though the C++ member is named Input.
    connect(shore_depth, "", shore, "")
    foam_mask = expression(material, unreal.MaterialExpressionMultiply, -120, 450)
    connect(shore, "", foam_mask, "A")
    connect(foam_noise, "R", foam_mask, "B")
    foam_mask = binary(material, unreal.MaterialExpressionMultiply, foam_mask, foam_strength, 60, 450)
    final_roughness = expression(material, unreal.MaterialExpressionLinearInterpolate, 470, 260)
    connect(roughness, "", final_roughness, "A")
    connect(foam_roughness, "", final_roughness, "B")
    connect(foam_mask, "", final_roughness, "Alpha")
    final_color = expression(material, unreal.MaterialExpressionLinearInterpolate, 260, -350)
    connect(depth_color, "", final_color, "A")
    connect(foam_color, "", final_color, "B")
    connect(foam_mask, "", final_color, "Alpha")

    opacity = expression(material, unreal.MaterialExpressionLinearInterpolate, 260, 360)
    connect(opacity_shallow, "", opacity, "A")
    connect(opacity_deep, "", opacity, "B")
    connect(depth_fade, "", opacity, "Alpha")
    foam_opacity = binary(material, unreal.MaterialExpressionAdd, opacity, foam_mask, 470, 360)
    saturate = expression(material, unreal.MaterialExpressionSaturate, 650, 360)
    connect(foam_opacity, "", saturate, "")

    connect_property(final_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    connect_property(normalized_normal, "", unreal.MaterialProperty.MP_NORMAL)
    connect_property(final_roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    connect_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    connect_property(saturate, "", unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def main():
    paths = author_textures()
    unreal.EditorAssetLibrary.make_directory(DESTINATION)
    textures = {
        name: import_texture(name, path, name.startswith("T_LT_WaterNormal"))
        for name, path in paths.items()
    }
    create_material(textures)
    log("SUCCESS: authored 3x512 tileable inputs and saved " + MATERIAL_PATH)


if __name__ == "__main__":
    main()
