# LOW TIDE test plan

## M1 automated evidence

Current run results, package evidence and any remaining failures are recorded in CURRENT_STATE. The regression set is grouped as five retained M0.5 checks and six M1 checks; rerun it against the fresh candidate after any source or asset change:

1. M0.5 regressions (five): expedition round trip, safe-edge recovery, sprint/containment, individual-sale atomicity, and inventory capacity/quantity boundaries.
2. M1 mission transaction: Mara begins the expedition, logbook return resolves once, and the optional artifact can be retained or sold.
3. M1 living tide and alternate route: `CharacterMovement` walks every main-route, optional-risk and elevated blue-escape segment with gravity, slopes and collision; rising water closes the shortcut while the escape remains walkable, including wet-ground warning/grace and salvage retention.
4. M1 phenomenon recovery and second trip: grounding counterplay/recovery resolves the risk and a later low tide provides collectible ordinary salvage again.
5. M1 scene containment and clearance: route floor, boundaries, shortcut closure and interaction-line clearance are checked independently of decorative meshes.
6. M1 traversal safety: single jump, automatic step-up, invalid freefall recovery and inventory/mission/expedition snapshot retention are checked without allowing tide-blocker bypass.
7. M1 terrain continuity: imported bounds, route-array guard, visual ribbon alignment and direct/union ray checks cover the intended route surfaces; this does not replace manual visual-quality review.

The consolidated traversal acceptance checks are:

1. Measure actual walk and sprint speeds, then traverse the complete main, optional-risk and elevated-return routes at both speeds without timing assumptions that hide a collision break.
2. Exercise Space near small rocks, curbs, route seams, tide blockers and important boundaries; confirm a stable single jump and that jumping is not required to cross missing or broken geometry.
3. Force an invalid freefall below -1000 cm; confirm recovery to the last supported dry spot, or expedition start when submerged, with inventory, mission and expedition snapshot retained and no new penalty.
4. Verify tide jump recovery: the rising tide closes the low shortcut, jumping cannot bypass the closure, and the elevated escape remains physically traversable.
5. Inspect visual terrain/collision continuity along every intended route and every M0.5-to-M1 transition; record any remaining visible gap, drop-off or invisible-space walk before calling the pass complete.

Passing automation does not establish keyboard/mouse usability, audible playback, visual quality, accessibility, player timing or performance. Technical route-image review is recorded in CURRENT_STATE; Director visual-quality review and acceptance remain pending.

## M1 delivery and Director acceptance checks

The focused trader-hub regression also requires all seven imported meshes and three structural wall proxies, with collision owned by the scene rather than Mara. Existing front interaction visibility and complete route containment checks remain mandatory. Visually review the actual packaged shop/front approach and departure; Blender renders alone do not establish the in-game result.

1. Verify the fresh M1 package against the latest evidence in CURRENT_STATE, launch the executable outside the Editor, and inspect cook and runtime logs. The final asset recook/package has passed; repeat it after later source or asset changes.
2. Manually play the full first trip with physical input: receive Mara's mission, read objective/risk communication, collect the protected logbook, take or decline the shrine artifact, and return. Confirm a normal first-play duration of 8–12 minutes.
3. Manually traverse the main route, optional branch and blue escape. At rising tide, verify water, warning, closure and route markers communicate the shortcut loss and that the upper escape is usable and understandable.
4. Confirm wet-ground warning, five-second grace and recovery in a packaged build. Verify recovery preserves prior stock, evidence, credits and permanent mission state while removing only current-trip ordinary salvage; then complete a second trip.
5. Inspect representative shore, wreck, signal-station, shrine and return views for the approved stylized semi-cartoon coastal direction. Review actual ambience, interaction, tide and phenomenon cues where audio output is available.
6. Capture representative packaged performance on the development hardware using the conservative rendering configuration; record settings, route/view, frame-time result and limitations.
7. Director review judges duration, discovery, valuable-loot excitement, voluntary risk, tide/escape readability, mystery, audio and art quality. M1 is not Director-accepted until that review and the fresh package evidence are recorded.

## Accepted M0.5 regression evidence

M0.5 is a Director-accepted historical greybox. Its accepted retest covered sprint, containment and core interactions; tide-warning readability was explicitly non-blocking and is carried into M1. Its historical Win64 package and stationary 1080p performance capture must not be reported as M1 validation.
