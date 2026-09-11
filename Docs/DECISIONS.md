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

## 2026-09-11 — engine reassessment (supersedes initial 5.6 decision)

Select UE 5.8.2. Hardware profile and engine version are separate decisions.

| Concern | UE 5.6 versus UE 5.8.2 for LOW TIDE |
| --- | --- |
| Existing compatibility | Empty project; no asset/plugin dependency favors 5.6 |
| Hardware | Both require restrained content/rendering on 16 GB RAM / 6 GB VRAM; no measured local advantage for 5.6 |
| Stability | 5.8.2 is a released hotfix, including editor/build/cook fixes; no identified blocker relevant to our minimal feature set; not a guarantee of stability |
| Packaging | Both support Windows C++; verify minimal packaging on 5.8.2 before gameplay |
| Project lifetime | Starting at 5.8.2 avoids an immediate baseline migration; freeze until a demonstrated need to upgrade |

Retain conservative DX11-first rendering, conventional shadows and no Lumen/Nanite/ray tracing. Benchmark locally; do not install both engines for a speculative comparison.

Use VS Community 2026 with targeted MSVC 14.50 x64/x86 and Windows SDK 26100 components. Avoid blanket optional/recommended workloads, mobile/console SDKs and samples. Community is the free individual-developer edition; no paid license selected. Engine-bundled .NET where available.

Sources: [5.8.2 hotfix](https://forums.unrealengine.com/t/5-8-2-hotfix-released/2746335), [5.8 toolchain](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine), [hardware](https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-for-unreal-engine?lang=en-US), [free VS Community](https://visualstudio.microsoft.com/downloads/).

Director authorized required free installations and GitHub pushes; retain manual approvals and pause at unavoidable authentication/UAC/license actions. No Fast mode or approval-setting changes.
