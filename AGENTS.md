# LOW TIDE — agent instructions

Agents own technical production: code, Blueprints, assets, debugging, Git, testing and packaging. The human is Game Director/Product Owner; never assign them technical repairs. Decide technical choices and document them. When a human action is unavoidable, keep it to a simple one- or two-click install or UI action; retain configuration, diagnosis and repair work. Avoid costly UI automation. Ask only for creative/product decisions and unavoidable permissions, logins, purchases or GUI actions.

Pilot budget: EUR 0 purchased credits, assets, servers or paid APIs. Use included allowance and legitimately free tools. Offline Windows game. No Fast mode without explicit authorization; never recommend buying credits during the pilot.

Read Docs/CURRENT_STATE.md first, then PROJECT_BRIEF, GAME_DESIGN, ARCHITECTURE, DECISIONS, MILESTONES and TEST_PLAN as relevant. Keep documentation concise and current.

This is the authoritative repository. Inspect status, preserve unrelated work and commit verified milestones. Extend the existing Unreal 5.8.2 C++/Win64 project; verified toolchain is recorded in Docs/CURRENT_STATE.md. Prefer text-readable code/config/data, stable item IDs and explicit inventory/trading transactions. Use LFS for Unreal assets and ignore generated output.

Reduce content before architectural quality. No speculative frameworks, combat, backend, advanced ocean, elaborate crafting or extra NPCs for M0.5. Use primitive/engine assets; record provenance for external assets.

Compile, launch, inspect logs, test and package before declaring playable completion. Document actual results, bugs and next action. Use Scripts/Build.ps1 and the commands in README.md.

Keep tool output and delegation economical. Classify each task before substantial work and use the cheapest capable worker: Astra for architecture, cross-system integration, difficult Unreal debugging, risky refactors and milestone audits; Sol for substantial features, moderate debugging and Blender tooling; Terra for routine implementation, tests, UI, data, docs, refactors and routine installations; Luna for discovery, boilerplate, repetitive edits, simple logs and Git. Do not default to Astra for routine delegation or installations. Do not delegate tiny tasks when handoff/review costs more. Bounded tasks require explicit acceptance criteria; the lead reviews shared-system changes and remains architecturally accountable. The Director never coordinates workers; canonical docs and Git are the source of truth. Preserve EUR 0 and no Fast mode.

The Director authorizes necessary game production, builds and free tool installations without repeated confirmation. This supersedes the earlier manual-approval preference; mandatory OS/tool approval controls still apply. Keep the EUR 0 budget and no Fast mode. Before ending substantial work, update CURRENT_STATE with evidence, incomplete work, bugs and explicit next action; commit and push known-good changes to origin, verify remote contents, and leave a clean tree where practical.

Art direction is Director-approved high-quality stylized semi-cartoon 3D, not realism: clean silhouettes, simplified attractive geometry, moderate detail and painterly/clean materials; warm coastal daylight and mysterious dark-fantasy low tide. Follow Docs/GAME_DESIGN.md. Preserve economical reusable materials and conservative scalable rendering; no photorealistic asset pipeline without explicit approval. Greybox is not the final quality target.
