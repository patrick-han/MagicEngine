#include "ResourceDatabase.h"
#include "Common/Log.h"
#include <cassert>

namespace Magic
{

ResourceDatabase* GResourceDB = nullptr;

ResourceType ResourceDatabase::StrToResourceType(const char* name)
{
    static const std::unordered_map<std::string_view, ResourceType> table = 
    {
        { "staticmesh", ResourceType::StaticMesh }
    };
    if (auto it = table.find(name); it != table.end())
    {
        return it->second;
    }
    return ResourceType::Unknown;
}

const char * ResourceDatabase::ResourceTypeToStr(ResourceType resType)
{
    switch (resType)
    {
        case ResourceType::StaticMesh:
        {
            return "StaticMesh";
        }
        default:
        {
            return "Unknown";
        }
    }
}

void ResourceDatabase::UnregisterResource(UUID uuid)
{
    m_uuids.erase(uuid);
    m_uuid_to_name.erase(uuid);
    m_uuid_to_type.erase(uuid);
    m_uuid_to_node.erase(uuid);
    assert(m_uuids.size() == m_uuid_to_name.size() == m_uuid_to_type.size() == m_uuid_to_node.size());
}

void ResourceDatabase::RegisterResource(UUID uuid,
                                        const std::string &name,
                                        const ResourceType resType,
                                        pugi::xml_node node)
{
    m_uuids.insert(uuid);
    m_uuid_to_name.insert({uuid, name});
    m_uuid_to_type.insert({uuid, resType});
    m_uuid_to_node.insert({uuid, node});

    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_node.size()
        );
}

void ResourceDatabase::Reload()
{
    m_uuids.clear();
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_node.clear();

    for (pugi::xml_node resource : m_db.children("resource"))
    {
        assert(resource.attribute("name"));
        assert(resource.attribute("uuid"));
        assert(resource.attribute("type"));
        ResourceType resType = StrToResourceType(resource.attribute("type").as_string());
        if (resType == ResourceType::Unknown)
        {
            continue;
        }
        UUID uuid;
        const char* name = resource.attribute("name").as_string();
        assert(UUID::TryParse(resource.attribute("uuid").as_string(), uuid));
        RegisterResource(uuid, name, resType, resource);
    }
}

void ResourceDatabase::Init(const char* dbPath)
{
    m_filePath = dbPath;
    auto result = m_db.load_file(dbPath);
    if (result.status != pugi::status_ok)
    {
        Logger::Err("Could not open resource database");
        Logger::Err(std::format("{}", result.description()));
        exit(1);
    }
    Reload();
}

void ResourceDatabase::Save()
{
    if(m_db.save_file(m_filePath.c_str()))
    {
        Logger::Info("Saved resource database successfully");
    }
    else
    {
        Logger::Err("Failed to save resource database");
    }
}

bool ResourceDatabase::CheckIfResourceExists(const char *resourceName)
{
    for (pugi::xml_node resource : m_db.children())
    {
        if (strcmp(resource.attribute("name").as_string(), resourceName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool ResourceDatabase::CheckIfResourceExists(UUID uuid)
{
    if (m_uuids.find(uuid) != m_uuids.end())
    {
        return true;
    }
    return false;
}

void ResourceDatabase::AddStaticMeshResource(const char *resourceName, const char *resourcePath)
{
    pugi::xml_node node = AddResource(resourceName, ResourceType::StaticMesh);
    if (node.empty())
    {
        return;
    }
    node.append_attribute("type").set_value("staticmesh");
    node.append_child("path").text().set(resourcePath);
    assert(m_uuid_to_name.size() == m_uuid_to_type.size());
}

void ResourceDatabase::RemoveResource(const char* resourceName)
{
    pugi::xml_node resource = m_db.find_child_by_attribute("resource", "name", resourceName);

    if (!resource)
    {
        Logger::Err(std::format("Tried to remove resource \"{}\" that doesn't exist", resourceName));
    }

    const char* uuidStr = resource.attribute("uuid").as_string();
    assert((uuidStr != nullptr) && (uuidStr[0] != '\0')); // uuid not nullptr (exists) and not empty
    UUID uuid;
    assert(UUID::TryParse(uuidStr, uuid));


    resource.parent().remove_child(resource); // this must happen only here lest uuidStr become invalid!!!
    UnregisterResource(uuid);
}

void ResourceDatabase::RemoveResource(UUID uuid)
{
    if (!CheckIfResourceExists(uuid))
    {
        Logger::Err(std::format("Tried to remove resource \"{}\" that doesn't exist", uuid.ToString()));
        return;
    }
    {
        pugi::xml_node resource = m_uuid_to_node.find(uuid)->second;
        resource.parent().remove_child(resource);
    }
    UnregisterResource(uuid);
}

pugi::xml_node ResourceDatabase::AddResource(const char *resourceName, ResourceType resourceType)
{
    if (CheckIfResourceExists(resourceName))
    {
        Logger::Err(std::format("Resource \"{}\" already exists", resourceName));
        return {};
    }
    pugi::xml_node resource = m_db.append_child("resource");
    resource.append_attribute("name").set_value(resourceName);
    UUID uuid;
    resource.append_attribute("uuid").set_value(uuid.ToString().c_str());
    RegisterResource(uuid, resourceName, resourceType, resource);
    return resource;
}

const std::unordered_set<UUID> &ResourceDatabase::GetAllUUIDs() const
{
    return m_uuids;
}

const char * ResourceDatabase::GetResPath(UUID uuid) const
{
    return m_uuid_to_node.at(uuid).child("path").text().as_string();
}
ResourceType ResourceDatabase::GetResType(UUID uuid) const
{
    return m_uuid_to_type.at(uuid);
}
const char *ResourceDatabase::GetResName(UUID uuid) const
{
    return m_uuid_to_name.at(uuid).c_str();
}
}
