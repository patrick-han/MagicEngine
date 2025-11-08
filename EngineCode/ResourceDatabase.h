#pragma once
#include <string>
#include "pugixml.h"
#include "UUID.h"
#include <unordered_map>
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

    void AddStaticMeshResource(const char* resourceName, const char* path);
private:
    void Reload();
    [[nodiscard]] pugi::xml_node AddResource(const char* resourceName, ResourceType resourceType);
    pugi::xml_document m_db;
    std::string m_filePath;
public: // temp
    std::unordered_map<UUID, std::string> m_uuid_to_name;
    std::unordered_map<UUID, ResourceType> m_uuid_to_type;

    [[nodiscard]] const char* GetResPath(UUID uuid);
    std::unordered_map<UUID, pugi::xml_node> m_uuid_to_node;
};

}