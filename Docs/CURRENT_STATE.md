# Current state — 2026-09-12

## Active milestone and visual verdict

M0.5 is Director-accepted. M1 remains a candidate. This is a technically validated rich-hub/jobs-board checkpoint, **not closure of the Director's visual-quality request**. The base now has a distinct job station, more working clutter, vegetation and continuous wooden foreground, but the shop's flat material finish, sparse departure/backdrop and remaining primitive surroundings still fall below the supplied reference. Do not describe build/test success as art acceptance. No M2 or whole-world art pass is authorized.

## Current hub and mission flow

- Press E at the modeled JOBS board to accept the existing Signal Station Logbook expedition for 75 credits. This starts the existing expedition/tide flow. Repeat interaction reviews active/return/completed status without resetting the clock or paying again. Mara remains the trader and receives the protected logbook for its one-time reward; talking to her before acceptance directs the player to the board.
- Seven project-authored trader meshes now total 33,776 triangles. Six additional hub meshes total 32,098: deck, work/goods clusters, maritime/ground details and a separate local-origin hero board. Nine shared palette colors serve the added kit. The board includes modeled framing, canopy, papers, pins, route strings, tags and lantern; labels use managed TextRender. Initial view yaw is -30 degrees; spawn position is unchanged.
- Ten selected Quaternius Stylized Nature MegaKit Standard meshes supply bushes, grasses, ferns, plants, rocks and two trees. Eight original color textures feed explicit masked/opaque materials; tree foliage is tinted green. HISM placement/culling is scoped to the hub and immediate approach. Nearby decorative cliff instances are replaced while collision floors and boundaries remain unchanged.
- The wood deck follows existing elevated main/alternate route surfaces where they cross the settlement. This fixes the grey terrain wedges that previously cut through a flat apron. A clipped skin follows visual terrain +2 cm (collision +1 cm), with narrow seams; 635 sampled elevated points passed coverage/height checks with 0.0000 cm maximum error. Other five hub exports were unchanged by that correction.
- Existing sunlight/ambient lighting is tuned and an engine sky sphere supplies inexpensive drifting 2D clouds. Sky assets are CDO references for cooking. No Lumen, Nanite, volumetric clouds, new renderer baseline or additional gameplay system.
- Actual asset spend: EUR 0. Named free Fab candidates were evaluated; no Fab content was acquired. CC0 sources/licence, authoring scripts and manifests are retained; see ASSET_PROVENANCE. GenerateM1Assets.py must create the canonical terrain OBJ exports before GenerateHubSlice.py on a fresh checkout.

## Fresh validation

- Final Editor compile succeeded in 34.65 seconds. Final automation report at 2026-09-11 23:53:45 UTC: 11 succeeded, zero warnings/failures/not-run. Coverage includes board acceptance/range/review, Mara reward, inventory/trading, sprint/jump, containment, tide/alternate route, ordinary-salvage loss, protected evidence and second expedition.
- Real-RHI review found and fixed a leaf shader connection failure that NullRHI automation did not detect. Material graph connections now fail explicitly if wiring is rejected; a materials-only import option avoids needless mesh reimports. Final Editor and packaged views show the correct material rather than a grey fallback.
- Windows BuildCookRun completed in 190.00 seconds, exit 0; full cook summary zero errors/warnings. Launcher timestamp 2026-09-11 23:58:08 UTC. Executable: `Artifacts/Windows/LowTide.exe`. Source/staged Items.json SHA-256 matches: 01AFF1FD67EBD6B851BFEFB4886A56168DEB60DDD947E3503745B2469F1274EF.
- Six fresh packaged captures (spawn, trader close, board, exit, route entry and shore) were inspected, each launch exit 0. Evidence: `Artifacts/HubSliceReview/packaged`. The board prompt/label is readable, wood is no longer cut by underlying route terrain, and imported foliage/sky are present. All seven runtime logs including the profile have no errors, fatals, assertions, material compile failures or default-material fallbacks; only the existing r.MotionVectorSimulation warning remains. These positioned-camera checks are not a manual playthrough.
- Same stationary shore 1080p profile: 900 frames, first 300 discarded; remaining 600 mean 20.053 ms / 49.87 FPS, p95 21.4461 ms, mean draw calls 492.4. Previous a633669 sample: 19.386 ms / 51.58 FPS, p95 20.9509 ms, 290 draw calls. Frame time rose about 3.4%; draw calls rose materially. This is a short controlled comparison, not whole-route or lower-end GPU certification. Raw CSV/log and summary: `Artifacts/HubSliceReview`.

## Preserved gameplay

Walk 650 cm/s; hold Shift sprint 1040; Space single jump 420, gravity 1.3, air control 0.2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores a supported dry position without penalty. Existing objective/tide/shard clarity and menu-focus behavior remain.

## Limits and explicit next action

Director may retest the board/base loop in this build. Full 8–12 minute pacing, walking experience and artistic appeal still need manual evaluation. Mara, UI, secondary huts, broad terrain, much of the departure area and distant environment remain placeholders. The current shop surfaces and surrounding density are still below the requested commercial benchmark; the art pass remains open.

For the next material/prop quality step, the lead recommends evaluating **3DT Pirate Bay Modular Pack for Blender & Game Design — Commercial License, USD 19.99 listed on Superhive**, checked September 12; exact source and licence distinction are in ASSET_PROVENANCE. Its textured coastal modules/props could replace the current flat wood finish; catalog suitability is not a guarantee of final quality. Nothing was purchased. Obtain the Director's separate purchase approval before acquisition; do not silently call the current free/authored result finished, widen to M2, or put restricted marketplace raw assets in public Git.

## Environment and workflow

UE 5.8.2 CL56702186; VS Community 2026/MSVC 14.50; Windows SDK 10.0.26100.0; bundled .NET 10. Build/shader concurrency 2 on 16 GB RAM / GTX 1660 Ti Max-Q. Conservative DX11 renderer and conventional shadows. Use Scripts/Build.ps1 and Scripts/Test.ps1. EUR 0, no Fast mode. Sol handled bounded implementation/Blender/import/build work, Terra asset discovery, with lead integration and visual review. Current collaboration exposes these model choices but not Spark; retain its preference when actually available.

A lost shell/tool session was investigated and work safely resumed: no surviving hung shell or active Unreal/Blender/build was found, and no files were reset, reverted, cleaned or discarded. Completed compiles were reused. Generated build/review/profile outputs remain ignored; Git and these docs remain the source of truth.
