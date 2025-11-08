#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include <queue>
#include "Renderer.h" // TODO: move to .cpp file
#include "Common/Log.h"
#include "Timing.h"
#include "JobSystem.h"

#include "MemoryManager.h"
#include "World.h"
#include "Entity.h"
namespace Magic
{

class ResourceManager
// class Renderer;
{
public:
    ResourceManager(Renderer* pRenderer, World* pWorld, MemoryManager* pMemoryManager)
    {
        Logger::Info("Initializing ResourceManager");
        m_rctx = pRenderer;
        m_pWorld = pWorld;
        m_pMemoryManager = pMemoryManager;
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

            MeshEntity* pMeshEntity = m_pWorld->CreateMeshEntity();

            size_t meshCounter = 0;
            for (const MeshData& meshData : testModel->m_meshes)
            {
                // We can go ahead and fill out some of the data now and wait for buffers later
                // SubMesh* pSubMesh = pMeshEntity->AddSubMesh();
                SubMesh* pSubMesh = m_pMemoryManager->AllocateSubMesh();
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
            JobSystem::Execute([job, this](){
                AllocatedBuffer buffer = m_rctx->UploadBuffer(job.numBytes, job.data, job.usage);
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
            SubMesh* pSubMesh = job.pAssociatedSubMesh;
            pSubMesh->diffuseImage = m_rctx->CreateGPUOnlyImage(job.imageCreateInfo);
            const auto extent = job.extent;
            constexpr size_t bytesPerChannel = 1;
            const size_t dataSize = extent.width * extent.height * job.numChannels * bytesPerChannel;
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
        // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
        for (MeshEntity* pMeshEntity : m_pWorld->GetMeshEntities())
        {
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
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
            for (MeshEntity* pMeshEntity : m_pWorld->GetMeshEntities())
            {
                for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
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
    World* m_pWorld = nullptr;
    MemoryManager* m_pMemoryManager = nullptr;

    // These data structures should only be accessed by a single render thread
    std::vector<AllocatedImage> m_renderableImages;

    
public:
    int GetRAMResidentModelCount() { std::scoped_lock lock(m_loadedModelDataMutex); return m_loadedModels.size(); }
    int GetTextureCount() { return m_renderableImages.size(); }
    int GetPendingModelUploadJobCount() { return m_pendingModelUploads.size(); }
    int GetPendingBufferUploadJobCount() { return m_pendingBufferUploads.size(); }
    int GetPendingImageUploadJobCount() { return m_pendingImageUploads.size(); }
};
}