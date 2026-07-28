#include "USDImporter.h"

#include "../CommonCode/Log.h"
#include "../CommonCode/StaticMeshData.h"
#include "../DataLibCode/stb_image.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <optional>
#include <unordered_map>

namespace Magic
{

void USDImporter::ImportUSDPrimAsStaticMesh(
    const pxr::UsdPrim& entityPrim,
    StaticMeshData& staticMeshData)
{
    // Accept either a Mesh prim directly or an Xform with a direct Mesh child.
    pxr::UsdPrim meshPrim = entityPrim;
    if (!meshPrim.IsA<pxr::UsdGeomMesh>())
    {
        meshPrim = {};
        for (const pxr::UsdPrim& child : entityPrim.GetChildren())
        {
            if (child.IsA<pxr::UsdGeomMesh>())
            {
                meshPrim = child;
                break;
            }
        }
    }

    // Geometry
    // Wrapping the selected prim in UsdGeomMesh validates its schema.
    const pxr::UsdGeomMesh entityMesh(meshPrim);
    if (!entityMesh)
    {
        Logger::Err(std::format(
            "USD entity '{}' does not contain a direct Mesh child",
            entityPrim.GetPath().GetString()));
        return;
    }

    // USD stores polygon topology as:
    //   - points: the unique position table
    //   - faceVertexIndices: point indices for every polygon corner
    //   - faceVertexCounts: the number of corners belonging to each polygon
    // Normals and other primvars may have a different interpolation domain
    // than points, so they cannot necessarily be indexed with a point index.
    pxr::VtArray<pxr::GfVec3f> vertices;
    pxr::VtArray<int> indices;
    pxr::VtArray<int> faceVertexCounts;
    pxr::VtArray<pxr::GfVec3f> normals;

    if (!entityMesh.GetPointsAttr().Get(&vertices)
        || !entityMesh.GetFaceVertexIndicesAttr().Get(&indices)
        || !entityMesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts)
        || !entityMesh.GetNormalsAttr().Get(&normals))
    {
        Logger::Err("Could not read USD mesh geometry");
        return;
    }

    // "st" is USD's conventional primary UV primvar. ComputeFlattened applies
    // any primvar indices, giving us a directly indexable array of UV values.
    const pxr::UsdGeomPrimvar stPrimvar = pxr::UsdGeomPrimvarsAPI(meshPrim).GetPrimvar(pxr::TfToken("st"));
    pxr::VtArray<pxr::GfVec2f> uvs;
    const bool hasUvs = stPrimvar && stPrimvar.ComputeFlattened(&uvs);

    // Interpolation determines which topology domain owns each normal or UV:
    // one value for the mesh, one per face, one per point, or one per corner.
    const pxr::TfToken normalsInterpolation = entityMesh.GetNormalsInterpolation();
    const pxr::TfToken uvInterpolation = hasUvs ? stPrimvar.GetInterpolation() : pxr::UsdGeomTokens->constant;
    auto getElementIndex = []( const pxr::TfToken& interpolation, std::size_t faceIndex, std::size_t cornerIndex, std::size_t pointIndex) -> std::optional<std::size_t>
    {
        if (interpolation == pxr::UsdGeomTokens->constant) // The same over the entire mesh
        {
            return 0;
        }
        if (interpolation == pxr::UsdGeomTokens->uniform) // same over entire face
        {
            return faceIndex;
        }
        if (interpolation == pxr::UsdGeomTokens->vertex || interpolation == pxr::UsdGeomTokens->varying) // 1 per point
        {
            return pointIndex;
        }
        if (interpolation == pxr::UsdGeomTokens->faceVarying) // 1 per face corner, likely what we want most of the time where a vertex can have multiple normals, uvs etc
        {
            return cornerIndex;
        }
        return std::nullopt;
    };

    SubMeshData combinedGeometry;
    // In the worst case every face corner has a distinct normal or UV, so the
    // final engine vertex count can be as large as the USD index count.
    combinedGeometry.m_vertices.reserve(indices.size());
    combinedGeometry.m_indices.reserve(indices.size());

    struct FaceIndexRange
    {
        std::size_t firstIndex;
        std::size_t indexCount;
    };
    std::vector<FaceIndexRange> faceIndexRanges(faceVertexCounts.size()); // Keep track of which index ranges correspond to each face if all faces are triangles indexCount is always 3, but this works for quads too. This is indexed by the indices in a GeomSubset (the indices are face indices)

    // MagicEngine stores one position, normal, and UV per vertex. USD can
    // assign different normals or UVs to corners that reference the same
    // point. This composite key shares compatible corners while splitting
    // vertices at hard-normal edges and UV seams.
    struct VertexKey
    {
        std::size_t pointIndex;
        float normalX;
        float normalY;
        float normalZ;
        float uvX;
        float uvY;

        bool operator==(const VertexKey&) const = default; // The hash is only for finding the right bucket quickly, this is the actual equality comparison to check if a vertex is equal
    };

    auto vertexKeyHash = [](const VertexKey& key)
    {
        std::size_t seed = std::hash<std::size_t>{}(key.pointIndex);
        auto hashCombine = [&seed](float value)
        {
            const std::size_t valueHash = std::hash<float>{}(value);
            seed ^= valueHash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        hashCombine(key.normalX);
        hashCombine(key.normalY);
        hashCombine(key.normalZ);
        hashCombine(key.uvX);
        hashCombine(key.uvY);
        return seed;
    };

    // Maps each unique engine vertex tuple to its index in m_vertices.
    std::unordered_map<VertexKey, uint32_t, decltype(vertexKeyHash)> sharedVertices(0, vertexKeyHash);
    sharedVertices.reserve(indices.size());

    // faceVertexIndices is one flat array. cornerIndex tracks our position in
    // that array as faceVertexCounts divides it into individual polygons.
    std::size_t cornerIndex = 0;
    for (std::size_t faceIndex = 0; faceIndex < faceVertexCounts.size(); ++faceIndex)
    {
        const int faceVertexCount = faceVertexCounts[faceIndex];
        if (faceVertexCount < 0)
        {
            Logger::Err("USD mesh has a negative face-vertex count");
            return;
        }

        const std::size_t firstFaceIndex = combinedGeometry.m_indices.size();
        for (int faceCorner = 0; faceCorner < faceVertexCount; ++faceCorner)
        {
            if (cornerIndex >= indices.size())
            {
                Logger::Err("USD mesh topology has too few indices");
                return;
            }

            const int usdPointIndex = indices[cornerIndex]; // indices is faceVertexIndices
            if (usdPointIndex < 0 || static_cast<std::size_t>(usdPointIndex) >= vertices.size())
            {
                Logger::Err("USD mesh contains an invalid point index");
                return;
            }

            // The topology index selects the position. The normal index is
            // selected separately according to the normal interpolation mode.
            const std::size_t pointIndex = static_cast<std::size_t>(usdPointIndex);
            const std::optional<std::size_t> normalIndex = getElementIndex(normalsInterpolation, faceIndex, cornerIndex, pointIndex);
            if (!normalIndex || *normalIndex >= normals.size())
            {
                Logger::Err("Unsupported or invalid USD normal interpolation");
                return;
            }

            SimpleVertex vertex{};
            const pxr::GfVec3f& position = vertices[pointIndex];
            const pxr::GfVec3f& normal = normals[*normalIndex];
            vertex.position = Vector3f(position[0], position[1], position[2]);
            vertex.normal = Vector3f(normal[0], normal[1], normal[2]);
            vertex.color = Vector3f(1.0f, 1.0f, 1.0f);

            if (hasUvs)
            {
                const std::optional<std::size_t> uvIndex = getElementIndex(uvInterpolation, faceIndex, cornerIndex, pointIndex);
                if (!uvIndex || *uvIndex >= uvs.size())
                {
                    Logger::Err("Unsupported or invalid USD UV interpolation");
                    return;
                }

                vertex.uv_x = uvs[*uvIndex][0];
                // The renderer's texture convention has the opposite V axis
                // from these Blender-authored USD UVs.
                vertex.uv_y = 1.0f - uvs[*uvIndex][1];
            }
            // At this point we have a completed candidate SimpleVertex with all its fields populated


            // Reuse an existing vertex only when position ownership, normal,
            // and UV all match. Otherwise append a split vertex and index it.
            const VertexKey key
            {
                .pointIndex = pointIndex,
                .normalX = vertex.normal.x,
                .normalY = vertex.normal.y,
                .normalZ = vertex.normal.z,
                .uvX = vertex.uv_x,
                .uvY = vertex.uv_y
            };
            const uint32_t newVertexIndex = static_cast<uint32_t>(combinedGeometry.m_vertices.size());
            const auto [sharedVertex, inserted] = sharedVertices.try_emplace(key, newVertexIndex);
            if (inserted) // If this is truly a unique vertex, it gets inserted with a new associated index, and also added to the vertex buffer
            {
                combinedGeometry.m_vertices.push_back(vertex);
            }
            combinedGeometry.m_indices.push_back(sharedVertex->second); // Otherwise, we look up the index of the existing vertex and add it to the index buffer
            ++cornerIndex;
        }
        faceIndexRanges[faceIndex] = { .firstIndex = firstFaceIndex, .indexCount = combinedGeometry.m_indices.size() - firstFaceIndex };
    }

    // Every entry in faceVertexIndices must belong to exactly one face.
    if (cornerIndex != indices.size())
    {
        Logger::Err("USD mesh topology has too many indices");
        return;
    }


    // Materials: For a given material start at UsdPreviewSurface/Principled BSDF and discover all its connections (diffuse, normal, etc.). If those connections exist go load the textures and enforce gltf metallic-roughness format

    struct TextureConnection
    {
        // The UsdUVTexture node owns the image path. outputName records which
        // output socket (rgb, r, g, b, or a) feeds the surface input.
        pxr::UsdShadeShader shader;
        pxr::TfToken outputName;
    };

    // Follow a UsdPreviewSurface input through any intervening node-graph
    // connections until its value-producing UsdUVTexture is found
    // UsdPreviewSurface in most cases ~ Principled BSDF from Blender export
    auto findTextureConnection = [](const pxr::UsdShadeInput& input) -> std::optional<TextureConnection>
    { // input is diffuse, normal, etc.
        if (!input)
        {
            return std::nullopt;
        }

        for (const pxr::UsdAttribute& source : input.GetValueProducingAttributes(true)) // recursively follows a shader input’s connections to find the attributes that actually supply its value
        // ex: float inputs:metallic.connect = <ImageTexture.outputs:b> -> ImageTexture.outputs:b
        // source.GetPrim() -> the UsdUVTexture shader
        // source.GetBaseName() -> "b"
        {
            const pxr::UsdShadeShader candidate(source.GetPrim());
            pxr::TfToken shaderId;
            if (candidate && candidate.GetIdAttr().Get(&shaderId) && shaderId == pxr::TfToken("UsdUVTexture"))
            {
                return TextureConnection {
                    .shader = candidate, // def Shader "Image_Texture" { uniform token info:id = "UsdUVTexture" ... }
                    .outputName = source.GetBaseName() // for roughness this should be "g", metallic this should be "b"
                };
            }
        }
        return std::nullopt;
    };

    // Multiple material inputs and submeshes often reference the same image.
    // Reuse its TextureData so the decoded pixels are appended only once to
    // this StaticMeshData. TextureCache performs the later GPU-side reuse.
    std::unordered_map<std::string, TextureData> loadedTextures;
    auto loadTexture = [&staticMeshData, &loadedTextures](const TextureConnection& connection, const char* usage, TextureData& destination) -> bool
    { // usage = "diffuse", "normal", etc.
        pxr::SdfAssetPath textureAsset;
        const pxr::UsdShadeInput fileInput = connection.shader.GetInput(pxr::TfToken("file"));
        if (!fileInput || !fileInput.Get(&textureAsset))
        {
            Logger::Err(std::format("Could not read the {} texture asset path", usage));
            return false;
        }

        const std::string& resolvedPath = textureAsset.GetResolvedPath();
        if (resolvedPath.empty())
        {
            Logger::Err(std::format("Could not resolve {} texture '{}'", usage, textureAsset.GetAuthoredPath()));
            return false;
        }

        const auto previouslyLoaded = loadedTextures.find(resolvedPath);
        if (previouslyLoaded != loadedTextures.end())
        {
            destination = previouslyLoaded->second;
            return true;
        }

        // Normalize all material images to RGBA8. This also makes packed
        // material channels directly addressable by the renderer.
        constexpr int desiredChannels = 4;
        int textureWidth = 0;
        int textureHeight = 0;
        int sourceChannels = 0;
        stbi_uc* pixels = stbi_load(resolvedPath.c_str(), &textureWidth, &textureHeight, &sourceChannels, desiredChannels);
        if (!pixels)
        {
            Logger::Err(std::format("Could not load {} texture '{}': {}", usage, resolvedPath, stbi_failure_reason()));
            return false;
        }

        const std::size_t textureByteCount = static_cast<std::size_t>(textureWidth) * static_cast<std::size_t>(textureHeight) * desiredChannels;

        TextureData texture;
        texture.width = textureWidth;
        texture.height = textureHeight;
        texture.numChannels = desiredChannels;
        texture.baseTextureDataOffset = static_cast<int>(staticMeshData.textureData.size());
        texture.sourcePath = resolvedPath;

        staticMeshData.textureData.insert(staticMeshData.textureData.end(), pixels, pixels + textureByteCount);
        stbi_image_free(pixels);
        destination = texture;
        loadedTextures.emplace(resolvedPath, std::move(texture));
        return true;
    };

    // Resolve the material bound to a mesh or material subset (submesh), then translate
    // the supported UsdPreviewSurface inputs into the engine material layout.
    auto populateMaterial = [&findTextureConnection, &loadTexture](const pxr::UsdPrim& bindingPrim, MaterialData& materialData) -> bool
    {
        const pxr::UsdShadeMaterial material = pxr::UsdShadeMaterialBindingAPI(bindingPrim).ComputeBoundMaterial();
        if (!material)
        {
            Logger::Warn(std::format("USD prim '{}' has no bound material", bindingPrim.GetPath().GetString()));
            return true;
        }

        const pxr::UsdShadeShader surfaceShader = material.ComputeSurfaceSource(); // typically UsdPreviewSurface/Principled BSDF
        if (!surfaceShader)
        {
            Logger::Warn(std::format("USD material '{}' has no surface shader", material.GetPrim().GetPath().GetString()));
            return true;
        }

        const pxr::UsdShadeInput diffuseInput = surfaceShader.GetInput(pxr::TfToken("diffuseColor"));
        const std::optional<TextureConnection> diffuseConnection = findTextureConnection(diffuseInput);
        if (diffuseConnection)
        {
            if (!loadTexture(*diffuseConnection, "diffuse", materialData.diffuseData))
            {
                return false;
            }
        }
        else
        {
            Logger::Warn(std::format("USD material '{}' has no diffuse UsdUVTexture", material.GetPrim().GetPath().GetString()));
        }

        const pxr::UsdShadeInput normalInput = surfaceShader.GetInput(pxr::TfToken("normal"));
        if (const std::optional<TextureConnection> normalConnection = findTextureConnection(normalInput))
        {
            if (!loadTexture(*normalConnection, "normal", materialData.normalData))
            {
                return false;
            }
            // This importer flips Blender-authored V coordinates when creating
            // vertices, which also flips the tangent-space bitangent.
            materialData.normalYSign = -1.0f;
        }

        // When no metallic-roughness texture is connected we can fetch constants:
        const pxr::UsdShadeInput metallicInput = surfaceShader.GetInput(pxr::TfToken("metallic"));
        if (metallicInput)
        {
            metallicInput.Get(&materialData.metallicFactor);
        }
        const pxr::UsdShadeInput roughnessInput = surfaceShader.GetInput(pxr::TfToken("roughness"));
        if (roughnessInput)
        {
            roughnessInput.Get(&materialData.roughnessFactor);
        }

        const std::optional<TextureConnection> metallicConnection = findTextureConnection(metallicInput);
        const std::optional<TextureConnection> roughnessConnection = findTextureConnection(roughnessInput);
        if (metallicConnection || roughnessConnection)
        {
            // IMPORTANT: Importer assumes gltf metallic-roughness channel convention
            // Both surface inputs must come from the same UsdUVTexture node (i.e. same texture) with metallic in blue and
            // roughness in green. Separate images or other channel layouts are deliberately unsupported
            const bool followsMetallicRoughnessConvention =
                metallicConnection
                && roughnessConnection
                && metallicConnection->shader.GetPrim().GetPath() == roughnessConnection->shader.GetPrim().GetPath()
                && metallicConnection->outputName == pxr::TfToken("b")
                && roughnessConnection->outputName == pxr::TfToken("g");

            if (!followsMetallicRoughnessConvention)
            {
                Logger::Warn(std::format(
                    "USD material '{}' does not use one gltf metallic-roughness texture (B=metallic, G=roughness); ignoring its metallic/roughness texture connections",
                    material.GetPrim().GetPath().GetString()));
            }
            else if (!loadTexture(*metallicConnection, "metallic-roughness", materialData.metallicRoughnessData))
            {
                return false;
            }
            else
            {
                // When both conditions are true, this ensures multiplying by the sampled roughness/metallic texture remains unchanged
                // i.e. so we don't need to branch to check if the mesh is using the default (1.0f, 1.0f) texture or a real metallic-roughness texture, the behavior is the same in the end
                materialData.metallicFactor = 1.0f;
                materialData.roughnessFactor = 1.0f;
            }
        }
        return true;
    };

    // global = entire static mesh, local = submesh
    // partition each submesh's vertices/indices from the entire static mesh
    auto appendFaces = [&combinedGeometry, &faceIndexRanges](const pxr::VtArray<int>& faceIndices, SubMeshData& destination) -> bool
    {
        std::unordered_map<uint32_t, uint32_t> globalToLocal;

        for (const int usdFaceIndex : faceIndices)
        {
            if (usdFaceIndex < 0 || static_cast<std::size_t>(usdFaceIndex) >= faceIndexRanges.size())
            {
                return false;
            }

            const FaceIndexRange& range = faceIndexRanges[static_cast<std::size_t>(usdFaceIndex)];

            for (std::size_t index = range.firstIndex; index < range.firstIndex + range.indexCount; ++index) // index into the index buffer of the entire static mesh
            {
                const uint32_t globalVertexIndex = combinedGeometry.m_indices[index];
                const uint32_t newLocalVertexIndex = static_cast<uint32_t>(destination.m_vertices.size());
                const auto [localVertex, inserted] = globalToLocal.try_emplace(globalVertexIndex, newLocalVertexIndex); // Succeeds if globalVertexIndex is new, or in other words, a new never encountered vertex
                if (inserted)
                {
                    destination.m_vertices.push_back(combinedGeometry.m_vertices[globalVertexIndex]);
                }
                destination.m_indices.push_back(localVertex->second);
            }
        }

        return true;
    };

    pxr::UsdShadeMaterialBindingAPI meshBindingAPI(meshPrim);
    const std::vector<pxr::UsdGeomSubset> materialSubsets = meshBindingAPI.GetMaterialBindSubsets();

    // If there are no materials, we can just add the geometry and be done with this function
    if (materialSubsets.empty())
    {
        if (!populateMaterial(meshPrim, combinedGeometry.materialData))
        {
            return;
        }
        staticMeshData.m_transforms.emplace_back(); // Identity for now
        staticMeshData.m_subMeshes.push_back(std::move(combinedGeometry));
        return;
    }

    // Each subset corresponds to a submesh, this is where the main stuff happen
    for (const pxr::UsdGeomSubset& subset : materialSubsets)
    {
        pxr::TfToken elementType;
        if (!subset.GetElementTypeAttr().Get(&elementType) || elementType != pxr::UsdGeomTokens->face)
        {
            Logger::Warn(std::format("Ignoring non-face material subset '{}'", subset.GetPrim().GetPath().GetString()));
            continue;
        }

        pxr::VtArray<int> subsetFaceIndices;
        if (!subset.GetIndicesAttr().Get(&subsetFaceIndices) || subsetFaceIndices.empty())
        {
            Logger::Warn(std::format("Ignoring empty material subset '{}'", subset.GetPrim().GetPath().GetString()));
            continue;
        }

        SubMeshData subsetData;
        // HERE is where it actually all gets called: Receive vertices/indices, material data
        if (!appendFaces(subsetFaceIndices, subsetData) || !populateMaterial(subset.GetPrim(), subsetData.materialData))
        {
            return;
        }
        staticMeshData.m_transforms.emplace_back(); // Every material subset shares the Mesh transform.
        staticMeshData.m_subMeshes.push_back(std::move(subsetData));
    }

    // If there are remaining faces not included in a material subset, makes a new submesh just for them
    const pxr::VtArray<int> unassignedFaces = pxr::UsdGeomSubset::GetUnassignedIndices(entityMesh, pxr::UsdGeomTokens->face, pxr::UsdShadeTokens->materialBind);
    if (!unassignedFaces.empty())
    {
        SubMeshData fallbackData;
        if (!appendFaces(unassignedFaces, fallbackData) || !populateMaterial(meshPrim, fallbackData.materialData))
        {
            return;
        }
        staticMeshData.m_transforms.emplace_back();
        staticMeshData.m_subMeshes.push_back(std::move(fallbackData));
    }
}

}
