# Asset provenance

## M1 procedural audio

The coastal ambience and all M1 cue sounds are generated at runtime by `ACoastalAudio` from repository-authored PCM synthesis code. Surf uses filtered noise with slow wave sets; wind uses restrained filtered gust noise. Interaction feedback uses soft wood/bell-like partials. Tide state and route-loss cues use short tonal swells, and the anomalous entity uses a low, detuned harmonic pulse.

No recorded, downloaded, marketplace, generative-service, or third-party audio assets are included. The code and its generated PCM are authored for LOW TIDE and may be used within this project under the repository's existing project terms. There are no attribution, licence, runtime network, editor-only, paid-service, or redistribution dependencies introduced by this audio work.

Runtime guard: each ambience wave is mono 22.05 kHz / 16-bit, generated in 7-second chunks (308,700 bytes). `GetAvailableAudioByteCount()` prevents more than two chunks (617,400 bytes) from being queued per wave, including when playback is paused. In `-nosound` automation or worlds without an audio device, the actor does not create waves, queue PCM, or start its refill timer.

## M1 stylized materials and mesh

`Scripts/GenerateM1Assets.py` authors the opaque and water materials and imports project-authored meshes into `/Game/Generated/M1`. The source geometry is written in-project to ignored `Intermediate/GeneratedM1`; it is not downloaded or derived from external assets.

- `SM_LT_FacetedRock` is the same low-poly faceted basalt mesh, authored from `SM_LT_FacetedRock.obj`.
- `SM_LT_CoastalTerrainSand`, `SM_LT_CoastalTerrainStone`, and `SM_LT_CoastalTerrainDeep` are three authored material sections totaling 2,908 triangles. Independent pitched route ribbons and junction caps follow the hidden floor proxies with a 1 cm visual offset beneath them. Edge skirts and submerged shoulders cover their ends; a flat settlement landing at Z=125 cm and a gentle ramp connect the original floor to the main route. They are visual-only; imported coordinates and 2,832 triangle-ray probes validate the generated surface.
  Terrain OBJ export pre-reflects Y and reverses winding to compensate for Unreal's right-handed OBJ to left-handed world conversion; imported bounds therefore match the authored gameplay coordinates.
  Before writing assets, the generator parses all three route initializers from `CoastalScene.cpp`, resolves their symbolic main-route endpoints, and requires every terrain point and count to match: 11 main, 8 alternate and 7 optional points. A mismatch is a negative guard failure. Imported terrain bounds are also checked against the authored coordinates within 0.1 cm.

Materials are authored only from Unreal built-in material expressions:

- `M_LT_StylizedOpaque`: base color includes broad world-position sine variation using `WorldColorScale` and `WorldColorVariation` parameters (approximately 4.5% default peak variation).
- `M_LT_StylizedWater`: base color uses a slow animated two-direction world-band term from world XY + time, with `WaveScale`, `WaveSpeed`, and `WaveColorAmount` controlling broad approximately 10 m bands and restrained color amplitude.

No textures, runtime plugins, paid tools, remote services, or third-party licenses are introduced. The PythonScriptPlugin remains editor-only tooling and is not a packaged-game dependency.

## M1 focal landmark kit — 2026-09-11

The trading-stall finish, tide-road entry posts, signal-station finish/semaphore and asymmetric shrine frame are authored in `ACoastalDressing`. They reuse Unreal engine primitives, the existing project-authored faceted rock and opaque master material with a small color family. No assets were purchased, downloaded or sourced from a marketplace; no new textures, plugins or external licence obligations are introduced. Their transforms and construction helpers remain editable project source. This pass does not change terrain assets or collision proxies.
## Focused authored trader-shop replacement — 2026-09-11

The prior primitive trader-stall finish is superseded by seven project-authored meshes from `Scripts/GenerateTraderHub.py`, exported as OBJ/MTL with `SourceAssets/TraderHub/manifest.json`. Blender 5.2.1 generates beveled timber framing/curved roof, striped sagging canvas, an open counter, slatted storage, ropes/net/anchor, workbench/map/lantern and a raised-letter shop sign. The 25,144-triangle kit uses eight shared palette material instances of the existing opaque master. No third-party environment kit, texture, paid asset or downloaded model is used. Source generator and exports are retained; the intermediate `.blend` and review renders are generated outputs.

SALVAGE lettering uses Blender's bundled Inter font (SIL Open Font License 1.1), converted to mesh; no font binary is distributed. Copyright and licence text are preserved in `Docs/INTER_FONT_LICENSE.md`; font source is https://rsms.me/inter/ . All other new geometry is authored for this project.

`Scripts/ImportTraderHub.py` imports only `/Game/Generated/TraderHub`, validates manifest material identities and local bounds, and assigns palette material instances. OBJ Y pre-reflection plus reversed winding preserves the authored Unreal coordinate contract. The runtime anchor is (900,-1350,125) cm at yaw -38 degrees; three manifest-matched rear/side wall proxies belong to CoastalScene, leaving Mara and the front approach separate. This source is suitable for the existing repository without restricted marketplace raw files. Actual asset spend for the pass is EUR 0.

## Rich hub micro-slice — 2026-09-12

Selected external source: [Quaternius Stylized Nature MegaKit](https://quaternius.itch.io/stylized-nature-megakit), free Standard edition, downloaded from the author's official itch.io page. Its included licence explicitly declares CC0 1.0; retained verbatim at `SourceAssets/QuaterniusNature/LICENSE-CC0.txt`. Ten selected meshes and nine original texture dependencies are retained with a source/dependency table in that folder's README. Raw FBX, glTF buffers and textures use scoped Git LFS; the complete archive remains ignored. No paid Pro/Source edition or marketplace content was acquired. The earlier Ultimate Stylized Nature download was quota-blocked and is not used.

The requested free candidates were evaluated: [Free Stylized Market Stands](https://www.fab.com/listings/529a77f7-f370-42fa-9adc-87bc216489a9), [FANTASTIC – Village Pack](https://www.fab.com/listings/52529a12-e88e-41a0-8834-b87306f20c24), and [Stylized Environment Pack](https://www.fab.com/listings/ba992476-1ad4-4138-b2b0-abc92326fa6e). They were not imported: the selected CC0 nature subset addresses the largest environmental gap while preserving public source reproducibility without Fab standalone-redistribution restrictions. No claim is made that free Fab assets are open-source.

New shop-apron, working/storage clusters, maritime dressing and three-dimensional jobs board are authored in `Scripts/GenerateHubSlice.py`; exports and placement/material manifest live in `SourceAssets/HubSlice`. The Director's illustration is composition/quality reference only, not an imported game asset or permission to reproduce its interface/mechanics. No purchase has been made for this selected asset set.

The final rich-hub revision totals 33,776 triangles across the seven trader meshes and 32,098 across six additional hub/board meshes. It adds authored wood grain, closed ribbed shells, working clutter, a flush plank apron and a modeled canopy/frame/paper/lantern board. Eight imported color textures supply the selected nature materials; the retained bark normal is not used by the conservative runtime materials. The previous 25,144 count above describes the superseded shop revision.

The sky uses Unreal Engine's built-in SM_SkySphere and M_Sky_Panning_Clouds2_Inst with existing engine textures, referenced through the scene CDO for cooking; these are engine content, not a separate purchased pack. Twisted-tree foliage is recolored at runtime-material import using source luminance and a green tint, preserving the original opacity mask and unmodified CC0 source texture.

Potential next quality step, not acquired or approved: [3DT Pirate Bay Modular Pack for Blender & Game Design](https://superhivemarket.com/products/3dt-pirate-bay-modular-pack-for-blender--game-design), by 3D Tudor. Store page checked September 12 lists Commercial License at USD 19.99, with unlimited commercial projects/sales; the cheaper USD 9.99 Standard license has sales/views limits and is not the recommended option. The catalog describes 53 UV-unwrapped coastal pieces, ropes/nets/lanterns, 2K texture sets, FBX/GLB and a UE5 scene. It is a candidate to replace the current flat wood/prop finish with a coherent textured kit, not a guarantee of reference-level results. No checkout, purchase, import or redistribution has occurred. Any future purchase needs the Director's exact cost approval; marketplace source assets would stay outside public Git.

The apron generator reads canonical terrain OBJ exports from Intermediate/GeneratedM1 (run GenerateM1Assets.py first on a fresh checkout). A clipped wood skin follows the existing elevated route surfaces inside the deck footprint at visual terrain +2 cm, equivalent to collision +1 cm; it leaves collision and route topology unchanged. A 635-point grid check found no elevated coverage gaps and a maximum surface error of 0.0000 cm. The other five hub exports were unchanged by this correction.

## Hub liveliness and donor integration — 2026-09-12

Kenney Pirate Kit 2.1 (CC0) supplies eight selected mesh families: barrel, crate, bottle crate, chest, rowboat, paddle, mast/ropes and wreck. `SourceAssets/HubDonors/README.md` records the original archive URL, SHA-256, licence and retained files. This is Kenney's kit, not Quaternius' Pirate Kit. Its shared atlas is a color palette; it is not a textured structural-building solution. Imported meshes use one shared material and collision-free instance clusters, customized in scale, orientation and placement for the LOW TIDE hub. The existing Quaternius nature subset remains in use.

The requested structural alternatives were evaluated but not acquired. Quaternius Pirate Kit downloads were quota-blocked; Pirate Town Free was Cloudflare-blocked and its ArtStation licence has commercial caps. FANTASTIC Village Pack is free, but the Windows browser controller could not verify the current URL and ended computer use before acquisition. Creatus' authorized $0 checkout was not completed. None is represented as an imported donor. Restricted marketplace raw assets may be kept locally in ignored `Content/LocalLicensed` and `Intermediate/ExternalAssets`; public Git restrictions alone are no longer a reason to reject a suitable donor.

The perched parrot, perch and optional lantern are project-authored in `Scripts/GenerateHubLiveliness.py`, with separate component pivots for the simple idle animation. No external animal mesh, animal AI framework or purchased content is used. Hub wood detail is a project-authored reusable texture and material treatment; source and upgrade scripts are retained. Asset spend remains EUR 0. The earlier paid structural recommendation remains unpurchased and requires separate Director approval.

Next structural evaluation should still prioritize the free FANTASTIC Village Pack when the browser/library acquisition is available. Its absence from this build is an access limitation, not evidence that the pack is insufficient. The 3DT Pirate Bay Commercial License remains a USD 19.99 paid fallback (listing rechecked this pass), not a necessary purchase or an approved dependency.

Final composition reuses the authored work and goods groups with runtime offsets of (+100,+400,0) and (-550,-750,0) cm respectively in `CoastalScene.BuildHubSlice`; source OBJ coordinates remain unchanged. The mug and bucket were reduced to 12×14 cm and 36×40 cm respectively through a work-cluster-only export/import. The conforming deck, job-board mesh and gameplay collision were not regenerated by that correction.

## Director-generated Meshy hero mission board — 2026-09-12

The Director generated and supplied `SourceAssets/Generated/JobBoard/SM_JobBoard_Hero_v01.glb` externally via Meshy and explicitly selected it to replace the mission-board visual. Original SHA-256: `20016CBAF14EBE6276AAE9A20A67A01AA8C3979BF0AAFEEB1BDAAA827603082B` (31,617,420 bytes). This is a Director-provided generated asset, not CC0, a purchased marketplace donor, or project-authored modeling. No Meshy account/plan or licence document was supplied; this entry records provenance without asserting additional licence terms. No generation API or paid service was used by the agent. The separate older Meshy GLB in this directory is not the selected source and remains untouched.

Import inspection, derivation and material handling are recorded in `Docs/HERO_JOB_BOARD_IMPORT.md`; the original GLB is preserved without modification. Runtime use is limited to the existing mission-board actor and retains its expedition/mission interaction.

## Director-generated Meshy Budka and Papug — 2026-09-12

The Director generated both assets externally using Meshy.ai and supplied them as LOW TIDE project hero assets. Preserve these originals verbatim through scoped Git LFS:

| Source | Format / bytes | Original SHA-256 |
| --- | --- | --- |
| `SourceAssets/Generated/Budka/Budka.glb` | binary glTF 2.0 / 120,760,744 | `1113EB9FB407EDAA6D42B830F2A80B1E70C7081C7A60CC2B0C0F0AD4DD280AE0` |
| `SourceAssets/Generated/Papug/Papug.glb` | binary glTF 2.0 / 94,359,692 | `DB7F65A62CC3065E26DA49D67A6C0AE47D8B5C0391EF6E82B6A6F7D20AEEF1B6` |

These are Director-provided generated assets, not CC0 assets or agent-authored models. No Meshy account/plan or licence document was supplied; no additional licence terms are asserted. No paid generation/API or asset purchase was performed by the agent. Geometry, textures, derivation and Unreal import details are recorded in `MESHY_HUB_IMPORT.md`. They supersede older prototype hut/parrot visuals as the approved hero-quality reference while retaining the existing mission/trader gameplay.
