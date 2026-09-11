# Current state — 2026-09-11

## Active milestone

M0.5 is Director-accepted. M1 remains a candidate for Director evaluation. This checkpoint is a bounded clarity/presentation pass on the existing traversal-corrected candidate. M1 is not yet Director-accepted.

## Bounded clarity pass

- Immediate objective, secured logbook/return step and completed 75-credit reward are distinguished. The optional 180-credit Singing Shard is advertised only while still available.
- M1 tide HUD explains exploration opportunity, changing/closed access and the blue-ridge escape. Normal exploration has no persistent countdown; the existing critical closure countdown and transition cues remain. Tide timings, route logic and recovery rules are unchanged.
- The shard's longer pickup message and existing anomaly cue connect its value to a reaction in the cove. Explicit movement/stop rules and watcher-distance telemetry are removed from the HUD. Ordinary salvage feedback stays brief; inventory identifies protected evidence as not for sale.
- Inventory/trading suppress world prompts and crosshair, and menu transitions clear focus. Layout fixes remain temporary HUD work.
- Intentionally deferred all new frameworks, mechanics, balance, art/assets, Mara model/collision, missions, save/load, crafting, menus/settings and soundtrack. No architecture expansion was needed.
## Clarity validation

- Editor rebuild succeeded in 21.20 s. Fresh existing automation report at 2026-09-11 19:31:09 UTC: 11/11 success, zero failures/not-run/in-process. Added only assertions for optional-objective availability and rare pickup value/reaction to the existing M1 mission regression.

- Windows BuildCookRun succeeded in 224.99 s, exit 0; launcher timestamp 19:34:35 UTC and UTOC 19:35:13 UTC. Source/staged item catalog hashes match.
- Fresh packaged low/rising HUD captures at 1280x720 were inspected: readable phase/action text without ordinary countdown, transition feedback separate from header. Both capture runs exited 0. Evidence: `Artifacts/ClarityReview/low.png`, `rising.png` and accompanying logs. No errors/fatals; only the existing `r.MotionVectorSimulation` warning.
- Inventory/trade layout and critical-warning visibility were code-reviewed, but the short hidden menu launch exposed no targetable native window; that owned process was stopped. No fresh interactive menu or full-expedition playthrough is claimed. Director retest should check menu readability, pickup emphasis and whether restrained phenomenon feedback conveys causality without over-explaining.
## Previous traversal correction (retained)

- Walking 650 cm/s; hold Shift sprint 1040 cm/s. Space gives one jump at 420 cm/s vertical launch, gravity scale 1.3 (roughly 69 cm ballistic height), air control 0.2, automatic step 45 cm and walkable slope limit 45 degrees. No stamina or traversal expansion.
- Visible route ribbons and junction caps follow the actual floor proxies, including upper/lower overlaps. The surface is deliberately 1 cm beneath collision. Floors extend behind existing boundaries. A full-width landing at Z=125 cm and a 7.8% ramp remove the settlement-to-expedition lip. A tapered 60–150 cm irregular outer lip softens straight shore cuts while edge skirts and submerged shoulders cover exposed terrain endings; these remain simple prototype coast shapes.
- Generator validation covers all three route arrays (11/8/7 points), source width agreement, imported coordinates and 2,832 triangle-ray probes across five lanes, caps and transitions. Maximum unexplained surface delta is zero after the deliberate 1 cm offset; sampled join step is at most 0.5591 cm. The three visual terrain meshes total 2,908 triangles. Import evidence: `Artifacts/TraversalTerrainLipImport.log` (exit 0, two deprecation warnings).
- A geometry fall below Z=-1000 restores the last supported dry location, or shore if that location is now flooded. It retains items, credits, mission state and the expedition snapshot. Below valid M1 walking elevations, the falling player cannot accidentally receive a tide/watcher penalty before this fallback. Normal wet-ground and watcher recovery rules remain intact.
- Low/rising tide timings remain 300/180 seconds. Jumping over wet ground cannot reset the five-second escape grace. Watcher speed is 845 cm/s to preserve its original 1.3x walking-speed ratio after the movement increase. The optional branch, reward choices and elevated escape are unchanged.

## Previous traversal evidence (historical)

- Editor build succeeded in 11.52 s after the test fixture include correction.
- `Scripts/Test.ps1`: 11/11 successes, no failures/skips/warnings, report `Artifacts/Tests/index.json` created 2026-09-11 18:53:37 UTC. Five M0.5 regressions and six M1 regressions include physical jump/no-double-jump/landing/40 cm curb, full main/optional/elevated routes at walk and sprint without jumping, bidirectional settlement transition, actual sprint-jump attempts at the closed shortcut and representative boundaries, invalid-fall retention, wet-ground grace during a jump, ordinary salvage loss, protected evidence/credit retention, reward/rare choices and second expedition.
- Windows package succeeded: final asset recook BuildCookRun 71.19 s (preceding full package 201.68 s), zero cook errors/warnings. Launcher `Artifacts/Windows/LowTide.exe` is the fresh corrective build; PAK timestamp 18:55:00 UTC, UCAS/UTOC 18:55:03 UTC. Source/staged Items.json SHA-256 agree: `01AFF1FD67EBD6B851BFEFB4886A56168DEB60DDD947E3503745B2469F1274EF`.
- Editor first-person settlement-exit screenshot confirms continuous visible ground (`Artifacts/TraversalExitPreview.log`, `Saved/Screenshots/Windows/M1-return.png`). All 23 route-segment views plus both settlement joins were captured and reviewed (`Artifacts/TraversalReview/manifest.json`). After the outer-lip-only revision, six fresh packaged views confirmed the final surface and softened coastline (`Artifacts/TraversalFinalReview/manifest.json`). Actual camera coordinates were checked against runtime logs. No obvious visible ground gaps remain on the reviewed intended routes; all game runs exited cleanly. The only final runtime warning is the existing Unreal `r.MotionVectorSimulation` render-thread warning.

## Limits and next action

Next action: wait for Director M1 evaluation of this clarity build: movement feel, overall pacing, tide readability, optional risk, entity behavior, atmosphere, visual quality and fun. The 8–12 minute first-play target is not a measured manual result. Automated traversal uses real CharacterMovement/input bindings, while image capture uses positioned review cameras; neither is a Director keyboard/mouse playthrough. Jump checks cover representative approaches plus broad perimeter sweeps, not every possible oblique trajectory. Mara's temporary pass-through representation remains explicitly non-blocking and deferred.

Prior-candidate 52–54 FPS stationary 1080p measurements are historical; this pass does not certify full-route performance or lower-end GPUs. Do not start another broad M1 feature pass or M2 before the next Director feedback.

## Environment and workflow

UE 5.8.2 CL56702186; VS Community 2026/MSVC 14.50; Windows SDK 10.0.26100.0; bundled .NET 10. Build/shader concurrency two on 16 GB RAM / GTX 1660 Ti Max-Q. Commands: `Scripts/Build.ps1 -Mode Editor`, `Scripts/Test.ps1`, `Scripts/Build.ps1 -Mode Package`. Generated evidence/package output is ignored under Artifacts/Saved.

EUR 0 and no Fast mode. Spark invocation hit its temporary preview allowance cap, so the bounded clarity implementation/validation used Terra; prior traversal controls/tests used Terra, terrain debugging Sol and routine docs/tooling Luna. AGENTS records the soft preference for filesystem/scripts/CLI and releasing idle GUI control without interrupting work.
