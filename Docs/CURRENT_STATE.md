# Current state — 2026-09-11

## Verified implementation
- M0.5 playable greybox in the existing UE 5.8.2 CL56702186 C++ project: first-person movement/look, settlement, causeway, salvage shelf, visible tide/access, recovery, five item definitions, six-slot pack, Mara and individual sales. Retained evidence is protected and does not respawn to fill the pack.
- Fixed layout is built by GameMode on /Engine/Maps/Entry. Geometry/materials come from Unreal engine primitives; item text/data and code are authored in this repository. No external art assets used.
- Director-approved final art target is high-quality stylized semi-cartoon 3D, with economical rendering. Recorded in GAME_DESIGN, DECISIONS and AGENTS; current primitives are mechanics validation only.
- Editor build succeeds. Scripts/Test.ps1 report at 2026-09-11 15:00:22 (as recorded in automation report): 4 successes, 0 warnings, 0 failures in 0.834 s. It includes RoundTrip, SafeEdgeReturn, AtomicIndividualSale and CapacityAndQuantityBoundaries; the test guard now requires all four names.
- Final Win64 Development package completed 2026-09-11 17:04 local: BUILD SUCCESSFUL, exit 0. Source and staged Items.json SHA256 match; Build.ps1 checks this.
- The fresh root executable `Artifacts/Windows/LowTide.exe` started without the editor at 17:05 local and remained responsive. Its log reaches LowTideGameMode on Entry with no fatal or catalog error.
- Acceptance fixes are implemented: sprint hold/release uses the 500-to-800 movement range, the route has 12 joined invisible boundaries, the closure warning lasts 20 seconds, and recovery removes only ordinary salvage from the current expedition while preserving prior stock, evidence and credits. M0.5 remains awaiting Director retest; no M1 work follows yet.
- Director previously confirmed walking/basic interaction and visible tide gating. New manual acceptance retest remains: hold/release Left Shift feel at about 1.6×, full perimeter at sprint, warning clarity, recovery's exact current-expedition salvage loss with money/evidence retained, and the next expedition.

## Performance / limits
- Prior checkpoint f632996 (not rerun for current acceptance fixes): GTX 1660 Ti Max-Q, 1920x1080, D3D11 SM5, default Epic scalability, VSync off, conservative Lumen/Nanite/ray tracing settings unchanged. CSV frames 500–3499: mean 17.160 ms (~58.3 fps), p95 18.579 ms, max 31.304 ms; GPU mean 17.160 ms; local GPU allocation ~539 MB; ~68 draw calls average. Process snapshot: ~543 MiB working set / ~958 MiB private memory.
- Stationary coast and inventory sample only, with other desktop apps running. Not a full-route stress test or lower-end GPU certification. Forced 1080p window exceeded the desktop logical bounds at 150% Windows scaling; ordinary launch is the user entry point.
- Full keyboard/mouse expedition and physically disconnected-network acceptance remain to be checked. Automated gameplay tests call interaction methods directly. No save/load, finished art, audio, story or settings menu yet.
- Editor startup still emits built-in LogAutomationTest 'Condition failed' diagnostics during reflected-struct initialization, outside the passing project tests. The fresh packaged runtime still emits the engine r.MotionVectorSimulation render-thread warning; it also reports an unavailable Windows ROOT certificate store, which has no observed impact on this offline launch. Do not hide these when reporting validation.

## Toolchain / evidence
- UE: C:\Program Files\Epic Games\UE_5.8. VS Community 2026 18.10, MSVC 14.50 (19.50.35738), Windows SDK 10.0.26100.0, .NET Framework 4.8 SDK; engine-bundled .NET 10.
- Scripts/Build.ps1 -Mode Editor; Scripts/Test.ps1; Scripts/Build.ps1 -Mode Package. Build concurrency and shader workers capped at two on the 16 GB development machine.
- Tests: Artifacts/Tests/index.json. Runtime: Artifacts/Windows/LowTide/Saved/Logs/LowTide.log. Latest package log: %APPDATA%/Unreal Engine/AutomationTool/Logs/C+Program+Files+Epic+Games+UE_5.8/Log.txt. The earlier profile remains prior-checkpoint evidence, not a new performance measurement. Generated evidence/builds stay outside Git.
- Git/LFS configured; remote https://github.com/Russiansky777/GAME.git. Minimal toolchain checkpoint 6a8bb84 preceded this gameplay checkpoint. Fab/Bridge and AndroidFileServer disabled in the project.

## Next action
Director can review the packaged mechanical prototype at `Artifacts/Windows/LowTide.exe` using README controls. Complete the listed manual sprint/perimeter/warning/recovery/second-expedition retest before marking M0.5 acceptance closed or expanding into M1. The tide penalty remains provisional; no M1, UI polish or art expansion follows from this package. Keep the same project and conservative renderer; necessary game work/free installations are authorized without repeated confirmation; EUR 0 budget and no Fast mode still apply.
