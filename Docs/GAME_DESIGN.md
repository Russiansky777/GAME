# M0.5 gameplay contract

First-person coastal settlement greybox, one trader, one causeway to a salvage shelf. Objective: recover salvage during low tide and return to sell it. Walking/look, interaction prompt, pickup, minimal inventory and individual-sale UI.

Provisional tuning implemented for Director acceptance; manual retest remains:
- Hold Left Shift to sprint at about 1.6× walking speed. There is no stamina system.
- High → falling → low → rising tide. Visible water and physical accessibility agree; timer/warning communicates remaining access.
- The settlement, causeway and salvage shelf have a continuous physical perimeter. Sprinting through corners or across phase transitions must not escape the playable space.
- Five test pickups: scrap metal, copper wire, sea glass, sealed supply tin and weathered evidence token. Stable IDs, names, descriptions, values and sellable flags. Evidence is kept and clearly marked unsellable.
- Finite inventory capacity with clear full feedback. Sale explicitly shows price.
- Give a clear warning before access closes and forced recovery can occur.
- Stranded players return to shore and lose only ordinary, unsold salvage acquired during the current expedition. Evidence/quest-critical items, credits, and permanent state remain. This is reversible greybox tuning, not a final story decision.

No creatures, combat, elaborate crafting, additional NPCs/story or advanced ocean. Story remains a Director decision; approved art direction is recorded below.

## Director-approved art direction

LOW TIDE targets high-quality stylized semi-cartoon first-person 3D, not semi-realism or photorealism. Fell & Sell and Chop Chop Inc. inform stylization/production efficiency only; do not copy their assets, designs or identity. Aim for the richer stylized quality selected by the Director, with disciplined production complexity. The previously selected visual reference is not attached in the repository; do not invent its specifics.

Clean silhouettes, simplified attractive geometry, moderate detail, painterly/clean materials. Warm, appealing coastal daylight contrasts with mystery/dark-fantasy mood during anomalous low tide. Target commercial Steam presentation; avoid a cheap mobile/low-poly prototype appearance. M0.5 greybox is a mechanics test, not final art quality.

Use reusable master materials and small material families. Budget geometry/textures by visual importance; keep shaders, lights, foliage, overdraw and draw calls restrained. Economical attractive water and lower-end GPU scalability are required. Keep the conservative rendering baseline unless measured visual payoff justifies changes. No photorealistic asset pipeline without explicit Director approval.
