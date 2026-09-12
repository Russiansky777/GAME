"""Import and validate only the normalized Papug2 donor through Interchange."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import unreal

from ImportMeshyHub import import_one, log


ROOT = Path(unreal.Paths.project_dir())
SPEC = {
    "source": str(ROOT / "Artifacts/MeshyHubReview/Papug2/SM_Meshy_Papug2_import.glb"),
    "destination": "/Game/Generated/MeshyHub/Papug2",
    "folder": "/Game/Generated/MeshyHub/Papug2/SM_Meshy_Papug2_import",
    "mesh": "/Game/Generated/MeshyHub/Papug2/SM_Meshy_Papug2_import/StaticMeshes/SM_Meshy_Papug2_import",
    "material": "/Game/Generated/MeshyHub/Papug2/M_MeshyHub_Papug2_V1",
    "expected_size": (57.069, 67.767, 140.0),
    "texture_names": ("texture_0", "texture_0_metallic_roughness"),
    "native_front": "UE +Y (source bird face is Blender -Y)",
    "imported_material": "BakedMaterial",
}


if __name__ == "__main__":
    import_one("Papug2", SPEC)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    log("SUCCESS: imported and validated Papug2 only")
