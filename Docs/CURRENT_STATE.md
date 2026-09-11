# Current state — 2026-09-11

## Verified implementation
- M0.5 playable greybox in the existing UE 5.8.2 CL56702186 C++ project: first-person movement/look, settlement, causeway, salvage shelf, visible tide/access, recovery, five item definitions, six-slot pack, Mara and individual sales. Retained evidence is protected and does not respawn to fill the pack.
- Fixed layout is built by GameMode on /Engine/Maps/Entry. Geometry/materials come from Unreal engine primitives; item text/data and code are authored in this repository. No external art assets used.
- Director-approved final art target is high-quality stylized semi-cartoon 3D, with economical rendering. Recorded in GAME_DESIGN, DECISIONS and AGENTS; current primitives are mechanics validation only.
- Editor build succeeds. Scripts/Test.ps1 report at 2026-09-11 13:54:21 UTC: 3 successes, 0 warnings, 0 failures. Includes actual CharacterMovement across the causeway and back over steps, pickups, sale, evidence protection, forced recovery and second expedition.
- Final Win64 Development package: BUILD SUCCESSFUL, exit 0; cook 0 errors / 0 warnings. Fixed the JSON staging setting to DirectoriesToAlwaysStageAsNonUFS. Packaged/source Items.json SHA256 match; Build.ps1 now checks this.
- Artifacts/Windows/LowTide.exe runs without the editor. Packaged scene and five-row inventory inspected; I key opens/closes the panel. No catalog/runtime errors in the profiled run.
- Ordinary packaged EXE launch was visually verified at the user display size: the full HUD, warning and feedback remain visible, and the game was left open for Director review.

## Performance / limits
- GTX 1660 Ti Max-Q, 1920x1080, D3D11 SM5, default Epic scalability, VSync off, conservative Lumen/Nanite/ray tracing settings unchanged. CSV frames 500–3499: mean 17.160 ms (~58.3 fps), p95 18.579 ms, max 31.304 ms; GPU mean 17.160 ms; local GPU allocation ~539 MB; ~68 draw calls average. Process snapshot: ~543 MiB working set / ~958 MiB private memory.
- Stationary coast and inventory sample only, with other desktop apps running. Not a full-route stress test or lower-end GPU certification. Forced 1080p window exceeded the desktop logical bounds at 150% Windows scaling; ordinary launch is the user entry point.
- Full keyboard/mouse expedition and physically disconnected-network acceptance remain to be checked. Automated gameplay tests call interaction methods directly. No save/load, finished art, audio, story or settings menu yet.
- Editor startup still emits built-in LogAutomationTest 'Condition failed' diagnostics during reflected-struct initialization, outside the passing project tests. Packaged runtime emits the engine r.MotionVectorSimulation render-thread warning. Do not hide these when reporting validation.

## Toolchain / evidence
- UE: C:\Program Files\Epic Games\UE_5.8. VS Community 2026 18.10, MSVC 14.50 (19.50.35738), Windows SDK 10.0.26100.0, .NET Framework 4.8 SDK; engine-bundled .NET 10.
- Scripts/Build.ps1 -Mode Editor; Scripts/Test.ps1; Scripts/Build.ps1 -Mode Package. Build concurrency and shader workers capped at two on the 16 GB development machine.
- Tests: Artifacts/Tests/index.json. Runtime: Artifacts/Windows/LowTide/Saved/Logs/LowTide.log. Profile: Artifacts/Windows/LowTide/Saved/Profiling/CSV/Profile(20260911_160134).csv. Summary: Artifacts/performance-summary.json. Generated evidence/builds stay outside Git.
- Git/LFS configured; remote https://github.com/Russiansky777/GAME.git. Minimal toolchain checkpoint 6a8bb84 preceded this gameplay checkpoint. Fab/Bridge and AndroidFileServer disabled in the project.

## Next action
Director can review the packaged mechanical prototype using README controls. Complete remaining manual input/offline coverage and address feedback before marking all M0.5 acceptance checks closed or expanding into M1's 8–12 minute slice. Keep the same project and conservative renderer; no substantial art production before referencing the approved stylized direction. Necessary game work/free installations are authorized without repeated confirmation; mandatory tool/OS controls, EUR 0 budget and no Fast mode still apply.
