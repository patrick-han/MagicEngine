#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include "Renderable.h"
#include "Renderer.h" // TODO: move to .cpp file
#include "Common/Log.h"
#include "Timing.h"
#include "JobSystem.h"
namespace Magic
{

using ResourceHandle = std::uint64_t;

class ResourceManager
// class Renderer;
{
public:
    ResourceManager(Renderer* pRenderer)
    {
        Logger::Info("Initializing AssetManager");
        m_rctx = pRenderer;
    }

    ~ResourceManager()
    {
        Logger::Info("Destroying AssetManager");
    }


    /*
     * Loads the model from disk and returns a pointer to the data
     */
    ResourceHandle LoadModelFromDisk(const std::string& filePath, const std::string& name)
    {
        auto start = std::chrono::steady_clock::now();
        ModelData* pModelData = new ModelData(Data::DeserializeModelData(filePath));
        Logger::Info(std::format("LoadModelFromDisk({}) = {} ms", name, since(start).count()));
        ResourceHandle newHandle = m_nextAvailableHandle.load();
        m_nextAvailableHandle.fetch_add(1);
        {
            std::scoped_lock lock(m_mutex);
            m_loadedModels[newHandle] = pModelData;
            m_nameToModelHandle[name] = newHandle;
        }
        return newHandle;
    }

    bool UploadModel(const ResourceHandle handle)
    {
        auto start = std::chrono::steady_clock::now();

        ModelData* testModel = nullptr;
        {
            std::scoped_lock lock(m_mutex);
            if (m_loadedModels.find(handle) != m_loadedModels.end())
            {
                testModel = m_loadedModels[handle];
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
                            VkImageCreateInfo imci = DefaultImageCreateInfo(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
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
        m_handleToRenderableMeshIndices[handle] = std::move(renderableMeshArrayIndices);
        return true; // success
    }

    std::optional<ResourceHandle> GetResourceHandleByName(const std::string& name)
    {
        std::scoped_lock lock(m_mutex);
        if (auto it = m_nameToModelHandle.find(name); it != m_nameToModelHandle.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    const std::vector<int>& GetRenderableMeshIndicesByHandle(const ResourceHandle handle)
    {
        return m_handleToRenderableMeshIndices[handle];
    }

private:
    // These data structures may be accessed by multiple threads
    std::mutex m_mutex;
    std::map<std::string, ResourceHandle> m_nameToModelHandle;
    std::map<ResourceHandle, ModelData*> m_loadedModels;
    std::atomic<ResourceHandle> m_nextAvailableHandle = 0;

public:
    [[nodiscard]] RenderableMesh GetRenderableMeshByIndex(size_t index) const
    {
        return m_renderableMeshes[index];
    }
    void DestroyAllAssets()
    {
        DestroyAllLoadedModels();
        m_rctx->WaitIdle();
        DestroyAllGPUResidentMeshes();
        DestroyAllGPUResidentTextures();
    }
private:
    void DestroyAllLoadedModels()
    {
        std::scoped_lock lock(m_mutex);
        for (auto& [handle, ptr] : m_loadedModels)
        {
            delete ptr;
        }
        m_loadedModels.clear();
        m_nameToModelHandle.clear(); // likely also want to clear name map
    }
    void DestroyAllGPUResidentMeshes()
    {
        Logger::Info("AssetManager: Destroying all meshes");
        for (const RenderableMesh& renderable : m_renderableMeshes)
        {
            m_rctx->DestroyBuffer(renderable.vertexBuffer);
            m_rctx->DestroyBuffer(renderable.indexBuffer);
        }
    }
    void DestroyAllGPUResidentTextures()
    {
        Logger::Info("AssetManager: Destroying all textures");
        for (const AllocatedImage& image : m_renderableImages)
        {
            m_rctx->DestroyImage(image);
        }
    }
    Renderer* m_rctx;

    // These data structures should only be accessed by a single render thread
    std::map<ResourceHandle, std::vector<int>> m_handleToRenderableMeshIndices;
    std::vector<RenderableMesh> m_renderableMeshes;
    std::vector<AllocatedImage> m_renderableImages;
};
}