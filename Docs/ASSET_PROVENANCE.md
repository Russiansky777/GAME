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