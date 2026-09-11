# Current state — 2026-09-11

## Acceptance and active milestone

- M0.5 is the Director-accepted historical greybox. Its tide-warning readability limitation was explicitly non-blocking.
- M1 is the active authored coastal-expedition candidate. It is **not Director-accepted**: manual 8–12 minute timing, listening/audio-quality, final art-quality review and the Director decision remain pending.

## Verified M1 candidate

- The default Entry-map launch provides Mara's signal-logbook expedition, optional singing-shard risk route, rising-tide shortcut closure, elevated blue-post escape, one phenomenon and recovery/second-trip loop.
- `Scripts/Test.ps1` reported **8/8 successes, 0 failures** at 2026-09-11 17:47:34 UTC (`Artifacts/Tests/index.json`; 4.634 s): four M0.5 regressions and four M1 tests. M1 coverage includes real `CharacterMovement` traversal of main/optional/escape routes, tide closure, five-second wet-exposure grace and recovery retention, phenomenon recovery/second trip, mission transaction and containment.
- Final asset recook/package completed: `BuildCookRun` took **87.73 s** with **zero cook errors and warnings**. The fresh M1 root executable is `Artifacts/Windows/LowTide.exe`; the `.pak` timestamp is 17:49:06 UTC and `.ucas` timestamp is 17:49:08 UTC. The prior full C++ package took 322.04 s; the last C++ Editor build took 165.67 s.
- Source and staged `Content/Data/Items.json` SHA-256 match: `01AFF1FD67EBD6B851BFEFB4886A56168DEB60DDD947E3503745B2469F1274EF`.
- The project-authored terrain recook completed after the Y-reflection fix. Generation validates source route arrays of 11 main, 8 alternate and 7 optional points, rejects mismatches, and verifies imported terrain bounding boxes against authored coordinates within 0.1 cm. The visible terrain remains non-colliding; hidden route-floor cubes retain the walkable transforms.
- Fresh packaged station and shrine screenshots were verified at 17:50–17:51 UTC: coordinate reflection is fixed and landmarks align. The runtime exited cleanly; the only recorded runtime warning was `r.MotionVectorSimulation` on the render thread.
- The fresh shore screenshot was verified at approximately 17:57 UTC. Stationary packaged performance was captured at 1920×1080/D3D11 on GTX 1660 Ti Max-Q, quality groups 3, VSync 0 and idle 0: 1,500 frames per view, first 300 discarded and 1,200 analyzed. Shore: 52.1 FPS, 19.194 ms mean frame, 20.928 ms p95, 19.194 ms GPU, 539.210 MB local memory and 192 mean draws. Rising station: 54.1 FPS, 18.498 ms mean frame, 20.034 ms p95, 18.501 ms GPU, 539.238 MB local memory and 199 mean draws. `Artifacts/M1-performance-shore.json`, `Artifacts/M1-performance-station-rising.json`, and `Artifacts/Profiles/M1Final` hold the summaries and logs; no errors/fatals were recorded, with only the existing `r.MotionVectorSimulation` warning.

## Evidence limits and next action

- Automation, package and stationary views do not establish physical-input usability, first-play duration, audible output, visual quality, accessibility, a full-route performance budget or lower-GPU certification.
- This is the first authored stylized M1 candidate and remains sparse/simple; it needs Director art-quality evaluation and is not claimed as commercial-quality acceptance. Complete manual route/timing/audio/art review and record the Director decision.

## Toolchain and constraints

- UE 5.8.2 CL56702186; Visual Studio Community 2026/MSVC 14.50; Windows SDK 10.0.26100.0; engine-bundled .NET 10. Build and shader concurrency are capped at two on the 16 GB development machine.
- Offline Win64, EUR 0 purchases, assets, servers or paid APIs, and no Fast mode. GPT-5.3-Codex-Spark was verified working; this run's allowance cap is temporary capacity, not permanent unavailability.
