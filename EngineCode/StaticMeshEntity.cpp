#include "StaticMeshEntity.h"
#include "GLTFImporter.h"
#include "TextureCache.h"
#include "MeshCache.h"
#include "MemoryManager.h"
#include "StaticMeshData.h"
#include "Renderer.h"
#include "DefaultTexture.h"
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

bool StaticMeshEntity::Load(const vjson::Object* meshComponent)
{
    // Top level properties
    // const std::string entityName = meshComponent->StringAtKey("name", "?");
#if !MAGIC_USE_SIMPLE_UUID
    UUID entityUUID; // TODO: embed entities in Blender via a plugin? this is just a random runtime entity for now
#endif // !MAGIC_USE_SIMPLE_UUID


#if !MAGIC_USE_SIMPLE_UUID
    SetUUID(entityUUID);
#endif // !MAGIC_USE_SIMPLE_UUID

    // StaticMeshData staticMeshData;
    // GLTFImporterNew importer;
    // std::filesystem::path filePath = std::filesystem::path(meshComponent->StringAtKey("mesh_file", "?"));
    // importer.ImportGLTFAsStaticMesh(filePath, staticMeshData);

    // if (staticMeshData.m_subMeshes.empty())
    // {
    //     return false;
    // }
    std::filesystem::path filePath = std::filesystem::path(meshComponent->StringAtKey("mesh_file", "?"));
    CachedMesh cachedMesh = GMeshCache->GetOrUpload(filePath);
    if (cachedMesh.subMeshes.size() == 0)
    {
        Logger::Err("Failed to get or load cached mesh");
        return false;
    }

    std::size_t subMesh_i = 0;
    for (const CachedSubMesh& cachedSubMesh : cachedMesh.subMeshes)
    {
        SubMesh* pSubMesh = GMemoryManager->New<SubMesh>();
        pSubMesh->indexCount = cachedSubMesh.indexCount;
        pSubMesh->m_transform = cachedSubMesh.transform;
        pSubMesh->aabb = cachedSubMesh.aabb;
        pSubMesh->vertexBuffer = cachedSubMesh.vertexBuffer;
        pSubMesh->indexBuffer = cachedSubMesh.indexBuffer;
        pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
        pSubMesh->normalTextureBindlessArraySlot = DefaultTexture::g_flatNormalTextureImageBindlessSlot;
        pSubMesh->metallicRoughnessTextureBindlessArraySlot = DefaultTexture::g_whiteTextureImageBindlessSlot;
        pSubMesh->metallicFactor = 1.0f;
        pSubMesh->roughnessFactor = 1.0f;
        pSubMesh->normalYSign = 1.0f;
        // Finalize
        m_subMeshes.push_back(pSubMesh);
        subMesh_i++;
        //Logger::Info(std::format("{} submesh, vertexCount: {}, indexCount: {}", entityName, subMeshData.m_vertices.size(), subMeshData.m_indices.size()));
    }
    return true;
}

bool StaticMeshEntity::Unload()
{
    for (SubMesh* pSubMesh : m_subMeshes)
    {
        // GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
        // GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
        GMemoryManager->Delete<SubMesh>(pSubMesh);
    }
    return true;
}

std::span<SubMesh* const> StaticMeshEntity::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}

void StaticMeshEntity::DrawInspectorElementsSpecific()
{
    ImGui::Text("SubMesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(1,1,1,1), "%zu", m_subMeshes.size());
    ImGui::Separator(); 
}
}
