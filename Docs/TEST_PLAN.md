# M0.5 test plan

Not executed: engine/project absent. Record build version, date and actual results when available; retain logs outside Git.

1. Compile editor/game; package Win64 and launch without editor or internet. Inspect compile/cook/runtime logs.
2. Movement/look/collision work; player cannot fall through map. Closing inventory/trader restores movement focus.
3. Complete tide cycle: exposed causeway opens, water/access agree, timer/warning accurate. Test stranded recovery.
4. Collect five item types; counts correct, repeated pickup cannot duplicate. Range/capacity rejection leaves world pickup intact.
5. Sale removes exact selected quantity and credits exact integer value. Repeated/invalid sales cannot duplicate money; evidence cannot be sold.
6. Two expeditions including failed return: no stuck UI, stale access, negative quantities, evidence loss or softlock. Restart begins valid session.
7. Measure packaged frame times at 1080p on NVIDIA GPU; provisional 30 fps target. Record settings, pacing and memory.
8. Supply executable path, controls and limitations for Director review. Separate subjective feedback from technical failures.

Implement focused automated checks for quantity/capacity boundaries, invalid sales and tide transitions once those systems exist. Avoid tests that merely mirror implementation.
