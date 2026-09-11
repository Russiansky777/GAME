# M0.5 architecture — planned, not implemented

UE 5.8.2 Launcher binary release, one C++ game module, offline Win64. Pin patch after installation. Small authored map, primitive geometry; no world partition or external framework.

| Boundary | Responsibility |
| --- | --- |
| Character/controller | First-person movement, input and focus trace |
| Interaction interface | Prompt and validated interaction attempt |
| Item catalog | Data-driven immutable definitions, stable IDs, CSV/DataTable import |
| Inventory component | Capacity/quantity validation, add/remove and change events |
| Pickup actor | Item ID; disappear only after successful transfer |
| Trader | Validate sale; remove quantity and credit integer currency atomically |
| Tide controller | Single phase/time authority; drive water height and access together |
| HUD | Display state and request actions, never own gameplay state |

Expose acquisition, sale and tide events for later objectives; no general quest framework yet. Separate item definitions from actor instances for later saves/quests. Persistence is not an M0.5 requirement.

16 GB RAM / 6 GB VRAM baseline: start DX11, conventional shadows, no Lumen/Nanite/ray tracing/ocean plugin. Provisional 1080p/30 fps target, subject to measurement. Limit concurrent compilation/shader work. Use binary engine, not source build. Blender is unnecessary for primitive greyboxing.

First technical gate: minimal project compiles, packages and launches on Win64 before gameplay implementation. Extend the same systems incrementally.
