# Meshy deck, dock and crane import

`Scripts/InspectMeshyHub.py --modules-only` inspects the thirteen Director-provided GLBs, records SHA-256 metadata in `Artifacts/MeshyHubReview/<Label>/report.json`, renders a source preview, and writes a normalized GLB without modifying the source. `Scripts/ImportMeshyDeckDock.py` batch-imports those normalized files through UE 5.8 Interchange into `/Game/Generated/MeshyModules`.

Normalization is uniform: deck modules use a 200 cm maximum plan dimension, dock modules 400 cm, and the crane 300 cm overall height. Every pivot is plan-centred at ground Z=0. `X,-Y,Z` describes the Blender-space to UE-space reflection used while measuring previews; native glTF axes map into UE as `X,Z,Y`. Source primitives all provide `POSITION`, `NORMAL`, and `TEXCOORD_0`; each asset is one merged static mesh with one material and no skin or animation.

| Label | Source SHA-256 | Verts / tris | UE size cm | Walk/top Z cm |
|---|---|---:|---:|---:|
| DeckCorner | `57FEA4B12D074AA7630B41FAA04DCF7D624AD69314C6D3C0ACF9F50213F07940` | 12,758 / 9,544 | 200.00 x 198.80 x 27.18 | 26.91 |
| DeckLong | `9988D90971ADC7E7580C1B0F1E592872F1E0D856C7E238A0C1EE16E619AAAB6C` | 8,790 / 5,613 | 200.00 x 83.44 x 11.47 | 11.13 |
| DeckOuterEdge | `8C3B981D0B528D0AE143AB5961D4AC4E425B1C9D13722CE38785061850492CEE` | 11,349 / 7,529 | 200.00 x 170.23 x 25.45 | 24.10 |
| DeckRepaired | `BECA2AA2E6133BA849F7D0ABA0638A9CDA6CDCDA4E7D9A6A3FA9CCD9C8E66D6D` | 12,211 / 8,592 | 200.00 x 187.48 x 29.12 | 27.95 |
| DeckStandard | `9AFAF2A24B384ABB38299A0FD8C68BE6A13DD8AE751AAE42854BBDF6A7522004` | 9,677 / 5,950 | 200.00 x 181.37 x 25.31 | 24.28 |
| DeckStepRamp | `6895D22C9E5916CF628DE782892D0EC237189739ADC9CCFEC6AC1390F45A6EC8` | 13,743 / 9,149 | 200.00 x 196.29 x 37.64 | 37.02 upper; 21.78 lower |
| DeckTransition | `77F107992DEF96B92B848F1C0B84D00DC291480BE81EB499A168C4E988E4A12B` | 12,812 / 8,628 | 186.01 x 200.00 x 25.87 | 25.18 |
| DockCornerPlatform | `3E9501DE8A0CE19E6C91536B71D737E4BCA4C12FF93D37C28EF33185B703AE7B` | 1,045,120 / 1,973,774 | 357.92 x 400.00 x 80.16 | 62.36 |
| DockEndBerth | `3DE8066ED470598E713F4AC29DC5303ED50B857BBF68F8EDBDB05340E24FDBCC` | 1,028,477 / 1,966,166 | 400.00 x 302.27 x 155.91 | 127.88 |
| DockLadderAccess | `A778FD238D512151C7158D3C1ACD9EAE778B4E3F4C8AA17B3FB5495071E2A9AC` | 1,024,644 / 1,952,582 | 400.00 x 188.02 x 144.47 | 99.63 |
| DockRepaired | `6484FE903A06846F92E4B998FBD938FD5B753949F2C625B806FF7AB24EE418D2` | 1,227,962 / 1,953,318 | 400.00 x 151.47 x 89.63 | 66.96 |
| DockStraight | `EC606C2E3BE3A905A41B363DD261312913D6C7A651F47A26D175FF12C147D166` | 1,024,826 / 1,944,998 | 400.00 x 154.16 x 88.88 | 67.23 |
| Crane | `F3BCEA6876A5F39BAF45079B597862CBBF7E96EC1FE1578BAA54E85B2A6B1645` | 48,231 / 30,890 | 226.05 x 130.27 x 300.00 | base at 0 |

Deck and crane sources contain 4096 px base-colour and packed metallic-roughness images. Dock sources contain 2048 px base-colour, packed metallic-roughness, and normal images. Import keeps base colour sRGB, treats packed maps as linear Masks (`G=roughness`, `B=metallic`), and treats normals as Normal sampler data. No source declares glTF occlusion, so the packed red channel is not connected as ambient occlusion. Every versioned master enables `used_with_instanced_static_meshes` for runtime HISM use. Simple collision generation is disabled because runtime uses authored proxy collision.

`DeckStandard` has a dominant usable walking span of about 196.3–196.8 cm in X by 164.3–175.1 cm in Y at Z 23.75–24.39 cm; its repeat pitch is the full 200.00 x 181.37 cm frame. `DeckLong` has a 197.79 x 81.42 cm walking top at Z 11.13 cm and a 200.00 x 83.44 cm frame pitch. Use these surface spans for seams and the full frame dimensions for repeated instance spacing.

The source folder formerly named `SourceAssets/Generated/Gauge` was renamed to `SourceAssets/Generated/Crane`; the original GLB bytes and filename were preserved. The Blender-native comparison sheet is `Artifacts/MeshyHubReview/module-contact-sheet.png`.

Prepare the derivative with `Scripts/PrepareMeshyDockPiling.py --no-render`, then run `Scripts/ImportMeshyDockPiling.py -MeshyPilingReimport`. The importer hash-locks the GLB against its report and validates the declared target dimensions before replacing the existing asset. The final accepted GLB is SHA-256 `4D3926A4739061E5253877E7AB2E57118AE044A722FC32DC4E225BAD53894694`, 1,834 vertices / 3,548 triangles. It imports at `/Game/Generated/MeshyModules/Dock/DockPiling/SM_Meshy_DockPiling_import/StaticMeshes/SM_Meshy_DockPiling_import`; verified UE bounds are min `(-15.000009,-13.506618,0.000025)` and max `(15.000009,13.506607,400.000000)` cm. It reuses `M_Meshy_DockStraight_V1`; redundant derivative texture and material imports are deleted after binding.

The piling is a project-authored derivative of the Director-provided DockStraight GLB: only the lower wooden shaft of the southeast corner post was retained, excluding metal brackets/fins. Source UVs and material assignment were preserved; the shaft was resized to 400 cm tall and approximately 30 cm wide. The untouched DockStraight source remains the provenance root.

The crane's ground pivot is the full bounding-box centre rather than its base centre. In UE coordinates its low base footprint (Z 0–15 cm) spans X `-113.03..-1.16`, Y `-65.14..28.37`, with sampled vertex centroid about `(-56.57,-18.07)`. The upper boom extends primarily toward UE +X and slightly toward -Y.

The thirteen primary meshes use the stable CDO pattern `/Game/Generated/MeshyModules/<Deck|Dock|Crane>/<Label>/SM_Meshy_<Label>_import/StaticMeshes/SM_Meshy_<Label>_import.SM_Meshy_<Label>_import`; the exact labels are the table entries. Their shared versioned material paths are `/Game/Generated/MeshyModules/Materials/M_Meshy_<Label>_V1`. The UE commandlet completed all thirteen imports with exit code 0 and verified their actual bounds, ground pivots, PBR sampler contract, and disabled simple collision. Runtime placement and packaged-play acceptance are validated separately by the lead.

## Runtime composition

The main apron is a 10 by 8 grid centred at (650,-550), yaw -30, with selective repaired/corner/outer-edge pieces, 19 long trim pieces, and a transition connector. The northeast return slab has a 3 by 7 cover at its original elevation. Repeated pieces share HISM meshes/materials. Each deck tile fits the existing hidden route floor using 49 samples, a maximum 5-degree slope and 3 cm clearance; measured maximum local lift is 18.4 cm, with zero missing floor samples. Actual edge-plane seam tests retain a 20 cm bound. CharacterMovement uses unchanged smooth floor proxies, avoiding detailed plank/nail collision.

The dock run uses Straight (2450,300), Repaired (2830,300), Corner (3200,300) and End Berth (3575,300), all yaw 0 with walking tops at Z125. Eight source-derived wooden piles support the corner and outer berth. Crane origin (3360.2,168.28,125), yaw -50, compensates for its offset base and directs the boom toward the boat. Lodka is parallel to the southern berth at (3575,0,-174), yaw 0 at low tide, with approximately 59 cm side clearance. It follows the existing tide actor transform at constant draft without physics or an extra tick.

The outer berth/crane remain decorative beyond the preserved east containment wall; only the inner approach is playable. The northeast alternate return and southeast expedition exit remain clear. The boat is visible from the existing departure route and close review camera; the low-tide shoreline hides it from deep inside the hub. This is a documented composition limitation, not new boat access.

Retired local visuals: the settlement slab and northeast slab (retained as hidden collision), old HubSlice apron and maritime frame, two crude nearby huts, and the northern rope fence. Plants intersecting the new deck were moved outside it. Broad shore/expedition greybox and remote geometry remain outside this pass. Step/ramp and ladder are imported but unused; the existing grades and small unloading composition do not benefit from adding them. Crane/boat secondary animation is deferred because the sources are merged; the existing board motion remains.

## Packaged validation and measured cost

Final Editor build and Windows BuildCookRun passed; the latter took 231.70 seconds. All 12 required gameplay tests passed after final placement (one unrelated Unreal HTTP-check timeout warning). Nine actual packaged screenshots and logs under `Artifacts/DeckDockReview/final` cover spawn, hub overview, trader, board, bench, departure, route exit, dock and crane. All launches exited 0 with no runtime/material failures. Repetition, visible fitted-deck seams and broad surrounding greybox remain; Director visual acceptance is pending.

Same 1080p spawn benchmark (900 frames, first 300 discarded): 22.452 to 24.688 ms mean, 44.54 to 40.51 FPS, 24.714 to 25.728 ms p95, 358.7 to 363.7 mean draw calls. Thus approximately +5 draws and -9.05% FPS in this local sample; assets were not decimated or downscaled. Final evidence: `Artifacts/DeckDockReview/after-final/performance-summary.json`. Two earlier profiler runs returned 777003 after CSV save/engine teardown and are excluded; the final helper uses Unreal `csv.ForceExit 1` after CSV finalization and exits 0. Ordinary runtime captures need no workaround. This diagnostic-only shutdown issue remains documented, not fixed in the game.
