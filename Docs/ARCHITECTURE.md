# LOW TIDE architecture

## M1 integration boundaries

M1 extends the existing character, interaction, catalog, inventory, trader, tide and HUD. `ALowTideGameMode` owns the fixed expedition session and small Mara/logbook mission state; it does not introduce a general quest framework. `ATideController` remains the sole authority for phase, water height and shortcut access. One bounded phenomenon owns artifact pursuit and grounding-refuge counterplay. `ACoastalAudio` supplies repository-authored procedural ambience and cues; HUD displays state without owning it.

`ACoastalScene` builds the authored layout on the engine Entry map and exposes route and interaction anchors to GameMode. The visible coast is 2,908 triangles across three static terrain sections: sand 1,244, stone 920 and deep ground 744. Independent pitched visual ribbons/caps follow the established floor proxies with a 1 cm visual offset beneath them; width is scaled by 1.16 to remain behind route boundaries. The full-width settlement landing is flat at Z=125 cm before a 7.8% ramp joins the existing route, with a maximum sampled join of 0.5591 cm across five lanes at ±5 cm. Visual terrain has no gameplay collision. A hidden `RouteFloorCollision` instanced-cube family remains the walkable collision surface, so visual terrain changes cannot silently alter containment or traversal. `ACoastalDressing` is non-colliding, while selected authored landmarks and huts retain collision. Authored boundary geometry and the tide shortcut blocker contain the playable space.

The asset generator guards the terrain against source/layout drift: it parses and requires the 11 main, 8 alternate and 7 optional route points to match `CoastalScene.cpp`, rejects a mismatch, and checks imported terrain bounds against authored coordinates within 0.1 cm. OBJ export pre-reflects Y and reverses winding so imported terrain remains aligned with the gameplay layout. The terrain import evidence in `Artifacts/TraversalTerrainLipImport.log` records zero import errors and two deprecation warnings; source route arrays and the width constant guard are fixed inputs. Direct and union ray checks total 2,832 checks with zero unexplained delta after the intentional 1 cm visual offset, and imported bounds are valid.

Mission reward must be idempotent. Protected evidence cannot be lost or made unobtainable by an ordinary full pack. Wet lower ground shows a warning grace period and triggers recovery after five continuous seconds; recovery removes only ordinary salvage newly acquired during that expedition while retaining prior stock, evidence, credits and permanent mission state. The next low tide can begin a second trip. Shortcut closure must retain a physically walkable elevated blue-route escape, not merely a state flag.

Keep the M0.5 fixture and its regressions available while validating the default M1 layout independently. M1 automation exercises actual `CharacterMovement` walking, gravity, slopes and collision on the main, optional-risk and elevated escape routes. It does not replace a manual input, timing, listening, accessibility, performance or visual-quality review.

UE 5.8.2 CL56702186 Launcher binary release, one C++ game module, offline Win64. The conservative baseline remains DX11 with conventional shadows and without Lumen, Nanite, ray tracing or an ocean plugin. No world partition, backend or external framework is required.

| Boundary | Responsibility |
| --- | --- |
| Character/controller | First-person movement, input and focus trace |
| Interaction interface | Prompt and validated interaction attempt |
| Item catalog | Immutable definitions and stable IDs loaded from staged `Content/Data/Items.json` |
| Inventory component | Capacity/quantity validation, protected-item handling, transactions and change events |
| Trader | Validate sale; remove quantity and credit integer currency atomically |
| GameMode | Mission and phenomenon state; wet-exposure grace and stranded-player recovery |
| Tide controller | Single phase/time authority; drive water, phase warning and shortcut access |
| Coastal scene | Authored visual layout, route anchors, hidden route-floor collision and containment |
| HUD | Display state and request actions, never own gameplay state |

Implementation and asset details follow verified source. Package, performance and Director review evidence are tracked in CURRENT_STATE and TEST_PLAN; no M1 delivery claim follows from the Editor build or automation alone.
