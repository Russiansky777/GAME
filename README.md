# LOW TIDE

Offline first-person island exploration game. Current implementation is the minimal
Unreal C++ project used to verify the Windows build pipeline; gameplay is not yet implemented.

## Development

Unreal Engine 5.8.2 (CL 56702186), Visual Studio Community 2026, MSVC 14.50,
Windows SDK 10.0.26100.0 and .NET Framework 4.8 SDK. Unreal uses its bundled .NET 10.

From the repository root in PowerShell:

```powershell
.\Scripts\Build.ps1 -Mode Editor
.\Scripts\Build.ps1 -Mode Package
```

The script defaults to `C:\Program Files\Epic Games\UE_5.8`; override with
`-EngineRoot` if needed. Packaged output goes to ignored `Artifacts/Windows`.
Open `LowTide.uproject` to work in the editor. The initial Entry map is intentionally
empty and only checks the toolchain; it is not a playable milestone.

See `Docs/CURRENT_STATE.md` for actual validation results and next steps.
