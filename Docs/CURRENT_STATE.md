# Current state — 2026-09-12

## Active milestone and latest checkpoint

M0.5 is Director-accepted; M1 remains a candidate. Lodka and Verstak are integrated with Budka, the hero job board and Papug as the five Director-approved Meshy quality anchors. Technical validation is complete; Director visual/playtest acceptance is pending. No M2, broad world-art pass, new mechanics, paid service or Fast mode.

## Five-hero start hub

- Original `SourceAssets/Generated/Lodka/Lodka.glb` and `Verstak/Verstak.glb` are preserved verbatim through Git LFS. Both are binary glTF 2.0, one merged mesh/material, UV0 and source vertex normals, without a useful separate-part hierarchy, rig or animation. Exact source hashes, import paths and reproduction: `MESHY_WORK_BOAT_IMPORT.md` and `ASSET_PROVENANCE.md`.
- Lodka: 45,814 triangles / 65,256 vertices; 4096 px Base Color and packed Metallic-Roughness, no source Normal/AO. Uniform 400 × 179.68 × 121.89 cm. Hauled up at (2150,-50,105) cm, yaw -35, with two simple hull boxes. Lowered 20 cm after visual inspection so the hull seats on shore rather than appearing suspended above it.
- Verstak: 3,079,551 triangles / 1,688,476 vertices; 2048 px Base Color, packed Metallic-Roughness and Normal, no AO. Uniform 200 × 87.77 × 115.90 cm. Salvage working zone at (400,-700,125), yaw 30, with one under-worktop collision box at local Z 0..80 cm.
- Source PBR preserved: Base Color sRGB, packed G roughness/B metallic in linear Masks compression/sampler; normal compression/sampler and the Interchange green-channel conversion for Verstak. No invented Normal for Lodka, duplicated default materials, geometry/texture reduction or renderer change. Both masters remain intact and static; no new lamp/motion/boat/crafting system was added.
- Budka and Mara stay unchanged, including Mara's exact (900,-1350,215) interaction position. Job board remains (1300,-850,125), yaw -17, with its existing subtle paper/lantern motion, warm light and E mission flow. Papug remains static and intact. Prior metadata: `MESHY_HUB_IMPORT.md` and `HERO_JOB_BOARD_IMPORT.md`.
- Removed from runtime: old WorkCluster/GoodsCluster, five foreground donor props (crate, bottle crate, barrel, rowboat, paddle), the crude hut at (1250,-2100), and the procedural boat at (1450,1250). Source assets remain available. Remote huts/boat/wreck and suitable support dressing remain.
- An additional composition pass brought Lodka fully into the spawn view and added a small existing rock/plant/grass group at the right apron edge. Both explicit and repeated old foliage placements were moved clear of the workbench; final close capture confirms no plant intersection.
- Deck now uses contiguous warm-wood planks with dark backing under bevel joints. Cyan accent planks and clipped seam slivers were removed. The conforming overlay retains 635 checked samples at terrain +2 cm, zero deviation, and unchanged gameplay collision. OBJ reimport assigns materials by actual slot name because Interchange can retain old slot order.

## Validation and deliverable

- Final Editor build passed in 41.48 seconds. All 12 required automation tests passed with zero warnings, failures or skipped tests, after the final placement corrections. Coverage includes actual walking/sprinting/jumping, routes/containment, board/Mara sightlines, hero asset/collision contracts, trade, mission reward, protected evidence, tide/phenomenon recovery and another expedition. The extra hull-clearance query isolates boat proxies; normal full-world traversal tests remain intact.
- Final Windows BuildCookRun passed in 161.79 seconds; cook reported zero errors/warnings. Staged Items.json matches source. Executable: `Artifacts/Windows/LowTide.exe`; inner game binary: `Artifacts/Windows/LowTide/Binaries/Win64/LowTide.exe` (UTC 2026-09-12 11:20:56).
- Seven final packaged DX11 captures exited 0. Inspected `Artifacts/WorkBoatReview/final/{hub,trader,bench,boat,overview,counter,board}.png`. Mara and board E prompts are readable; no error/assertion/material/default-material failure occurs in these logs. Fresh final build/test logs and screenshots remain in ignored `Artifacts/WorkBoatReview`.
- At 1920×1080, same spawn camera, 900-frame capture with first 300 discarded: previous checkpoint 22.463 ms mean / 44.52 FPS / 23.923 ms p95 / 424.5 mean draws; current 22.452 ms / 44.54 FPS / 24.714 ms p95 / 358.7 draws. This local sample is not a whole-expedition performance guarantee. Hero LOD0/textures were retained.
- Import diagnostics: new GLBs imported with zero errors and one commandlet-only deprecated collision-API warning; apron import had zero errors and two existing OBJ smoothing-keyword warnings. Final real DX11 materials are verified. No known remaining hero import failure. The surrounding broad grey shore, distant coarse geometry, large open apron and Mara placeholder still fall below the hero quality bar; this pass does not claim final environment-art acceptance.

## Preserved baseline and next action

Walk 650 cm/s; Shift sprint 1040; Space jump 420, gravity 1.3, air control 0.2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Protected logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores supported dry ground without penalty.

UE 5.8.2 CL56702186; VS2026/MSVC14.50, SDK10.0.26100.0, .NET10. Conservative DX11, no Lumen/Nanite, two workers on 16 GB RAM / GTX1660Ti Max-Q. Asset spend EUR 0; no Fast mode or paid generation/API. Sol handled Blender/import diagnosis, Terra bounded actor/tests, lead reviewed integration and validated the build.

Next action: Director reviews the new workbench/boat scale, five-hero composition and shore placement, and retests Mara/board E interaction and departure. Screenshots and automation do not replace hands-on acceptance. Newly appearing untracked SourceAssets/Generated/Deck/ is preserved outside this task. No further art pass or M2 starts automatically.
