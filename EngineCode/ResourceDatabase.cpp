#include "ResourceDatabase.h"
#include "../CommonCode/Log.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <format>
#include <string_view>

namespace Magic
{

namespace
{
vjson::Array* GetResourcesArray(vjson::Object& db)
{
    return db.ArrayPtrAtKey("resources");
}

const vjson::Array* GetResourcesArray(const vjson::Object& db)
{
    return db.ArrayPtrAtKey("resources");
}
}

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
            return "staticmesh";
        }
        default:
        {
            return "unknown";
        }
    }
}

void ResourceDatabase::UnregisterResource(UUID uuid)
{
    m_uuids.erase(uuid);
    m_uuid_to_name.erase(uuid);
    m_uuid_to_type.erase(uuid);
    m_uuid_to_resource_node_index.erase(uuid);

    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_resource_node_index.size()
        );
}

void ResourceDatabase::RegisterResource(UUID uuid,
                                        const std::string &name,
                                        const ResourceType resType,
                                        std::size_t resource_i)
{
    m_uuids.insert(uuid);
    m_uuid_to_name.insert({uuid, name});
    m_uuid_to_type.insert({uuid, resType});
    m_uuid_to_resource_node_index.insert({uuid, resource_i});

    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_resource_node_index.size()
        );
}

void ResourceDatabase::Reload()
{
    m_uuids.clear();
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_resource_node_index.clear();

    const vjson::Array* resources = GetResourcesArray(m_db);

    for (std::size_t resource_i = 0; resource_i < resources->size(); ++resource_i)
    {
        const vjson::Value* resource = resources->ValuePtrAtIndex(resource_i);
        assert(resource);

        const char* name = resource->AtKey("name").AsCString(nullptr);
        const char* uuidString = resource->AtKey("uuid").AsCString(nullptr);
        const char* typeString = resource->AtKey("type").AsCString(nullptr);
        ResourceType resType = StrToResourceType(typeString);
        if (resType == ResourceType::Unknown)
        {
            continue;
        }
        UUID uuid;
        bool parseUUID = UUID::TryParse(uuidString, uuid);
        assert(parseUUID);
        RegisterResource(uuid, name, resType, resource_i);
    }
    Logger::Info(std::format("Resource Database Reload() finished with {} resources: ", m_uuids.size()));
    for (auto it = m_uuid_to_name.begin(); it != m_uuid_to_name.end(); ++it)
    {
        Logger::Info(std::format("UUID: {}, {}", it->first.ToString(), it->second));
    }
}

void ResourceDatabase::Init(const char* dbPath)
{
    m_filePath = dbPath;
    vjson::ParseContext ctx;
    std::string sjson;
    if (!LoadJsonToString(dbPath, sjson))
    {
        Logger::Err("Could not open resource database");
        std::exit(1);
    }
    if (!m_db.ParseJSON(sjson, &ctx))
    {
        Logger::Err(std::format("Resource database parse failed line {}: {}", ctx.error_line, ctx.error_message));
        std::exit(1);
    }
    Reload();
}

void ResourceDatabase::Save()
{
    std::string p = m_db.PrintJSON();
    if(SaveJsonToFile(m_filePath, p))
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
    for (const auto& resourceNameEntry : m_uuid_to_name)
    {
        const std::string& name = resourceNameEntry.second;
        if (name == resourceName)
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
    vjson::Value* resource = AddResource(resourceName, ResourceType::StaticMesh);
    if (!resource)
    {
        return;
    }
    resource->SetAtKey("path", resourcePath);

    const std::size_t s = m_uuid_to_name.size();
    assert(s == m_uuid_to_type.size()
        && s == m_uuid_to_resource_node_index.size()
        );
}

vjson::Value* ResourceDatabase::AddResource(const char *resourceName, ResourceType resourceType)
{
    if (CheckIfResourceExists(resourceName))
    {
        Logger::Err(std::format("Resource \"{}\" already exists", resourceName));
        return nullptr;
    }

    vjson::Array* resources = GetResourcesArray(m_db);
    if (!resources)
    {
        m_db.SetAtKey("resources", vjson::Array{});
        resources = GetResourcesArray(m_db);
    }
    assert(resources);

    const std::size_t resourceIndex = resources->size();
    vjson::Value& resource = resources->push_back(vjson::Object{});

    resource.SetAtKey("name", resourceName);
    UUID uuid;
    resource.SetAtKey("uuid", uuid.ToString());
    resource.SetAtKey("type", ResourceTypeToStr(resourceType));

    RegisterResource(uuid, resourceName, resourceType, resourceIndex);
    return &resource;
}

const std::unordered_set<UUID> &ResourceDatabase::GetAllUUIDs() const
{
    return m_uuids;
}

const char * ResourceDatabase::GetResPath(UUID uuid) const
{
    const vjson::Array* resources = GetResourcesArray(m_db);
    const std::size_t resourceIndex = m_uuid_to_resource_node_index.at(uuid);

    return (*resources)[resourceIndex].AtKey("path").AsCString(nullptr);
}

const char *ResourceDatabase::GetResPath(const char *resName) const
{
    for (const auto& resourceNameEntry : m_uuid_to_name)
    {
        const UUID& uuid = resourceNameEntry.first;
        const std::string& name = resourceNameEntry.second;
        if (name == resName)
        {
            return GetResPath(uuid);
        }
    }
    return nullptr;
}

UUID ResourceDatabase::GetResUUID(const char *resName) const
{
    const auto it = std::find_if(
        m_uuid_to_name.begin()
        , m_uuid_to_name.end()
        , [&](const auto& r)
        {
            return r.second == resName;
        });

    if (it == m_uuid_to_name.end())
    {
        Logger::Err(std::format("Tried to get UUID for resource \"{}\" that doesn't exist", resName));
        assert(false);
        std::abort();
    }

    return it->first;
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
