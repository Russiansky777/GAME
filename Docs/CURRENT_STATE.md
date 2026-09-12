# Current state — 2026-09-12

## Active milestone and latest checkpoint

M0.5 is Director-accepted; M1 remains a candidate. The Director-generated Meshy Budka hut and Papug parrot are integrated alongside the existing Meshy job board. Technical integration and packaged visual inspection are complete; Director visual/playtest acceptance is pending. No M2, broad world-art pass, new mechanics or paid services.

## Hero hub integration

- Sources preserved verbatim with Git LFS: `SourceAssets/Generated/Budka/Budka.glb` and `SourceAssets/Generated/Papug/Papug.glb` (binary glTF 2.0). Exact hashes, source provenance and import reproduction are in `ASSET_PROVENANCE.md` and `MESHY_HUB_IMPORT.md`.
- Budka: 3,029,367 triangles / 1,710,089 vertices, one merged mesh/material, uniformly scaled to 500 × 375.24 × 344.39 cm. Replaces all seven old trader kit visual components (structure, awning, counter, storage, nautical, workbench and sign). Old source/assets are retained; old components are no longer spawned.
- Papug: 1,953,478 triangles / 1,434,202 vertices, one merged bird + full floor stand/material, uniformly scaled to 61.40 × 67.03 × 140 cm. Replaces the old procedural bird. No rig/animation exists; static fallback preserves the source instead of wobbling its wooden stand or building a character system. No tick or collision.
- Both preserve UV0, source vertex normals, 4096 px Base Color, 4096 px Normal and 2048 px packed Metallic-Roughness. Base Color uses sRGB; Normal uses normal compression/sampler and the Interchange green-channel conversion; packed G roughness/B metallic use linear Masks compression/sampler. No invented AO, duplicate default material, texture reduction, LOD reduction, Nanite or renderer change.
- Mara remains exactly (900,-1350,215) cm with unchanged trading/mission/reward code. Hut root is (900,-1350,125), yaw -38, with visual basis correction. Five compact structural boxes block movement and occlude closed walls; visual meshes do not use per-poly collision. Front counter remains accessible and directly traceable.
- Papug stands near the open front corner at approximately (844.6,-1014.8,125), actor yaw -146. Board root moved to (1300,-850,125), yaw -17; its existing lantern/paper motion, light and E acceptance/review logic are preserved. Current board import details: `HERO_JOB_BOARD_IMPORT.md`.

## Validation and Windows deliverable

- Final Editor compilation passed in 35.72 seconds. All 12 required tests passed: zero warnings, failures or not-run. Coverage includes walking/sprint/jump and route containment, board/Mara sightlines, structural hut collision, decorative parrot contract, trade, unique rewards, protected evidence, tide/phenomenon recovery and second expedition.
- Real DX11 inspection caught reversed asset fronts: Blender-to-glTF-to-Interchange maps Blender (X,Y,Z) to UE (X,-Y,Z). Mesh-only 180-degree corrections restore both fronts without moving gameplay anchors. The first board placement blocked the departure line; moving it south restored tested walking and sprinting. Neither issue remains in the package.
- Windows BuildCookRun passed in 201.83 seconds; cook reported zero errors/warnings. Packaged Items.json hash matches source. Executable: `Artifacts/Windows/LowTide.exe` (inner binary `Artifacts/Windows/LowTide/Binaries/Win64/LowTide.exe`).
- Five final packaged DX11 captures exited 0 with no error, assertion or material/default-material failure. Inspected `Artifacts/MeshyHubReview/packaged/hub.png`, `trader.png`, `counter.png`, `parrot.png` and `board.png`. Close counter and board captures show their E prompts. Build/test/import/capture evidence remains under ignored `Artifacts/MeshyHubReview`.
- Same 1080p spawn-camera profile, 900 frames with first 300 discarded: before 22.451 ms mean / 44.54 FPS / 24.119 ms p95 / 626.7 mean draws; after 22.463 ms / 44.52 FPS / 23.923 ms p95 / 424.5 draws. This is one local camera sample, not a whole-expedition performance guarantee. Full hero LOD0 is retained; high triangle counts still warrant distance-LOD evaluation if later profiling demonstrates a bottleneck.
- Remaining visual limits: Mara and nearby secondary environment/deck are still placeholders, with visible coarse shapes and deck seams; small generated details retain source irregularities, and Unreal daylight differs from the Meshy preview. These were not restyled or represented as final art. No known remaining import/material failure. Bird animation is deliberately deferred.

## Preserved baseline and next action

Walk 650 cm/s; Shift sprint 1040; Space jump 420, gravity 1.3, air control 0.2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Protected logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores supported dry ground without penalty.

UE 5.8.2 CL56702186; VS2026/MSVC14.50, SDK10.0.26100.0, .NET10. Conservative DX11, no Lumen/Nanite, two workers on 16 GB RAM / GTX1660Ti Max-Q. Asset spend EUR 0; no Fast mode or paid generation/API. Sol handled Blender/import diagnosis, Terra bounded actor/tests, lead reviewed integration and validated the build.

Next action: Director retests the new hub's visual scale/composition, counter approach and E trading, board acceptance/review, and departure. Automated checks and screenshots do not replace hands-on acceptance. The newly appearing untracked `SourceAssets/Generated/Verstak/` is preserved outside this task. Do not start another art pass or M2 automatically.
