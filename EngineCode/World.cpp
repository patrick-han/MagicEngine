#include "World.h"
#include "MemoryManager.h"
#include "Renderer.h"
#include "ResourceDatabase.h"
#include <algorithm>
#include "../CommonCode/Log.h"
#include <cassert>

namespace Magic
{

const std::unordered_set<UUID> &World::GetAllUUIDs() const
{
    return m_uuids;
}

EntityType World::GetEntityType(UUID uuid) const
{
    return m_uuid_to_type.at(uuid);
}
const char *World::GetEntityName(UUID uuid) const
{
    return m_uuid_to_name.at(uuid).c_str();
}

EntityType World::StrToEntityType(const char* name)
{
    static const std::unordered_map<std::string_view, EntityType> table = 
    {
        { "staticmesh", EntityType::StaticMesh }
    };
    if (auto it = table.find(name); it != table.end())
    {
        return it->second;
    }
    return EntityType::Unknown;
}

const char * World::EntityTypeToStr(EntityType entityType)
{
    switch (entityType)
    {
        case EntityType::StaticMesh:
        {
            return "StaticMesh";
        }
        default:
        {
            return "Unknown";
        }
    }
}

void World::Reload()
{
    Clear();

    for (pugi::xml_node entity : m_world.children("entity"))
    {
        assert(entity.attribute("name"));
        assert(entity.attribute("uuid"));
        assert(entity.attribute("type"));
        EntityType entityType = StrToEntityType(entity.attribute("type").as_string());

        UUID uuid;
        const char* entityName = entity.attribute("name").as_string();
        bool uuidParse = UUID::TryParse(entity.attribute("uuid").as_string(), uuid);
        assert(uuidParse);

        if (entityType == EntityType::Unknown)
        {
            continue;
        }
        else if (entityType == EntityType::StaticMesh)
        {
            auto resource_staticmesh = entity.child("resource_staticmesh");
            assert(resource_staticmesh);
            assert(resource_staticmesh.attribute("name"));
            assert(resource_staticmesh.attribute("uuid"));
            // EntityUUID, resource name
            m_resourcePendingStaticMeshEntities.emplace_back(uuid, resource_staticmesh.attribute("name").as_string());
        }
        
        RegisterEntity(uuid, entityName, entityType, entity);
    }
}

void World::Init(const char* worldPath)
{
    m_world.reset();
    auto result = m_world.load_file(worldPath);
    if (result.status != pugi::status_ok)
    {
        Logger::Err("Could not open world file");
        Logger::Err(std::format("{}", result.description()));
    }
    Reload();
}

void World::Save(const char* filePath)
{
    if(m_world.save_file(filePath))
    {
        Logger::Info("Saved world successfully");
    }
    else
    {
        Logger::Err("Failed to save world");
    }
}

std::optional<UUID> World::GetStaticMeshEntityResourceUUID(UUID uuid) const
{
    if (!CheckIfEntityExists(uuid))
    {
        Logger::Err("GetStaticMeshEntityResourceUUID(): Entity doesn't exist");
        return std::nullopt;
    }
    EntityType type = m_uuid_to_type.at(uuid);
    if (type != EntityType::StaticMesh)
    {
        Logger::Err("GetStaticMeshEntityResourceUUID(): Not a StaticMesh Entity");
        return std::nullopt;
    }
    pugi::xml_node entity = m_uuid_to_node.at(uuid);
    const char* uuid_str = entity.child("resource_staticmesh").attribute("uuid").as_string();
    UUID resource_uuid;
    if (!UUID::TryParse(uuid_str, resource_uuid))
    {
        return std::nullopt;
    }
    return resource_uuid;
}

bool World::CheckIfEntityExists(const char *entityName) const
{
    for (pugi::xml_node entity : m_world.children())
    {
        if (strcmp(entity.attribute("name").as_string(), entityName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool World::CheckIfEntityExists(UUID uuid) const
{
    if (m_uuids.find(uuid) != m_uuids.end())
    {
        return true;
    }
    return false;
}

void World::RemoveEntity(const char *entityName)
{
    pugi::xml_node entity = m_world.find_child_by_attribute("entity", "name", entityName);

    if (!entity)
    {
        Logger::Err(std::format("Tried to remove entity \"{}\" that doesn't exist", entityName));
    }

    const char* uuidStr = entity.attribute("uuid").as_string();
    assert((uuidStr != nullptr) && (uuidStr[0] != '\0')); // uuid not nullptr (exists) and not empty
    UUID uuid;
    bool parseUUID = UUID::TryParse(uuidStr, uuid);
    assert(parseUUID);


    entity.parent().remove_child(entity); // this must happen only here lest uuidStr become invalid!!!
    UnregisterEntity(uuid);
}

void World::RemoveEntity(UUID uuid)
{
    if (!CheckIfEntityExists(uuid))
    {
        Logger::Err(std::format("Tried to remove entity \"{}\" that doesn't exist", uuid.ToString()));
        return;
    }
    {
        pugi::xml_node entity = m_uuid_to_node.find(uuid)->second;
        entity.parent().remove_child(entity);
    }
    UnregisterEntity(uuid);
}

static void AddEntityTransform(pugi::xml_node node)
{
    pugi::xml_node xform = node.append_child("transform");
    xform.append_child("row0").text().set("1,0,0,0");
    xform.append_child("row1").text().set("0,1,0,0");
    xform.append_child("row2").text().set("0,0,1,0");
    xform.append_child("row3").text().set("0,0,0,1");
}

void World::AddNewStaticMeshEntity(const char* entityName)
{
    pugi::xml_node node = AddEntityNode(entityName);
    UUID uuid;
    bool uuidParse = UUID::TryParse(node.attribute("uuid").as_string(), uuid);
    assert(uuidParse);
    node.append_attribute("type").set_value("staticmesh");
    AddEntityTransform(node);
    pugi::xml_node resource_staticmesh = node.append_child("resource_staticmesh");
    resource_staticmesh.append_attribute("name").set_value("NULL");
    resource_staticmesh.append_attribute("uuid").set_value("NULL");

    // There is no resource connected to this static mesh entity yet
    m_resourcePendingStaticMeshEntities.emplace_back(uuid, resource_staticmesh.attribute("name").as_string());
    RegisterEntity(uuid, entityName, EntityType::StaticMesh, node);
}

void World::UpdateStaticMeshEntityResourceEntry(UUID entityUUID, const char *resourceName)
{
    UUID resUUID = GResourceDB->GetResUUID(resourceName);

    pugi::xml_node entity = m_uuid_to_node.at(entityUUID);
    pugi::xml_node resource_staticmesh = entity.child("resource_staticmesh");
    resource_staticmesh.attribute("name").set_value(resourceName);
    resource_staticmesh.attribute("uuid").set_value(resUUID.ToString());
}

bool World::UpdateStaticMeshEntityResource(UUID entityUUID, const char *resourceName)
{
    if (!CheckIfEntityExists(entityUUID))
    {
        return false;
    }
    if (m_uuid_to_pMeshEntity.find(entityUUID) != m_uuid_to_pMeshEntity.end())
    {
        auto n = m_uuid_to_pMeshEntity.erase(entityUUID);
        assert(n > 0);
        m_resourcePendingStaticMeshEntities.emplace_back(entityUUID, std::string(resourceName));
    }
    // I have no idea why I wrote this here...
    // for (ResourcePendingStaticMeshEntity& pending : m_resourcePendingStaticMeshEntities)
    // {
    //     if (pending.entityUUID == entityUUID)
    //     {
    //         pending.resourceName = std::string(resourceName);
    //         return true;
    //     }
    // }
    UpdateStaticMeshEntityResourceEntry(entityUUID, resourceName);
    return true;
}

void World::UnregisterEntity(UUID uuid)
{
    EntityType type = m_uuid_to_type.at(uuid);
    m_uuids.erase(uuid);
    m_uuid_to_name.erase(uuid);
    m_uuid_to_type.erase(uuid);
    m_uuid_to_node.erase(uuid);
    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_node.size()
        );
    switch (type)
    {
        case EntityType::StaticMesh:
        {
            const auto n = m_uuid_to_pMeshEntity.erase(uuid);
            bool wasPending = false;
            auto it = std::find_if(
                m_resourcePendingStaticMeshEntities.begin()
                , m_resourcePendingStaticMeshEntities.end()
                , [&](const auto& r) 
                { 
                    return r.entityUUID == uuid;
                }
            );

            if (it != m_resourcePendingStaticMeshEntities.end())
            {
                m_resourcePendingStaticMeshEntities.erase(it);
                wasPending = true;
            }
            assert((n > 0) || (wasPending)); // The entity was either ready or still pending a resource
        }
        default:
        {
            Logger::Warn("Trying to unregister unknown Entity Type!");
        }
    }
}

void World::RegisterEntity(UUID uuid, const std::string &name, const EntityType type, pugi::xml_node node)
{
    m_entityCount++;
    m_uuids.insert(uuid);
    m_uuid_to_name.insert({uuid, name});
    m_uuid_to_type.insert({uuid, type});
    m_uuid_to_node.insert({uuid, node});

    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_node.size()
        );
}

pugi::xml_node World::AddEntityNode(const char *entityName)
{
    if (CheckIfEntityExists(entityName))
    {
        Logger::Err(std::format("Entity \"{}\" already exists", entityName));
        return {};
    }
    pugi::xml_node entity = m_world.append_child("entity");
    entity.append_attribute("name").set_value(entityName);
    UUID uuid;
    entity.append_attribute("uuid").set_value(uuid.ToString().c_str());
    return entity;
}

void World::Clear()
{
    m_entityCount = 0;
    m_uuids.clear();
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_node.clear();
    {
        m_resourcePendingStaticMeshEntities.clear();
    }
    m_uuid_to_pMeshEntity.clear();
}
}
