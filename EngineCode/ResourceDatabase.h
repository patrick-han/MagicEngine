#pragma once
#include "vjson_header.h"
#include "UUID.h"
#include <cstddef>
#include <string>
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
private:
    void Reload();
    [[nodiscard]] vjson::Value* AddResource(const char* resourceName, ResourceType resourceType);
    vjson::Object m_db;
    std::string m_filePath;
public: // temp
    [[nodiscard]] const char* GetResName(UUID uuid) const;
    [[nodiscard]] const char* GetResPath(UUID uuid) const;
    [[nodiscard]] const char* GetResPath(const char* resName) const;
    [[nodiscard]] UUID GetResUUID(const char *resName) const;
    [[nodiscard]] ResourceType GetResType(UUID uuid) const;
    [[nodiscard]] const std::unordered_set<UUID>& GetAllUUIDs() const;
private:
    void UnregisterResource(UUID uuid);
    void RegisterResource(UUID uuid,
                            const std::string& name,
                            const ResourceType resType,
                            std::size_t resource_i);
    std::unordered_set<UUID> m_uuids;
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, ResourceType> m_uuid_to_type;
    std::unordered_map<UUID, std::size_t> m_uuid_to_resource_node_index;
};

extern ResourceDatabase* GResourceDB;

}
