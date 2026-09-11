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
ROUTE_FLOOR_WIDTH_SCALE = 1.16
ROUTE_FLOOR_CENTER_OFFSET = -38.0
ROUTE_FLOOR_HEIGHT = 70.0
VISIBLE_SURFACE_OFFSET = -1.0
SETTLEMENT_LANDING = ((2000.0, -800.0, 128.0), (3430.0, -3400.0, 128.0),
                      900.0 * ROUTE_FLOOR_WIDTH_SCALE)
SETTLEMENT_RAMP = ((3430.0, -3400.0, 128.0), (4002.0, -4440.0, 35.4),
                   900.0 * ROUTE_FLOOR_WIDTH_SCALE)

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
    width_scale_match = re.search(
        r"RouteFloorWidthScale\s*=\s*({})".format(_CPP_FLOAT), source)
    if not width_scale_match:
        raise RuntimeError("Could not find CoastalScene::RouteFloorWidthScale")
    cpp_width_scale = float(width_scale_match.group(1).rstrip("f"))
    if abs(cpp_width_scale - ROUTE_FLOOR_WIDTH_SCALE) > 0.0001:
        raise RuntimeError("Terrain floor width scale {} does not match C++ {}".format(
            ROUTE_FLOOR_WIDTH_SCALE, cpp_width_scale))
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


def _mesh():
    return {"vertices": [], "faces": []}


def _add_polygon(mesh, vertices):
    """Append an upward-facing convex polygon expressed counter-clockwise in gameplay XY."""
    base = len(mesh["vertices"])
    mesh["vertices"].extend(vertices)
    triangles = []
    for index in range(1, len(vertices) - 1):
        face = (base, base + index, base + index + 1)
        mesh["faces"].append(face)
        triangles.append(tuple(mesh["vertices"][vertex] for vertex in face))
    return triangles


def _add_quad(mesh, a, b, c, d):
    return _add_polygon(mesh, (a, b, c, d))


def _floor_surface_z(start, end, alpha):
    horizontal = math.hypot(end[0] - start[0], end[1] - start[1])
    length = math.sqrt(horizontal * horizontal + (end[2] - start[2]) ** 2)
    route_z = start[2] + (end[2] - start[2]) * alpha
    return (route_z + ROUTE_FLOOR_CENTER_OFFSET
            + ROUTE_FLOOR_HEIGHT * 0.5 * length / max(1.0, horizontal))


def _triangle_z_at_xy(triangle, x, y):
    a, b, c = triangle
    denominator = ((b[1] - c[1]) * (a[0] - c[0])
                   + (c[0] - b[0]) * (a[1] - c[1]))
    if abs(denominator) < 0.000001:
        return None
    wa = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / denominator
    wb = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / denominator
    wc = 1.0 - wa - wb
    if min(wa, wb, wc) < -0.000001:
        return None
    return wa * a[2] + wb * b[2] + wc * c[2]


def _collision_heights_at_xy(x, y):
    heights = []
    for _, width, points in TERRAIN_ROUTES:
        half_width = width * ROUTE_FLOOR_WIDTH_SCALE * 0.5
        for start, end in zip(points, points[1:]):
            dx, dy = end[0] - start[0], end[1] - start[1]
            length_squared = dx * dx + dy * dy
            alpha = ((x - start[0]) * dx + (y - start[1]) * dy) / max(1.0, length_squared)
            if -0.0001 <= alpha <= 1.0001:
                side_distance = abs((x - start[0]) * -dy + (y - start[1]) * dx) / math.sqrt(length_squared)
                if side_distance <= half_width + 0.1:
                    heights.append(_floor_surface_z(start, end, max(0.0, min(1.0, alpha))))
        for point in points[:-1]:
            if abs(x - point[0]) <= half_width + 0.1 and abs(y - point[1]) <= half_width + 0.1:
                heights.append(point[2] + ROUTE_FLOOR_CENTER_OFFSET + ROUTE_FLOOR_HEIGHT * 0.5)

    # BuildSettlement's rotated 6200 x 6800 x 180 collision-bearing cube.
    radians = math.radians(4.0)
    dx, dy = x, y + 200.0
    local_x = math.cos(radians) * dx - math.sin(radians) * dy
    local_y = math.sin(radians) * dx + math.cos(radians) * dy
    if abs(local_x) <= 3100.1 and abs(local_y) <= 3400.1:
        heights.append(125.0)
    for transition_start, transition_end, transition_width in (SETTLEMENT_LANDING, SETTLEMENT_RAMP):
        transition_dx = transition_end[0] - transition_start[0]
        transition_dy = transition_end[1] - transition_start[1]
        transition_length_squared = transition_dx * transition_dx + transition_dy * transition_dy
        transition_alpha = ((x - transition_start[0]) * transition_dx
                            + (y - transition_start[1]) * transition_dy) / transition_length_squared
        transition_side = abs((x - transition_start[0]) * -transition_dy
                              + (y - transition_start[1]) * transition_dx) / math.sqrt(transition_length_squared)
        if (-0.0001 <= transition_alpha <= 1.0001
                and transition_side <= transition_width * 0.5 + 0.1):
            heights.append(_floor_surface_z(
                transition_start, transition_end, max(0.0, min(1.0, transition_alpha))))
    return heights


def _validate_route_surfaces(meshes, route_triangles, cap_triangles, transition_triangles):
    """Ray-test exported logical triangles against every hidden floor proxy in five lanes."""
    max_difference = 0.0
    sample_count = 0
    all_triangles = []
    for mesh in meshes.values():
        all_triangles.extend(tuple(mesh["vertices"][index] for index in face) for face in mesh["faces"])

    max_union_difference = [0.0]

    def validate_exported_union(x, y, label):
        collision_heights = _collision_heights_at_xy(x, y)
        if not collision_heights:
            raise RuntimeError("No floor proxy covers intended sample {}".format(label))
        expected = max(collision_heights) + VISIBLE_SURFACE_OFFSET
        hits = [_triangle_z_at_xy(triangle, x, y) for triangle in all_triangles]
        hits = [value for value in hits if value is not None and abs(value - expected) <= 250.0]
        if not hits:
            raise RuntimeError("Exported terrain misses intended sample {} at {:.1f},{:.1f}".format(label, x, y))
        actual = max(hits)
        difference = abs(actual - expected)
        max_union_difference[0] = max(max_union_difference[0], difference)
        if difference > 2.0:
            raise RuntimeError("Exported terrain/floor mismatch {}: visual {:.3f}, collision {:.3f}".format(
                label, actual, expected - VISIBLE_SURFACE_OFFSET))

    for route_index, (_, width, points) in enumerate(TERRAIN_ROUTES):
        walk_half_width = width * ROUTE_FLOOR_WIDTH_SCALE * 0.5
        for segment_index, (start, end) in enumerate(zip(points, points[1:])):
            dx, dy = end[0] - start[0], end[1] - start[1]
            horizontal = math.hypot(dx, dy)
            side_x, side_y = -dy / horizontal, dx / horizontal
            triangles = route_triangles[(route_index, segment_index)]
            for alpha_step in range(21):
                alpha = (alpha_step + 0.5) / 21.0
                expected = _floor_surface_z(start, end, alpha)
                for lane in (-0.48, -0.24, 0.0, 0.24, 0.48):
                    x = start[0] + dx * alpha + side_x * walk_half_width * lane / 0.5
                    y = start[1] + dy * alpha + side_y * walk_half_width * lane / 0.5
                    hits = [_triangle_z_at_xy(triangle, x, y) for triangle in triangles]
                    hits = [value for value in hits if value is not None]
                    if not hits:
                        raise RuntimeError("Visible route gap at route {} segment {} alpha {:.3f} lane {:.2f}".format(
                            route_index, segment_index, alpha, lane))
                    difference = min(abs(value - (expected + VISIBLE_SURFACE_OFFSET)) for value in hits)
                    max_difference = max(max_difference, difference)
                    validate_exported_union(x, y, "route {} segment {} alpha {:.3f} lane {:.2f}".format(
                        route_index, segment_index, alpha, lane))
                    sample_count += 1

        # BuildPathRibbon adds a horizontal box at every segment start. Validate its full usable square.
        for point_index in range(len(points) - 1):
            expected = points[point_index][2] + ROUTE_FLOOR_CENTER_OFFSET + ROUTE_FLOOR_HEIGHT * 0.5
            triangles = cap_triangles[(route_index, point_index)]
            for x_lane in (-0.45, 0.0, 0.45):
                for y_lane in (-0.45, 0.0, 0.45):
                    x = points[point_index][0] + x_lane * width * ROUTE_FLOOR_WIDTH_SCALE
                    y = points[point_index][1] + y_lane * width * ROUTE_FLOOR_WIDTH_SCALE
                    hits = [_triangle_z_at_xy(triangle, x, y) for triangle in triangles]
                    hits = [value for value in hits if value is not None]
                    if not hits:
                        raise RuntimeError("Visible junction gap at route {} point {} lane {},{}".format(
                            route_index, point_index, x_lane, y_lane))
                    difference = min(abs(value - (expected + VISIBLE_SURFACE_OFFSET)) for value in hits)
                    max_difference = max(max_difference, difference)
                    validate_exported_union(x, y, "route {} point {} cap {},{}".format(
                        route_index, point_index, x_lane, y_lane))
                    sample_count += 1

    for transition_name, transition, triangles in zip(
            ("landing", "ramp"), (SETTLEMENT_LANDING, SETTLEMENT_RAMP), transition_triangles):
        start, end, width = transition
        dx, dy = end[0] - start[0], end[1] - start[1]
        horizontal = math.hypot(dx, dy)
        side_x, side_y = -dy / horizontal, dx / horizontal
        for alpha_step in range(21):
            alpha = (alpha_step + 0.5) / 21.0
            expected = _floor_surface_z(start, end, alpha) + VISIBLE_SURFACE_OFFSET
            for lane in (-0.48, -0.24, 0.0, 0.24, 0.48):
                x = start[0] + dx * alpha + side_x * width * lane
                y = start[1] + dy * alpha + side_y * width * lane
                hits = [_triangle_z_at_xy(triangle, x, y) for triangle in triangles]
                hits = [value for value in hits if value is not None]
                if not hits or min(abs(value - expected) for value in hits) > 0.02:
                    raise RuntimeError("Settlement {} gap at alpha {:.3f} lane {:.2f}".format(
                        transition_name, alpha, lane))
                validate_exported_union(x, y, "settlement {} alpha {:.3f} lane {:.2f}".format(
                    transition_name, alpha, lane))
                sample_count += 1

    # Measure the top envelope immediately before/after all three old-floor transition joins.
    transition_joins = (SETTLEMENT_LANDING[0], SETTLEMENT_LANDING[1], SETTLEMENT_RAMP[1])
    main_dx = TERRAIN_ROUTES[0][2][2][0] - TERRAIN_ROUTES[0][2][1][0]
    main_dy = TERRAIN_ROUTES[0][2][2][1] - TERRAIN_ROUTES[0][2][1][1]
    main_horizontal = math.hypot(main_dx, main_dy)
    forward_x, forward_y = main_dx / main_horizontal, main_dy / main_horizontal
    side_x, side_y = -forward_y, forward_x
    max_join_step = 0.0
    for join_index, join in enumerate(transition_joins):
        for lane in (-0.48, -0.24, 0.0, 0.24, 0.48):
            heights = []
            for direction in (-1.0, 1.0):
                x = join[0] + side_x * SETTLEMENT_RAMP[2] * lane + forward_x * 5.0 * direction
                y = join[1] + side_y * SETTLEMENT_RAMP[2] * lane + forward_y * 5.0 * direction
                collision_heights = _collision_heights_at_xy(x, y)
                if not collision_heights:
                    raise RuntimeError("No collision at settlement join {} lane {:.2f}".format(join_index, lane))
                heights.append(max(collision_heights))
                validate_exported_union(x, y, "settlement join {} lane {:.2f}".format(join_index, lane))
            max_join_step = max(max_join_step, abs(heights[1] - heights[0]))
    if max_join_step > 1.0:
        raise RuntimeError("Settlement top-envelope step is {:.3f} cm".format(max_join_step))

    if max_difference > 0.02:
        raise RuntimeError("Visible route surface differs from floor proxies by {:.4f} cm".format(max_difference))
    log("Verified {} triangle-ray samples across five route lanes and every junction cap; "
        "max direct delta {:.4f} cm, exported-union delta {:.4f} cm, transition join step {:.4f} cm".format(
            sample_count, max_difference, max_union_difference[0], max_join_step))


def _rotate_xy(point, yaw_degrees, center):
    radians = math.radians(yaw_degrees)
    return (center[0] + math.cos(radians) * point[0] - math.sin(radians) * point[1],
            center[1] + math.sin(radians) * point[0] + math.cos(radians) * point[1], point[2])


def _add_settlement_shelf(mesh):
    center = (0.0, -200.0)
    # Match the rotated 6200 x 6800 collision floor exactly. Uneven shoulder reach softens
    # the outer shoreline without extending an unsupported flat plateau past the proxy.
    top_local = ((-3100.0, -3400.0, 124.0), (3100.0, -3400.0, 124.0),
                 (3100.0, 3400.0, 124.0), (-3100.0, 3400.0, 124.0))
    reach = (1300.0, 1550.0, 1420.0, 1680.0)
    top = [_rotate_xy(vertex, -4.0, center) for vertex in top_local]
    outer = []
    for vertex, extra in zip(top_local, reach):
        length = math.hypot(vertex[0], vertex[1])
        outer_local = (vertex[0] * (length + extra) / length,
                       vertex[1] * (length + extra) / length, -660.0)
        outer.append(_rotate_xy(outer_local, -4.0, center))
    _add_polygon(mesh, top)
    lower = [(vertex[0], vertex[1], vertex[2] - 120.0) for vertex in top]
    for index in range(len(top)):
        next_index = (index + 1) % len(top)
        _add_quad(mesh, top[index], lower[index], lower[next_index], top[next_index])
        _add_quad(mesh, lower[index], outer[index], outer[next_index], lower[next_index])


def _add_station_islet(mesh):
    center = (22900.0, -500.0)
    top = []
    outer = []
    for index in range(12):
        angle = index * math.tau / 12.0
        top.append((center[0] + math.cos(angle) * 900.0,
                    center[1] + math.sin(angle) * 900.0, 49.0))
        outer_radius = 1900.0 + 170.0 * math.sin(angle * 5.0 + 0.4)
        outer.append((center[0] + math.cos(angle) * outer_radius,
                      center[1] + math.sin(angle) * outer_radius, -650.0))
    _add_polygon(mesh, top)
    for index in range(len(top)):
        next_index = (index + 1) % len(top)
        _add_quad(mesh, top[index], outer[index], outer[next_index], top[next_index])


def _lip_width(route_index, segment_index, sample_index, side_index):
    # The elevated return terminates over the broad settlement shelf; a lip there would overlap
    # the lower settlement walking plane, so the shelf itself supplies that final shoreline blend.
    if route_index == 1 and segment_index == len(TERRAIN_ROUTES[1][2]) - 2:
        return 0.0
    # Let junction caps and intersecting route planes own segment ends; taper the lip in only
    # after the turn so an adjacent segment's decorative edge cannot cover the walk surface.
    if sample_index <= 3 or sample_index >= 7:
        return 0.0
    phase = route_index * 1.13 + segment_index * 1.71 + sample_index * 1.97 + side_index * 0.83
    width = 104.0 + 31.0 * math.sin(phase) + 14.0 * math.sin(phase * 2.37 + 0.4)
    return max(60.0, min(150.0, width))


def _lip_z(surface_z):
    # The lowest optional shelf sits almost exactly at the -134 cm low-water surface.
    # Keep its outer lip just visible; elsewhere the lip rests roughly 1 cm below the floor-matched core.
    return max(surface_z - 0.9, -133.75)


def _safe_lip_width(inner, outward_x, outward_y, proposed_width, surface_z):
    """Stop a decorative lip before it crosses a separate walking layer at another height."""
    previous_distance = 0.0
    for step in range(1, 11):
        distance = proposed_width * step / 10.0
        support = _collision_heights_at_xy(
            inner[0] + outward_x * distance, inner[1] + outward_y * distance)
        if support and abs(max(support) + VISIBLE_SURFACE_OFFSET - surface_z) > 2.0:
            return previous_distance
        previous_distance = distance
    return proposed_width


def write_terrain_objs():
    meshes = {family: _mesh() for family in range(3)}
    route_triangles = {}
    cap_triangles = {}

    for route_index, (family, width, points) in enumerate(TERRAIN_ROUTES):
        mesh = meshes[family]
        half_width = width * ROUTE_FLOOR_WIDTH_SCALE * 0.5
        for segment_index, (start, end) in enumerate(zip(points, points[1:])):
            dx, dy = end[0] - start[0], end[1] - start[1]
            horizontal = math.hypot(dx, dy)
            side_x, side_y = -dy / horizontal, dx / horizontal
            start_z = _floor_surface_z(start, end, 0.0) + VISIBLE_SURFACE_OFFSET
            end_z = _floor_surface_z(start, end, 1.0) + VISIBLE_SURFACE_OFFSET
            start_right = (start[0] - side_x * half_width, start[1] - side_y * half_width, start_z)
            end_right = (end[0] - side_x * half_width, end[1] - side_y * half_width, end_z)
            end_left = (end[0] + side_x * half_width, end[1] + side_y * half_width, end_z)
            start_left = (start[0] + side_x * half_width, start[1] + side_y * half_width, start_z)
            route_triangles[(route_index, segment_index)] = _add_quad(
                mesh, start_right, end_right, end_left, start_left)

            subdivisions = 10
            right_inner = []
            left_inner = []
            right_lip = []
            left_lip = []
            right_lower = []
            left_lower = []
            right_outer = []
            left_outer = []
            for sample_index in range(subdivisions + 1):
                alpha = sample_index / float(subdivisions)
                center_x = start[0] + dx * alpha
                center_y = start[1] + dy * alpha
                surface_z = _floor_surface_z(start, end, alpha) + VISIBLE_SURFACE_OFFSET
                inner_right = (center_x - side_x * half_width, center_y - side_y * half_width, surface_z)
                inner_left = (center_x + side_x * half_width, center_y + side_y * half_width, surface_z)
                right_width = _safe_lip_width(
                    inner_right, -side_x, -side_y,
                    _lip_width(route_index, segment_index, sample_index, 0), surface_z)
                left_width = _safe_lip_width(
                    inner_left, side_x, side_y,
                    _lip_width(route_index, segment_index, sample_index, 1), surface_z)
                right_height = _lip_z(surface_z)
                left_height = _lip_z(surface_z)
                lip_right = (inner_right[0] - side_x * right_width,
                             inner_right[1] - side_y * right_width, right_height)
                lip_left = (inner_left[0] + side_x * left_width,
                            inner_left[1] + side_y * left_width, left_height)
                reach = 920.0 + 150.0 * math.sin(
                    segment_index * 1.71 + sample_index * 1.19 + family * 0.83)
                right_inner.append(inner_right)
                left_inner.append(inner_left)
                right_lip.append(lip_right)
                left_lip.append(lip_left)
                right_lower.append((lip_right[0], lip_right[1], lip_right[2] - 120.0))
                left_lower.append((lip_left[0], lip_left[1], lip_left[2] - 120.0))
                right_outer.append((lip_right[0] - side_x * reach, lip_right[1] - side_y * reach, -650.0))
                left_outer.append((lip_left[0] + side_x * reach, lip_left[1] + side_y * reach, -650.0))
            for sample_index in range(subdivisions):
                next_index = sample_index + 1
                _add_quad(mesh, right_inner[sample_index], right_lip[sample_index],
                          right_lip[next_index], right_inner[next_index])
                _add_quad(mesh, right_lip[sample_index], right_lower[sample_index],
                          right_lower[next_index], right_lip[next_index])
                _add_quad(mesh, right_lower[sample_index], right_outer[sample_index],
                          right_outer[next_index], right_lower[next_index])
                _add_quad(mesh, left_inner[sample_index], left_inner[next_index],
                          left_lip[next_index], left_lip[sample_index])
                _add_quad(mesh, left_lip[sample_index], left_lip[next_index],
                          left_lower[next_index], left_lower[sample_index])
                _add_quad(mesh, left_lower[sample_index], left_lower[next_index],
                          left_outer[next_index], left_outer[sample_index])

        # Mirror each horizontal hidden waypoint box so junctions have no invisible square corners.
        for point_index, point in enumerate(points[:-1]):
            cap_half = half_width
            cap_z = point[2] + ROUTE_FLOOR_CENTER_OFFSET + ROUTE_FLOOR_HEIGHT * 0.5 + VISIBLE_SURFACE_OFFSET
            cap_triangles[(route_index, point_index)] = _add_quad(
                mesh,
                (point[0] - cap_half, point[1] - cap_half, cap_z),
                (point[0] + cap_half, point[1] - cap_half, cap_z),
                (point[0] + cap_half, point[1] + cap_half, cap_z),
                (point[0] - cap_half, point[1] + cap_half, cap_z))

    transition_triangles = []
    for transition_start, transition_end, transition_width in (SETTLEMENT_LANDING, SETTLEMENT_RAMP):
        transition_dx = transition_end[0] - transition_start[0]
        transition_dy = transition_end[1] - transition_start[1]
        transition_horizontal = math.hypot(transition_dx, transition_dy)
        transition_side_x = -transition_dy / transition_horizontal
        transition_side_y = transition_dx / transition_horizontal
        transition_half = transition_width * 0.5
        transition_start_z = (_floor_surface_z(transition_start, transition_end, 0.0)
                              + VISIBLE_SURFACE_OFFSET)
        transition_end_z = (_floor_surface_z(transition_start, transition_end, 1.0)
                            + VISIBLE_SURFACE_OFFSET)
        transition_triangles.append(_add_quad(
            meshes[0],
            (transition_start[0] - transition_side_x * transition_half,
             transition_start[1] - transition_side_y * transition_half, transition_start_z),
            (transition_end[0] - transition_side_x * transition_half,
             transition_end[1] - transition_side_y * transition_half, transition_end_z),
            (transition_end[0] + transition_side_x * transition_half,
             transition_end[1] + transition_side_y * transition_half, transition_end_z),
            (transition_start[0] + transition_side_x * transition_half,
             transition_start[1] + transition_side_y * transition_half, transition_start_z)))

    _add_settlement_shelf(meshes[1])
    _add_station_islet(meshes[1])
    _validate_route_surfaces(meshes, route_triangles, cap_triangles, transition_triangles)

    asset_names = tuple(TERRAIN_OBJ_PATHS.keys())
    for family, asset_name in enumerate(asset_names):
        vertices = meshes[family]["vertices"]
        faces = meshes[family]["faces"]
        TERRAIN_EXPECTED_BOUNDS[asset_name] = (
            tuple(min(vertex[axis] for vertex in vertices) for axis in range(3)),
            tuple(max(vertex[axis] for vertex in vertices) for axis in range(3)),
        )
        output_path = TERRAIN_OBJ_PATHS[asset_name]
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, "w", encoding="utf-8", newline="\n") as obj:
            obj.write("# LOW TIDE continuous route terrain section {}\n".format(family))
            for vertex in vertices:
                # UE's OBJ converter reflects source Y. Pre-reflect to retain authored gameplay coordinates.
                obj.write("v {:.3f} {:.3f} {:.3f}\n".format(vertex[0], -vertex[1], vertex[2]))
            for index in range(len(vertices)):
                obj.write("vt {:.6f} {:.6f}\n".format((index % 17) / 16.0, (index % 13) / 12.0))
            for face in faces:
                # Pre-reflection changes handedness, so reverse winding to retain upward top faces.
                obj.write("f {}\n".format(" ".join("{0}/{0}".format(index + 1) for index in reversed(face))))
        log("Authored continuous terrain {}: {} vertices, {} triangles".format(
            asset_name, len(vertices), len(faces)))


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
