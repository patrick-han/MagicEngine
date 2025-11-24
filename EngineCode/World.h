#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include "pugixml.h"
#include "MeshEntity.h"
#include "UUID.h"

namespace Magic
{
class MemoryManager;
class Renderer;
class World
{
public:
    World() = default;
    ~World() = default;
    static EntityType StrToEntityType(const char* name);
    static const char* EntityTypeToStr(EntityType entityType);
    void Init(const char* dbPath);
    void Save();
    [[nodiscard]] const char* GetEntityStaticMeshResourceName(UUID uuid) const;
    [[nodiscard]] std::optional<UUID> GetStaticMeshEntityResourceUUID(UUID uuid) const;
    [[nodiscard]] const char* GetEntityName(UUID uuid) const;
    [[nodiscard]] EntityType GetEntityType(UUID uuid) const;
    [[nodiscard]] const std::unordered_set<UUID>& GetAllUUIDs() const;
    bool CheckIfEntityExists(const char* entityName) const;
    bool CheckIfEntityExists(UUID uuid) const;
    void RemoveEntity(const char* entityName);
    void RemoveEntity(UUID uuid);

    void AddStaticMeshEntity(const char* entityName);


    void Destroy();

    [[nodiscard]] MeshEntity* CreateMeshEntity();
    void RemoveMeshEntity(MeshEntity* pMeshEntity);
    [[nodiscard]] std::span<MeshEntity* const> GetMeshEntities() const;
    [[nodiscard]] int GetSubMeshCount() const
    {
        int count = 0;
        for (const MeshEntity* const mesh : m_meshEntities)
        {
            count += mesh->GetSubMeshes().size();
        }
        return count;
    }
    [[nodiscard]] int GetEntityCount() const { return m_entityCount; }
private:
    pugi::xml_document m_db;
    std::string m_filePath;
    void Reload();
    void UnregisterEntity(UUID uuid);
    void RegisterEntity(UUID uuid,
                            const std::string& name,
                            const EntityType resType,
                            pugi::xml_node node);
    std::unordered_set<UUID> m_uuids;
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, EntityType> m_uuid_to_type;
    std::unordered_map<UUID, pugi::xml_node> m_uuid_to_node;
    [[nodiscard]] pugi::xml_node AddEntity(const char* entityName, EntityType type);

    void DestroyAllMeshEntities();
    std::vector<MeshEntity*> m_meshEntities;
    std::string m_mapName;
    int m_entityCount = 0;
};


}