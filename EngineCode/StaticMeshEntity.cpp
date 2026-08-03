#include "StaticMeshEntity.h"
#include "USDImporter.h"
#include "TextureCache.h"
#include "MemoryManager.h"
#include "../CommonCode/StaticMeshData.h"
#include "Renderer.h"
#include "DefaultTexture.h"

#include <pxr/usd/usdGeom/xformCache.h>
#include "ThirdParty/imgui/imgui.h"
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
    importer.ImportUSDPrimAsStaticMesh(entityPrim, staticMeshData);
    if (staticMeshData.m_subMeshes.empty())
    {
        return false;
    }

    Logger::Info(std::format("Load: {}", entityName));

    std::size_t subMesh_i = 0;
    for (const SubMeshData& subMeshData : staticMeshData.m_subMeshes)
    {
        SubMesh* pSubMesh = GMemoryManager->New<SubMesh>();
        pSubMesh->indexCount = static_cast<uint32_t>(subMeshData.m_indices.size());
        pSubMesh->m_transform = staticMeshData.m_transforms[subMesh_i];
        // calculate aabb, TODO: this can be spun off into a separate job, or better yet done in the cooker
        // keep this AABB in mesh-local space. The renderer transforms it to world space with the same model matrix used to draw the mesh

        // Since each submesh contains the entire vertex buffer, we need to calculate the aabb based only on its subset
        for (const uint32_t vertexIndex : subMeshData.m_indices)
        {
            pSubMesh->aabb.Update(subMeshData.m_vertices[vertexIndex].position);
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
        auto uploadTexture = [&staticMeshData](const TextureData& texture, int defaultBindlessSlot, TextureType textureType) -> int
        {
            if (texture.width == 0)
            {
                return defaultBindlessSlot;
            }
            const unsigned char* pixels = staticMeshData.textureData.data() + texture.baseTextureDataOffset;
            return GTextureCache->GetOrUpload(texture.sourcePath, texture, pixels, textureType);
        };

        const MaterialData& materialData = subMeshData.materialData;
        pSubMesh->diffuseTextureBindlessArraySlot = uploadTexture(materialData.diffuseData, DefaultTexture::g_defaultTextureImageBindlessSlot, TextureType::Color);
        pSubMesh->normalTextureBindlessArraySlot = uploadTexture(materialData.normalData, DefaultTexture::g_flatNormalTextureImageBindlessSlot, TextureType::Data);
        pSubMesh->metallicRoughnessTextureBindlessArraySlot = uploadTexture(materialData.metallicRoughnessData, DefaultTexture::g_whiteTextureImageBindlessSlot, TextureType::Data);
        pSubMesh->metallicFactor = materialData.metallicFactor;
        pSubMesh->roughnessFactor = materialData.roughnessFactor;
        pSubMesh->normalYSign = materialData.normalYSign;


        // Finalize
        m_subMeshes.push_back(pSubMesh);
        subMesh_i++;
        Logger::Info(std::format("{} submesh, vertexCount: {}, indexCount: {}", entityName, subMeshData.m_vertices.size(), subMeshData.m_indices.size()));
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

void StaticMeshEntity::DrawInspectorElements()
{
    ImGui::Text("SubMesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(1,1,1,1), "%zu", m_subMeshes.size());
    ImGui::Separator(); 
}
}
