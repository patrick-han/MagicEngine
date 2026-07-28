# Magic Engine

Purposefully left unlicensed, as I'm not sure what I want to do with this yet

Currently very content bare on the graphics side of things, since part of this project is also about learning to build an editor/'game object' model

Magic Engine uses a right-handed, Z-up coordinate system in world, local, and
camera space: +X is right, +Y is forward, and +Z is up.


# Blender USD Export Notes
- Object Types > World Dome Light ❌
- Geometry > Triangulate Meshes  ✅
- Materials > Export Textures > **Keep**

Assets/Levels should contain .usd* files, the rest of Assets can be structured however

## Material texture convention

Assumes the gltf metallic roughness channel layout, including when materials are imported from USD:

- Base color and tangent-space normal maps are separate images
- Metallic and roughness must share one non-color image
- Roughness (G) and metallic (B)
- Both inputs must connect to the same `UsdUVTexture` node through its `b` and `g` outputs respectively. Other layouts are warned about and ignored
