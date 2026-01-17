#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include <queue>
#include "Renderer.h" // TODO: move to .cpp file
#include "Common/Log.h"
#include "Timing.h"
#include "MemoryManager.h"
#include "World.h"
#include "DefaultTexture.h"
#include "Threading.h"
namespace Magic
{

class ResourceManager
// class Renderer;
{
    AllocatedImage defaultTextureImage;
    int defaultTextureImageBindlessSlot = -1;
public:
    ResourceManager()
    {
        Logger::Info("Initializing ResourceManager");
    }

    ~ResourceManager()
    {
        Logger::Info("Destroying ResourceManager");
    }

    void UploadDefaultTexture()
    {
        constexpr VkExtent3D extent { .width = 128 , .height = 128 , .depth = 1 };
        const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
        constexpr size_t bytesPerChannel = 1;
        constexpr size_t numChannels = 4;
        constexpr size_t dataSize = extent.width * extent.height * numChannels * bytesPerChannel;
        AllocatedBuffer stagingBuffer = GRenderer->UploadBuffer(dataSize, g_DefaultTexture.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        defaultTextureImage = GRenderer->UploadImage(g_DefaultTexture.data(), 4, imci);
        auto imageViewCreateInfo = DefaultImageViewCreateInfo(defaultTextureImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        defaultTextureImage.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
        defaultTextureImageBindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(defaultTextureImage);
        GRenderer->DestroyBuffer(stagingBuffer);
    }


    /*
     * Loads the model from disk and assigns it the given name
     */
    // void LoadModelFromDisk(const std::string& filePath, const std::string& name)
    void LoadModelFromDisk(const char* filePath, const char* name)
    {
        {
            std::scoped_lock lock(m_loadedModelDataMutex);
            if (m_loadedModels.find(name) != m_loadedModels.end())
            {
                Logger::Warn(std::format("Skipping LoadModelFromDisk({}): WARN (Already loaded in RAM)", name));
                return;
            }
        }

        auto start = std::chrono::steady_clock::now();
        std::optional<ModelData> modelOpt = Data::DeserializeModelDataBlob(filePath);
        if (!modelOpt) 
        {
            Logger::Err(std::format("LoadModelFromDisk({}): FAILED (could not load '{}')", name, filePath));
            return;
        }

        ModelData* pModelData = GMemoryManager->New<ModelData>(std::move(*modelOpt));
        Logger::Info(std::format("LoadModelFromDisk({}) = {} ms", name, Timing::SinceMS(start).count()));
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
        SubMesh* pAssociatedSubMesh;
    };

    struct ImageUploadJob
    {
        VkExtent3D extent;
        VkImageCreateInfo imageCreateInfo;
        // VkFormat format;
        const unsigned char* imageData;
        int numChannels;
        SubMesh* pAssociatedSubMesh;
    };

    static const VkFormat g_defaultTextureFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format

    std::vector<ModelUploadJob> m_pendingModelUploads;
    std::queue<BufferUploadJob> m_pendingBufferUploads;
    std::queue<ImageUploadJob> m_pendingImageUploads;

    void EnqueueUploadModel(const std::string& name)
    {
        if (m_staticMeshResNameToArrayIndex.find(name) !=  m_staticMeshResNameToArrayIndex.end())
        {
            Logger::Warn(std::format("Skipping EnqueueUploadModel({}): WARN (Already uploaded to GPU)", name));
            return;
        }
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

            StaticMeshEntity* pMeshEntity = GMemoryManager->New<StaticMeshEntity>();
            assert(pMeshEntity);
            m_meshEntities.push_back(pMeshEntity);
            m_staticMeshResNameToArrayIndex[job.modelName] = m_meshEntities.size() - 1;

            size_t meshCounter = 0;
            for (const SubMeshData& meshData : testModel->m_subMeshes)
            {
                // We can go ahead and fill out some of the data now and wait for buffers later
                // SubMesh* pSubMesh = pMeshEntity->AddSubMesh();
                SubMesh* pSubMesh = GMemoryManager->AllocateSubMesh();
                pMeshEntity->AddSubMesh(pSubMesh);
                pSubMesh->indexCount = static_cast<uint32_t>(meshData.m_indices.size());
                Matrix4f matrix;
                pSubMesh->m_worldMatrix = testModel->m_transforms[meshCounter];

                // calculate aabb, TODO: this can be spun off into a separate job, or better yet done in the cooker
                for (const auto& vertex : meshData.m_vertices)
                {
                    pSubMesh->aabb.Update(vertex.position);
                }

                BufferUploadJob vertexBufferJob = {
                    .numBytes = sizeof(SimpleVertex) * meshData.m_vertices.size()
                    , .data = static_cast<const void*>(meshData.m_vertices.data())
                    , .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    , .pAssociatedSubMesh = pSubMesh
                };

                BufferUploadJob indexBufferJob = {
                    .numBytes = sizeof(uint32_t) * meshData.m_indices.size()
                    , .data = static_cast<const void*>(meshData.m_indices.data())
                    , .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                    , .pAssociatedSubMesh = pSubMesh
                };
                m_pendingBufferUploads.push(vertexBufferJob);
                m_pendingBufferUploads.push(indexBufferJob);

                const bool hasDiffuseTexture = pSubMesh->hasTexture = meshData.materialData.diffuseData.data.size() != 0;
                if (hasDiffuseTexture) // TODO:
                {
                    VkExtent3D imageExtent
                    {
                        .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                        , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                        , .depth = 1
                    };
                    const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
                    m_pendingImageUploads.emplace(imageExtent, imci, /*format,*/ meshData.materialData.diffuseData.data.data(), meshData.materialData.diffuseData.numChannels, pSubMesh);
                }
                else
                {
                    // TODO: this sucks
                    pSubMesh->texturesReady = true; 
                }
                meshCounter++;
            }
            it = m_pendingModelUploads.erase(it);
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
            Job::Pool.detach_task([job, this](){
                AllocatedBuffer buffer = GRenderer->UploadBuffer(job.numBytes, job.data, job.usage);
                SubMesh* pSubMesh = job.pAssociatedSubMesh;
                if (job.usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                {
                    pSubMesh->vertexBuffer = buffer;
                    pSubMesh->vertexBufferReady = true;
                }
                else
                {
                    pSubMesh->indexBuffer = buffer;
                    pSubMesh->indexBufferReady = true;
                }
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
        Renderer::StreamingCommandBuffer* sbuf = GRenderer->ResetAndBeginStreamingCommandBuffer();
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
            SubMesh* pSubMesh = job.pAssociatedSubMesh;
            pSubMesh->diffuseImage = GRenderer->CreateGPUOnlyImage(job.imageCreateInfo);
            const auto extent = job.extent;
            constexpr size_t bytesPerChannel = 1;
            const size_t dataSize = extent.width * extent.height * job.numChannels * bytesPerChannel;
            AllocatedBuffer stagingBuffer = GRenderer->UploadBuffer(dataSize, job.imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
            pSubMesh->diffuseTextureStreamingTimelineReadyValue = GRenderer->EnqueueImageUploadJob(sbuf, pSubMesh->diffuseImage, stagingBuffer, extent);
            // GRenderer->DestroyBuffer(stagingBuffer); // TODO: Round robin staging buffers
            toDestroy.push_back(stagingBuffer);
            batchDone++;
        }

        GRenderer->EndAndSubmitStreamingCommandBuffer(sbuf);
    }

    void PollImageUploadJobsFinishedAndUpdateRenderables()
    {
        uint64_t value = GRenderer->GetCurrentStreamingTimelineValue();
        // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
        // for (StaticMeshEntity* pMeshEntity : m_meshEntities)
        for (StaticMeshEntity* pMeshEntity : m_meshEntities)
        {
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
            {
                bool imageFinishedUploading = value >= pSubMesh->diffuseTextureStreamingTimelineReadyValue;
                bool hasImage = pSubMesh->diffuseImage.image != VK_NULL_HANDLE;
                if (imageFinishedUploading && (pSubMesh->texturesReady == false) && hasImage)
                {
                    auto imageViewCreateInfo = DefaultImageViewCreateInfo(pSubMesh->diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                    pSubMesh->diffuseImage.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);

                    m_renderableImages.push_back(pSubMesh->diffuseImage);
                    int bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(pSubMesh->diffuseImage);
                    if (bindlessSlot < 0)
                    {
                        pSubMesh->diffuseTextureBindlessArraySlot = defaultTextureImageBindlessSlot;
                    }
                    else
                    {
                        pSubMesh->diffuseTextureBindlessArraySlot = bindlessSlot;
                    }
                    pSubMesh->texturesReady = true;
                }
            }
        }
    }

private:
    // These data structures may be accessed by multiple threads when loading from disk
    std::mutex m_loadedModelDataMutex;
    std::map<std::string, ModelData*> m_loadedModels;

    void ClearPendingUploadJobs()
    {
        m_pendingModelUploads.clear();
        {
            std::queue<BufferUploadJob> empty;
            empty.swap(m_pendingBufferUploads);
        }
        {
            std::queue<ImageUploadJob> empty;
            empty.swap(m_pendingImageUploads);
        }
    }

public:
    void Shutdown()
    {
        GRenderer->WaitIdle();
        GRenderer->DestroyImage(defaultTextureImage);
    }
    void DestroyAllAssets()
    {
        GRenderer->WaitIdle();
        Job::Pool.wait();
        ClearPendingUploadJobs();
        for (auto& b: toDestroy)
        {
            GRenderer->DestroyBuffer(b);
        }
        toDestroy.clear();
        DestroyAllLoadedModels();
        DestroyAllGPUResidentMeshes();
        DestroyAllGPUResidentTextures();
        Job::Pool.wait();
    }
// private:
    void DestroyAllLoadedModels()
    {
        std::map<std::string, ModelData*> to_free;
        {
            std::scoped_lock lock(m_loadedModelDataMutex);
            to_free.swap(m_loadedModels);
        }
        for (auto& [_, p] : to_free) GMemoryManager->Delete<ModelData>(p);
        Logger::Info("ResourceManager: Destroyed all RAM loaded models");
        
    }
    void DestroyAllGPUResidentMeshes()
    {
        // First destroy all GPU resources associated with static meshes
        // And then we can free our CPU side representations        
        for (StaticMeshEntity* pMeshEntity : m_meshEntities)
        {
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
            {
                GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
                GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
            }
        }
        for (StaticMeshEntity* pMeshEntity : m_meshEntities)
        {
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
            {
                GMemoryManager->FreeSubMesh(pSubMesh);
            }
            GMemoryManager->Delete(pMeshEntity);
        }
        Logger::Info("ResourceManager: Destroyed all meshes");
        m_meshEntities.clear();
        m_staticMeshResNameToArrayIndex.clear();
    }
    void DestroyAllGPUResidentTextures()
    {
        for (const AllocatedImage& image : m_renderableImages)
        {
            GRenderer->DestroyImage(image);
        }
        Logger::Info("ResourceManager: Destroyed all textures");
        m_renderableImages.clear();
        GRenderer->m_bindlessManager.Reset();
        defaultTextureImageBindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(defaultTextureImage);
    }

    // These data structures should only be accessed by a single render thread
public:
    std::unordered_map<std::string, std::size_t> m_staticMeshResNameToArrayIndex; // string is slow key, but this lookup should only happen once or twice during the lifetime of an entity
    std::vector<StaticMeshEntity*> m_meshEntities;
private:
    std::vector<AllocatedImage> m_renderableImages;

    
public:
    int GetRAMResidentModelCount() { std::scoped_lock lock(m_loadedModelDataMutex); return m_loadedModels.size(); }
    int GetTextureCount() { return m_renderableImages.size(); }
    int GetPendingModelUploadJobCount() { return m_pendingModelUploads.size(); }
    int GetPendingBufferUploadJobCount() { return m_pendingBufferUploads.size(); }
    int GetPendingImageUploadJobCount() { return m_pendingImageUploads.size(); }
};

extern ResourceManager* GResourceManager;

}