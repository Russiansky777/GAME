# Current state — 2026-09-12

## Active milestone and latest checkpoint

M0.5 is Director-accepted. M1 remains a candidate; no M2 or wider hub redesign was started. The latest task integrates the Director-generated Meshy hero mission board. Its packaged materials and assembly are verified; Director visual/playtest acceptance is still pending. The rest of the hub retains its previous quality limitations and is not represented as final art.

## Hero mission board

- Source: `SourceAssets/Generated/JobBoard/SM_JobBoard_Hero_v01.glb`, preserved verbatim through Git LFS. SHA-256 `20016CBAF14EBE6276AAE9A20A67A01AA8C3979BF0AAFEEB1BDAAA827603082B`. Director-generated via Meshy; provenance and source/licence limitations are recorded in `ASSET_PROVENANCE.md`.
- Inspected 24,516 vertices / 16,112 triangles, one node/material/UV set, geometric normals and three embedded 4096 px maps. No normal or occlusion texture exists. UE 5.8 Interchange imports the normalized 180 cm wide × 210 cm tall × 41.56 cm deep board. Source Base Color/Emissive use sRGB; packed G Roughness/B Metallic use linear Masks. No synthetic normal map or photorealistic production pipeline was added.
- A narrow, reviewed lantern/hanger/charm split preserves all original triangles. Two authored paper-edge accents add 16 triangles. Separate anchors counter Interchange's baked board-space coordinates and Y flip. Lantern sway is ±0.85 degrees; papers ±0.45/0.35 degrees, 30 Hz near and 2 Hz checks beyond 32 m. A small warm shadowless light follows the lantern; materials remain opaque.
- The existing board root remains (1450,170,125) cm, yaw 7. Old floating text is removed. Three simple structural collision boxes cover posts/panel; decorative meshes have no collision. The E trace target blocks only Visibility and ignores Pawn. Acceptance/review methods and mission/reward logic are unchanged.
- Reproduction and exact imported paths/pivots: `HERO_JOB_BOARD_IMPORT.md`. Generated inspection files and superseded import aliases are preserved locally but excluded from Git. Unrelated Director-generated `Budka/Budka.glb` and `Papug/Papug.glb` remain untouched and untracked, outside this task.

## Validation and deliverable

- Final Editor build passed in 31.78 seconds after correcting a UE 5.8 TObjectPtr test call. Automation: 12/12 success, zero warnings/failures/not-run. Tests include board assembly/size, pivot cancellation, animation, structural clearance, E sightline and light cost, plus existing mission/reward/trade/traversal/tide/recovery/second-expedition regressions.
- Real DX11 review caught and fixed a metallic/roughness sampler mismatch that caused a default grey material. The importer now uses `TC_MASKS` with `SAMPLERTYPE_MASKS`, including existing graphs. Final material repair and paper-material commandlets each reported zero errors/warnings.
- Windows BuildCookRun passed in 173.92 seconds. Cook: 603 packages, seven platform skips, zero errors/warnings. Source/staged Items.json SHA-256 matches: `01AFF1FD67EBD6B851BFEFB4886A56168DEB60DDD947E3503745B2469F1274EF`.
- Executable: `Artifacts/Windows/LowTide.exe` (launcher UTC 2026-09-12 02:10:45; game binary UTC 02:09:44). Both final packaged launches exited 0. Close board and normal spawn screenshots were inspected; their logs contain no errors, assertions or material/default-material failures. Evidence: `Artifacts/HeroJobBoardReview/packaged/board.png` and `hub.png`. These camera captures plus automated regressions do not replace the Director's hands-on retest.
- Remaining visual limits: the two moving paper accents are simple authored additions; the source has no normal map, and Unreal lighting differs from Meshy's preview. No known remaining import/material failure was found. No new performance benchmark was taken for this board-only change.

## Preserved game and production baseline

Walk 650 cm/s; Shift sprint 1040; Space jump 420, gravity 1.3, air control 0.2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Protected logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores supported dry ground without penalty.

The previous Kenney/Quaternius donor dressing, authored shop/deck, wood/cloth materials and perched parrot remain unchanged. Broad grey ground, secondary buildings, distant rocks, foliage finish and Mara remain below the reference benchmark. Previous hub profile (not remeasured here): 20.470 ms / 48.85 FPS mean, 21.826 ms p95, 658.5 mean draws at 1080p.

UE 5.8.2 CL56702186; VS2026/MSVC14.50, SDK10.0.26100.0, bundled .NET10. Conservative DX11, no Lumen/Nanite, two workers on 16 GB RAM / GTX1660Ti Max-Q. Asset spend remains EUR 0; no Fast mode or paid generation/API was used. Sol handled import/Blender work, Terra actor/tests, and the lead reviewed/fixed integration and validated the package.

Next action: Director retests the new board's E acceptance/review, approach/collision, subtle motion and visual fit. Do not start another hub pass or M2 from this task.
