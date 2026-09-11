"""Create LOW TIDE's small, project-authored M1 material and coastal mesh family.

Run with the UE editor commandlet. This script deliberately uses no downloaded content.
It writes a simple OBJ into Intermediate for import, then saves only the .uasset products.
"""
import math
import os
import re
import unreal


ASSET_PATH = "/Game/Generated/M1"
PROJECT_DIR = unreal.Paths.project_dir()
FACET_OBJ_PATH = os.path.join(PROJECT_DIR, "Intermediate", "GeneratedM1", "SM_LT_FacetedRock.obj")
SHORE_WEDGE_OBJ_PATH = os.path.join(PROJECT_DIR, "Intermediate", "GeneratedM1", "SM_LT_ShoreWedge.obj")
TERRAIN_OBJ_PATHS = {
    "SM_LT_CoastalTerrainSand": os.path.join(PROJECT_DIR, "Intermediate", "GeneratedM1", "SM_LT_CoastalTerrainSand.obj"),
    "SM_LT_CoastalTerrainStone": os.path.join(PROJECT_DIR, "Intermediate", "GeneratedM1", "SM_LT_CoastalTerrainStone.obj"),
    "SM_LT_CoastalTerrainDeep": os.path.join(PROJECT_DIR, "Intermediate", "GeneratedM1", "SM_LT_CoastalTerrainDeep.obj"),
}
TERRAIN_EXPECTED_BOUNDS = {}
COASTAL_SCENE_CPP = os.path.join(PROJECT_DIR, "Source", "LowTide", "Private", "CoastalScene.cpp")

# These fixed points mirror FCoastalSceneLayout. They are authored level data, not runtime generation.
TERRAIN_ROUTES = (
    (0, 900.0, ((0.0, 0.0, 140.0), (2000.0, -800.0, 90.0), (4200.0, -4800.0, 30.0),
                   (7200.0, -6400.0, -20.0), (8800.0, -2000.0, -60.0), (11200.0, 1800.0, -30.0),
                   (13400.0, -1200.0, -55.0), (15600.0, 4800.0, 100.0), (18400.0, 2200.0, -80.0),
                   (20200.0, -3200.0, -100.0), (22000.0, -1400.0, 90.0))),
    (1, 820.0, ((22000.0, -1400.0, 90.0), (20500.0, 3000.0, 300.0), (17400.0, 6900.0, 520.0),
                   (13200.0, 6700.0, 420.0), (9400.0, 4100.0, 330.0), (6000.0, -300.0, 210.0),
                   (3200.0, 1600.0, 180.0), (0.0, 0.0, 140.0))),
    (2, 760.0, ((22000.0, -1400.0, 90.0), (21000.0, -3000.0, -65.0), (18500.0, -5500.0, -105.0),
                   (18500.0, -7300.0, -125.0), (22500.0, -7300.0, -130.0),
                   (22500.0, -5000.0, -115.0), (25000.0, -5800.0, -20.0))),
)


def log(message):
    unreal.log("[GenerateM1Assets] " + message)


_CPP_FLOAT = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?f?"
_CPP_VECTOR = re.compile(
    r"FVector\(\s*(" + _CPP_FLOAT + r")\s*,\s*(" + _CPP_FLOAT + r")\s*,\s*(" + _CPP_FLOAT + r")\s*\)")
_CPP_ROUTE_TOKEN = re.compile(
    _CPP_VECTOR.pattern + r"|Layout\.RouteWaypoints\.Last\(\)|Layout\.RouteWaypoints\[\s*(\d+)\s*\]")


def _parse_cpp_route(source, property_name, main_route=None):
    assignment = re.search(r"Layout\.{}\s*=\s*\{{(.*?)\}};".format(re.escape(property_name)), source, re.DOTALL)
    if not assignment:
        raise RuntimeError("Could not find Layout.{} initializer in {}".format(property_name, COASTAL_SCENE_CPP))
    body = re.sub(r"//[^\n]*", "", assignment.group(1))
    points = []
    for token_match in _CPP_ROUTE_TOKEN.finditer(body):
        token = token_match.group(0)
        vector_match = _CPP_VECTOR.fullmatch(token)
        if vector_match:
            points.append(tuple(float(value.rstrip("f")) for value in vector_match.groups()))
        elif ".Last()" in token:
            if not main_route:
                raise RuntimeError("{} uses Main.Last before main route parsing".format(property_name))
            points.append(main_route[-1])
        else:
            if not main_route:
                raise RuntimeError("{} uses a main-route index before main route parsing".format(property_name))
            index_match = re.search(r"\[\s*(\d+)\s*\]", token)
            points.append(main_route[int(index_match.group(1))])
    remainder = _CPP_ROUTE_TOKEN.sub("", body).replace(",", "").strip()
    if remainder:
        raise RuntimeError("Unparsed expression in Layout.{} initializer: {}".format(property_name, remainder))
    return tuple(points)


def _assert_routes_match(cpp_routes, terrain_routes=TERRAIN_ROUTES):
    names = ("RouteWaypoints", "AlternateRouteWaypoints", "OptionalRouteWaypoints")
    for route_index, name in enumerate(names):
        cpp_points = cpp_routes[name]
        terrain_points = terrain_routes[route_index][2]
        if len(cpp_points) != len(terrain_points):
            raise RuntimeError("Terrain route {} has {} points but C++ has {}".format(
                name, len(terrain_points), len(cpp_points)))
        for point_index, (cpp_point, terrain_point) in enumerate(zip(cpp_points, terrain_points)):
            if any(abs(cpp_point[axis] - terrain_point[axis]) > 0.01 for axis in range(3)):
                raise RuntimeError("Terrain route {} point {} {} does not match C++ {}".format(
                    name, point_index, terrain_point, cpp_point))


def validate_authored_routes():
    with open(COASTAL_SCENE_CPP, "r", encoding="utf-8") as source_file:
        source = source_file.read()
    main_route = _parse_cpp_route(source, "RouteWaypoints")
    cpp_routes = {
        "RouteWaypoints": main_route,
        "AlternateRouteWaypoints": _parse_cpp_route(source, "AlternateRouteWaypoints", main_route),
        "OptionalRouteWaypoints": _parse_cpp_route(source, "OptionalRouteWaypoints", main_route),
    }
    _assert_routes_match(cpp_routes)
    log("Verified terrain routes against CoastalScene.cpp: main {}, alternate {}, optional {} points".format(
        len(cpp_routes["RouteWaypoints"]), len(cpp_routes["AlternateRouteWaypoints"]),
        len(cpp_routes["OptionalRouteWaypoints"])))
    return cpp_routes


def remove_if_present(asset_name):
    asset = ASSET_PATH + "/" + asset_name
    if unreal.EditorAssetLibrary.does_asset_exist(asset):
        unreal.EditorAssetLibrary.delete_asset(asset)


def create_or_reset_material(name):
    remove_if_present(name)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, ASSET_PATH, unreal.Material, unreal.MaterialFactoryNew())


def expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)


def scalar(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def scalar_constant(material, value, x, y):
    node = expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", float(value))
    return node


def vector(material, name, value, x, y):
    node = expression(material, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*value, 1.0))
    return node


def connect(node, property_name):
    if not unreal.MaterialEditingLibrary.connect_material_property(node, "", property_name):
        raise RuntimeError("Failed to connect material property '{}'".format(property_name))


def connect_expr(output_node, output_name, input_node, input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(output_node, output_name, input_node, input_name):
        raise RuntimeError("Failed to connect material node chain")


def add(material, x, y):
    return expression(material, unreal.MaterialExpressionAdd, x, y)


def multiply(material, x, y):
    return expression(material, unreal.MaterialExpressionMultiply, x, y)


def sine(material, x, y):
    return expression(material, unreal.MaterialExpressionSine, x, y)


def lerp(material, x, y):
    return expression(material, unreal.MaterialExpressionLinearInterpolate, x, y)


def clamp_0_1(material, x, y):
    clamp_node = expression(material, unreal.MaterialExpressionSaturate, x, y)
    return clamp_node


def create_opaque_material():
    name = "M_LT_StylizedOpaque"
    material = create_or_reset_material(name)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    base = vector(material, "BaseColor", (0.42, 0.38, 0.31), -900, -100)
    roughness = scalar(material, "Roughness", 0.72, -900, 180)
    specular = scalar(material, "Specular", 0.18, -900, 260)

    world_color_scale = scalar(material, "WorldColorScale", 0.0025, -900, 320)
    world_color_variation = scalar(material, "WorldColorVariation", 0.045, -900, 380)

    # Broad world-position sine variation: sin(WorldX*Scale) + sin(WorldY*Scale).
    world_pos = expression(material, unreal.MaterialExpressionWorldPosition, -760, 250)
    world_x = expression(material, unreal.MaterialExpressionComponentMask, -700, 250)
    world_x.set_editor_property("r", True)
    world_x.set_editor_property("g", False)
    world_x.set_editor_property("b", False)
    world_x.set_editor_property("a", False)
    world_y = expression(material, unreal.MaterialExpressionComponentMask, -660, 250)
    world_y.set_editor_property("r", False)
    world_y.set_editor_property("g", True)
    world_y.set_editor_property("b", False)
    world_y.set_editor_property("a", False)

    connect_expr(world_pos, "XYZ", world_x, "")
    connect_expr(world_pos, "XYZ", world_y, "")

    x_scaled = multiply(material, -620, 210)
    y_scaled = multiply(material, -540, 210)
    connect_expr(world_x, "", x_scaled, "A")
    connect_expr(world_color_scale, "", x_scaled, "B")
    connect_expr(world_y, "", y_scaled, "A")
    connect_expr(world_color_scale, "", y_scaled, "B")

    x_wave = sine(material, -460, 210)
    y_wave = sine(material, -380, 210)
    connect_expr(x_scaled, "", x_wave, "")
    connect_expr(y_scaled, "", y_wave, "")

    wave_sum = add(material, -260, 210)
    connect_expr(x_wave, "", wave_sum, "A")
    connect_expr(y_wave, "", wave_sum, "B")

    # Normalize and bias amplitude, then apply as a vector scale.
    wave_bias = multiply(material, -180, 220)
    connect_expr(wave_sum, "", wave_bias, "A")
    connect_expr(scalar_constant(material, 0.5, -180, 250), "", wave_bias, "B")

    variation = multiply(material, -100, 220)
    connect_expr(wave_bias, "", variation, "A")
    connect_expr(world_color_variation, "", variation, "B")

    base_scale = add(material, -20, 220)
    connect_expr(scalar_constant(material, 1.0, -20, 250), "", base_scale, "A")
    connect_expr(variation, "", base_scale, "B")

    shaded_base = multiply(material, 60, 220)
    connect_expr(base, "", shaded_base, "A")
    connect_expr(base_scale, "", shaded_base, "B")
    connect(shaded_base, unreal.MaterialProperty.MP_BASE_COLOR)
    connect(roughness, unreal.MaterialProperty.MP_ROUGHNESS)
    connect(specular, unreal.MaterialProperty.MP_SPECULAR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def create_water_material():
    name = "M_LT_StylizedWater"
    material = create_or_reset_material(name)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    base = vector(material, "BaseColor", (0.025, 0.25, 0.34), -900, -50)
    shallow = vector(material, "ShallowColor", (0.10, 0.48, 0.54), -900, 40)
    roughness = scalar(material, "Roughness", 0.24, -900, 240)
    wave_speed = scalar(material, "WaveSpeed", 0.35, -900, 340)
    wave_scale = scalar(material, "WaveScale", 0.001, -900, 420)
    wave_amount = scalar(material, "WaveColorAmount", 0.14, -900, 500)

    world_pos = expression(material, unreal.MaterialExpressionWorldPosition, -700, -40)
    world_x = expression(material, unreal.MaterialExpressionComponentMask, -620, -40)
    world_x.set_editor_property("r", True)
    world_x.set_editor_property("g", False)
    world_x.set_editor_property("b", False)
    world_x.set_editor_property("a", False)
    world_y = expression(material, unreal.MaterialExpressionComponentMask, -560, -40)
    world_y.set_editor_property("r", False)
    world_y.set_editor_property("g", True)
    world_y.set_editor_property("b", False)
    world_y.set_editor_property("a", False)

    connect_expr(world_pos, "XYZ", world_x, "")
    connect_expr(world_pos, "XYZ", world_y, "")

    scaled_x = multiply(material, -500, -20)
    scaled_y = multiply(material, -440, -20)
    connect_expr(world_x, "", scaled_x, "A")
    connect_expr(wave_scale, "", scaled_x, "B")
    connect_expr(world_y, "", scaled_y, "A")
    connect_expr(wave_scale, "", scaled_y, "B")

    dot_term = add(material, -380, -20)
    connect_expr(scaled_x, "", dot_term, "A")
    connect_expr(scaled_y, "", dot_term, "B")

    time_node = expression(material, unreal.MaterialExpressionTime, -700, 60)
    time_term = multiply(material, -620, 60)
    connect_expr(time_node, "", time_term, "A")
    connect_expr(wave_speed, "", time_term, "B")

    wave_a_input = add(material, -300, 60)
    connect_expr(dot_term, "", wave_a_input, "A")
    connect_expr(time_term, "", wave_a_input, "B")
    wave_b_input = add(material, -220, 60)
    connect_expr(scaled_y, "", wave_b_input, "A")
    connect_expr(time_term, "", wave_b_input, "B")

    wave_a = sine(material, -160, 60)
    wave_b = sine(material, -100, 60)
    connect_expr(wave_a_input, "", wave_a, "")
    connect_expr(wave_b_input, "", wave_b, "")

    band_mix = add(material, -20, 60)
    connect_expr(wave_a, "", band_mix, "A")
    connect_expr(wave_b, "", band_mix, "B")
    band_mix_scale = multiply(material, 40, 60)
    connect_expr(band_mix, "", band_mix_scale, "A")
    connect_expr(scalar_constant(material, 0.5, 40, 90), "", band_mix_scale, "B")
    band_mix_bias = add(material, 90, 90)
    connect_expr(band_mix_scale, "", band_mix_bias, "A")
    connect_expr(scalar_constant(material, 0.5, 90, 120), "", band_mix_bias, "B")
    band_weight = clamp_0_1(material, 130, 100)
    connect_expr(band_mix_bias, "", band_weight, "")

    band_alpha = multiply(material, 170, 120)
    connect_expr(band_weight, "", band_alpha, "A")
    connect_expr(wave_amount, "", band_alpha, "B")

    final_color = lerp(material, 230, 140)
    connect_expr(base, "", final_color, "A")
    connect_expr(shallow, "", final_color, "B")
    connect_expr(band_alpha, "", final_color, "Alpha")
    connect(final_color, unreal.MaterialProperty.MP_BASE_COLOR)
    connect(roughness, unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def write_rock_obj():
    os.makedirs(os.path.dirname(FACET_OBJ_PATH), exist_ok=True)
    # Three deliberately offset 12-sided rings form a broad, blocky basalt stone.
    # Its flattened cap avoids the repeated pointed-cone silhouette of the first draft.
    count = 12
    irregular = [1.04, 0.91, 1.08, 0.96, 1.03, 0.88, 1.07, 0.94, 1.01, 0.90, 1.06, 0.97]
    lower = []
    shoulder = []
    top = []
    for index in range(count):
        angle = index * math.tau / count
        radius = irregular[index]
        lower.append((math.cos(angle) * radius, math.sin(angle) * radius * 0.92, -0.82 + (index % 3) * 0.025))
        shoulder.append((0.78 * math.cos(angle + 0.08) * radius + 0.05,
                         0.73 * math.sin(angle + 0.08) * radius - 0.04,
                         0.04 + (index % 4) * 0.035))
        top.append((0.52 * math.cos(angle - 0.05) * radius + 0.10,
                    0.46 * math.sin(angle - 0.05) * radius - 0.06,
                    0.61 + (index % 3) * 0.025))
    vertices = lower + shoulder + top + [(0.08, -0.05, 0.65), (-0.02, 0.01, -0.86)]
    faces = []
    for index in range(count):
        next_index = (index + 1) % count
        # Explicit triangles: 72 total, which holds the low-poly facet read at distance.
        faces.extend(((index + 1, next_index + 1, next_index + 13), (index + 1, next_index + 13, index + 13),
                      (index + 13, next_index + 13, next_index + 25), (index + 13, next_index + 25, index + 25),
                      (index + 25, next_index + 25, 37), (next_index + 1, index + 1, 38)))
    with open(FACET_OBJ_PATH, "w", encoding="utf-8", newline="\n") as obj:
        obj.write("# LOW TIDE project-authored faceted rock\n")
        for vertex in vertices:
            # OBJ values are imported by UE as centimetres; 50 yields approximately 100 cm width.
            obj.write("v {:.6f} {:.6f} {:.6f}\n".format(*(value * 50.0 for value in vertex)))
        # UE 5.8's OBJ interchange translator requires valid UV indices even for a texture-free mesh.
        for index in range(len(vertices)):
            obj.write("vt {:.6f} {:.6f}\n".format((index % 4) / 3.0, (index // 4) / 4.0))
        for face in faces:
            obj.write("f {}\n".format(" ".join("{0}/{0}".format(value) for value in face)))


def write_shore_wedge_obj():
    os.makedirs(os.path.dirname(SHORE_WEDGE_OBJ_PATH), exist_ok=True)
    # Single low-poly wedge designed for sloped/irregular shoreline shoulders.
    vertices = [
        # Top profile (left to right around a shallow irregular polygon).
        (-50.0, -20.0, 24.0),  # 1: inner lip near path edge
        (50.0, -20.0, 20.0),   # 2
        (50.0, 62.0, -12.0),   # 3
        (-50.0, 64.0, -4.0),   # 4
        (-44.0, 94.0, -28.0),  # 5
        (46.0, 94.0, -40.0),   # 6
        # Bottom profile duplicates with depth.
        (-50.0, -20.0, -80.0),
        (50.0, -20.0, -80.0),
        (50.0, 62.0, -80.0),
        (-50.0, 64.0, -82.0),
        (-44.0, 94.0, -90.0),
        (46.0, 94.0, -90.0),
    ]
    faces = []
    # One explicit, non-self-intersecting perimeter. Top winds upward; bottom winds downward.
    top_perimeter = (1, 2, 3, 6, 5, 4)
    bottom_for_top = {1: 7, 2: 8, 3: 9, 4: 10, 5: 11, 6: 12}
    faces.extend(((1, 2, 3), (1, 3, 6), (1, 6, 5), (1, 5, 4)))
    faces.extend(((7, 9, 8), (7, 12, 9), (7, 11, 12), (7, 10, 11)))
    # For a counter-clockwise top perimeter, top0 -> bottom0 -> bottom1 points outward.
    for index, top0 in enumerate(top_perimeter):
        top1 = top_perimeter[(index + 1) % len(top_perimeter)]
        bottom0 = bottom_for_top[top0]
        bottom1 = bottom_for_top[top1]
        faces.append((top0, bottom0, bottom1))
        faces.append((top0, bottom1, top1))

    with open(SHORE_WEDGE_OBJ_PATH, "w", encoding="utf-8", newline="\n") as obj:
        obj.write("# LOW TIDE project-authored shore wedge\n")
        for vertex in vertices:
            obj.write("v {:.6f} {:.6f} {:.6f}\n".format(*vertex))
        for index in range(len(vertices)):
            obj.write("vt {:.6f} {:.6f}\n".format((index % 4) / 3.0, (index // 4) / 4.0))
        for face in faces:
            obj.write("f {}\n".format(" ".join("{0}/{0}".format(value) for value in face)))


def _smoothstep(value):
    value = max(0.0, min(1.0, value))
    return value * value * (3.0 - 2.0 * value)


def _terrain_candidates(x, y):
    candidates = []
    for family, width, points in TERRAIN_ROUTES:
        inner_radius = width * 0.5 + 60.0
        for segment_index in range(len(points) - 1):
            start = points[segment_index]
            end = points[segment_index + 1]
            delta_x = end[0] - start[0]
            delta_y = end[1] - start[1]
            length_squared = max(1.0, delta_x * delta_x + delta_y * delta_y)
            alpha = max(0.0, min(1.0, ((x - start[0]) * delta_x + (y - start[1]) * delta_y) / length_squared))
            closest_x = start[0] + delta_x * alpha
            closest_y = start[1] + delta_y * alpha
            distance = math.hypot(x - closest_x, y - closest_y)
            organic_phase = alpha * math.tau + segment_index * 1.71 + family * 0.83
            shoulder_reach = 1250.0 + 600.0 * (0.5 + 0.5 * math.sin(organic_phase))
            outer_radius = inner_radius + shoulder_reach
            if distance > outer_radius:
                continue
            route_z = start[2] + (end[2] - start[2]) * alpha
            falloff = _smoothstep(max(0.0, distance - inner_radius) / shoulder_reach)
            low_detail = abs(math.sin(x * 0.0019 + family) * math.sin(y * 0.0013 - segment_index)) * 14.0
            segment_length = math.sqrt(length_squared + (end[2] - start[2]) ** 2)
            # Mirrors AddBoxBetween's -38 cm centre offset and 35 cm rotated cube half-height exactly.
            inner_height = route_z - 38.0 + 35.0 * segment_length / math.sqrt(length_squared)
            submerged_edge = -640.0 - low_detail
            height = inner_height + (submerged_edge - inner_height) * falloff - low_detail * falloff * (1.0 - falloff)
            candidates.append((height, family, distance <= inner_radius))

    # Mara's coast is a broad irregular shelf whose plateau stays just beneath the collision floor.
    center_x, center_y = 0.0, -200.0
    local_yaw = math.radians(4.0)
    delta_x, delta_y = x - center_x, y - center_y
    local_x = math.cos(local_yaw) * delta_x - math.sin(local_yaw) * delta_y
    local_y = math.sin(local_yaw) * delta_x + math.cos(local_yaw) * delta_y
    outside_x = max(abs(local_x) - 3100.0, 0.0)
    outside_y = max(abs(local_y) - 3400.0, 0.0)
    outside_distance = math.hypot(outside_x, outside_y)
    outline_angle = math.atan2(local_y / 3400.0, local_x / 3100.0)
    coast_reach = 1550.0 + 380.0 * math.sin(outline_angle * 3.0 + 0.55) + 180.0 * math.sin(outline_angle * 7.0)
    if outside_distance <= coast_reach:
        falloff = _smoothstep(outside_distance / max(800.0, coast_reach))
        plateau = 110.0 - abs(math.sin(x * 0.0011) * math.sin(y * 0.0014)) * 8.0
        candidates.append((plateau + (-660.0 - plateau) * falloff, 1, outside_distance <= 80.0))

    # A stone islet grounds the signal-station plinth and joins it to the main-route shoulder.
    station_distance = math.hypot(x - 22900.0, y + 500.0)
    station_inner = 850.0
    station_reach = 2150.0 + 180.0 * math.sin(math.atan2(y + 500.0, x - 22900.0) * 5.0)
    if station_distance <= station_reach:
        falloff = _smoothstep(max(0.0, station_distance - station_inner) / max(1.0, station_reach - station_inner))
        candidates.append((50.0 + (-650.0 - 50.0) * falloff, 1, False))

    return candidates


def _terrain_sample(x, y):
    candidates = _terrain_candidates(x, y)
    if not candidates:
        return -680.0, None
    # Where walk ribbons overlap, the lower authored route owns the ground so a raised ridge cannot cover it.
    inner_routes = [candidate for candidate in candidates if candidate[2]]
    chosen = min(inner_routes, key=lambda candidate: candidate[0]) if inner_routes else max(candidates, key=lambda candidate: candidate[0])
    return chosen[0], chosen[1]


def write_terrain_objs():
    grid_step = 200.0
    x_values = [float(value) for value in range(-6000, 28001, int(grid_step))]
    y_values = [float(value) for value in range(-10000, 10001, int(grid_step))]
    column_count = len(x_values)
    heights = []
    for y in y_values:
        heights.append([_terrain_sample(x, y)[0] for x in x_values])

    faces_by_family = {0: [], 1: [], 2: []}
    for row in range(len(y_values) - 1):
        for column in range(column_count - 1):
            center_x = (x_values[column] + x_values[column + 1]) * 0.5
            center_y = (y_values[row] + y_values[row + 1]) * 0.5
            _, family = _terrain_sample(center_x, center_y)
            if family is None:
                continue
            lower_left = row * column_count + column
            lower_right = lower_left + 1
            upper_left = lower_left + column_count
            upper_right = upper_left + 1
            faces_by_family[family].append((lower_left, lower_right, upper_right))
            faces_by_family[family].append((lower_left, upper_right, upper_left))

    asset_names = tuple(TERRAIN_OBJ_PATHS.keys())
    for family, asset_name in enumerate(asset_names):
        faces = faces_by_family[family]
        used_indices = sorted({index for face in faces for index in face})
        remap = {old_index: new_index + 1 for new_index, old_index in enumerate(used_indices)}
        logical_vertices = []
        for old_index in used_indices:
            row, column = divmod(old_index, column_count)
            logical_vertices.append((x_values[column], y_values[row], heights[row][column]))
        TERRAIN_EXPECTED_BOUNDS[asset_name] = (
            tuple(min(vertex[axis] for vertex in logical_vertices) for axis in range(3)),
            tuple(max(vertex[axis] for vertex in logical_vertices) for axis in range(3)),
        )
        output_path = TERRAIN_OBJ_PATHS[asset_name]
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, "w", encoding="utf-8", newline="\n") as obj:
            obj.write("# LOW TIDE authored coastal terrain section {}\n".format(family))
            for old_index in used_indices:
                row, column = divmod(old_index, column_count)
                # UE's OBJ converter reflects source Y when converting right-handed OBJ into Unreal space.
                # Pre-reflect terrain Y so the imported mesh lands on the authored gameplay coordinates.
                obj.write("v {:.3f} {:.3f} {:.3f}\n".format(x_values[column], -y_values[row], heights[row][column]))
            for old_index in used_indices:
                row, column = divmod(old_index, column_count)
                obj.write("vt {:.6f} {:.6f}\n".format(column / max(1, column_count - 1), row / max(1, len(y_values) - 1)))
            for face in faces:
                # The pre-reflection changes handedness, so reverse source winding to keep top faces upward.
                obj.write("f {}\n".format(" ".join("{0}/{0}".format(remap[index]) for index in reversed(face))))
        log("Authored terrain {}: {} vertices, {} triangles".format(asset_name, len(used_indices), len(faces)))


def import_rock(material):
    name = "SM_LT_FacetedRock"
    write_rock_obj()
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", FACET_OBJ_PATH)
    task.set_editor_property("destination_path", ASSET_PATH)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    rock = unreal.EditorAssetLibrary.load_asset(ASSET_PATH + "/" + name)
    if not rock:
        raise RuntimeError("Faceted rock import did not create an asset")
    rock.set_material(0, material)
    try:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(rock, unreal.ScriptingCollisionShapeType.BOX)
    except Exception as error:
        log("Simple collision helper unavailable; imported mesh collision retained: {}".format(error))
    unreal.EditorAssetLibrary.save_loaded_asset(rock)
    return rock


def import_shore_wedge(material):
    name = "SM_LT_ShoreWedge"
    write_shore_wedge_obj()
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SHORE_WEDGE_OBJ_PATH)
    task.set_editor_property("destination_path", ASSET_PATH)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    shore = unreal.EditorAssetLibrary.load_asset(ASSET_PATH + "/" + name)
    if not shore:
        raise RuntimeError("Shore wedge import did not create an asset")
    shore.set_material(0, material)
    try:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(shore, unreal.ScriptingCollisionShapeType.BOX)
    except Exception as error:
        log("Simple collision helper unavailable for shore wedge; imported mesh collision retained: {}".format(error))
    unreal.EditorAssetLibrary.save_loaded_asset(shore)
    return shore


def import_terrain(material):
    write_terrain_objs()
    imported_assets = []
    for name, source_path in TERRAIN_OBJ_PATHS.items():
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source_path)
        task.set_editor_property("destination_path", ASSET_PATH)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        terrain = unreal.EditorAssetLibrary.load_asset(ASSET_PATH + "/" + name)
        if not terrain:
            raise RuntimeError("Coastal terrain import did not create {}".format(name))
        expected_min, expected_max = TERRAIN_EXPECTED_BOUNDS[name]
        imported_box = terrain.get_bounding_box()
        actual_min = (imported_box.min.x, imported_box.min.y, imported_box.min.z)
        actual_max = (imported_box.max.x, imported_box.max.y, imported_box.max.z)
        tolerance_cm = 0.1
        if any(abs(actual_min[axis] - expected_min[axis]) > tolerance_cm or
               abs(actual_max[axis] - expected_max[axis]) > tolerance_cm for axis in range(3)):
            raise RuntimeError("{} imported bounds {}..{} do not match authored bounds {}..{}".format(
                name, actual_min, actual_max, expected_min, expected_max))
        log("Verified imported bounds {}: {} .. {}".format(name, actual_min, actual_max))
        terrain.set_material(0, material)
        unreal.EditorAssetLibrary.save_loaded_asset(terrain)
        imported_assets.append(terrain)
    return tuple(imported_assets)


def main():
    validate_authored_routes()
    unreal.EditorAssetLibrary.make_directory(ASSET_PATH)
    opaque = create_opaque_material()
    water = create_water_material()
    rock = import_rock(opaque)
    terrain = import_terrain(opaque)
    for asset in (opaque, water, rock) + terrain:
        log("Created " + asset.get_path_name())
    log("SUCCESS")


if __name__ == "__main__":
    main()
