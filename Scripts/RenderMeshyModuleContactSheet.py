"""Render a Blender-native contact sheet from Meshy module inspection previews."""

from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[1]
REVIEW = ROOT / "Artifacts/MeshyHubReview"
LABELS = [
    "DeckCorner", "DeckLong", "DeckOuterEdge", "DeckRepaired",
    "DeckStandard", "DeckStepRamp", "DeckTransition",
    "DockCornerPlatform", "DockEndBerth", "DockLadderAccess",
    "DockRepaired", "DockStraight", "Crane",
]


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    cols, cell, image_size = 4, 3.6, 3.15
    rows = (len(LABELS) + cols - 1) // cols
    for index, label in enumerate(LABELS):
        path = REVIEW / label / "preview.png"
        if not path.is_file():
            raise FileNotFoundError(path)
        col, row = index % cols, index // cols
        x = (col - (cols - 1) * 0.5) * cell
        y = ((rows - 1) * 0.5 - row) * cell
        bpy.ops.mesh.primitive_plane_add(size=image_size, location=(x, y, 0))
        plane = bpy.context.object
        material = bpy.data.materials.new(label + "_Preview")
        material.use_nodes = True
        nodes = material.node_tree.nodes
        bsdf = nodes.get("Principled BSDF")
        image = bpy.data.images.load(str(path), check_existing=False)
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = image
        material.node_tree.links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])
        material.node_tree.links.new(texture.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 0.15
        plane.data.materials.append(material)
        bpy.ops.object.text_add(location=(x, y - image_size * 0.54, 0.02))
        text = bpy.context.object
        text.data.body = label
        text.data.align_x = "CENTER"
        text.data.size = 0.24
        text.data.extrude = 0.004
        text.data.materials.append(bpy.data.materials.new(label + "_Label"))
        text.data.materials[0].diffuse_color = (0.95, 0.95, 0.95, 1.0)
    bpy.ops.object.camera_add(location=(0, 0, 20))
    camera = bpy.context.object
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = rows * cell
    camera.rotation_euler = (0, 0, 0)
    bpy.context.scene.camera = camera
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1600
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(REVIEW / "module-contact-sheet.png")
    if scene.world is None:
        scene.world = bpy.data.worlds.new("ModuleContactWorld")
    scene.world.color = (0.015, 0.02, 0.03)
    bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main()
