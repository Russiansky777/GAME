# Decisions — 2026-09-11

- Adopt the Director's brief and EUR 0 pilot limits. Establish working tools before accumulating gameplay code.
- Select Unreal 5.6 binary release and VS 2022 17.14 C++ tooling as a stable baseline, not a claim of latest release. Pin available engine patch and compiler after installation/build verification.
- Required: Epic Launcher, UE Win64 components, MSVC, Windows SDK 22621 or newer compatible version, engine prerequisites. Prefer bundled .NET; install a separate SDK only if required.
- Defer Blender, sample packs, debug symbols and non-Windows target support. Plan roughly 100–140 GiB headroom for tools/cache/builds; this is an estimate. Check actual installer sizes first.
- Use conservative rendering and compact authored content for local hardware.
- GitHub remote read verified; no advertised refs. Commit locally; publication/push has not been performed.

Sources checked:
- [Epic UE 5.6 toolchain compatibility](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.6): VS 17.14 recommended, MSVC 14.38.33130, SDK 22621 or newer recommended.
- [Epic installation](https://www.unrealengine.com/download): Launcher and Epic sign-in are required for the binary installation path.
