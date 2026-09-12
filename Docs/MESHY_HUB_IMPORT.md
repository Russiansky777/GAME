# Meshy hero hut and parrot import

## Retained sources and inspection

The Director supplied two Meshy-generated GLBs. The inspector reads them without
modification and writes all derived files to ignored `Artifacts/MeshyHubReview`.

- `SourceAssets/Generated/Budka/Budka.glb`: 120,760,744 bytes; SHA-256
  `1113EB9FB407EDAA6D42B830F2A80B1E70C7081C7A60CC2B0C0F0AD4DD280AE0`.
- `SourceAssets/Generated/Papug/Papug.glb`: 94,359,692 bytes; SHA-256
  `DB7F65A62CC3065E26DA49D67A6C0AE47D8B5C0391EF6E82B6A6F7D20AEEF1B6`.

Blender 5.2.1 finds one merged mesh and one material in each source. Budka has
1,710,089 vertices and 3,029,367 triangles. Papug has 1,434,202 vertices and
1,953,478 triangles. Both have UV0, geometric vertex normals declared by glTF,
a 4096 px Base Color, 2048 px Metallic-Roughness and 4096 px normal map. Their
glTF materials are opaque and two-sided, with Base Color/Metallic/Roughness
factors all 1. Neither declares an occlusion texture, skin, armature or animation.
Papug is one merged mesh containing the bird and full floor perch, so this
pass keeps it intact and static. Budka likewise remains visually merged.

Source inspection previews are `Artifacts/MeshyHubReview/Budka/preview.png` and
`Artifacts/MeshyHubReview/Papug/preview.png`. Full raw glTF declarations,
topology/bounds, image metadata and node hierarchies are in each `report.json`.
The very high LOD0 cost is intentional for initial hero-quality evaluation; the
source and close LOD are not aggressively reduced.

No distance LODs are configured in this focused import. Both meshes are unusually
expensive, so runtime profiling should drive later quality-safe reduction while
retaining this full LOD0 and the verbatim sources.

## Scale, axes and pivots

`Scripts/InspectMeshyHub.py` applies uniform scale only, preserving proportions,
normals, UVs, textures and the single-mesh visual. It centres each asset in plan
and places its lowest point at Z=0. Blender exports vertex positions from
`(X,Y,Z)` to glTF `(X,Z,-Y)`. UE 5.8 Interchange then maps glTF axes to Unreal
`(X,Z,Y)`, so the resulting native basis is Blender `(X,-Y,Z)`. The source
storefront and bird face point Blender -Y and therefore point **Unreal local +Y**.
Local X is width and local Z is up. This sign is confirmed by the first Editor
capture, where treating the asset as front -Y presented Budka's closed rear wall.

- Budka: raw Blender bounds are 2.000000 × 1.500964 × 1.377560 m (X/Y/Z).
  Uniform scale 2.5 makes it 500 cm wide × 375.241 cm deep × 344.390 cm high.
  The front counter is approximately normal standing counter height in the
  rendered preview. Expected UE bounds are about X -250..250,
  Y -187.621..187.621, Z 0..344.390 cm.
- Papug: raw Blender bounds are 0.834426 × 0.911009 × 1.902762 m (X/Y/Z).
  Uniform scale to 140 cm overall puts the bird's head near player eye level and
  leaves the visible bird body/tail around 65–75 cm. Expected UE bounds are about
  X -30.697..30.697, Y -33.515..33.515, Z 0..140 cm.

## Unreal assets and materials

Run Blender 5.2 with `--background --factory-startup --python
Scripts/InspectMeshyHub.py`, then execute `Scripts/ImportMeshyHub.py` through an
UE 5.8 Python commandlet. The importer uses stable Interchange output paths:

- `/Game/Generated/MeshyHub/Budka/SM_MeshyHub_Budka_import/StaticMeshes/SM_MeshyHub_Budka_import`
- `/Game/Generated/MeshyHub/Papug/SM_MeshyHub_Papug_import/StaticMeshes/SM_MeshyHub_Papug_import`
- `/Game/Generated/MeshyHub/Materials/M_MeshyHub_Budka_V1`
- `/Game/Generated/MeshyHub/Materials/M_MeshyHub_Papug_V1`

The custom opaque, two-sided materials preserve the source PBR contract. Base
Color is sRGB. Metallic-Roughness is linear `TC_MASKS` and the material sample is
explicitly `SAMPLERTYPE_MASKS`, with G connected to Roughness and B to Metallic.
The normal texture is linear `TC_NORMALMAP` with a Normal sampler. UE 5.8
Interchange's `flip_green_channel=true` is retained and asserted once; the
importer does not double-flip it. AO remains
unconnected because neither source declares glTF occlusion. On repeat runs the
importer updates the three actual V1 texture-sample nodes and refuses structural
graph drift; structural changes use a new version instead of deleting graph roots.
Unused Interchange default materials are removed after the custom material is
assigned. Visual mesh collision is removed; runtime code owns simple collision
and placement.
