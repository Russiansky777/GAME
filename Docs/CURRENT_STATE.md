# Current state — 2026-09-12

## Active milestone and visual verdict

M0.5 is Director-accepted. M1 remains a candidate. This hub/liveliness checkpoint improves the warm material treatment, foreground composition and animated detail, but **the requested hero-quality benchmark is not achieved or accepted**. The shop still uses the authored simplified structure; broad grey ground, secondary huts, distant rocks, foliage finish and Mara remain below the supplied reference. No M2 or whole-island art pass was started.

## Current hub

- Kenney Pirate Kit 2.1 CC0 supplies eight mesh families and 19 collision-free instances: crates, bottle crates, barrels, chest, boats, paddles, mast/ropes and a wreck facing the departure. Mesh bounds determine bottom-centre placement; the central route and interaction logic remain unchanged. This is Kenney, not Quaternius Pirate Kit. Existing Quaternius Stylized Nature MegaKit assets remain in use.
- A reusable project-authored 512 px wood texture adds directional grain; versioned wood/cloth/fibre materials preserve economical rendering. Timber is warmer, canvas has a top-anchored approximately 1 cm breeze displacement. No added dynamic lights or expensive rendering features. Corrected the native directional-light rotation offset that previously made the sunlight almost vertical.
- The existing workbench is now foreground staging, with sorted goods beside the jobs board. Runtime work/goods offsets are (+100,+400,0) and (-550,-750,0) cm. The mug and bucket were corrected to human scale by isolated export/import. The conforming deck and gameplay collision are preserved.
- A project-authored perched parrot has separate head, wing and tail movement, breathing, 30 Hz near animation and 2 Hz distance checks beyond 32 m. Its eight-part bird/perch/optional-lantern kit totals 12,248 triangles. The optional lantern is disabled in this composition. The bird is collision-free; existing surf/wind ambience remains. No animal AI, audio framework or gameplay mechanic was added.
- E at the JOBS board still accepts the Signal Station Logbook expedition and starts the tide. Mara remains the trader and awards 75 credits once for the protected logbook. Repeat board interaction reviews the current job without restarting it.

## Validation

- Final composition Editor build succeeded in 43.84 seconds. Required automation: 12/12 success, zero warnings/failures/not-run. Existing mission, trade, sprint/jump, containment, tide/alternate route, ordinary-salvage loss, protected evidence and second-expedition regressions pass. New checks cover donor mesh availability/noncollision and parrot assembly overlap plus animated head transform.
- Windows BuildCookRun succeeded in 231.05 seconds; cook processed 594 packages with zero errors/warnings. Executable: `Artifacts/Windows/LowTide.exe`.
- Editor reviews covered spawn, trader, jobs board, departure and parrot close-up. Those reviews caught and fixed doubled OBJ hinge offsets, oversized blurred wood grain, the sun orientation and oversized household props. Second/composition captures have no runtime/material errors. OBJ import emits harmless unsupported smoothing-keyword warnings; versioned material graphs avoid a UE 5.8 rooted-expression deletion assertion during regeneration.
- Seven packaged views (spawn, trader, jobs board, departure, route entry, shore and corrected parrot close-up) were inspected. All launches exited 0; the eight runtime logs including profiling have no errors, fatals, assertions or material/default-material failures. Evidence: `Artifacts/HubLivelinessReview/packaged`. These positioned-camera checks do not replace a manual playthrough.
- Launcher timestamp: 2026-09-12 01:12:10 UTC. Source/staged Items.json SHA-256 matches: `01AFF1FD67EBD6B851BFEFB4886A56168DEB60DDD947E3503745B2469F1274EF`.
- Same 1080p shore profile, 900 frames with 300 warm-up discarded: 600 retained frames average 20.470 ms / 48.85 FPS, p95 21.8256 ms, 658.5 mean draw calls. Previous checkpoint: 20.053 ms / 49.87 FPS, p95 21.4461 ms, 492.4 draws. Mean frame time increased 2.08%, draw calls 33.73%; retain this cost for subsequent optimization decisions. This is a short comparison, not whole-route or lower-end GPU certification. Summary: `Artifacts/HubLivelinessReview/performance-summary.json`.

## Preserved gameplay

Walk 650 cm/s; hold Shift sprint 1040; Space single jump 420, gravity 1.3, air control 0.2, step 45 cm, slope 45 degrees. No stamina. Low/rising tide 300/180 seconds; five-second wet escape grace cannot reset by jumping. Watcher 845 cm/s. Logbook reward 75 once; Singing Shard 180 may be kept, sold or grounded. Wet recovery loses only current-expedition ordinary salvage, preserving prior stock, evidence, money and permanent state. Invalid-ground recovery below Z=-1000 restores a supported dry position without penalty. Existing objective/tide/shard clarity and menu-focus behavior remain.

## Sources, limits and next action

Asset spend remains EUR 0. Raw sources, CC0 licences and acquisition evidence are in `Docs/ASSET_PROVENANCE.md` and `SourceAssets/HubDonors/README.md`. No restricted marketplace pack was acquired. The browser controller could not verify Chrome's current URL and ended computer use before Fab acquisition; this was not a public-Git restriction. Quaternius Pirate Kit was quota-blocked, Pirate Town Free was Cloudflare-blocked/cap-limited, and Creatus checkout was not completed.

Next: Director reviews this packaged checkpoint's hub, movement and animation. For the remaining structural quality gap, evaluate the **free FANTASTIC Village Pack first** when library acquisition is available; it has not been proven insufficient. The previously evaluated 3DT Pirate Bay Commercial License remains an optional USD 19.99 fallback, unpurchased and requiring separate approval. Do not represent this checkpoint as final art acceptance or start M2.

UE 5.8.2 CL56702186; VS2026/MSVC14.50, SDK10.0.26100.0, bundled .NET10. Conservative DX11 renderer, conventional shadows, no Lumen/Nanite; two build/shader workers on 16 GB RAM / GTX1660Ti Max-Q. Use `Scripts/Build.ps1`, `Scripts/Test.ps1` and the README regeneration order. Sol handled bounded implementation/validation, Terra acquisition, with lead integration and visual review; current worker delegation still does not expose Spark. No Fast mode.
