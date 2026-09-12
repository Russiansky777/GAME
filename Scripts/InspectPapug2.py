"""Inspect and normalize only the Director-provided Papug2 GLB."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from InspectMeshyHub import ROOT, inspect


if __name__ == "__main__":
    inspect(
        "Papug2",
        {
            "path": ROOT / "SourceAssets/Generated/Papug2/Papug2.glb",
            "target_height_m": 1.40,
            "native_front": "UE +Y (source bird face is Blender -Y; confirmed by preview)",
        },
        ROOT / "Artifacts/MeshyHubReview",
        False,
    )
