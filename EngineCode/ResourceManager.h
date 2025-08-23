#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include <queue>
#include "Components/RenderableMeshComponent.h"
#include "Renderer.h" // TODO: move to .cpp file
#include "Common/Log.h"
#include "Timing.h"
#include "JobSystem.h"
#include "ECS.h"
#include "Components/TransformComponent.h"
namespace Magic
{

class ResourceManager
// class Renderer;
{
public:
    ResourceManager(Renderer* pRenderer, Registry* pRegistry)
    {
        Logger::Info("Initializing ResourceManager");
        m_rctx = pRenderer;
        m_ecs = pRegistry;
    }

    ~ResourceManager()
    {
        Logger::Info("Destroying ResourceManager");
    }


    /*
     * Loads the model from disk and returns a pointer to the data
     */
    void LoadModelFromDisk(const std::string& filePath, const std::string& name)
    {
        auto start = std::chrono::steady_clock::now();
        ModelData* pModelData = new ModelData(Data::DeserializeModelData(filePath));
        Logger::Info(std::format("LoadModelFromDisk({}) = {} ms", name, since(start).count()));
        {
            std::scoped_lock lock(m_loadedModelDataMutex);
            m_loadedModels[name] = pModelData;
        }
    }

    struct ModelUploadJob
    {
        std::string modelName;
    };

    struct BufferUploadJob
    {
        size_t numBytes = 0;
        const void* data = nullptr;
        VkBufferUsageFlagBits usage;
        Entity associatedEntity;
    };

    struct ImageUploadJob
    {
        VkExtent3D extent;
        VkImageCreateInfo imageCreateInfo;
        // VkFormat format;
        const unsigned char* imageData;
        int numChannels;
        Entity associatedEntity;
    };

    static const VkFormat g_defaultTextureFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format

    struct ImageUploadJobSynchronous
    {
        VkExtent3D extent;
        VkImageCreateInfo imageCreateInfo;
        VkFormat format;
        const unsigned char* imageData;
        int numChannels;
        size_t associatedMeshIndex;
    };

    std::vector<ModelUploadJob> m_pendingModelUploads;
    std::queue<BufferUploadJob> m_pendingBufferUploads;
    std::queue<ImageUploadJob> m_pendingImageUploads;

    void EnqueueUploadModel(const std::string& name)
    {
        m_pendingModelUploads.emplace_back(name);
    }

    void ProcessModelUploadJobs()
    {
        if (m_pendingModelUploads.empty())
        {
            return;
        }

        for (auto it = m_pendingModelUploads.begin(); it != m_pendingModelUploads.end();)
        {
            const ModelUploadJob& job = *it;

            ModelData* testModel = nullptr;
            {
                std::scoped_lock lock(m_loadedModelDataMutex);
                if (m_loadedModels.find(job.modelName) != m_loadedModels.end())
                {
                    testModel = m_loadedModels[job.modelName];
                }
                else
                {
                    ++it;
                    continue;
                }
            }

            std::vector<Entity> meshEntities;

            size_t meshCounter = 0;
            for (const MeshData& meshData : testModel->m_meshes)
            {
                Entity meshEntity = m_ecs->EnqueueCreateEntity();
                // We can go ahead and fill out some of the data now and wait for buffers later
                RenderableMeshComponent renderable;
                renderable.indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                renderable.transform = testModel->m_transforms[meshCounter];
                meshEntity.AddComponent<RenderableMeshComponent>(renderable);
                Matrix4f matrix;
                meshEntity.AddComponent<TransformComponent>(matrix); // TODO
                meshEntities.push_back(meshEntity);

                BufferUploadJob vertexBufferJob = {
                    .numBytes = sizeof(SimpleVertex) * meshData.m_vertices.size()
                    , .data = static_cast<const void*>(meshData.m_vertices.data())
                    , .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    , .associatedEntity = meshEntity
                };

                BufferUploadJob indexBufferJob = {
                    .numBytes = sizeof(uint32_t) * meshData.m_indices.size()
                    , .data = static_cast<const void*>(meshData.m_indices.data())
                    , .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                    , .associatedEntity = meshEntity
                };
                m_pendingBufferUploads.push(vertexBufferJob);
                m_pendingBufferUploads.push(indexBufferJob);

                if (meshData.materialData.diffuseData.data.size() != 0) // TODO:
                {
                    VkExtent3D imageExtent
                    {
                        .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                        , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                        , .depth = 1
                    };
                    const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);

                    m_pendingImageUploads.emplace(imageExtent, imci, /*format,*/ meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, meshEntity);
                }
            }
            it = m_pendingModelUploads.erase(it);
            m_renderableMeshes.insert(m_renderableMeshes.end(), meshEntities.begin(), meshEntities.end());
            // return meshEntities;
        }
    }

    void ProcessBufferUploadJobs()
    {
        if (m_pendingBufferUploads.empty())
        {
            return;
        }


        while(!m_pendingBufferUploads.empty())
        {
            auto job = m_pendingBufferUploads.front();
            m_pendingBufferUploads.pop();
            JobSystem::Execute([job, this](){
                AllocatedBuffer buffer = m_rctx->UploadBuffer(job.numBytes, job.data, job.usage);
                Entity meshEntity = job.associatedEntity;
                auto& renderable = meshEntity.GetComponent<RenderableMeshComponent>();
                if (job.usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                {
                    renderable.vertexBuffer = buffer;
                }
                else
                {
                    renderable.indexBuffer = buffer;
                }
                renderable.buffersReady = true;
            });
        }
    }

    void ProcessImageUploadJobs()
    {
        if (m_pendingImageUploads.empty())
        {
            return;
        }
        m_rctx->ResetAndBeginStreamingCommandBuffer();

        while(!m_pendingImageUploads.empty())
        {
            auto job = m_pendingImageUploads.front();
            auto& renderable = job.associatedEntity.GetComponent<RenderableMeshComponent>();
            m_pendingImageUploads.pop();
            renderable.diffuseImage = m_rctx->CreateGPUOnlyImage(job.imageCreateInfo);
            const auto extent = job.extent;
            constexpr size_t bytesPerChannel = 1;
            const size_t dataSize = extent.width * extent.height * job.numChannels * bytesPerChannel;
            AllocatedBuffer stagingBuffer = m_rctx->UploadBuffer(dataSize, job.imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            renderable.diffuseTextureStreamingTimelineReadyValue = m_rctx->EnqueueImageUploadJob(renderable.diffuseImage, stagingBuffer, extent);
            // m_rctx->DestroyBuffer(stagingBuffer); // TODO: Round robin staging buffers
        }

        m_rctx->EndAndSubmitStreamingCommandBuffer();
    }

    void PollImageUploadJobsFinishedAndUpdateRenderables()
    {
        uint64_t value = m_rctx->GetCurrentStreamingTimelineValue();
        for (auto& entity : m_renderableMeshes) // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
        {
            auto& renderable = entity.GetComponent<RenderableMeshComponent>();
            bool imageFinishedUploading = value >= renderable.diffuseTextureStreamingTimelineReadyValue;
            bool hasImage = renderable.diffuseImage.image != VK_NULL_HANDLE;
            if (imageFinishedUploading && (renderable.texturesReady == false) && hasImage)
            {
                auto imageViewCreateInfo = DefaultImageViewCreateInfo(renderable.diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                renderable.diffuseImage.view = m_rctx->CreateViewForAllocatedImage(imageViewCreateInfo);

                m_renderableImages.push_back(renderable.diffuseImage);
                renderable.diffuseTextureBindlessArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(renderable.diffuseImage);
                renderable.texturesReady = true;
            }
        }
    }


    // Synchronous upload, stalls entire frame!
    std::vector<Entity> UploadModel(const std::string& name)
    {
        ModelData* testModel = nullptr;
        if (m_loadedModels.find(name) != m_loadedModels.end())
        {
            testModel = m_loadedModels[name];
        }
        else
        {
            Logger::Err("Could not find Model name");
            std::abort();
        }
        
        std::map<Entity, ImageUploadJobSynchronous> imageUploadJobs;

        std::vector<Entity> meshEntities;

        size_t meshCounter = 0;
        for (const MeshData& meshData : testModel->m_meshes)
        {
            Entity meshEntity = m_ecs->EnqueueCreateEntity();
            
            RenderableMeshComponent renderable;
            {
                renderable.vertexBuffer = m_rctx->UploadBuffer(sizeof(SimpleVertex) * meshData.m_vertices.size(), meshData.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                renderable.indexBuffer = m_rctx->UploadBuffer(sizeof(uint32_t) * meshData.m_indices.size(), meshData.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                renderable.indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                renderable.transform = testModel->m_transforms[meshCounter];

                // Queue mesh texture jobs
                if (meshData.materialData.diffuseData.data.size() != 0) // TODO:
                {
                    VkExtent3D imageExtent 
                    {
                        .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                        , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                        , .depth = 1
                    };
                    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format
                    VkImageCreateInfo imci = DefaultImageCreateInfo(format, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
                    {
                        imageUploadJobs.emplace(std::make_pair(meshEntity, ImageUploadJobSynchronous(imageExtent, imci, format, meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, meshCounter)));
                    }
                }
            }
            meshEntity.AddComponent<RenderableMeshComponent>(std::move(renderable));
            Matrix4f matrix;
            meshEntity.AddComponent<TransformComponent>(matrix);
            meshEntities.push_back(meshEntity);
            meshCounter++;
        }

        m_renderableMeshes.insert(m_renderableMeshes.end(), meshEntities.begin(), meshEntities.end());

        // Upload all textures and assign them slots in the global bindless array
        for (const auto& [meshEntity, job] : imageUploadJobs)
        {
            AllocatedImage diffuseTexture= m_rctx->UploadImage(job.imageData, job.numChannels, job.imageCreateInfo);
            auto imageViewCreateInfo = DefaultImageViewCreateInfo(diffuseTexture.image, job.format, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
            diffuseTexture.view = m_rctx->CreateViewForAllocatedImage(imageViewCreateInfo);

            m_renderableImages.push_back(diffuseTexture);
            auto& renderableMesh = meshEntity.GetComponent<RenderableMeshComponent>();
            renderableMesh.diffuseTextureBindlessArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(diffuseTexture); // TODO: temp
        }

        for (auto& entity : meshEntities)
        {
            auto& renderable = entity.GetComponent<RenderableMeshComponent>();
            renderable.buffersReady = true;
            renderable.texturesReady = true;
        }

        return meshEntities;
    }

private:
    // These data structures may be accessed by multiple threads when loading from disk
    std::mutex m_loadedModelDataMutex;
    std::map<std::string, ModelData*> m_loadedModels;

public:
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
        std::scoped_lock lock(m_loadedModelDataMutex);
        for (auto& [name, ptr] : m_loadedModels)
        {
            delete ptr;
        }
        m_loadedModels.clear();
    }
    void DestroyAllGPUResidentMeshes()
    {
        Logger::Info("ResourceManager: Destroying all meshes");
        for (const Entity& meshEntity : m_renderableMeshes)
        {
            const auto& renderable = meshEntity.GetComponent<RenderableMeshComponent>();
            m_rctx->DestroyBuffer(renderable.vertexBuffer);
            m_rctx->DestroyBuffer(renderable.indexBuffer);
        }
    }
    void DestroyAllGPUResidentTextures()
    {
        Logger::Info("ResourceManager: Destroying all textures");
        for (const AllocatedImage& image : m_renderableImages)
        {
            m_rctx->DestroyImage(image);
        }
    }
    Renderer* m_rctx;
    Registry* m_ecs;

    // These data structures should only be accessed by a single render thread
    std::vector<Entity> m_renderableMeshes;
    std::vector<AllocatedImage> m_renderableImages;
};
}