# LOW TIDE

Offline first-person island exploration game. M0.5 implements a primitive coastal
greybox, tide access, salvage collection, inventory and one trader. The approved
final direction is high-quality stylized 3D; these primitives test mechanics only.

## Development

Unreal Engine 5.8.2 (CL 56702186), Visual Studio Community 2026, MSVC 14.50,
Windows SDK 10.0.26100.0 and .NET Framework 4.8 SDK. Unreal uses its bundled .NET 10.

From the repository root in PowerShell:

```powershell
.\Scripts\Build.ps1 -Mode Editor
.\Scripts\Test.ps1
.\Scripts\Build.ps1 -Mode Package
```

The script defaults to `C:\Program Files\Epic Games\UE_5.8`; override with
`-EngineRoot` if needed. Packaged output goes to ignored `Artifacts/Windows`.
Open `LowTide.uproject` to work in the editor. Play starts the C++ authored greybox
on the engine Entry map. Launch the packaged game with `Artifacts/Windows/LowTide.exe`.

WASD moves, mouse looks, E interacts/closes trade, I opens/closes inventory,
1–5 sells one matching item at Mara, Esc quits. Hold Left Shift to sprint at
about 1.6× walking speed without stamina. The settlement, causeway and shelf
have a closed playable perimeter, with a warning before tide recovery. Director
manual retest remains. Follow the causeway during low tide, collect salvage, return to the
orange trader and sell. The provisional recovery rule retains evidence, credits
and permanent state while removing only ordinary unsold salvage from the current
expedition. The accelerated tide cycle repeats every two minutes. Sessions do not save yet.

See `Docs/CURRENT_STATE.md` for actual validation results and next steps.
