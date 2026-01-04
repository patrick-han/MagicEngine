#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include <queue>
#include "pugixml.h"
#include "MeshEntity.h"
#include "UUID.h"

namespace Magic
{
class MemoryManager;
class Renderer;

// The World is sort of a logical view of entities, and has mappings to the "real entities (like MeshEntity)" that are stored elsewhere
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
};

class World
{
public:
    World() = default;
    ~World() = default;
    static EntityType StrToEntityType(const char* name);
    static const char* EntityTypeToStr(EntityType entityType);
    void Init(const char* worldPath);
    void Save(const char* filePath);
    [[nodiscard]] const char* GetEntityName(UUID uuid) const;
    [[nodiscard]] EntityType GetEntityType(UUID uuid) const;
    [[nodiscard]] const std::unordered_set<UUID>& GetAllUUIDs() const;
    bool CheckIfEntityExists(const char* entityName) const;
    bool CheckIfEntityExists(UUID uuid) const;
    void RemoveEntity(const char* entityName);
    void RemoveEntity(UUID uuid);

    void AddStaticMeshEntity(const char* entityName);
    [[nodiscard]] std::optional<UUID> GetStaticMeshEntityResourceUUID(UUID uuid) const;


    void Clear();
    void Reload();
public:
    [[nodiscard]] int GetEntityCount() const { return m_entityCount; }
private:
    pugi::xml_document m_world;
    void UnregisterEntity(UUID uuid);
    void RegisterEntity(UUID uuid,
                            const std::string& name,
                            const EntityType resType,
                            pugi::xml_node node);
    std::unordered_set<UUID> m_uuids;
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, EntityType> m_uuid_to_type;
    std::unordered_map<UUID, pugi::xml_node> m_uuid_to_node;

public:
    ////// Static Meshes START //////
    std::queue<ResourcePendingStaticMeshEntity> m_resourcePendingStaticMeshEntities;
    // Entity UUID to meshEntity mapping as retrieved from ResourceManager
    // Entities only appear here once they've found their resources
    std::unordered_map<UUID, MeshEntity*> m_uuid_to_pMeshEntity;
    ////// Static Meshes END //////


private:

    [[nodiscard]] pugi::xml_node AddEntityNode(const char* entityName, EntityType type);
    int m_entityCount = 0;
};


}