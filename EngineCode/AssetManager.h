#pragma once

#include "../DataLibCode/Data.h"
#include <vector>
#include "Renderable.h"

#include "Renderer.h" // TODO: move to .cpp file

namespace Magic
{
class AssetManager
// class Renderer;
{
public:
    AssetManager(Renderer* pRenderer);
    ~AssetManager();
    std::vector<int> LoadModel(const std::string& filePath) // Should this return a list of Renderable components instead? make distinguishment between submesh/mesh?
    {
        ModelData testModel = Data::DeserializeModelData(filePath);
        std::vector<int> renderableMeshArrayIndices; // Stores indices into m_renderableMeshes;
        size_t meshCounter = 0;
        for (const MeshData& meshData : testModel.m_meshes)
        {
            RenderableMesh renderable;
            renderable.vertexBuffer = m_rctx->UploadBuffer(sizeof(SimpleVertex) * meshData.m_vertices.size(), meshData.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            renderable.indexBuffer = m_rctx->UploadBuffer(sizeof(uint32_t) * meshData.m_indices.size(), meshData.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            renderable.indexCount = meshData.m_indices.size();
            renderable.transform = testModel.m_transforms[meshCounter];

            renderableMeshArrayIndices.push_back(m_renderableMeshes.size());
            m_renderableMeshes.push_back(renderable);
            meshCounter++;
        }
        return renderableMeshArrayIndices;
    }
    [[nodiscard]] RenderableMesh GetRenderableMeshByIndex(size_t index) const
    {
        return m_renderableMeshes[index];
    }
    void DestroyAllAssets();
private:
    void DestroyAllMeshes();
    Renderer* m_rctx;
    std::vector<RenderableMesh> m_renderableMeshes; // TODO: This probably doesn't belong here
};
}