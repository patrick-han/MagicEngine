#include "ResourceDatabase.h"
#include "Common/Log.h"
#include <cassert>

namespace Magic
{

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

void ResourceDatabase::Reload()
{
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_node.clear();

    for (pugi::xml_node resource : m_db.children("resource"))
    {
        ResourceType resType = StrToResourceType(resource.attribute("type").as_string());
        if (resType == ResourceType::Unknown)
        {
            continue;
        }
        UUID uuid;
        const char* name = resource.attribute("name").as_string();
        UUID::TryParse(resource.attribute("uuid").as_string(), uuid);
        m_uuid_to_name[uuid] = name;
        m_uuid_to_type[uuid] = resType;
        m_uuid_to_node[uuid] = resource;
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
    for (auto resource : m_db.children())
    {
        if (strcmp(resource.attribute("name").as_string(), resourceName) == 0)
        {
            return true;
        }
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
    m_uuid_to_name[uuid] = resourceName;
    m_uuid_to_type[uuid] = resourceType;
    m_uuid_to_node[uuid] = resource;
    return resource;
}

const char * ResourceDatabase::GetResPath(UUID uuid)
{
    return m_uuid_to_node[uuid].child("path").text().as_string();
}
}
