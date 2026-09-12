# Hub donor assets

This directory holds the small, source-controlled subset of external art approved for evaluating and importing into the LOW TIDE start hub.  It is deliberately not a dump of complete third-party packs.  Complete downloads are kept under ignored `Intermediate/ExternalAssets`.

## Kenney Pirate Kit 2.1 — retained CC0 donor

- Author: Kenney / `www.kenney.nl`
- Product/source page: <https://opengameart.org/content/pirate-kit>
- Direct archive acquired: <https://opengameart.org/sites/default/files/kenney_pirate-kit_2.1.zip>
- Archive SHA-256: `667ED2CAF92954DDB98F7B7CEDE831FE99AB75063C26B25E23D32715BEE9C943`
- Licence: CC0 1.0, retained verbatim in `KenneyPirateKitCC0/LICENSE-CC0.txt`; canonical licence text: <https://creativecommons.org/publicdomain/zero/1.0/>.
- Formats supplied: FBX, OBJ and GLB.  The selected files here are FBX for the Unreal import path.

This is **Kenney's Pirate Kit**, not Quaternius' similarly named Pirate Kit.  It is a verified substitute donor, selected because the requested Quaternius FBX folder was temporarily download-quota blocked (see below).

`KenneyPirateKitCC0/Textures/colormap.png` is the pack's single shared colour map.  The `Previews/*.png` images in the full archive are reference renders, not diffuse textures and are not retained here.

### Selected import candidates

| Hub use | Source FBX |
| --- | --- |
| Hut side-frame and canopy support | `structure.fbx`, `structure-roof.fbx` |
| Dock/deck silhouette extension | `structure-platform-dock.fbx`, `structure-platform-dock-small.fbx`, `platform-planks.fbx` |
| Edge/route dressing | `structure-fence.fbx`, `flag-pirate.fbx`, `mast-ropes.fbx` |
| Trader/work clusters | `barrel.fbx`, `crate.fbx`, `crate-bottles.fbx`, `chest.fbx`, `tool-paddle.fbx`, `tool-shovel.fbx` |
| Coastal identity | `boat-row-small.fbx`, `ship-wreck.fbx` |

The meshes are a simple stylized/low-poly kit.  The shared colour map alone will not solve the current material-richness goal; use these for silhouette, clutter and route structure, while preferring a separately verified textured structural pack when one is available.

## Requested donor status — 2026-09-12

| Requested source | Result | Licence / practical note |
| --- | --- | --- |
| Quaternius Pirate Kit | Official page verified: <https://quaternius.com/packs/piratekit.html>. Its public FBX folder is <https://drive.google.com/drive/folders/103WUlc7ttjHegfyDZaQ-GQy2PkCGrjOv>. Acquisition was attempted but Google Drive returned an active download-quota block. | Official page declares CC0 and FBX/OBJ/Blend/glTF. No Quaternius files are represented in this repository. Retry later; do not confuse it with Kenney's kit. |
| Creatus Pirate Pack | Official listing: <https://creatusdev.gumroad.com/l/creatus-pirate>. | CC0; advertises 90+ props and characters, including lanterns, map, telescope, barrels, boxes, boat and chest, in FBX/OBJ/DAE/glTF. Its authorised $0 checkout was not completed, so no files are retained. |
| Pirate Town Free | Official listing: <https://www.artstation.com/marketplace/p/dNq7q/pirate-town-free>. | Reports a 17 MB archive under ArtStation Standard License (one commercial project, up to 2,000 sales or 20,000 views). Cloudflare blocked scripted acquisition. |
| Pirate town (same Korboleev product on Fab) | <https://www.fab.com/listings/c97cbe45-1607-4832-9a53-369042882623> | Paid, not a free replacement; the listing is therefore not acquired. |
| FANTASTIC - Village Pack | <https://www.fab.com/listings/52529a12-e88e-41a0-8834-b87306f20c24> | Free Fab Standard License and the strongest evaluated structure/prop candidate. Its raw content must remain local/ignored. A signed-in browser exists on this host, but the approved `@oai/sky` controller could not verify the current browser URL and ended the Computer Use turn before navigation. The pack was not added to the library or downloaded. |

No parrot/bird mesh was found in the acquired Kenney Pirate Kit or the Quaternius Pirate Kit catalogue.  Use the project-authored animated bird fallback rather than misrepresenting an unverified donor.

No further browser attempts were made after the controller stop.  The only retained external donor is the CC0 Kenney subset above.
