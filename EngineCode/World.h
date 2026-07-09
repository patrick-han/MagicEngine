#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include "StaticMeshEntity.h"
#include "UUID.h"
#include "vjson_header.h"

namespace Magic
{
class MemoryManager;
class Renderer;

// The World is sort of a logical view of entities, and has mappings to the "real entities (like StaticMeshEntity)" that are stored elsewhere
// For example with static mesh entities, the World doesn't actually own the CPU or GPU static data, it can only
// request that the owners (ResourceManager atm) destroy or create

// Basically this class should only be used to store references like:
// - Entity UUIDs
// - Pointers
// - Strings


struct ResourcePendingStaticMeshEntity
{
    UUID entityUUID;
    std::string resourceName;
    Matrix4f transform;
};


class World
{
public:
    World() = default;
    ~World() = default;
    void Init(const char* worldPath);
    void Save(const char* filePath);
    [[nodiscard]] const char* GetEntityName(UUID uuid) const;
    [[nodiscard]] EntityType GetEntityType(UUID uuid) const;
    [[nodiscard]] const std::unordered_set<UUID>& GetAllUUIDs() const;
    bool CheckIfEntityExists(const char* entityName) const;
    bool CheckIfEntityExists(UUID uuid) const;

    void AddNewStaticMeshEntity(const char* entityName);
    [[nodiscard]] bool UpdateStaticMeshEntityResource(UUID entityUUID, const char *resourcePath);
    [[nodiscard]] std::optional<UUID> GetStaticMeshEntityResourceUUID(UUID uuid) const;
    void SetStaticMeshEntityTransform(UUID uuid, const Matrix4f& transform);
    [[nodiscard]] std::optional<Matrix4f> GetStaticMeshEntityTransform(UUID uuid) const;


    void Clear();
    void Reload();
public:
    [[nodiscard]] int GetEntityCount() const { return m_entityCount; }
private:
    vjson::Object jsonWorld;
    void UnregisterEntity(UUID uuid);
    void RegisterEntity(UUID uuid,
                            const std::string& name,
                            const EntityType type,
                            std::size_t entity_i);
    std::unordered_set<UUID> m_uuids;
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, EntityType> m_uuid_to_type;
    std::unordered_map<UUID, std::size_t> m_uuid_to_entity_node_index;

public:
    ////// Static Meshes START //////
     std::vector<ResourcePendingStaticMeshEntity> m_resourcePendingStaticMeshEntities;
    // Entity UUID to StaticMeshEntity mapping as retrieved from ResourceManager
    // Entities only appear here once they've found their resources
    std::unordered_map<UUID, StaticMeshEntity> m_uuid_to_meshEntity;
    ////// Static Meshes END //////


private:
    int m_entityCount = 0;
};


}