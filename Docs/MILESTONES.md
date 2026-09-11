# Delivery gates

| Gate | Required evidence | Status |
| --- | --- | --- |
| M0 audit | Hardware/tool audit, verified remote, canonical docs, local Git checkpoint | Complete in this documentation checkpoint |
| Toolchain | Minimal project compiles, packages and launches; logs checked | Passed in 6a8bb84: editor build/launch, Win64 package/launch |
| M0.5 movement | First-person coastal greybox, collision, reachable trader | Implemented; CharacterMovement round trip passes automation; scene visually inspected |
| M0.5 tide | Visible water, matching access, warning, stranded recovery | Implemented; phase/access/recovery and second cycle pass automation |
| M0.5 economy | Five pickups, inventory, individual sales, protected evidence | Implemented; three automation tests pass; retained evidence does not respawn |
| M0.5 acceptance | Offline Windows executable passes test plan; ready for Director | Packaged prototype ready for review; launch/UI/profile verified; full manual input and disconnected-network checks remain |
| M1 slice | Extend same project into 8–12 minute expedition after M0.5 | Deferred |

Commit verified checkpoints. Deliver executable path, controls and known limitations. Source alone does not satisfy a playable gate. No full-game content expansion yet.
