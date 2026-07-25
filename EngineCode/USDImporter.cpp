#include "USDImporter.h"

#include "../CommonCode/Log.h"
#include "../CommonCode/StaticMeshData.h"
#include "../DataLibCode/stb_image.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>

#include <optional>
#include <unordered_map>

namespace Magic
{

void USDImporter::ImportUSDPrim(
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

    SubMeshData subMeshData;
    // In the worst case every face corner has a distinct normal or UV, so the
    // final engine vertex count can be as large as the USD index count.
    subMeshData.m_vertices.reserve(indices.size());
    subMeshData.m_indices.reserve(indices.size());

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
            const uint32_t newVertexIndex = static_cast<uint32_t>(subMeshData.m_vertices.size());
            const auto [sharedVertex, inserted] = sharedVertices.try_emplace(key, newVertexIndex);
            if (inserted) // If this is truly a unique vertex, it gets inserted with a new associated index, and also added to the vertex buffer
            {
                subMeshData.m_vertices.push_back(vertex);
            }
            subMeshData.m_indices.push_back(sharedVertex->second); // Otherwise, we look up the index of the existing vertex and add it to the index buffer
            ++cornerIndex;
        }
    }

    // Every entry in faceVertexIndices must belong to exactly one face.
    if (cornerIndex != indices.size())
    {
        Logger::Err("USD mesh topology has too many indices");
        return;
    }

    // Material
    const pxr::UsdShadeMaterial material = pxr::UsdShadeMaterialBindingAPI(meshPrim).ComputeBoundMaterial();
    if (material)
    {
        const pxr::UsdShadeShader surfaceShader = material.ComputeSurfaceSource(); // typically UsdPreviewSurface
        if (!surfaceShader)
        {
            Logger::Warn(std::format("USD material '{}' has no surface shader", material.GetPrim().GetPath().GetString()));
        }
        else
        {
            const pxr::UsdShadeInput diffuseInput = surfaceShader.GetInput(pxr::TfToken("diffuseColor"));
            pxr::UsdShadeShader textureShader;
            if (diffuseInput)
            {
                for (const pxr::UsdAttribute& source : diffuseInput.GetValueProducingAttributes(true))
                {
                    const pxr::UsdShadeShader candidate(source.GetPrim());
                    pxr::TfToken shaderId;
                    if (candidate && candidate.GetIdAttr().Get(&shaderId) && shaderId == pxr::TfToken("UsdUVTexture"))
                    {
                        textureShader = candidate;
                        break;
                    }
                }
            }

            if (!textureShader)
            {
                Logger::Warn(std::format("USD material '{}' has no diffuse UsdUVTexture", material.GetPrim().GetPath().GetString()));
            }
            else
            {
                pxr::SdfAssetPath textureAsset;
                const pxr::UsdShadeInput fileInput = textureShader.GetInput(pxr::TfToken("file"));
                if (!fileInput || !fileInput.Get(&textureAsset))
                {
                    Logger::Err("Could not read the diffuse texture asset path");
                    return;
                }

                const std::string& resolvedPath = textureAsset.GetResolvedPath();
                if (resolvedPath.empty())
                {
                    Logger::Err(std::format("Could not resolve diffuse texture '{}'", textureAsset.GetAuthoredPath()));
                    return;
                }

                constexpr int desiredChannels = 4;
                int textureWidth = 0;
                int textureHeight = 0;
                int sourceChannels = 0;
                stbi_uc* pixels = stbi_load(resolvedPath.c_str(), &textureWidth, &textureHeight, &sourceChannels, desiredChannels);
                if (!pixels)
                {
                    Logger::Err(std::format("Could not load diffuse texture '{}': {}", resolvedPath, stbi_failure_reason()));
                    return;
                }

                const std::size_t textureByteCount = static_cast<std::size_t>(textureWidth) * static_cast<std::size_t>(textureHeight) * desiredChannels;

                TextureData diffuseTexture;
                diffuseTexture.width = textureWidth;
                diffuseTexture.height = textureHeight;
                diffuseTexture.numChannels = desiredChannels;
                diffuseTexture.baseTextureDataOffset = static_cast<int>(staticMeshData.textureData.size());

                staticMeshData.textureData.insert(staticMeshData.textureData.end(), pixels, pixels + textureByteCount);
                stbi_image_free(pixels);

                subMeshData.materialData.diffuseData = diffuseTexture;
            }
        }
    }
    else
    {
        Logger::Warn(std::format("USD mesh '{}' has no bound material",meshPrim.GetPath().GetString()));
    }

    staticMeshData.m_transforms.emplace_back(); // Identity for now
    staticMeshData.m_subMeshes.push_back(std::move(subMeshData));
}

}
