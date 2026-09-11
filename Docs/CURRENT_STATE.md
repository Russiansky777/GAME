# Current state — 2026-09-11

## Active milestone

M0.5 is Director-accepted. M1 remains a candidate. The Director rejected the previous primitive landmark pass as visually insufficient. This checkpoint replaces the trader shop with a focused authored environment; no whole-world art or M2 is authorized.

## Focused trader hub

- Seven reusable Blender-authored meshes replace the primitive stall: curved wooden roof/framing, striped sagging canvas, counter, storage, nautical dressing, workbench and raised SALVAGE sign. 25,144 triangles, eight palette instances of the existing opaque master, no new textures or lights. Source generator, OBJ/MTL exports and import manifest are retained.
- Mara and the shop move together to (900,-1350), preserving their floor heights and clearing the initial expedition route. Three rear/side wall proxies belong to the scene, leaving the front trade approach open. Decorative meshes remain non-colliding.
- Removed the superseded stall, floating location labels and overhead route frame. Compact outward/return markers and existing distant signal-station/shrine landmarks remain. Gameplay timings, rewards, movement and routes are unchanged.
- Actual UE review caught and corrected mirrored sign lettering; automation caught and corrected a side wall obstructing the route. Final tests and packaged visual review pass. The prior pass remains Director-rejected, and this replacement still requires Director art judgment.
- Project-authored assets cost EUR 0. Bundled Inter is converted to sign mesh with SIL OFL notice in INTER_FONT_LICENSE; no marketplace purchase or external environment kit. Provenance is recorded in ASSET_PROVENANCE.

## Validation

Final Editor compile passed in 41.30 seconds. Fresh automation at 21:23:29 UTC passed all 11 tests, including required hub assets/proxies, route containment, Mara visibility, movement, mission/reward, tide recovery, protected evidence and second-trip regressions.
- Windows BuildCookRun passed in 213.28 seconds, exit 0; cook zero errors/warnings. Launcher timestamp 21:26:35 UTC, UTOC 21:27:13 UTC. Source/staged Items.json hashes match. Playable executable: `Artifacts/Windows/LowTide.exe`.
- Five fresh packaged 1280x720 captures (start, facade approach, departure, route entry and shore overview) were inspected under `Artifacts/HubQuality/packaged`; all launches exited 0. SALVAGE reads correctly, shop supports reach the ground and the shop no longer obstructs departure. Runtime logs contain no errors/fatals; existing Unreal `r.MotionVectorSimulation` warning remains. These are positioned-camera checks, not a manual playthrough.
- Same stationary shore 1080p capture, 900 frames with first 300 excluded: mean 19.386 ms / 51.58 FPS, p95 20.951 ms; mean draw calls 290. Prior short sample was 19.363 ms / 51.65 FPS and 199.4 draw calls. Frame time is similar in these samples, but draw calls increased; this is not a full-route benchmark or low-end GPU certification. Evidence: `Artifacts/HubQuality/performance-summary.json` and raw CSV/logs.
- Existing decorative instances decrease 242 to 202 alongside the seven new authored mesh components. Terrain stays 2,908 triangles. No renderer-setting changes. Import validates all seven mesh bounds and palette slots; font/source provenance is retained.

## Preserved gameplay / previous evidence

- Walking 650 cm/s, hold Shift sprint 1040, Space single jump 420 cm/s vertical launch, gravity 1.3, air control 0.2, step 45 cm / slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s.
- Protected logbook returns to Mara for one 75-credit reward. Optional Singing Shard 180 credits may be kept, sold or grounded. Wet recovery removes only current-expedition ordinary salvage; prior stock, evidence, money and permanent state survive. Invalid-ground recovery below Z=-1000 restores the last supported dry spot/shore without penalty.
- Previous clarity pass retained: immediate/main-complete/optional objective text, phase/action labels without ordinary countdown, critical closure warning even with menus open, restrained shard reaction feedback, protected-evidence labeling and menu focus cleanup. Full interactive menu readability remains for Director retest.
- Terrain remains 2,908 triangles across three meshes; collision proxies are unchanged. Prior generator validation covered 2,832 surface probes, deliberate 1 cm visual offset, max sampled join 0.5591 cm, full-width Z=125 settlement landing and 7.8% ramp. Evidence remains in `Artifacts/TraversalTerrainLipImport.log` and `Artifacts/TraversalFinalReview`.

## Limits and next action

Director retest: assess shop quality, safe-base identity, approach/exit readability, Mara interaction and the complete expedition loop. Mara, inventory/trading UI, nearby secondary huts and most of the island remain placeholders. Counter and small props remain non-colliding decoration; only the rear/side structural walls block the player. The 8–12 minute first-play target remains unmeasured; positioned screenshots and automation are not a manual playthrough. No M1 acceptance or M2 is implied.

## Environment and workflow

UE 5.8.2 CL56702186; VS Community 2026/MSVC 14.50; Windows SDK 10.0.26100.0; bundled .NET 10. Build/shader concurrency 2 on 16 GB RAM / GTX 1660 Ti Max-Q. Conservative DX11 renderer, conventional shadows, no Lumen/Nanite/raytracing. Use `Scripts/Build.ps1 -Mode Editor`, `Scripts/Test.ps1`, `Scripts/Build.ps1 -Mode Package`.

EUR 0, no Fast mode. Current collaboration exposes explicit Sol/Terra/Luna/Astra routing but not Spark; its earlier successful availability/cap is historical. This focused hub task used Sol for Blender authoring and Terra for bounded integration, with lead integration/visual review. Generated review/profile/build artifacts remain ignored under Artifacts/Saved.
