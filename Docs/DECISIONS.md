# Decisions — 2026-09-11

## Director acceptance and M1 authorization

M0.5 is Director-accepted after manual retest. Sprint, perimeter containment and core interactions pass. Tide-warning clarity is a non-blocking greybox limitation carried into M1, not grounds for further M0.5 polish.

M1 extends the same production architecture into ONE authored 8–12 minute expedition. Prove discovery, meaningful loot, voluntary risk, living tide and mystery together. A comfortable main story objective precedes an optional valuable deeper branch; rising water closes a convenient route while leaving an alternate escape. Add one understandable, avoidable non-combat entity/phenomenon linked to greed. The existing recovery/expedition-salvage penalty is only a final provisional fallback.

Concentrate stylized art and minimal coastal/interaction/tide/entity audio on this slice. No paid assets, realism pipeline, combat, swimming, survival meters, large catalogs, world generation, backend, broad island production or speculative save architecture. Record asset provenance. Compile, test, package, launch, inspect logs, measure the conservative renderer on the development GPU, and commit/push before Director review.

Reversible working story: Mara requests a drowned signal-station logbook whose entries suggest a reply arrived before its signal was sent. A rare singing object draws a phenomenon that reacts to its carrier's movement; stopping and marked grounding refuges offer counterplay. These names, lore and tuning are implementation hypotheses, not Director-approved major canon.

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

Model-routing update (2026-09-11): A successful explicitly selected GPT-5.3-Codex-Spark worker invocation verified availability in this environment. The Director reports a separate preview allowance; account billing was not independently inspected. For low to moderate bounded tasks (bounded code edits, input/UI/data/JSON work, tuning, and small bugfixes), use `gpt-5.3-codex-spark` unless acceptance criteria require a heavier model. Keep Astra for architecture and high-risk integration/debugging as previously defined. This is a task-level preference with no blanket model switch and no budget or Fast-mode policy change.

Sources used for routing policy:
- [Model speed configuration](https://learn.chatgpt.com/docs/agent-configuration/speed)
- [Subagent configuration](https://learn.chatgpt.com/docs/agent-configuration/subagents)

## 2026-09-11 — consolidated traversal/usability pass

Use the Director-approved traversal contract for the current M1 pass: 650 cm/s walking, 1040 cm/s sprinting, Space for one 420 cm/s jump, 1.3 gravity, 0.2 air control, 45 cm step height and 45° walkable slope. These numbers remain reversible playtest tuning. The speed increase preserves the existing 1.6× walk/sprint ratio. Jumping supports ordinary traversal only and must not compensate for broken geometry or bypass tide blockers.

Scene continuity is defined by visible ground and collision agreement across the complete main route, optional-risk branch, elevated return and every M0.5/M1 transition. The terrain uses independent pitched visual ribbons/caps over route-floor collision, a 1 cm visual offset beneath the proxies, and a 1.16 width scale behind boundaries. Keep tide timing at 300/180 seconds unless testing demonstrates that the speed increase obviously breaks the intended experience. Automated evidence and remaining review status belong in CURRENT_STATE; visual quality review remains pending.

The invalid-fall fallback threshold is -1000 cm. GameMode restores the last supported dry spot, or the expedition start if submerged, while retaining inventory, mission and expedition snapshot state. Normal tide-failure salvage penalties are unchanged. Watcher movement remains 845 cm/s, preserving its existing 1.3× relationship to the 650 cm/s player walk speed.

## Director-approved bounded M1 landmark pass — 2026-09-11

Concentrate first-impression work on the safe trading outpost, opening tide-road entrance, signal-station destination and optional Singing Shard site. Use reusable authored stylized geometry and existing economical materials. Preserve gameplay, validated routes, tide, movement and interaction foundations. This authorizes neither whole-world art/final polish nor M2; Mara may remain a placeholder. Director visual/gameplay evaluation remains the next acceptance gate.
## Director rejection and focused hub quality replacement — 2026-09-11

The prior landmark pass does not meet the Director's visual-quality bar: the trader hub still reads as prototype geometry. Replace the stall and immediate surroundings with a coherent authored/asset-based stylized coastal shop, rather than further primitive dressing. Prioritize the shop, grounded supports, trade frontage, immediate surroundings and route departure; preserve M1 gameplay and defer whole-world art/M2. Commercially usable external assets are allowed, including paid assets only after an exact asset/cost approval. Keep source/license records and restricted raw assets out of a public repository. The current chosen approach is project-authored Blender geometry, requiring no purchase. Neither this authorization nor technical tests imply Director art acceptance.

## Director-approved rich coastal hub micro-slice — 2026-09-12

The authored stall alone still falls short. Use the Director's supplied coastal illustration as a benchmark for layered composition, detail density, warm light and handcrafted surfaces, without literal copying or adopting its pictured HUD/tools/timer as mechanics. Upgrade the shop, foreground work/storage clusters, flush deck/ground treatment, coastal vegetation and short outward approach into one coherent small quality slice. Evaluate the named free packs first; selectively use legitimate free assets and record provenance. Preserve movement, routes, mission, tide and trading. No whole-island art, final NPC/UI or M2. Director visual judgment remains required; test success is not art acceptance.

The Director's addendum makes a detailed jobs board a core base element and the primary interaction for accepting the current expedition. Keep the smallest single-job implementation; Mara retains context, trading and logbook reward. Capture spawn, shop, board and exit before delivery. A sparse or crude result is not complete merely because it builds; if suitable free assets cannot efficiently meet the bar, present exact paid options for approval instead of silently substituting another weak placeholder pass.

## Director-approved hub quality and liveliness refinement — 2026-09-12

Build a substantially richer keeper-quality starting base using selected real free structural/prop packs first. Move material/proportion treatment toward the midpoint of stylization and realism while preserving stylized readability and conservative rendering. Prioritize trader hut, hero jobs board, working deck/shore edges, then short departure/landmarks. Add cheap environmental motion and one memorable animated living element; preserve all mission/trading/movement/tide behavior. Restricted source assets may remain local and ignored with a documented dependency rather than being rejected solely because Git is public. No paid purchase, broad world art or M2 is authorized. Technical tests alone do not close the visual gate.

## Director-approved Meshy hut/parrot integration — 2026-09-12

Use `SourceAssets/Generated/Budka/Budka.glb` and `Papug/Papug.glb` as preferred hero visuals alongside the new mission board. Preserve source files, PBR quality and believable uniform scale. Replace conflicting older trader structure/old bird visuals while preserving Mara's exact interaction position, trading/mission state and the expedition approach. Preserve merged masters rather than damaging textures to force separation. Papug may use a restrained existing rig or cheap clean idle; a documented static fallback is acceptable if proper animation is disproportionate. Retain the conservative renderer, inspect local asset/draw cost, and validate a Windows package and coherent spawn/close views. No broader world pass or M2.

## Director-approved Meshy workbench/boat composition — 2026-09-12

Integrate the supplied Verstak and Lodka as intact PBR hero assets: a salvage sorting zone near Budka and a small coastal boat beside the base. All five Meshy heroes define the quality target. Recompose the immediate hub, remove conflicting crude support geometry, preserve clear routes and Mara/board gameplay. Optional motion only if cheap and physically appropriate; no boat gameplay or new systems. Preserve sources and rendering baseline, profile before reducing quality, validate packaged close/spawn/shore views. No M2 or broad world-art pass.
