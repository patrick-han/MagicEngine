#include "ResourceManager.h"

namespace Magic
{

ResourceManager* GResourceManager = nullptr;

ResourceManager::ResourceManager()
{
    Logger::Info("Initializing ResourceManager");
}

ResourceManager::~ResourceManager()
{
    Logger::Info("Destroying ResourceManager");
}

void ResourceManager::UploadDefaultTexture()
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

bool ResourceManager::IsStaticMeshDataLoadedFromDisk(const char *name)
{
    {
        std::scoped_lock lock(m_loadedStaticMeshDataMutex);
        if (m_loadedStaticMeshDatas.find(name) != m_loadedStaticMeshDatas.end())
        {
            return true;
        }
        return false;
    }
}

bool ResourceManager::IsStaticMeshDataLoadingFromDisk(const char *name)
{
    {
        std::scoped_lock lock(m_loadingStaticMeshDataMutex);
        if(m_loadingStaticMeshDatas.find(name) != m_loadingStaticMeshDatas.end())
        {
            return true;
        }
        return false;
    }
}

void ResourceManager::SetStaticMeshDataLoadingFromDiskStatus(const char *name)
{
    {
        std::scoped_lock lock(m_loadingStaticMeshDataMutex);
        m_loadingStaticMeshDatas.insert(name);
    }
}

void ResourceManager::LoadStaticMeshDataFromDisk(const char *filePath, const char *name)
{
    auto start = std::chrono::steady_clock::now();
    std::optional<StaticMeshData> staticMeshDataOpt = Data::DeserializeStaticMeshDataBlob(filePath);
    if (!staticMeshDataOpt)
    {
        Logger::Err(std::format("LoadStaticMeshDataFromDisk({}): FAILED (could not load '{}')", name, filePath));
        return;
    }

    StaticMeshData* pStaticMeshData = GMemoryManager->New<StaticMeshData>(std::move(*staticMeshDataOpt));
    Logger::Info(std::format("LoadStaticMeshDataFromDisk({}) = {} ms", name, Timing::SinceMS(start).count()));
    {
        std::scoped_lock lock(m_loadedStaticMeshDataMutex);
        m_loadedStaticMeshDatas[name] = pStaticMeshData;
        {
            std::scoped_lock lock(m_loadingStaticMeshDataMutex);
            auto num_erased = m_loadingStaticMeshDatas.erase(name);
            assert(num_erased != 0);
        }
    }
}

std::size_t ResourceManager::GetGPUResidentStaticMeshDataIndex(const std::string &resName) const
{
    auto array_index_it = m_staticMeshResNameToArrayIndex.find(resName);
    return array_index_it->second;
}

bool ResourceManager::IsStaticMeshDataGPUResident(const std::string &resName)
{
    if (m_staticMeshResNameToArrayIndex.find(resName) !=  m_staticMeshResNameToArrayIndex.end())
    {
        return true;
    }
    return false;
}

void ResourceManager::ProcessStaticMeshDataUploadJobs()
{
    if (m_pendingStaticMeshDataUploads.empty())
    {
        return;
    }

    for (auto it = m_pendingStaticMeshDataUploads.begin(); it != m_pendingStaticMeshDataUploads.end();)
    {
        const StaticMeshDataUploadJob& job = *it;

        StaticMeshData* testStaticMeshData = nullptr;
        {
            std::scoped_lock lock(m_loadedStaticMeshDataMutex);
            if (m_loadedStaticMeshDatas.find(job.staticMeshDataName) != m_loadedStaticMeshDatas.end())
            {
                testStaticMeshData = m_loadedStaticMeshDatas[job.staticMeshDataName];
            }
            else
            {
                ++it;
                continue;
            }
        }

        StaticMesh* pStaticMesh = GMemoryManager->New<StaticMesh>();
        assert(pStaticMesh);
        m_staticMeshes.push_back(pStaticMesh);
        m_staticMeshResNameToArrayIndex[job.staticMeshDataName] = m_staticMeshes.size() - 1;

        size_t meshCounter = 0;
        for (const SubMeshData& meshData : testStaticMeshData->m_subMeshes)
        {
            // We can go ahead and fill out some of the data now and wait for buffers later
            // SubMesh* pSubMesh = pStaticMesh->AddSubMesh();
            SubMesh* pSubMesh = GMemoryManager->AllocateSubMesh();
            pStaticMesh->AddSubMesh(pSubMesh);
            pSubMesh->indexCount = static_cast<uint32_t>(meshData.m_indices.size());
            Matrix4f matrix;
            pSubMesh->m_transform = testStaticMeshData->m_transforms[meshCounter];

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

            const bool hasDiffuseTexture = pSubMesh->hasTexture = meshData.materialData.diffuseData.width != 0;
            if (hasDiffuseTexture) // TODO:
            {
                VkExtent3D imageExtent
                {
                    .width = static_cast<uint32_t>(meshData.materialData.diffuseData.width)
                    , .height = static_cast<uint32_t>(meshData.materialData.diffuseData.height)
                    , .depth = 1
                };
                const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, imageExtent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
                m_pendingImageUploads.emplace(imageExtent, imci, /*format,*/ testStaticMeshData->textureData.data() + meshData.materialData.diffuseData.baseTextureDataOffset, meshData.materialData.diffuseData.numChannels, pSubMesh);
            }
            else
            {
                // TODO: this sucks
                pSubMesh->texturesReady = true;
            }
            meshCounter++;
        }
        it = m_pendingStaticMeshDataUploads.erase(it);
    }
}

void ResourceManager::ProcessBufferUploadJobs()
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

void ResourceManager::ProcessImageUploadJobs()
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
        m_imageUploadStagingBuffers.push_back(stagingBuffer);
        batchDone++;
    }

    GRenderer->EndAndSubmitStreamingCommandBuffer(sbuf);
}

void ResourceManager::PollImageUploadJobsFinishedAndUpdateRenderables()
{
    uint64_t value = GRenderer->GetCurrentStreamingTimelineValue();
    // TODO: we actually don't need to loop through ALL mesh entities, should maintain a list of ones with pending uploads
    // for (StaticMesh* pStaticMesh : m_staticMeshes)
    for (StaticMesh* pStaticMesh : m_staticMeshes)
    {
        for (SubMesh* pSubMesh : pStaticMesh->GetSubMeshes())
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

void ResourceManager::ClearPendingUploadJobs()
{
    m_pendingStaticMeshDataUploads.clear();
    {
        std::queue<BufferUploadJob> empty;
        empty.swap(m_pendingBufferUploads);
    }
    {
        std::queue<ImageUploadJob> empty;
        empty.swap(m_pendingImageUploads);
    }
}

void ResourceManager::Shutdown()
{
    GRenderer->WaitIdle();
    GRenderer->DestroyImage(defaultTextureImage);
}

void ResourceManager::DestroyAllAssets()
{
    GRenderer->WaitIdle();
    Job::Pool.wait();
    ClearPendingUploadJobs();
    for (auto& b: m_imageUploadStagingBuffers)
    {
        GRenderer->DestroyBuffer(b);
    }
    m_imageUploadStagingBuffers.clear();
    DestroyAllLoadedStaticMeshDatas();
    DestroyAllGPUResidentMeshes();
    DestroyAllGPUResidentTextures();
    Job::Pool.wait();
}

void ResourceManager::DestroyAllLoadedStaticMeshDatas()
{
    std::map<std::string, StaticMeshData*> to_free;
    {
        std::scoped_lock lock(m_loadedStaticMeshDataMutex);
        to_free.swap(m_loadedStaticMeshDatas);
    }
    for (auto& [_, p] : to_free) GMemoryManager->Delete(p);
    Logger::Info("ResourceManager: Destroyed all RAM loaded StaticMeshData");

}

void ResourceManager::DestroyAllGPUResidentMeshes()
{
    // First destroy all GPU resources associated with static meshes
    // And then we can free our CPU side representations
    for (StaticMesh* pStaticMesh : m_staticMeshes)
    {
        for (SubMesh* pSubMesh : pStaticMesh->GetSubMeshes())
        {
            GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
            GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
        }
    }
    for (StaticMesh* pStaticMesh : m_staticMeshes)
    {
        for (SubMesh* pSubMesh : pStaticMesh->GetSubMeshes())
        {
            GMemoryManager->FreeSubMesh(pSubMesh);
        }
        GMemoryManager->Delete(pStaticMesh);
    }
    Logger::Info("ResourceManager: Destroyed all meshes");
    m_staticMeshes.clear();
    m_staticMeshResNameToArrayIndex.clear();
}

void ResourceManager::DestroyAllGPUResidentTextures()
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
}
