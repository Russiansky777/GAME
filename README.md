# LOW TIDE

Offline first-person coastal exploration game. M0.5 is an accepted historical
greybox. M1 is the current authored stylized expedition candidate. Its fresh
Win64 traversal-correction package and 11 passing regressions are recorded in
Docs/CURRENT_STATE.md. Continued Director evaluation is the next step.

## Development

Unreal Engine 5.8.2 (CL 56702186), Visual Studio Community 2026, MSVC 14.50,
Windows SDK 10.0.26100.0 and Unreal's bundled .NET 10.

From the repository root in PowerShell:

```powershell
.\Scripts\Build.ps1 -Mode Editor
.\Scripts\Test.ps1
.\Scripts\Build.ps1 -Mode Package
```

The script defaults to `C:\Program Files\Epic Games\UE_5.8`; override with
`-EngineRoot` if needed. Packaged output goes to ignored `Artifacts/Windows`.
Open `LowTide.uproject` for Editor work. The default game launch builds the
M1 slice on the engine Entry map.

## M1 play

WASD moves, mouse looks, Space jumps, E interacts or closes trade, I opens/closes
the inventory, 1–8 sell a matching inventory slot at Mara, Esc quits, and Left
Shift sprints. Sessions do not save yet. Movement is tuned to 650 cm/s walking
and 1040 cm/s sprinting; jumping uses a single 420 cm/s launch with restrained
air control. Small terrain changes should be handled by automatic step-up, not
used as a reason to jump around broken geometry.

Speak to Mara to accept the expedition and start the tide. Follow the amber
low-tide route to the signal logbook, then return to Mara. The shrine branch is
an optional deeper-risk route for the rare artifact. As water rises, the low
shortcut closes: follow the blue posts along the elevated escape route home.
The phenomenon can force recovery; ordinary unsold expedition salvage is removed,
while evidence, credits and permanent state remain.

The automated M1 suite verifies direct mission interactions, normal
CharacterMovement over the main/optional/escape routes, tide closure, recovery,
and containment. It does not replace a manual full playthrough or validate the
8–12 minute target duration. `Artifacts/Windows/LowTide.exe` is the current M1
candidate package; see `Docs/CURRENT_STATE.md` for its exact evidence and limits.
