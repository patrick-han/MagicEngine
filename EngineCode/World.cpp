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

// Parse 16 floats from "1 0 0 0, 0 1 ..." (spaces/commas/semicolons allowed)
static bool ParseFloat16(std::string_view s, float out[16])
{
    const char* cur = s.data();
    const char* end = s.data() + s.size();

    int count = 0;

    while (count < 16)
    {
        // Skip separators until we hit the next number
        while (cur < end)
        {
            char c = *cur;
            if (std::isspace((unsigned char)c) || c == ',' || c == ';')
                ++cur;
            else
                break;
        }

        if (cur >= end)
            return false;

        char* next = nullptr;
        float v = std::strtof(cur, &next);

        // No progress -> parse failure
        if (next == cur)
            return false;

        out[count++] = v;
        cur = next;
    }

    return true;
}

static bool ReadMatrix4f(pugi::xml_node transformNode, Matrix4f& outM)
{
    pugi::xml_attribute attr = transformNode.attribute("m");
    assert(attr);

    float tmp[16];
    if (!ParseFloat16(std::string_view{attr.value()}, tmp))
        return false;

    outM.m00 = tmp[0];
    outM.m01 = tmp[1];
    outM.m02 = tmp[2];
    outM.m03 = tmp[3];
    outM.m10 = tmp[4];
    outM.m11 = tmp[5];
    outM.m12 = tmp[6];
    outM.m13 = tmp[7];
    outM.m20 = tmp[8];
    outM.m21 = tmp[9];
    outM.m22 = tmp[10];
    outM.m23 = tmp[11];
    outM.m30 = tmp[12];
    outM.m31 = tmp[13];
    outM.m32 = tmp[14];
    outM.m33 = tmp[15];
    return true;
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

        Matrix4f transform;
        { // Transform
            pugi::xml_node transformNode = entity.child("transform");
            assert(ReadMatrix4f(transformNode, transform));
        }

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
            m_resourcePendingStaticMeshEntities.emplace_back(uuid, resource_staticmesh.attribute("name").as_string(), transform);
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

static void WriteMatrix4f(pugi::xml_node transformNode, const Matrix4f& M)
{
    std::ostringstream oss;
    oss.precision(9); // enough for stable round-trips for most game transforms

    oss << M.m00 << ' ';
    oss << M.m01 << ' ';
    oss << M.m02 << ' ';
    oss << M.m03 << ' ';
    oss << M.m10 << ' ';
    oss << M.m11 << ' ';
    oss << M.m12 << ' ';
    oss << M.m13 << ' ';
    oss << M.m20 << ' ';
    oss << M.m21 << ' ';
    oss << M.m22 << ' ';
    oss << M.m23 << ' ';
    oss << M.m30 << ' ';
    oss << M.m31 << ' ';
    oss << M.m32 << ' ';
    oss << M.m33 << ' ';

    auto attr = transformNode.attribute("m");
    if (!attr) attr = transformNode.append_attribute("m");
    attr.set_value(oss.str().c_str());
}

void World::UpdateStaticMeshEntityTransformEntry(UUID entityUUID, const Matrix4f& transform)
{
    pugi::xml_node entity = m_uuid_to_node.at(entityUUID);
    pugi::xml_node transform_staticmesh = entity.child("transform");
    WriteMatrix4f(transform_staticmesh, transform);
}

void World::SetStaticMeshEntityTransform(UUID uuid, const Matrix4f& transform)
{
    if (!CheckIfEntityExists(uuid))
    {
        Logger::Err("SetStaticMeshTransform(): Entity doesn't exist");
        return;
    }
    auto it = m_uuid_to_meshEntity.find(uuid);
    if (it == m_uuid_to_meshEntity.end())
    {
        assert(false); // This should never happen
    }
    it->second.m_transform = transform;
    UpdateStaticMeshEntityTransformEntry(uuid, transform);
}

std::optional<Matrix4f> World::GetStaticMeshEntityTransform(UUID uuid) const
{
    if (!CheckIfEntityExists(uuid))
    {
        Logger::Err("GetStaticMeshTransform(): Entity doesn't exist");
        return std::nullopt;
    }
    auto it = m_uuid_to_meshEntity.find(uuid);
    if (it == m_uuid_to_meshEntity.end())
    {
        return std::nullopt; // This path might hit if the static mesh entity is still in the pending list
    }
    return it->second.m_transform;
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
    xform.append_attribute("m").set_value("1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1");
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
    if (m_uuid_to_meshEntity.find(entityUUID) != m_uuid_to_meshEntity.end())
    {
        auto n = m_uuid_to_meshEntity.erase(entityUUID);
        assert(n > 0);
        m_resourcePendingStaticMeshEntities.emplace_back(entityUUID, std::string(resourceName));
    }
    // This path hits when the mesh was not in the map to begin with, i.e. add a new NULL static mesh, so it's been pending since it was created
    else
    {
        for (ResourcePendingStaticMeshEntity& pending : m_resourcePendingStaticMeshEntities)
        {
            if (pending.entityUUID == entityUUID)
            {
                pending.resourceName = std::string(resourceName);
            }
        }
    }
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
            const auto n = m_uuid_to_meshEntity.erase(uuid);
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
            break;
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
    m_uuid_to_meshEntity.clear();
}
}
