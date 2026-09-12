# Hero job board import

## Source inspection

`SourceAssets/Generated/JobBoard/SM_JobBoard_Hero_v01.glb` is a Director-generated Meshy source retained verbatim. Blender 5.2.1 inspected it as one object and one mesh: 24,516 vertices, 16,112 triangles, one `UVMap`, geometric normals, one `BakedMaterial`, and three embedded 4096 px textures. The source has no normal texture and no vertex-color or semantic mask.

Raw bounds are X `-0.946663..0.938097`, Y `-0.210376..0.205260`, Z `-0.951752..0.950661` metres (1.884760 × 0.415636 × 1.902413 m). Raw GLB metadata declares `doubleSided=true`, emissive factor `(1,1,1)`, emissive texture, Base Color texture, and Metallic-Roughness texture. Metallic and roughness factors are omitted, so glTF defaults both to 1. No occlusion texture is declared: packed R is not wired as AO. Base Color and Emissive are sRGB; Metallic-Roughness is linear/masks with G=roughness and B=metallic.

The raw file contains one named node, `output_unwrapped`. Position-welded connectivity yields one component (7,950 unique positions / 24,516 source vertices); unwelded UV/normal seams yield 4,398 fragments. There is no automatic semantic node split. The deterministic motion preparation makes a narrowly spatial, visually reviewed cut around the protruding lantern/hanger and supplies two tiny face-adjacent loose-paper overlays. Main plus lantern retain all 16,112 source triangles and preserve source UVs, normals and material indices.

Evidence is in `Artifacts/HeroJobBoardReview/blender`: `report.json`, `preview.png`, the inspection `.blend`, and the ignored normalized Interchange input. `Scripts/InspectHeroJobBoard.py` reproduces these from the retained GLB. Pass `--no-render` as the final argument to skip the evidence still.

## Import contract

The inspector normalizes width to 1.80 m and height to 2.10 m, preserves natural depth (about 0.4156 m), bakes a bottom-centre pivot, and rotates plan axes. Its GLB accessor contract is X depth, Y up, Z width. Unreal Interchange maps this to X depth/front, Y width, Z up: front `-X`, width `Y`, bottom `Z=0`.

`Scripts/ImportHeroJobBoard.py` imports through UE 5.8 Interchange glTF. Runtime uses these stable Interchange assets:

- `.../SM_JobBoard_Hero_motion/StaticMeshes/SM_JobBoard_Hero_Main`: bounds X `-20.7818..20.7818`, Y `-90..90`, Z `0..210` cm.
- `.../SM_JobBoard_Hero_Lantern`: board-space bounds X `-20.7818..-2.2833`, Y `66.5339..81.8290`, Z `89.1112..176.8394` cm; UE pivot `(-2.2833,74.1814,176.8394)` cm.
- `.../SM_JobBoard_Hero_Paper_A`: board-space bounds X `-9.45..-9.20`, Y `8..16`, Z `123..134` cm; UE pivot `(-9.2,12,134)` cm.
- `.../SM_JobBoard_Hero_Paper_B`: board-space bounds X `-9.65..-9.40`, Y `-23.5..-16.5`, Z `101..110` cm; UE pivot `(-9.4,-20,110)` cm.

Interchange bakes the motion nodes into board-space mesh coordinates and flips glTF width Y, so animated child components sit at the UE pivots above and apply the negative pivot as their mesh-relative offset. Main remains identity at board root `(1300,-850,125)`, yaw -17 degrees (relocated during the Meshy hut/parrot composition pass). Runtime collision is three simple boxes shaped around the two posts and panel; the query-only interaction target is centred near local Z 135. The imported visual meshes have collision removed.

Imported geometric normals are retained. `/Game/Generated/HeroJobBoard/Materials/M_JobBoard_Hero_V2` is two-sided and opaque, connects Base Color, Emissive, ORM G Roughness and ORM B Metallic, and intentionally leaves AO unconnected because the source declares no occlusion texture. Main and lantern use V2; the two paper overlays use `M_JobBoard_PaperEdge`, an opaque two-sided warm parchment material with roughness 0.9 and no extra textures.

## Reproduction and runtime check

Run Blender with `--background --factory-startup --python Scripts/InspectHeroJobBoard.py`, then `Scripts/PrepareHeroJobBoardMotion.py` the same way. Both default to ignored `Artifacts/HeroJobBoardReview` outputs. Execute `Scripts/ImportHeroJobBoard.py` with the UE Python commandlet while no other Editor/game process is using these assets. Runtime references the four stable Interchange `StaticMeshes` paths above; superseded local aliases are preserved but excluded from Git.

Real DX11 review caught a sampler mismatch that NullRHI/import validation could not detect. The linear metallic/roughness texture uses `TC_MASKS` **and** `SAMPLERTYPE_MASKS`; selecting Linear Color with Masks compression makes SM5 use the default grey material. The importer repairs this setting on existing material graphs as well. No synthetic normal map was generated.
