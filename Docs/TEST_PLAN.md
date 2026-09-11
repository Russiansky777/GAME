# M0.5 test plan

Toolchain gate passed 2026-09-11 at 6a8bb84. Gameplay editor build and Scripts/Test.ps1 now pass all three tests with no test warnings: capacity/quantity boundaries, atomic sales, and an isolated-world expedition round trip. The expedition exercises CharacterMovement across the causeway and return steps, actual pickups/trader, tide closure, evidence retention and a second salvage cycle. Test-world frames/local player follow engine requirements. Report: Artifacts/Tests/index.json (generated, outside Git).

1. Compile editor/game; package Win64 and launch without editor or internet. Inspect compile/cook/runtime logs.
2. Movement/look/collision work; player cannot fall through map. Closing inventory/trader restores movement focus.
3. Complete tide cycle: exposed causeway opens, water/access agree, timer/warning accurate. Test stranded recovery.
4. Collect five item types; counts correct, repeated pickup cannot duplicate. Range/capacity rejection leaves world pickup intact.
5. Sale removes exact selected quantity and credits exact integer value. Repeated/invalid sales cannot duplicate money; evidence cannot be sold.
6. Two expeditions including failed return: no stuck UI, stale access, negative quantities, evidence loss or softlock. Restart begins valid session.
7. Measure packaged frame times at 1080p on NVIDIA GPU; provisional 30 fps target. Record settings, pacing and memory.
8. Supply executable path, controls and limitations for Director review. Separate subjective feedback from technical failures.

The checks above remain the acceptance checklist, not a claim that every manual or performance scenario has been exercised. See CURRENT_STATE for packaged validation and outstanding coverage. Automated methods do not establish mouse/keyboard usability or final visual quality.
