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


#include "World.h"
#include "Entity.h"
namespace Magic
{

class ResourceManager
// class Renderer;
{
public:
    ResourceManager(Renderer* pRenderer, ECS::Registry* pRegistry, World* pWorld)
    {
        Logger::Info("Initializing ResourceManager");
        m_rctx = pRenderer;
        m_ecs = pRegistry;
        m_pWorld = pWorld;
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
        // ECS::Entity associatedEntity;
        SubMesh* pAssociatedSubMesh;
    };

    struct ImageUploadJob
    {
        VkExtent3D extent;
        VkImageCreateInfo imageCreateInfo;
        // VkFormat format;
        const unsigned char* imageData;
        int numChannels;
        // ECS::Entity associatedEntity;
        SubMesh* pAssociatedSubMesh;
    };

    static const VkFormat g_defaultTextureFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format

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

            // std::vector<ECS::Entity> meshEntities;
            MeshEntity* pMeshEntity = m_pWorld->CreateMeshEntity();

            size_t meshCounter = 0;
            for (const MeshData& meshData : testModel->m_meshes)
            {
                // ECS::Entity meshEntity = m_ecs->EnqueueCreateEntity();
                // // We can go ahead and fill out some of the data now and wait for buffers later
                // RenderableMeshComponent renderable;
                // renderable.indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                // renderable.transform = testModel->m_transforms[meshCounter];
                // meshEntity.AddComponent<RenderableMeshComponent>(renderable);
                // Matrix4f matrix;
                // meshEntity.AddComponent<TransformComponent>(matrix); // TODO
                // meshEntities.push_back(meshEntity);
                SubMesh* pSubMesh = pMeshEntity->AddSubMesh();

                pSubMesh->indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                pSubMesh->m_localMatrix = testModel->m_transforms[meshCounter];
                Matrix4f matrix;
                pSubMesh->m_worldMatrix = matrix;

                BufferUploadJob vertexBufferJob = {
                    .numBytes = sizeof(SimpleVertex) * meshData.m_vertices.size()
                    , .data = static_cast<const void*>(meshData.m_vertices.data())
                    , .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    // , .associatedEntity = meshEntity
                    , .pAssociatedSubMesh = pSubMesh
                };

                BufferUploadJob indexBufferJob = {
                    .numBytes = sizeof(uint32_t) * meshData.m_indices.size()
                    , .data = static_cast<const void*>(meshData.m_indices.data())
                    , .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                    // , .associatedEntity = meshEntity
                    , .pAssociatedSubMesh = pSubMesh
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

                    // m_pendingImageUploads.emplace(imageExtent, imci, /*format,*/ meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, meshEntity);
                    m_pendingImageUploads.emplace(imageExtent, imci, /*format,*/ meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, pSubMesh);
                }
            }
            it = m_pendingModelUploads.erase(it);
            // m_renderableMeshes.insert(m_renderableMeshes.end(), meshEntities.begin(), meshEntities.end());
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
                // ECS::Entity meshEntity = job.associatedEntity;
                SubMesh* pSubMesh = job.pAssociatedSubMesh;
                // auto& renderable = meshEntity.GetComponent<RenderableMeshComponent>();
                if (job.usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                {
                    // renderable.vertexBuffer = buffer;
                    pSubMesh->vertexBuffer = buffer;
                }
                else
                {
                    // renderable.indexBuffer = buffer;
                    pSubMesh->indexBuffer = buffer;
                }
                // renderable.buffersReady = true;
                pSubMesh->buffersReady = true; // TODO: this seems like a bug
            });
        }
    }

    std::vector<AllocatedBuffer> toDestroy;

    void ProcessImageUploadJobs()
    {
        if (m_pendingImageUploads.empty())
        {
            return;
        }
        Renderer::StreamingCommandBuffer* sbuf = m_rctx->ResetAndBeginStreamingCommandBuffer();
        if (!sbuf)
        {
            return; // No available command buffers
        }

        constexpr size_t batchSize = 3;
        size_t batchDone = 0;
        while(!m_pendingImageUploads.empty() && batchDone < batchSize)
        {
            auto job = m_pendingImageUploads.front();
            m_pendingImageUploads.pop();
            
            // auto& renderable = job.associatedEntity.GetComponent<RenderableMeshComponent>();
            // renderable.diffuseImage = m_rctx->CreateGPUOnlyImage(job.imageCreateInfo);
            SubMesh* pSubMesh = job.pAssociatedSubMesh;
            pSubMesh->diffuseImage = m_rctx->CreateGPUOnlyImage(job.imageCreateInfo);
            const auto extent = job.extent;
            constexpr size_t bytesPerChannel = 1;
            const size_t dataSize = extent.width * extent.height * job.numChannels * bytesPerChannel;
            // AllocatedBuffer stagingBuffer = m_rctx->UploadBuffer(dataSize, job.imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            // renderable.diffuseTextureStreamingTimelineReadyValue = m_rctx->EnqueueImageUploadJob(sbuf, renderable.diffuseImage, stagingBuffer, extent);
            AllocatedBuffer stagingBuffer = m_rctx->UploadBuffer(dataSize, job.imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            pSubMesh->diffuseTextureStreamingTimelineReadyValue = m_rctx->EnqueueImageUploadJob(sbuf, pSubMesh->diffuseImage, stagingBuffer, extent);
            // m_rctx->DestroyBuffer(stagingBuffer); // TODO: Round robin staging buffers
            toDestroy.push_back(stagingBuffer);
            batchDone++;
        }

        m_rctx->EndAndSubmitStreamingCommandBuffer(sbuf);
    }

    void PollImageUploadJobsFinishedAndUpdateRenderables()
    {
        uint64_t value = m_rctx->GetCurrentStreamingTimelineValue();
        // for (auto& entity : m_renderableMeshes) // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
        // {
        //     auto& renderable = entity.GetComponent<RenderableMeshComponent>();
        //     bool imageFinishedUploading = value >= renderable.diffuseTextureStreamingTimelineReadyValue;
        //     bool hasImage = renderable.diffuseImage.image != VK_NULL_HANDLE;
        //     if (imageFinishedUploading && (renderable.texturesReady == false) && hasImage)
        //     {
        //         auto imageViewCreateInfo = DefaultImageViewCreateInfo(renderable.diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        //         renderable.diffuseImage.view = m_rctx->CreateViewForAllocatedImage(imageViewCreateInfo);

        //         m_renderableImages.push_back(renderable.diffuseImage);
        //         renderable.diffuseTextureBindlessArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(renderable.diffuseImage);
        //         renderable.texturesReady = true;
        //     }
        // }

        // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
        for (MeshEntity* pMeshEntity : m_pWorld->m_meshEntities)
        {
            for (SubMesh* pSubMesh : pMeshEntity->m_subMeshes)
            {
                bool imageFinishedUploading = value >= pSubMesh->diffuseTextureStreamingTimelineReadyValue;
                bool hasImage = pSubMesh->diffuseImage.image != VK_NULL_HANDLE;
                if (imageFinishedUploading && (pSubMesh->texturesReady == false) && hasImage)
                {
                    auto imageViewCreateInfo = DefaultImageViewCreateInfo(pSubMesh->diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                    pSubMesh->diffuseImage.view = m_rctx->CreateViewForAllocatedImage(imageViewCreateInfo);

                    m_renderableImages.push_back(pSubMesh->diffuseImage);
                    pSubMesh->diffuseTextureBindlessArraySlot = m_rctx->m_bindlessManager.AddToBindlessTextureArray(pSubMesh->diffuseImage);
                    pSubMesh->texturesReady = true;
                }
            }
        }
    }

private:
    // These data structures may be accessed by multiple threads when loading from disk
    std::mutex m_loadedModelDataMutex;
    std::map<std::string, ModelData*> m_loadedModels;

public:
    void DestroyAllAssets()
    {
        for (auto& b: toDestroy)
        {
            m_rctx->DestroyBuffer(b);
        }
        DestroyAllLoadedModels();
        m_rctx->WaitIdle();
        DestroyAllGPUResidentMeshes();
        DestroyAllGPUResidentTextures();
        JobSystem::Wait();
    }
// private:
    void DestroyAllLoadedModels()
    {
        // std::scoped_lock lock(m_loadedModelDataMutex);
        // for (const auto& [name, ptr] : m_loadedModels)
        // {
        //     delete ptr;
        // }
        // m_loadedModels.clear();

        std::map<std::string, ModelData*> to_free;
        {
            std::scoped_lock lock(m_loadedModelDataMutex);
            to_free.swap(m_loadedModels);
        }
        for (auto& [_, p] : to_free) delete p; // safe; no one else can see them now
        Logger::Info("ResourceManager: Destroyed all RAM loaded models");
        
    }
    void DestroyAllGPUResidentMeshes()
    {
        
        JobSystem::Execute([this](){
            // for (const ECS::Entity& meshEntity : m_renderableMeshes)
            // {
            //     const auto& renderable = meshEntity.GetComponent<RenderableMeshComponent>();
            //     m_rctx->DestroyBuffer(renderable.vertexBuffer);
            //     m_rctx->DestroyBuffer(renderable.indexBuffer);
            // }
            
            for (MeshEntity* pMeshEntity : m_pWorld->m_meshEntities)
            {
                for (SubMesh* pSubMesh : pMeshEntity->m_subMeshes)
                {
                    m_rctx->DestroyBuffer(pSubMesh->vertexBuffer);
                    m_rctx->DestroyBuffer(pSubMesh->indexBuffer);
                }
            }
            Logger::Info("ResourceManager: Destroyed all meshes");
        });
    }
    void DestroyAllGPUResidentTextures()
    {
        JobSystem::Execute([this](){
            for (const AllocatedImage& image : m_renderableImages)
            {
                m_rctx->DestroyImage(image);
            }
            Logger::Info("ResourceManager: Destroyed all textures");
        });
    }
    Renderer* m_rctx = nullptr;
    ECS::Registry* m_ecs;

    // These data structures should only be accessed by a single render thread
    // std::vector<ECS::Entity> m_renderableMeshes;
    std::vector<AllocatedImage> m_renderableImages;

    World* m_pWorld = nullptr;
public:
    int GetRAMResidentModelCount() { std::scoped_lock lock(m_loadedModelDataMutex); return m_loadedModels.size(); }
    // int GetMeshCount() { return m_renderableMeshes.size(); }
    int GetMeshCount() { return -1; } // TODO
    int GetTextureCount() { return m_renderableImages.size(); }
    int GetPendingModelUploadJobCount() { return m_pendingModelUploads.size(); }
    int GetPendingBufferUploadJobCount() { return m_pendingBufferUploads.size(); }
    int GetPendingImageUploadJobCount() { return m_pendingImageUploads.size(); }
};
}