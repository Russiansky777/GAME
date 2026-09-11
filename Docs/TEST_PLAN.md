# M0.5 test plan

Toolchain gate passed 2026-09-11 at 6a8bb84. The latest gameplay editor build and Scripts/Test.ps1 pass four tests with no warnings or failures: capacity/quantity boundaries, atomic sales, expedition round trip and SafeEdgeReturn. The report was created at 15:00:22 (as recorded in automation report); total duration 0.834 s. The test guard requires all four named tests. The expedition covers actual pickups/trader, tide closure, evidence retention and a second salvage cycle; SafeEdgeReturn covers the revised recovery boundary. Report: Artifacts/Tests/index.json (generated, outside Git).

1. Compile editor/game; package Win64 and launch without editor or internet. Inspect compile/cook/runtime logs.
2. Movement/look/collision work; player cannot fall through or leave the settlement, causeway or shelf. Hold/release Left Shift and confirm sprint is about 1.6× walking speed with no stamina behaviour. Closing inventory/trader restores movement focus.
3. Complete tide cycle: exposed causeway opens, water/access agree, and a clear warning occurs before access closes and recovery. Sprint across each perimeter corner and through causeway/shelf transitions while the tide changes.
4. Collect five item types; counts correct, repeated pickup cannot duplicate. Range/capacity rejection leaves world pickup intact.
5. Sale removes exact selected quantity and credits exact integer value. Repeated/invalid sales cannot duplicate money; evidence cannot be sold.
6. Two expeditions including a failed return: recovery returns the player to shore, removes exactly the ordinary unsold salvage acquired in that expedition, and retains evidence, credits and permanent state. The second expedition replenishes salvage; no stuck UI, stale access, negative quantities, evidence loss or softlock. Restart begins valid session.
7. Measure packaged frame times at 1080p on NVIDIA GPU; provisional 30 fps target. Record settings, pacing and memory.
8. Supply executable path, controls and limitations for Director review. Separate subjective feedback from technical failures.

The requested tuning is implemented and covered by build/package plus automated checks, but the listed manual sprint, perimeter, warning and recovery scenarios remain Director acceptance retests. See CURRENT_STATE for packaged validation and outstanding coverage. Automated methods do not establish mouse/keyboard usability or final visual quality.
