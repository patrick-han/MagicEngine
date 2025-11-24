#include "World.h"
#include "MemoryManager.h"
#include "Renderer.h"
#include "MemoryManager.h"
#include <algorithm>
#include "Common/Log.h"
#include <cassert>

namespace Magic
{

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

const char * World::EntityTypeToStr(EntityType resType)
{
    switch (resType)
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
    m_uuids.clear();
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_node.clear();

    for (pugi::xml_node entity : m_db.children("entity"))
    {
        assert(entity.attribute("name"));
        assert(entity.attribute("uuid"));
        assert(entity.attribute("type"));
        EntityType entityType = StrToEntityType(entity.attribute("type").as_string());
        if (entityType == EntityType::Unknown)
        {
            continue;
        }
        UUID uuid;
        const char* name = entity.attribute("name").as_string();
        assert(UUID::TryParse(entity.attribute("uuid").as_string(), uuid));
        RegisterEntity(uuid, name, entityType, entity);
    }
}

void World::Init(const char* dbPath)
{
    m_filePath = dbPath;
    auto result = m_db.load_file(dbPath);
    if (result.status != pugi::status_ok)
    {
        Logger::Err("Could not open world file");
        Logger::Err(std::format("{}", result.description()));
        exit(1);
    }
    Reload();
}

void World::Save()
{
    if(m_db.save_file(m_filePath.c_str()))
    {
        Logger::Info("Saved world successfully");
    }
    else
    {
        Logger::Err("Failed to save world");
    }
}

bool World::CheckIfEntityExists(const char *entityName)
{
    for (pugi::xml_node entity : m_db.children())
    {
        if (strcmp(entity.attribute("name").as_string(), entityName) == 0)
        {
            return true;
        }
    }
    return false;
}

bool World::CheckIfEntityExists(UUID uuid)
{
    if (m_uuids.find(uuid) != m_uuids.end())
    {
        return true;
    }
    return false;
}

void World::RemoveEntity(const char *entityName)
{
    pugi::xml_node entity = m_db.find_child_by_attribute("entity", "name", entityName);

    if (!entity)
    {
        Logger::Err(std::format("Tried to remove entity \"{}\" that doesn't exist", entityName));
    }

    const char* uuidStr = entity.attribute("uuid").as_string();
    assert((uuidStr != nullptr) && (uuidStr[0] != '\0')); // uuid not nullptr (exists) and not empty
    UUID uuid;
    assert(UUID::TryParse(uuidStr, uuid));


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

MeshEntity* World::AddStaticMeshEntity(const char* entityName)
{
    pugi::xml_node node = AddEntity(entityName, EntityType::StaticMesh);
    node.append_attribute("type").set_value("staticmesh");
    node.append_attribute("genesis").set_value(true);
    AddEntityTransform(node);
    pugi::xml_node resource_staticmesh = node.append_child("resource_staticmesh");
    resource_staticmesh.append_attribute("name").set_value(entityName);
}

void World::UnregisterEntity(UUID uuid)
{
    m_uuids.erase(uuid);
    m_uuid_to_name.erase(uuid);
    m_uuid_to_type.erase(uuid);
    m_uuid_to_node.erase(uuid);
    assert(m_uuids.size() == m_uuid_to_name.size() == m_uuid_to_type.size() == m_uuid_to_node.size());
}

void World::RegisterEntity(UUID uuid, const std::string &name, const EntityType resType, pugi::xml_node node)
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

pugi::xml_node World::AddEntity(const char *entityName, EntityType type)
{
    if (CheckIfEntityExists(entityName))
    {
        Logger::Err(std::format("Entity \"{}\" already exists", entityName));
        return {};
    }
    pugi::xml_node entity = m_db.append_child("entity");
    entity.append_attribute("name").set_value(entityName);
    UUID uuid;
    entity.append_attribute("uuid").set_value(uuid.ToString().c_str());
    RegisterEntity(uuid, entityName, type, entity);
    return entity;
}

void World::Destroy()
{
    DestroyAllMeshEntities();
}

#define ENTITY_PLUS() m_entityCount++
#define ENTITY_MINUS() m_entityCount--

MeshEntity *World::CreateMeshEntity()
{
    MeshEntity* newMeshEntity = new MeshEntity;
    m_meshEntities.push_back(newMeshEntity);
    ENTITY_PLUS();
    return newMeshEntity;
}

void World::RemoveMeshEntity(MeshEntity* pMeshEntity)
{
    auto it = std::find(m_meshEntities.begin(), m_meshEntities.end(), pMeshEntity);
    if (it != m_meshEntities.end())
    {
        for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
        {
            GMemoryManager->FreeSubMesh(pSubMesh);
            // This doesnt work because a frame in flight might be using one of these resources, need to queue them for desctruction after mesh entity
            // gets removed from the world (and as a result can never get passed to the Renderer)
            // m_pRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
            // m_pRenderer->DestroyBuffer(pSubMesh->indexBuffer);
            // m_pRenderer->DestroyImage(pSubMesh->diffuseImage);
        }
        *it = m_meshEntities.back();
        m_meshEntities.pop_back();
        delete pMeshEntity;
    }
    ENTITY_MINUS();
}

std::span<MeshEntity *const> World::GetMeshEntities() const
{
    return std::span<MeshEntity *const>(m_meshEntities);
}

void World::DestroyAllMeshEntities()
{
    for (MeshEntity* pMeshEntity : m_meshEntities)
    {
        for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
        {
            GMemoryManager->FreeSubMesh(pSubMesh);
        }
        delete pMeshEntity;
    }
    m_meshEntities.clear();
}
}
