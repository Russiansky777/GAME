# Meshy workbench and boat import

## Sources and inspection

The Director supplied both assets as Meshy-generated binary glTF 2.0 files. The
originals remain unchanged. `Scripts/InspectMeshyHub.py -- --only=Lodka` and
`--only=Verstak` reproduce the inspection reports, previews, inspection blends
and normalized Interchange inputs under ignored `Artifacts/MeshyHubReview`.

- `SourceAssets/Generated/Lodka/Lodka.glb`: 30,394,452 bytes; SHA-256
  `18DD0BA514A26646188EF583AB9E3F7207AF41C68CA0385B5D6626C175880335`.
  One merged mesh, 65,256 vertices, 45,814 triangles, one material and UV0. The
  glTF primitive declares normals. It has one 4096 px Base Color and one 4096 px
  packed Metallic-Roughness texture. It declares no normal or occlusion texture.
- `SourceAssets/Generated/Verstak/Verstak.glb`: 100,025,792 bytes; SHA-256
  `E925FDAA7764455FB9F84991C684F490CD0ED7C4836971BEBF3F8867FC0AD89A`.
  One merged mesh, 1,688,476 vertices, 3,079,551 triangles, one material and UV0.
  The glTF primitive declares normals. It has 2048 px Base Color, packed
  Metallic-Roughness and Normal textures, with no declared occlusion texture.

Neither source contains a hierarchy, skin, armature or animation. Their ropes,
tools and secondary details are baked into the textured master, so both remain
merged and static. This avoids damaging the UV/material fidelity merely to add
minor motion. Source previews are `Artifacts/MeshyHubReview/Lodka/preview.png`
and `Artifacts/MeshyHubReview/Verstak/preview.png`.

## Scale, pivot and native axes

The preparation applies uniform scale only, centres each mesh in plan, and moves
its lowest point to Z=0. Blender export plus UE Interchange reflects Blender Y:
Blender `(X,Y,Z)` becomes Unreal `(X,-Y,Z)`.

- Lodka uses uniform scale `2.0002400567`, producing an actual UE size of
  400.000 × 179.679 × 121.885 cm. Bounds are X `-200..200`, Y
  `-89.839676..89.839676`, Z `0.000001..121.885033` cm. Its bow is native local
  -X and its stern/rudder is +X.
- Verstak retains uniform scale `1.0`, producing an actual UE size of 200.000 ×
  87.773 × 115.904 cm. Bounds are X `-100..100`, Y
  `-43.886398..43.886398`, Z `-0.000001..115.903999` cm. Its working/user side
  is native local +Y.

These dimensions give a believable four-metre small working boat and two-metre
salvage bench. Runtime placement and simple collision belong to the hub actor.

## Unreal assets and PBR contract

Run `Scripts/ImportMeshyHub.py` with `-MeshyWorkBoatOnly` to import only these two
assets through UE 5.8 Interchange. Stable paths are:

- `/Game/Generated/MeshyHub/Lodka/SM_MeshyHub_Lodka_import/StaticMeshes/SM_MeshyHub_Lodka_import`
- `/Game/Generated/MeshyHub/Verstak/SM_MeshyHub_Verstak_import/StaticMeshes/SM_MeshyHub_Verstak_import`
- `/Game/Generated/MeshyHub/Materials/M_MeshyHub_Lodka_V1`
- `/Game/Generated/MeshyHub/Materials/M_MeshyHub_Verstak_V1`

Both custom materials are opaque and two-sided, matching the source. Base Color
uses sRGB. Metallic-Roughness is linear `TC_MASKS` with an explicit
`SAMPLERTYPE_MASKS`; G drives Roughness and B drives Metallic. Verstak's normal
uses `TC_NORMALMAP`, the Normal sampler and Interchange's single green-channel
conversion. Lodka intentionally has no Normal connection because none exists in
the source. AO is unconnected because neither source declares occlusion. Unused
Interchange materials are removed after assignment, and visual mesh collision is
removed so runtime can use simple authored proxies.

The final scoped commandlet completed with zero errors. It emitted one known
deprecation warning because UE 5.8's newer static-mesh editor subsystem is absent
in commandlet context and the compatible collision-removal API is deprecated.
No texture reduction, geometry reduction, distance LOD, Nanite or renderer change
was introduced. Full hero LOD0 is retained; later measured profiling should guide
any quality-safe distance LOD work.

## Runtime composition

Verstak is grounded at (400,-700,125) cm, yaw 30, facing the player approach; one under-worktop box covers Z 0..80 cm. Lodka is hauled up at (2150,-50,105), yaw -35, with two compact hull boxes. Both masters remain non-colliding and static; no new lamp, animation, boat or crafting system is added. Mara and the existing board remain unchanged. Old work/goods groups, five foreground donor props, the nearby crude hut and procedural boat placement are suppressed. Existing vegetation/rocks frame the bench and the right apron edge. The old repeated foliage cluster was also moved away from the bench; the hull is lowered 20 cm to seat it visually on the shore rather than balancing its appearance on the low rudder.

The local deck generator now uses warm contiguous planks and a dark backing under bevel joints; cyan accent strips and clipped sliver seam geometry were removed. Its conforming overlay retains 635 checked samples at terrain +2 cm with zero deviation, without modifying gameplay collision. The OBJ reimport binds materials by their actual slot names because Interchange can retain the previous slot order. Apron import succeeded with only the existing OBJ smoothing-keyword warnings.
