# LOW TIDE test plan

## M1 automated evidence

The latest run recorded in CURRENT_STATE reports 8/8 successes and 0 failures: four retained M0.5 regressions and four M1 tests. Re-run the suite against the fresh candidate after any source or asset change:

1. Mission reward and rare-choice transaction: Mara begins the expedition, logbook return resolves once, and the optional artifact can be retained or sold.
2. Living tide and alternate route: normal `CharacterMovement` walks every main-route, optional-risk and elevated blue-escape segment with gravity, slopes and collision; rising water closes the shortcut while the escape remains walkable. It also checks the wet-ground warning/grace: under five continuous seconds does not recover, while five seconds removes only newly acquired ordinary salvage and keeps prior ordinary stock, protected evidence and credits.
3. Phenomenon recovery and second trip: marked grounding counterplay/recovery resolves the risk and a later low tide provides collectible ordinary salvage again.
4. Scene containment and clearance: route floor, boundaries, shortcut closure and interaction-line clearance are checked independently of decorative meshes.

The first four named M0.5 regressions remain: expedition round trip, safe-edge recovery, individual-sale atomicity, and inventory capacity/quantity boundaries. Passing automation does not establish keyboard/mouse usability, audible playback, visual quality, accessibility, player timing or performance.

## M1 delivery and Director acceptance checks

1. Verify the fresh M1 package against the latest evidence in CURRENT_STATE, launch the executable outside the Editor, and inspect cook and runtime logs. The final asset recook/package has passed; repeat it after later source or asset changes.
2. Manually play the full first trip with physical input: receive Mara's mission, read objective/risk communication, collect the protected logbook, take or decline the shrine artifact, and return. Confirm a normal first-play duration of 8–12 minutes.
3. Manually traverse the main route, optional branch and blue escape. At rising tide, verify water, warning, closure and route markers communicate the shortcut loss and that the upper escape is usable and understandable.
4. Confirm wet-ground warning, five-second grace and recovery in a packaged build. Verify recovery preserves prior stock, evidence, credits and permanent mission state while removing only current-trip ordinary salvage; then complete a second trip.
5. Inspect representative shore, wreck, signal-station, shrine and return views for the approved stylized semi-cartoon coastal direction. Review actual ambience, interaction, tide and phenomenon cues where audio output is available.
6. Capture representative packaged performance on the development hardware using the conservative rendering configuration; record settings, route/view, frame-time result and limitations.
7. Director review judges duration, discovery, valuable-loot excitement, voluntary risk, tide/escape readability, mystery, audio and art quality. M1 is not Director-accepted until that review and the fresh package evidence are recorded.

## Accepted M0.5 regression evidence

M0.5 is a Director-accepted historical greybox. Its accepted retest covered sprint, containment and core interactions; tide-warning readability was explicitly non-blocking and is carried into M1. Its historical Win64 package and stationary 1080p performance capture must not be reported as M1 validation.
