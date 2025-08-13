#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include "Renderable.h"
#include "Renderer.h" // TODO: move to .cpp file
#include "../EngineCode/Vulkan/Helpers.h" // TODO:
namespace Magic
{
class ResourceManager
// class Renderer;
{
public:
    ResourceManager(Renderer* pRenderer);
    ~ResourceManager();

    /*
     * Loads the model from disk and returns a pointer to the data
     */
    std::uint64_t LoadModelFromDisk(const std::string& filePath, const std::string& name)
    {
        ModelData* pModelData = new ModelData(Data::DeserializeModelData(filePath));
        std::uint64_t newHandle;
        {
            std::scoped_lock(m_mutex);
            newHandle = m_nextAvailableHandle;
            m_modelHandleToData[newHandle] = pModelData;
            m_modelNameToHandle[name] = newHandle;
            m_nextAvailableHandle++;
        }
        return newHandle;
    }

    void UploadModel(const uint64_t handle)
    {
        ModelData* testModel = nullptr;
        {
            std::scoped_lock(m_mutex);
            testModel = m_modelHandleToData[handle];
        }
        std::vector<int> renderableMeshArrayIndices; // Stores indices into m_renderableMeshes;
        size_t meshCounter = 0;
        for (const MeshData& meshData : testModel->m_meshes)
        {
            RenderableMesh renderable
            {
                .vertexBuffer = m_rctx->UploadBuffer(sizeof(SimpleVertex) * meshData.m_vertices.size(), meshData.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                , .indexBuffer = m_rctx->UploadBuffer(sizeof(uint32_t) * meshData.m_indices.size(), meshData.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                , .indexCount = static_cast<uint32_t>(meshData.m_indices.size())
                , .transform = testModel->m_transforms[meshCounter]
            };

            // Load mesh texture(s)
            if (meshData.materialData.diffuseData.data.size() != 0) // TODO:
            {
                VkExtent3D imageExtent {
                    .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                    , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                    , .depth = 1
                };
                VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format
                VkImageCreateInfo imci = TEMP_image_create_info(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
                AllocatedImage diffuseTexture = m_rctx->UploadImage(meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, imci);
                VkImageViewCreateInfo imvci = DefaultImageViewCreateInfo(diffuseTexture.image, format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                m_rctx->TEMP_CreateImageViewForAllocatedImage(imvci, diffuseTexture);
                m_renderableImages.push_back(diffuseTexture);

                renderable.diffuseTextureBindlessTextureArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(diffuseTexture); // TODO: temp
            }

            renderableMeshArrayIndices.push_back(m_renderableMeshes.size());
            m_renderableMeshes.push_back(renderable);
            meshCounter++;
        }
        m_modelHandleToRenderableMeshIndices[handle] = std::move(renderableMeshArrayIndices);
        delete testModel;

        {
            std::scoped_lock(m_mutex);
            m_modelHandleToData.erase(handle);
        }
    }

    void UploadModel(const std::string& name)
    {
        std::uint64_t handle;
        {
            std::scoped_lock(m_mutex);
            handle = m_modelNameToHandle[name];
        }
        UploadModel(handle);
    }

    const std::vector<int>& GetRenderableMeshIndices(std::uint64_t handle)
    {
        return m_modelHandleToRenderableMeshIndices[handle];
    }

private:
    // These data structures may be accessed by multiple threads
    std::mutex m_mutex;
    std::map<std::string, std::uint64_t> m_modelNameToHandle;
    std::map<std::uint64_t, ModelData*> m_modelHandleToData;
    std::uint64_t m_nextAvailableHandle = 0;

public:
    [[nodiscard]] RenderableMesh GetRenderableMeshByIndex(size_t index) const
    {
        return m_renderableMeshes[index];
    }
    void DestroyAllAssets();
private:
    void DestroyAllGPUResidentMeshes();
    void DestroyAllGPUResidentTextures();
    Renderer* m_rctx;

    // These data structures should only be accessed by a single render thread
    std::map<std::uint64_t, std::vector<int>> m_modelHandleToRenderableMeshIndices;
    std::vector<RenderableMesh> m_renderableMeshes; // TODO: This probably doesn't belong here
    std::vector<AllocatedImage> m_renderableImages; // TODO: This probably doesn't belong here
};
}