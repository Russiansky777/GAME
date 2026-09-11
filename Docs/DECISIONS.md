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

Director authorizes all necessary game production, required free installations and GitHub pushes without repeated confirmation. Mandatory tool/OS controls and unavoidable authentication still apply. EUR 0 budget and no Fast mode remain in force.

## Director-approved stylized art direction (supersedes any realism assumption)

Adopt high-quality stylized semi-cartoon first-person 3D: clean silhouettes, attractive simplified geometry, moderate detail and painterly/clean materials. Warm coastal daylight; mystery/dark fantasy during anomalous low tide. Fell & Sell / Chop Chop Inc. are production-philosophy references only, not assets/designs to copy. Commercial Steam quality is the target; technical greybox is not the art benchmark.

Production cost and hardware savings are part of the decision: reusable master materials, small material families, importance-based texture/geometry budgets, restrained shaders/lights/foliage/overdraw/draw calls and economical water. Retain conservative renderer settings and lower-end GPU scalability; expensive features need demonstrated payoff. Photorealistic pipelines require explicit approval. This decision is final Director guidance, not provisional tuning.

## Director-approved task routing — 2026-09-11

Classify work before substantial execution and select the cheapest capable available worker: Astra for architecture, cross-system integration, difficult Unreal debugging, risky refactors and milestone audits; Sol for substantial features, moderate debugging and Blender tooling; Terra for routine implementation, tests, UI, data, docs and refactors; Luna for discovery, boilerplate, repetitive edits, simple logs and Git. Do not delegate tiny tasks when handoff/review costs more. Bounded tasks require explicit acceptance criteria. The lead reviews shared-system changes and retains architectural authority; the Director does not coordinate workers. Canonical docs and Git remain the source of truth. EUR 0 and no Fast mode remain in force.

The current collaboration tool accepts explicit model and reasoning-effort selection, verified by a successful `gpt-5.6-luna` worker invocation. Routine delegation must not silently inherit Astra; re-check capability only when the environment changes, and if selection is unavailable record the limitation and work economically without asking the Director to coordinate chats. See the [subagent configuration guidance](https://learn.chatgpt.com/docs/agent-configuration/subagents).
