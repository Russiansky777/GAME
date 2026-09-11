# Current state — 2026-09-11

## Active milestone

M0.5 is Director-accepted. M1 remains a candidate for Director visual/gameplay evaluation. The current checkpoint is the Director-authorized bounded hero-landmark / first-impression pass, not whole-world art, final polish or M2.

## Landmark pass

- Mara's existing stall gains continuous colored awning strips, ground-to-roof supports, a painted counter fascia, backed sign and harbor pennant. Nearby barrel placement is corrected. Mara remains a placeholder.
- A shoulder-clear gateway and compact markers frame the initial tide-road departure. Existing amber outward and blue return cues, route surfaces and tide access remain authoritative.
- The signal station gains aligned cream/teal wall panels, coral accents and a connected tiered crown/semaphore silhouette. The front/desk approach remains open.
- The optional Singing Shard site gains a dark asymmetric arch behind the pickup, low stone accents and restrained colored insets. The actual collectible and its existing pickup/phenomenon behavior are unchanged.
- All additions live in the non-colliding `ACoastalDressing` HISM layer. Existing meshes/master material are reused; one additional opaque violet instance family, no new textures, lights, downloaded assets, terrain imports, plugins or runtime systems. Authorship is recorded in ASSET_PROVENANCE.
- Deliberately deferred whole-island art, dense foliage, final NPC/UI, additional missions/mechanics and large prop libraries. This is an incremental visual candidate; commercial art quality is still a Director judgment.

## Validation

- Final Editor build passed in 29.53 seconds. `Scripts/Test.ps1` report at 2026-09-11 20:37:43 UTC: 11/11 passed, no failures. Existing movement, route containment, interaction visibility, reward/rare choices and recovery/second-expedition regressions retained. No new tests were added for static decorative transforms.
- Windows BuildCookRun passed in 173.83 seconds, exit 0; cook zero errors/warnings. Launcher `Artifacts/Windows/LowTide.exe` timestamp 20:40:06 UTC, UTOC 20:40:44 UTC. Source/staged Items.json hashes match.
- Six fresh packaged 1280x720 views (hub, gateway approach, opening route, shore overview, station, shrine) inspected in `Artifacts/LandmarkReview/packaged`. All six capture launches exited 0, with no errors/fatals; only the existing Unreal `r.MotionVectorSimulation` warning. Before/after views caught and corrected detached awning/crown pieces and shrine trim. These are positioned-camera reviews, not a fresh manual expedition.
- Same stationary shore 1080p CSV comparison, 900 frames each with first 300 excluded: baseline 19.007 ms / 52.61 FPS, final 19.363 ms / 51.65 FPS; p95 frame time 20.576→22.111 ms. Mean draw calls 190.4→199.4. About 1.9% mean frame-time increase in this short sample; not a full-route benchmark or low-end GPU certification. Profiler logs confirm normal exit. Evidence: `Artifacts/LandmarkReview/performance-summary.json` and raw CSV/logs.
- Dressing instances 167→242 (+75), using eight HISM families instead of seven. No renderer setting changes.

## Preserved gameplay / previous evidence

- Walking 650 cm/s, hold Shift sprint 1040, Space single jump 420 cm/s vertical launch, gravity 1.3, air control 0.2, step 45 cm / slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s.
- Protected logbook returns to Mara for one 75-credit reward. Optional Singing Shard 180 credits may be kept, sold or grounded. Wet recovery removes only current-expedition ordinary salvage; prior stock, evidence, money and permanent state survive. Invalid-ground recovery below Z=-1000 restores the last supported dry spot/shore without penalty.
- Previous clarity pass retained: immediate/main-complete/optional objective text, phase/action labels without ordinary countdown, critical closure warning even with menus open, restrained shard reaction feedback, protected-evidence labeling and menu focus cleanup. Full interactive menu readability remains for Director retest.
- Terrain remains 2,908 triangles across three meshes; collision proxies are unchanged. Prior generator validation covered 2,832 surface probes, deliberate 1 cm visual offset, max sampled join 0.5591 cm, full-width Z=125 settlement landing and 7.8% ramp. Evidence remains in `Artifacts/TraversalTerrainLipImport.log` and `Artifacts/TraversalFinalReview`.

## Limits and next action

Wait for Director evaluation of the new package: first impression, safe-shop identity, opening path/destination readability, optional-area temptation, pickup/phenomenon clarity, movement feel and the full expedition loop. The 8–12 minute first-play target is not a measured manual result. Automated input/traversal and positioned screenshots do not constitute a Director playthrough. No M1 acceptance or M2 is implied.

## Environment and workflow

UE 5.8.2 CL56702186; VS Community 2026/MSVC 14.50; Windows SDK 10.0.26100.0; bundled .NET 10. Build/shader concurrency 2 on 16 GB RAM / GTX 1660 Ti Max-Q. Conservative DX11 renderer, conventional shadows, no Lumen/Nanite/raytracing. Use `Scripts/Build.ps1 -Mode Editor`, `Scripts/Test.ps1`, `Scripts/Build.ps1 -Mode Package`.

EUR 0, no Fast mode. Current collaboration exposes explicit Sol/Terra/Luna/Astra routing but not Spark; its earlier successful availability/cap is historical. This bounded scene/tooling task used Sol, with lead integration/visual review. Generated review/profile/build artifacts remain ignored under Artifacts/Saved.