# M0.5 architecture

UE 5.8.2 CL56702186 Launcher binary release, one C++ game module, offline Win64. GameMode builds a fixed authored primitive layout on the engine Entry map; no world partition or external framework.

| Boundary | Responsibility |
| --- | --- |
| Character/controller | First-person movement, input and focus trace |
| Interaction interface | Prompt and validated interaction attempt |
| Item catalog | Immutable definitions and stable IDs loaded from staged Content/Data/Items.json |
| Inventory component | Capacity/quantity validation, add/remove and change events |
| Pickup actor | Item ID; disappear only after successful transfer |
| Trader | Validate sale; remove quantity and credit integer currency atomically |
| Tide controller | Single phase/time authority; drive water height and access together |
| HUD | Display state and request actions, never own gameplay state |

Expose acquisition, sale and tide events for later objectives; no general quest framework yet. Separate item definitions from actor instances for later saves/quests. Persistence is not an M0.5 requirement.

16 GB RAM / 6 GB VRAM baseline: start DX11, conventional shadows, no Lumen/Nanite/ray tracing/ocean plugin. Provisional 1080p/30 fps target, subject to measurement. Limit concurrent compilation/shader work. Use binary engine, not source build. Blender is unnecessary for primitive greyboxing.

Minimal build/package/launch gate passed at 6a8bb84. Extend these gameplay systems incrementally; current validation is recorded in CURRENT_STATE and TEST_PLAN.
