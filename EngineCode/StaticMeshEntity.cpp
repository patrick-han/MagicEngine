#include "StaticMeshEntity.h"
#include "USDImporter.h"
#include "TextureCache.h"
#include "MemoryManager.h"
#include "../CommonCode/StaticMeshData.h"
#include "Renderer.h"
#include "DefaultTexture.h"

#include <pxr/usd/usdGeom/xformCache.h>

namespace Magic
{
StaticMeshEntity::StaticMeshEntity() 
    : IEntity()
{

}

StaticMeshEntity::~StaticMeshEntity()
{

}
bool StaticMeshEntity::Load(const pxr::UsdPrim& entityPrim)
{
    // Top level properties
    const std::string entityName = entityPrim.GetName().GetString();
    UUID entityUUID; // TODO: embed entities in Blender via a plugin? this is just a random runtime entity for now

    pxr::UsdGeomXformCache cache;
    const pxr::GfMatrix4d worldTransform = cache.GetLocalToWorldTransform(entityPrim);

    auto GfMatrix4dToMatrix4f = [](const pxr::GfMatrix4d& usdMatrix) -> Matrix4f {
        Matrix4f result;
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                // USD uses row vectors; MagicEngine uses column vectors.
                result(row, column) = static_cast<float>(usdMatrix[column][row]);
            }
        }
        return result;
    };

    SetName(entityName.c_str());
    SetUUID(entityUUID);
    m_transform = GfMatrix4dToMatrix4f(worldTransform);

    StaticMeshData staticMeshData;
    USDImporter importer;
    importer.ImportUSDPrim(entityPrim, staticMeshData);
    if (staticMeshData.m_subMeshes.empty())
    {
        return false;
    }

    std::size_t subMesh_i = 0;
    for (const SubMeshData& subMeshData : staticMeshData.m_subMeshes)
    {
        SubMesh* pSubMesh = GMemoryManager->New<SubMesh>();
        pSubMesh->indexCount = static_cast<uint32_t>(subMeshData.m_indices.size());
        pSubMesh->m_transform = staticMeshData.m_transforms[subMesh_i];
        // calculate aabb, TODO: this can be spun off into a separate job, or better yet done in the cooker
        // keep this AABB in mesh-local space. The renderer transforms it to world space with the same model matrix used to draw the mesh
        for (const auto& vertex : subMeshData.m_vertices)
        {
            pSubMesh->aabb.Update(vertex.position);
        }
        // Vertex and Index buffers
        AllocatedBuffer vertexBuffer = GRenderer->UploadBuffer(
            sizeof(SimpleVertex) * subMeshData.m_vertices.size()
            , static_cast<const void*>(subMeshData.m_vertices.data())
            , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        AllocatedBuffer indexBuffer = GRenderer->UploadBuffer(
            sizeof(uint32_t) * subMeshData.m_indices.size()
            , static_cast<const void*>(subMeshData.m_indices.data())
            , VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        pSubMesh->vertexBuffer = vertexBuffer;
        pSubMesh->indexBuffer = indexBuffer;

        // Textures
        const TextureData& diffuseTexture = subMeshData.materialData.diffuseData;
        pSubMesh->hasTexture = diffuseTexture.width != 0;
        if (pSubMesh->hasTexture)
        {
            const unsigned char* pixels = staticMeshData.textureData.data() + diffuseTexture.baseTextureDataOffset;
            pSubMesh->diffuseTextureBindlessArraySlot = GTextureCache->GetOrUpload(diffuseTexture.sourcePath, diffuseTexture, pixels);
        }
        else
        {
            pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
        }


        // Finalize
        m_subMeshes.push_back(pSubMesh);
        subMesh_i++;
    }
    return true;
}
bool StaticMeshEntity::Unload()
{
    for (SubMesh* pSubMesh : m_subMeshes)
    {
        GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
        GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
        GMemoryManager->Delete<SubMesh>(pSubMesh);
    }
    return true;
}

std::span<SubMesh* const> StaticMeshEntity::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}

}
