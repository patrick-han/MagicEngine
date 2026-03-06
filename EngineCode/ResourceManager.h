#pragma once

#include <map>
#include <mutex>

#include "../DataLibCode/DataSerialization.h"
#include <vector>
#include <queue>
#include "Renderer.h" // TODO: move to .cpp file
#include "../CommonCode/Log.h"
#include "Timing.h"
#include "MemoryManager.h"
#include "World.h"
#include "DefaultTexture.h"
#include "Threading.h"
#include "StaticMesh.h"
namespace Magic
{

class ResourceManager
// class Renderer;
{
    AllocatedImage defaultTextureImage;
    int defaultTextureImageBindlessSlot = -1;
public:
    ResourceManager();
    ~ResourceManager();

    void UploadDefaultTexture();

private:
    // These data structures may be accessed by multiple threads when loading from disk
    std::mutex m_loadingStaticMeshDataMutex;
    std::unordered_set<std::string> m_loadingStaticMeshDatas;
    std::mutex m_loadedStaticMeshDataMutex;
    std::map<std::string, StaticMeshData*> m_loadedStaticMeshDatas;
public:
    /*
     * Loads the StaticMeshData from disk and assigns it the given name
     */
    bool IsStaticMeshDataLoadedFromDisk(const char* name);
    bool IsStaticMeshDataLoadingFromDisk(const char* name);
    void SetStaticMeshDataLoadingFromDiskStatus(const char* name);
    void LoadStaticMeshDataFromDisk(const char* filePath, const char* name);

    struct StaticMeshDataUploadJob
    {
        std::string staticMeshDataName;
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

    std::vector<StaticMeshDataUploadJob> m_pendingStaticMeshDataUploads;
    std::queue<BufferUploadJob> m_pendingBufferUploads;
    std::queue<ImageUploadJob> m_pendingImageUploads;

    [[nodiscard]] std::size_t GetGPUResidentStaticMeshDataIndex(const std::string& resName) const;
    [[nodiscard]] bool IsStaticMeshDataGPUResident(const std::string& resName);

    void EnqueueUploadStaticMeshData(const std::string& resName)
    {
        m_pendingStaticMeshDataUploads.emplace_back(resName);
    }

    void ProcessStaticMeshDataUploadJobs();
    void ProcessBufferUploadJobs();
    std::vector<AllocatedBuffer> m_imageUploadStagingBuffers;
    void ProcessImageUploadJobs();

    void PollImageUploadJobsFinishedAndUpdateRenderables();

private:
    void ClearPendingUploadJobs();
public:
    void Shutdown();
    void DestroyAllAssets();
    void DestroyAllLoadedStaticMeshDatas();
    void DestroyAllGPUResidentMeshes();
    void DestroyAllGPUResidentTextures();

    // These data structures should only be accessed by a single render thread
    std::unordered_map<std::string, std::size_t> m_staticMeshResNameToArrayIndex; // string is slow key, but this lookup should only happen once or twice during the lifetime of an entity
    std::vector<StaticMesh*> m_staticMeshes;
private:
    std::vector<AllocatedImage> m_renderableImages;
public:
    // Statistic functions
    int GetDiskLoadingStaticMeshDataCount() { std::scoped_lock lock(m_loadingStaticMeshDataMutex); return m_loadingStaticMeshDatas.size(); }
    int GetRAMResidentStaticMeshDataCount() { std::scoped_lock lock(m_loadedStaticMeshDataMutex); return m_loadedStaticMeshDatas.size(); }
    int GetTextureCount() { return m_renderableImages.size(); }
    int GetPendingStaticMeshDataUploadJobCount() { return m_pendingStaticMeshDataUploads.size(); }
    int GetPendingBufferUploadJobCount() { return m_pendingBufferUploads.size(); }
    int GetPendingImageUploadJobCount() { return m_pendingImageUploads.size(); }
};

extern ResourceManager* GResourceManager;

}