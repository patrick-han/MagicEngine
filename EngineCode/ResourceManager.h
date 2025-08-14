#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include "Renderable.h"
#include "Renderer.h" // TODO: move to .cpp file
#include "../EngineCode/Vulkan/Helpers.h" // TODO:

#include "Timing.h"
#include "JobSystem.h"
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
        auto start = std::chrono::steady_clock::now();
        ModelData* pModelData = new ModelData(Data::DeserializeModelData(filePath));
        Logger::Info(std::format("LoadModelFromDisk({}) = {} ms", name, since(start).count()));
        std::uint64_t newHandle;
        {
            std::scoped_lock lock(m_mutex);
            newHandle = m_nextAvailableHandle;
            m_handleToLoadedModelData[newHandle] = pModelData;
            m_nameToModelHandle[name] = newHandle;
            m_nextAvailableHandle++;
        }
        return newHandle;
    }

    bool UploadModel(const uint64_t handle)
    {
        auto start = std::chrono::steady_clock::now();

        ModelData* testModel = nullptr;
        {
            std::scoped_lock lock(m_mutex);
            if (m_handleToLoadedModelData.find(handle) != m_handleToLoadedModelData.end())
            {
                testModel = m_handleToLoadedModelData[handle];
            }
            else
            {
                return false;
            }
        }
        std::vector<int> renderableMeshArrayIndices; // Stores indices into m_renderableMeshes;
        size_t meshCounter = 0;

        std::vector<RenderableMesh> renderableMeshesForThisModel;
        renderableMeshesForThisModel.resize(testModel->m_meshes.size());

        struct ImageUploadJob
        {
            VkExtent3D extent;
            VkImageCreateInfo imageCreateInfo;
            VkFormat format;
            const unsigned char* imageData;
            int numChannels;
            size_t associatedMeshIndex;
        };
        std::mutex imageUploadJobsMutex;
        std::vector<ImageUploadJob> imageUploadJobs;

        for (const MeshData& meshData : testModel->m_meshes)
        {
            JobSystem::Execute([&renderableMeshesForThisModel, &meshData = std::as_const(meshData), this, meshCounter, &testModel = std::as_const(testModel), &imageUploadJobs, &imageUploadJobsMutex]()
            {
                RenderableMesh renderable;
                {
                    renderable.vertexBuffer = m_rctx->UploadBuffer(sizeof(SimpleVertex) * meshData.m_vertices.size(), meshData.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                    renderable.indexBuffer = m_rctx->UploadBuffer(sizeof(uint32_t) * meshData.m_indices.size(), meshData.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                    renderable.indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                    renderable.transform = testModel->m_transforms[meshCounter];

                    // Queue mesh texture jobs
                    if (meshData.materialData.diffuseData.data.size() != 0) // TODO:
                    {
                        {
                            VkExtent3D imageExtent {
                                .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                                , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                                , .depth = 1
                            };
                            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format
                            VkImageCreateInfo imci = TEMP_image_create_info(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
                            {
                                std::scoped_lock lock(imageUploadJobsMutex);
                                imageUploadJobs.emplace_back(imageExtent, imci, format, meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, meshCounter);
                            }
                        }
                    }
                }
                renderableMeshesForThisModel[meshCounter] = renderable; // This is safe because we reserved ahead of time and each thread only touches a single mesh
            });
            renderableMeshArrayIndices.push_back(m_renderableMeshes.size() + meshCounter); // We will extend the global renderableMeshes vector by this Model's renderableMeshVector, so we know the indices already
            meshCounter++;
        }
        JobSystem::Wait();
        Logger::Info(std::format("UploadModel() meshupload = {} ms", since(start).count()));

        // Upload all images
        auto startUploadImages = std::chrono::steady_clock::now();
        for (const ImageUploadJob& imageUploadJob : imageUploadJobs)
        {
            AllocatedImage diffuseTexture;
            diffuseTexture = m_rctx->UploadImage(imageUploadJob.imageData, imageUploadJob.numChannels, imageUploadJob.imageCreateInfo);

            VkImageViewCreateInfo imvci = DefaultImageViewCreateInfo(diffuseTexture.image, imageUploadJob.format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
            m_rctx->CreateImageViewForAllocatedImage(imvci, diffuseTexture);
            m_renderableImages.push_back(diffuseTexture);
            renderableMeshesForThisModel[imageUploadJob.associatedMeshIndex].diffuseTextureBindlessTextureArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(diffuseTexture); // TODO: temp
        }
        Logger::Info(std::format("UploadModel() imageupload = {} ms", since(startUploadImages).count()));


        m_renderableMeshes.insert(m_renderableMeshes.end(), renderableMeshesForThisModel.begin(), renderableMeshesForThisModel.end());
        Logger::Info(std::format("UploadModel() total = {} ms\n\n", since(start).count()));
        m_modelHandleToRenderableMeshIndices[handle] = std::move(renderableMeshArrayIndices);
        delete testModel;

        {
            std::scoped_lock lock(m_mutex);
            m_handleToLoadedModelData.erase(handle);
        }

        return true; // success
    }

    bool UploadModel(const std::string& name)
    {
        std::uint64_t handle;
        {
            std::scoped_lock lock(m_mutex);
            if (m_nameToModelHandle.find(name) != m_nameToModelHandle.end())
            {
                handle = m_nameToModelHandle[name];
            }
            else
            {
                return false;
            }
        }
        return UploadModel(handle);
    }

    const std::vector<int>& GetRenderableMeshIndices(const std::string& name)
    {
        std::uint64_t handle;
        {
            std::scoped_lock lock(m_mutex);
            handle = m_nameToModelHandle[name];
        }
        return m_modelHandleToRenderableMeshIndices[handle];
    }

private:
    // These data structures may be accessed by multiple threads
    std::mutex m_mutex;
    std::map<std::string, std::uint64_t> m_nameToModelHandle;
    std::map<std::uint64_t, ModelData*> m_handleToLoadedModelData;
    std::uint64_t m_nextAvailableHandle = 0;

public:
    [[nodiscard]] RenderableMesh GetRenderableMeshByIndex(size_t index) const
    {
        return m_renderableMeshes[index];
    }
    void DestroyAllAssets();
private:
    void DestroyAllLoadedModels()
    {
        std::scoped_lock lock(m_mutex);
        for (const auto& [key, value] : m_handleToLoadedModelData)
        {
            std::uint64_t handle = key;
            ModelData* data = value;
            m_handleToLoadedModelData.erase(handle);
        }
    }
    void DestroyAllGPUResidentMeshes();
    void DestroyAllGPUResidentTextures();
    Renderer* m_rctx;

    // These data structures should only be accessed by a single render thread
    std::map<std::uint64_t, std::vector<int>> m_modelHandleToRenderableMeshIndices;
    std::vector<RenderableMesh> m_renderableMeshes; // TODO: This probably doesn't belong here
    std::vector<AllocatedImage> m_renderableImages; // TODO: This probably doesn't belong here
};
}