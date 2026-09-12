# Current state — 2026-09-12

## Active milestone and deliverable

M0.5 is Director-accepted; M1 remains a candidate awaiting Director playtest/art acceptance. Latest work is the focused water polish around the existing start hub and visible coast. No M2, new mechanics, broad world-art pass, paid service or Fast mode.

Windows executable: `Artifacts/Windows/LowTide.exe`. Inner binary: `Artifacts/Windows/LowTide/Binaries/Win64/LowTide.exe` (UTC 2026-09-12 19:12:27); final material recook/archive completed afterward. Start the current package, not an older copy.

## Coastal water polish

- Existing collision-free thin primitive and tide actor are reused. `/Game/Generated/Water/M_LT_CoastalWater` replaces the opaque sine-colour water. The shared one-sided DefaultLit translucent material uses surface per-pixel lighting from the existing scene; no extra light, reflection camera, refraction, particle actor or water simulation was added. The water surface no longer casts unnecessary shadows. Conservative global DX11 renderer settings remain unchanged.
- `Scripts/PolishWater.py` authors three deterministic seamless 512x512 textures: two linear Normalmap-compressed ripple maps and one linear Masks-compressed foam map. Sources are retained at `SourceAssets/Generated/Water` in LFS; four UE assets live at `Content/Generated/Water`. Repeat runs update the existing graph safely without deleting/recreating its roots. Provenance is recorded in `ASSET_PROVENANCE.md`.
- Two world-XY normal layers drift in different directions/speeds/scales, with normalized blending. Signed wave-vector directions reduce the first candidate's regular diagonal crosshatch. DepthFade drives shallow/deep colour, opacity and a noise-broken shoreline/contact foam mask. Water roughness .28 / specular .45; foam roughness .65; shallow/deep opacity .78/.98. More opaque deep water reduces visible seabed facets. These remain reversible art tuning.
- Tide heights/timings, boat draft/attachment, water geometry, collision, missions, inventory and recovery rules are unchanged. The foam is a screen-depth material effect, not an authored surf simulation; it cannot hide the coast's coarse silhouette. Angular grey shoreline and remote prototype geometry remain visible and outside this pass. No extra floating debris, boat rocking or broad environmental overhaul.

## Water validation

- Editor build passed in 104.15 seconds. All 12 existing automation tests passed with zero warnings/failures/skips after the C++ material binding/shadow change; subsequent edits were material/texture tuning only. Coverage retains dock round-trip CharacterMovement and safe-base behavior, walking/sprint/jump, boundaries, tide/alternate route, inventory/rewards, evidence retention, recovery and second expedition. Evidence: `Artifacts/WaterReview/build-editor.log`, `tests.log`, `test-index.json`.
- Final tuned material import exited 0 with 0 errors/warnings (`Artifacts/WaterPolish/import-tuned.log`). Windows BuildCookRun passed first in 263.79 seconds and final recook in 129.67 seconds; no cook error/warning was reported, and the staged item catalogue matches source. Logs: `Artifacts/WaterReview/package.log`, `package-tuned.log`.
- Four fresh packaged DX11 views were inspected: `Artifacts/WaterReview/final-tuned/{hub,shoreline,dock,sea}.png`. All exited 0 with no runtime/material failure. Additional stationary `motion-late/sea.png` at 12 seconds compared with the 5-second sea view shows changed ripple/foam patterns while the camera stays fixed. The early Editor image was discarded because shader compilation was still pending; only cooked runtime views establish the visual result.
- Same 1920x1080 spawn camera, 900 CSV frames with the first 300 discarded: before 24.741 ms / 40.42 FPS / p95 26.2513 ms / 363.7 mean draws; final 24.451 ms / 40.90 FPS / p95 25.3777 ms / 360.7 mean draws. No meaningful regression in this local sample; do not infer a whole-expedition GPU guarantee. Evidence: `Artifacts/WaterReview/{before,after-final}/performance-summary.json` plus CSV/logs. Both profiles use the previously documented engine `csv.ForceExit 1` diagnostic-only shutdown workaround and exit 0.

## Preserved hub and dock checkpoint

- Thirteen Director-generated Deck/Dock/Crane GLBs join Budka, Board, Papug, Verstak and Lodka. Originals, hashes, exact import paths and dimensions are in `ASSET_PROVENANCE.md`, `MESHY_DECK_DOCK_IMPORT.md`, `MESHY_HUB_IMPORT.md`, `HERO_JOB_BOARD_IMPORT.md`, and `MESHY_WORK_BOAT_IMPORT.md`. Source PBR quality is retained; Meshy modules are shared instances. The apron follows unchanged hidden floor proxies with up to 5-degree visual fitting.
- The working dock is walkable through the east-wall mouth Y150..450. Three smooth hidden floor proxies, stepped outer containment and a compact crane-base proxy permit walking around its north side to the berth and back. DockSafeBounds keeps this area within base safety rules. Northeast alternate return and southeast departure remain open; boat boarding/crane operation remain unavailable.
- Crane origin (3360.2,168.28,125), yaw -50; boat at (3575,0,-174), yaw 0 at low tide, follows the existing tide root. Mara (900,-1350,215), Budka (900,-1350,125), Board (1300,-850,125), yaw -17, and Verstak (400,-700,125), yaw 30, retain their interactions and placement. Existing board motion/light and Papug remain. Retired local prototype floors/huts/frame/fence remain retired.

## Gameplay/toolchain and next action

Walk 650 cm/s; Shift sprint 1040; Space jump 420, gravity 1.3, air control .2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Protected logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores supported dry ground without penalty.

UE 5.8.2 CL56702186; VS2026/MSVC14.50, SDK10.0.26100.0, .NET10; Blender 5.2.1 LTS. DX11, no Lumen/Nanite, two build workers, 16 GB RAM / GTX1660Ti Max-Q. EUR 0. Sol handled the bounded material authoring/import/tune; lead integrated and validated.

Next action: Director reviews the moving water from the start area, shoreline and working dock in the updated package, including colour, brightness, foam and visual richness. M1 art/gameplay acceptance remains pending. Do not start another pass or M2 automatically.
