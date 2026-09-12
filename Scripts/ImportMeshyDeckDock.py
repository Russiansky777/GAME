"""Batch-import the inspected Meshy Deck, Dock and Crane modules through UE 5.8."""

import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ImportMeshyHub as common


PROJECT = unreal.Paths.project_dir()
REVIEW = os.path.join(PROJECT, "Artifacts", "MeshyHubReview")
DEST = "/Game/Generated/MeshyModules"


def spec(label, size, textures=2, native="plan axes", imported_material=None):
    folder = DEST + "/{0}/{1}/SM_Meshy_{1}_import".format(
        "Deck" if label.startswith("Deck") else "Dock" if label.startswith("Dock") else "Crane", label)
    result = {
        "source": os.path.join(REVIEW, label, "SM_Meshy_{}_import.glb".format(label)),
        "destination": folder.rsplit("/", 1)[0],
        "folder": folder,
        "mesh": folder + "/StaticMeshes/SM_Meshy_{}_import".format(label),
        "material": DEST + "/Materials/M_Meshy_{}_V1".format(label),
        "expected_size": size,
        "native_front": native,
    }
    if textures == 2:
        result["texture_names"] = ("texture_0", "texture_0_metallic_roughness")
        result["imported_material"] = imported_material or "BakedMaterial"
    return result


SPECS = {
    "DeckCorner": spec("DeckCorner", (200.000, 198.800, 27.183)),
    "DeckLong": spec("DeckLong", (200.000, 83.440, 11.470), native="long axis X"),
    "DeckOuterEdge": spec("DeckOuterEdge", (200.000, 170.230, 25.450)),
    "DeckRepaired": spec("DeckRepaired", (200.000, 187.480, 29.120)),
    "DeckStandard": spec("DeckStandard", (200.000, 181.370, 25.310), native="long axis X"),
    "DeckStepRamp": spec("DeckStepRamp", (200.000, 196.290, 37.640), native="step direction Y after reflection"),
    "DeckTransition": spec("DeckTransition", (186.010, 200.000, 25.870), native="long axis Y after reflection"),
    "DockCornerPlatform": spec("DockCornerPlatform", (357.920, 400.000, 80.160), 3),
    "DockEndBerth": spec("DockEndBerth", (400.000, 302.270, 155.910), 3, "long axis X"),
    "DockLadderAccess": spec("DockLadderAccess", (400.000, 188.020, 144.470), 3, "long axis X; ladder side reflected Y"),
    "DockRepaired": spec("DockRepaired", (400.000, 151.470, 89.630), 3, "long axis X; repair side reflected Y"),
    "DockStraight": spec("DockStraight", (400.000, 154.160, 88.880), 3, "long axis X"),
    "Crane": spec("Crane", (226.050, 130.270, 300.000), 2, "boom endpoint determined from imported profile"),
}


def main():
    unreal.EditorAssetLibrary.make_directory(DEST)
    for label, item in SPECS.items():
        common.import_one(label, item)
        material = unreal.EditorAssetLibrary.load_asset(item["material"])
        if not isinstance(material, unreal.Material):
            common.fail("Missing reusable material " + item["material"])
        material.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("[ImportMeshyDeckDock] SUCCESS: imported and validated {} reusable modules".format(len(SPECS)))


if __name__ == "__main__":
    main()
