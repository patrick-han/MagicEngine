#pragma once
#include <string>
#include "pugixml.h"
#include "UUID.h"
#include <unordered_map>
#include <unordered_set>
namespace Magic
{

enum class ResourceType
{
    StaticMesh
    , Unknown
};

class ResourceDatabase
{
public:
    static ResourceType StrToResourceType(const char* name);
    static const char* ResourceTypeToStr(ResourceType resType);
    ResourceDatabase() = default;
    ~ResourceDatabase() = default;
    void Init(const char* dbPath);
    void Save();

    bool CheckIfResourceExists(const char* resourceName);
    bool CheckIfResourceExists(UUID uuid);

    void AddStaticMeshResource(const char* resourceName, const char* path);
    void RemoveResource(const char* resourceName);
    void RemoveResource(UUID uuid);
private:
    void Reload();
    [[nodiscard]] pugi::xml_node AddResource(const char* resourceName, ResourceType resourceType);
    pugi::xml_document m_db;
    std::string m_filePath;
public: // temp
    [[nodiscard]] const char* GetResName(UUID uuid) const;
    [[nodiscard]] const char* GetResPath(UUID uuid) const;
    [[nodiscard]] ResourceType GetResType(UUID uuid) const;
    [[nodiscard]] const std::unordered_set<UUID>& GetAllUUIDs() const;
private:
    void UnregisterResource(UUID uuid);
    void RegisterResource(UUID uuid,
                            const std::string& name,
                            const ResourceType resType,
                            pugi::xml_node node);
    std::unordered_set<UUID> m_uuids;
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, ResourceType> m_uuid_to_type;
    std::unordered_map<UUID, pugi::xml_node> m_uuid_to_node;
};

extern ResourceDatabase* GResourceDB;

}