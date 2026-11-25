#include "MeshCache.h"
#include "GLTFImporter.h"
#include "Renderer.h"
#include "../CommonCode/StaticMeshData.h"
#include "../CommonCode/Log.h"
#include "../CommonCode/StaticMeshData.h"

namespace Magic
{

MeshCache* GMeshCache = nullptr;

CachedMesh MeshCache::GetOrUpload(const std::filesystem::path &filePath)
{
    std::lock_guard lock(m_mutex);
    std::string filePathStr = filePath.string();

    const auto cachedMesh = m_meshes.find(filePathStr);
    if (cachedMesh != m_meshes.end())
    {
        Logger::Info(std::format("Reusing mesh '{}'", filePathStr));
        return cachedMesh->second;
    }

    StaticMeshData staticMeshData;
    GLTFImporterNew importer;
    importer.ImportGLTFAsStaticMesh(filePath, staticMeshData);
    if (staticMeshData.m_subMeshes.empty())
    {
        return {};
    }

    std::vector<CachedSubMesh> subMeshes;

    std::size_t subMesh_i = 0;
    for (const SubMeshData& subMeshData : staticMeshData.m_subMeshes)
    {
        CachedSubMesh subMesh;
        subMesh.indexCount = static_cast<uint32_t>(subMeshData.m_indices.size());
        subMesh.transform = staticMeshData.m_transforms[subMesh_i];
        // calculate aabb, TODO: this can be spun off into a separate job, or better yet done in the cooker
        // keep this AABB in mesh-local space. The renderer transforms it to world space with the same model matrix used to draw the mesh

        // Since each submesh contains the entire vertex buffer, we need to calculate the aabb based only on its subset
        for (const uint32_t vertexIndex : subMeshData.m_indices)
        {
            subMesh.aabb.Update(subMeshData.m_vertices[vertexIndex].position);
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

        subMesh.vertexBuffer = vertexBuffer;
        subMesh.indexBuffer = indexBuffer;

        // Textures
        // auto uploadTexture = [&staticMeshData](const TextureData& texture, int defaultBindlessSlot, TextureType textureType) -> int
        // {
        //     if (texture.width == 0)
        //     {
        //         return defaultBindlessSlot;
        //     }
        //     const unsigned char* pixels = staticMeshData.textureData.data() + texture.baseTextureDataOffset;
        //     return GTextureCache->GetOrUpload(texture.sourcePath, texture, pixels, textureType);
        // };

        // const MaterialData& materialData = subMeshData.materialData;
        // subMesh.diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
        // subMesh.normalTextureBindlessArraySlot = DefaultTexture::g_flatNormalTextureImageBindlessSlot;
        // subMesh.metallicRoughnessTextureBindlessArraySlot = DefaultTexture::g_whiteTextureImageBindlessSlot;
        // subMesh.metallicFactor = 1.0f;
        // subMesh.roughnessFactor = 1.0f;
        // subMesh.normalYSign = 1.0f;


        // Finalize
        subMeshes.push_back(subMesh);
        subMesh_i++;
    }
    CachedMesh newCachedMesh = { subMeshes };
    m_meshes.emplace(filePathStr, newCachedMesh);

    m_cachedMeshCount++;
    return newCachedMesh;
}

int MeshCache::GetCachedMeshCount()
{
    std::lock_guard lock(m_mutex);
    return m_cachedMeshCount;
}

void MeshCache::Destroy()
{
    std::lock_guard lock(m_mutex);
    for (const auto& [filePath, cachedMesh] : m_meshes)
    {
        for (const auto& cachedSubMesh : cachedMesh.subMeshes)
        {
            GRenderer->DestroyBuffer(cachedSubMesh.vertexBuffer);
            GRenderer->DestroyBuffer(cachedSubMesh.indexBuffer);
        }
    }
    m_cachedMeshCount = 0;
    m_meshes.clear();
}

}
