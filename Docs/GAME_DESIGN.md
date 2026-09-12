# LOW TIDE gameplay contract

## Active M1 slice — Director authorized

One compact authored expedition, approximately 8–12 minutes on a normal first playthrough. The five acceptance pillars are discovery, meaningful loot, voluntary risk, a geography-changing tide and mystery. This is a playable-content target, not merely a longer countdown.

Provisional mission: the hub jobs board posts Mara's request for the drowned signal station's logbook. Accept the current expedition at the board, follow a readable coastal route through a wreck and exposed ruins, retrieve the protected logbook and uncover an impossible reply recorded before transmission. Return it to Mara for a one-time payment; she remains the salvage trader. The board is the visible mission-acquisition anchor, with one reversible job rather than a general quest framework. Names, story details and rewards are reversible and do not establish major lore.

The main objective has comfortable timing. An optional deeper shrine advertises a substantially more valuable singing artifact and its danger before acquisition. Carrying it awakens one non-combat phenomenon: it follows movement, so stopping and using marked grounding refuges provide counterplay. The player may escape with it and sell or retain it; no speculative future-use mechanic is promised. Keep the catalog small and distinct: common salvage, rare valuable loot and protected evidence.

Low tide exposes the route. Returning water visibly closes a convenient low shortcut while the marked upper escape trail remains usable. Clear phase text, environmental markers and audible cues explain the change and the later final-escape risk. Recovering to shore and losing current-expedition ordinary salvage remains a provisional last fallback; preserve prior stock, evidence, credits and permanent mission state. No swimming, drowning, health, combat or stamina.

Concentrate attractive reusable stylized geometry/materials on this route, with a warm inhabited shore and stranger deep-water structures. Use minimal coast/wind, interaction, tide and phenomenon audio; record source/license or project authorship. The M0.5 warning was not intuitive to the Director: improve its communication as part of this slice, without a final UI project.

## Accepted M0.5 baseline (historical)

First-person coastal settlement greybox, one trader, one causeway to a salvage shelf. Objective: recover salvage during low tide and return to sell it. Walking/look, interaction prompt, pickup, minimal inventory and individual-sale UI.

Director-accepted provisional tuning:
- Hold Left Shift to sprint at about 1.6× walking speed. There is no stamina system.
- High → falling → low → rising tide. Visible water and physical accessibility agree; timer/warning communicates remaining access.
- The settlement, causeway and salvage shelf have a continuous physical perimeter. Sprinting through corners or across phase transitions must not escape the playable space.
- Five test pickups: scrap metal, copper wire, sea glass, sealed supply tin and weathered evidence token. Stable IDs, names, descriptions, values and sellable flags. Evidence is kept and clearly marked unsellable.
- Finite inventory capacity with clear full feedback. Sale explicitly shows price.
- Give a clear warning before access closes and forced recovery can occur.
- Stranded players return to shore and lose only ordinary, unsold salvage acquired during the current expedition. Evidence/quest-critical items, credits, and permanent state remain. This is reversible greybox tuning, not a final story decision.

No creatures, combat, elaborate crafting, additional NPCs/story or advanced ocean. Story remains a Director decision; approved art direction is recorded below.

## Director-approved art direction

LOW TIDE targets high-quality stylized first-person 3D. The Director refinement of 2026-09-12 moves the hero hub toward a midpoint between stylization and realism: believable wood, rope, cloth and proportions, less toy-like geometry, with readable stylized silhouettes and disciplined production cost. This does not authorize a photorealistic pipeline. Fell & Sell and Chop Chop Inc. inform stylization/production efficiency only; do not copy their assets, designs or identity. Aim for the richer stylized quality selected by the Director, with disciplined production complexity. The previously selected visual reference is not attached in the repository; do not invent its specifics.

Clean silhouettes, simplified attractive geometry, moderate detail, painterly/clean materials. Warm, appealing coastal daylight contrasts with mystery/dark-fantasy mood during anomalous low tide. Target commercial Steam presentation; avoid a cheap mobile/low-poly prototype appearance. M0.5 greybox is a mechanics test, not final art quality.

Use reusable master materials and small material families. Budget geometry/textures by visual importance; keep shaders, lights, foliage, overdraw and draw calls restrained. Economical attractive water and lower-end GPU scalability are required. Keep the conservative rendering baseline unless measured visual payoff justifies changes. No photorealistic asset pipeline without explicit Director approval.

## Consolidated traversal pass — 2026-09-11

The approved M1 traversal tuning is 650 cm/s normal walking and 1040 cm/s sprinting (the existing 1.6× advantage, approximately 30% above the prior 500/800 tuning). Space performs one standard jump: 420 cm/s launch velocity, 1.3 gravity scale, 0.2 air control, 45 cm automatic step-up and a 45° walkable-floor limit. Jumping is a normal traversal ability for modest terrain variation, never a workaround for missing route geometry, a tide-access bypass or a way through major blockers.

The main, optional-risk and elevated return routes must read as one continuous playable surface across the settlement/M0.5 to M1 transitions. Visual ground and collision should agree; simple collision proxies are preferred. The shortcut closure and elevated escape remain unchanged, with tide timing at 300 seconds low tide and 180 seconds rising tide pending evidence of a real balance break.

If the player falls below -1000 cm, the GameMode safety net restores the last supported dry spot, or the expedition start if submerged, while retaining inventory, mission and expedition snapshot state. Normal tide-failure salvage penalties remain unchanged. This is a fallback for rare geometry failures, not the intended route experience.

### Canonical hero-asset quality reference — Director-approved 2026-09-12

The externally generated Meshy Budka trader hut, Papug parrot and hero mission board establish the preferred starting-base quality: semi-stylized but believable, between stylized game art and realism, rich PBR, detailed hero assets, neither toy-like/cheap low-poly nor photorealistic. These assets supersede older primitive/procedural hub visuals as the reference. Preserve their quality and let nearby framing/composition improve toward it; do not restyle them downward. Moderate local rendering cost is acceptable with measurements and scalable settings. This authorizes their focused hub integration, not a broad world-art pass or M2.
