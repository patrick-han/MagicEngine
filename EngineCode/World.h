#pragma once
#include <vector>
#include "StaticMeshEntity.h"
#include "vjson_header.h"
#include "../CommonCode/Log.h"
#include "MemoryManager.h"
#include "UUID.h"


#include "StaticMesh.h"
#include "SubMesh.h"
#include "Renderer.h"
#include "../CommonCode/StaticMeshData.h"
#include "../DataLibCode/DataSerialization.h"
#include <optional>
#include <format>
#include "DefaultTexture.h"

namespace Magic
{
inline const VkFormat g_defaultTextureFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format

class World
{
    static constexpr char not_a_string[] = "NOTASTRING";
public:
    World() = default;
    ~World() = default;
    void Load(const char* worldPath);

    void Save(const char* worldPath)
    {

    }

    void Destroy();


    [[nodiscard]] std::size_t GetEntityCount(EntityType entityType) const;
    [[nodiscard]] std::vector<const IEntity*> GetEntitiesOfType(EntityType entityType) const;

    std::vector<StaticMeshEntity> m_entities;
};

}