#pragma once
#include "Buffer.h"
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/AABB.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>

namespace Magic
{

struct CachedSubMesh
{
    Matrix4f transform;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    AABB3f aabb;
};

struct CachedMesh
{
    std::vector<CachedSubMesh> subMeshes;
};

class MeshCache
{
public:
    [[nodiscard]] CachedMesh GetOrUpload(const std::filesystem::path& filePath);
    [[nodidscard]] int GetCachedMeshCount();
    void Destroy();

private:

    std::mutex m_mutex;
    int m_cachedMeshCount = 0;
    std::unordered_map<std::string, CachedMesh> m_meshes;
};

extern MeshCache* GMeshCache;

}
