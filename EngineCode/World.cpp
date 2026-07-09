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


void World::Reload()
{
    Clear();

    const vjson::Value* entities = jsonWorld.ValuePtrAtKey("root")->ValuePtrAtKey("entities");
    std::size_t entityCount = entities->GetArray().size();
    Logger::Info(std::format("Entity count: {}", entityCount));
    Logger::Info(entities->AtIndex(0).AtKey("name").AsCString("NOTASTRING"));
    for (std::size_t entity_i = 0; entity_i < entityCount; entity_i++)
    {
        const vjson::Value* entity = entities->ValuePtrAtIndex(entity_i);
        EntityType entityType = Entity::StrToEntityType(entity->AtKey("type").AsCString("NOTASTRING"));

        UUID uuid;
        const char* entityName = entity->AtKey("name").AsCString("NOTASTRING");
        bool uuidParse = UUID::TryParse(entity->AtKey("uuid").AsCString("NOTASTRING"), uuid);
        Logger::Info("UUID parse json: " + std::string(entity->AtKey("uuid").AsCString("NOTASTRING")));
        assert(uuidParse);

        Matrix4f transform;
        { // Transform
            auto transformMatrix = entity->AtKey("transform").AtKey("matrix");
            auto ReadMatrix4fJson = [](vjson::Value transformMatrix) -> Matrix4f {
                auto transformMatrixRows = transformMatrix.GetArray();
                // auto row0 = transformMatrixRows.ArrayAtIndexOrEmpty(0);

                Matrix4f result;

                auto ReadJsonMatrixRow = [&transformMatrixRows, &result](std::size_t row_i){
                    auto rowi = transformMatrixRows.ArrayAtIndexOrEmpty(row_i);
                    result.m[row_i * 4 + 0] = rowi.AtIndex(0).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 1] = rowi.AtIndex(1).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 2] = rowi.AtIndex(2).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 3] = rowi.AtIndex(3).InterpretAsDouble(1337.0);
                };

                ReadJsonMatrixRow(0);
                ReadJsonMatrixRow(1);
                ReadJsonMatrixRow(2);
                ReadJsonMatrixRow(3);

                return result;
            };
            transform = ReadMatrix4fJson(transformMatrix);
        }

        if (entityType == EntityType::Unknown)
        {
            continue;
        }
        else if (entityType == EntityType::StaticMesh)
        {
            auto resources = entity->AtKey("resources");
            auto staticmesh = resources.AtKey("staticmesh");

            // EntityUUID, resource name
            m_resourcePendingStaticMeshEntities.emplace_back(uuid, staticmesh.AtKey("name").AsCString("NOTASTRING"), transform);
        }

        RegisterEntity(uuid, entityName, entityType, entity_i);
    }
}

void World::Init(const char* worldPath)
{
    vjson::ParseContext ctx;
    std::string sjson;
    assert(LoadJsonToString(worldPath, sjson));
    if ( !jsonWorld.ParseJSON( sjson, &ctx ) )
    {
        fprintf( stderr, "Parse failed line %d: %s\n",
            ctx.error_line, ctx.error_message.c_str() );

        Logger::Err("Could not open world file");
    }

    Reload();
}

void World::Save(const char* filePath)
{
    std::string p = jsonWorld.PrintJSON();
    if(SaveJsonToFile("GameCode/world.json", p))
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

    const vjson::Array& entities = jsonWorld.ValuePtrAtKey("root")->ValuePtrAtKey("entities")->GetArray();
    const vjson::Value& entity = entities[m_uuid_to_entity_node_index.at(uuid)];    
    const char* uuid_str = entity.AtKey("resources").AtKey("staticmesh").AtKey("uuid").AsCString("NOTASTRING");
    UUID resource_uuid;
    if (!UUID::TryParse(uuid_str, resource_uuid))
    {
        return std::nullopt;
    }
    return resource_uuid;
}

static auto SetMatrixRows = [](vjson::Value* matrixRows, const Matrix4f& matrix)
{
    matrixRows->SetEmptyArray();

    vjson::Array& rows = matrixRows->GetArray();

    for (int rowIndex = 0; rowIndex < 4; ++rowIndex)
    {
        vjson::Array row;

        for (int colIndex = 0; colIndex < 4; ++colIndex)
        {
            const int matrixIndex = rowIndex * 4 + colIndex;
            row.push_back(matrix.m[matrixIndex]);
        }

        rows.push_back(row);
    }
};

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

    // Update the json side transofrm
    vjson::Array& entities = jsonWorld.ValuePtrAtKey("root")->ValuePtrAtKey("entities")->GetArray();
    vjson::Value& entity = entities[m_uuid_to_entity_node_index.at(uuid)];
    vjson::Value* entity_transform_matrix = entity.ValuePtrAtKey("transform")->ValuePtrAtKey("matrix");

    SetMatrixRows(entity_transform_matrix, transform);
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
    const vjson::Array& entities = jsonWorld.ValuePtrAtKey("root")->ValuePtrAtKey("entities")->GetArray();
    const std::size_t entity_count = entities.size();

    for (std::size_t i = 0; i < entity_count; i++)
    {
        const vjson::Value* entity = entities.ValuePtrAtIndex(i);
        const char* entityNameTest = entity->AtKey("name").AsCString("NOTASTRING");
        if (strcmp(entityNameTest, entityName) == 0)
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


void World::AddNewStaticMeshEntity(const char* entityName)
{
    if (CheckIfEntityExists(entityName))
    {
        Logger::Err(std::format("Entity \"{}\" already exists", entityName));
        return;
    }
    

    vjson::Array* entities = jsonWorld.ValuePtrAtKey("root")->ArrayPtrAtKey("entities");
    const std::size_t newEntityIndex = entities->size();
    vjson::Value& newEntity = entities->push_back(vjson::Object{});
    newEntity.SetAtKey("name", entityName);
    UUID uuid;
    newEntity.SetAtKey("uuid", uuid.ToString());
    newEntity.SetAtKey("type", "staticmesh");


    newEntity.SetAtKey("resources", vjson::Object{});
    vjson::Value* resources = newEntity.ValuePtrAtKey("resources");

    resources->SetAtKey("staticmesh", vjson::Object{});
    vjson::Value* staticmesh = resources->ValuePtrAtKey("staticmesh");

    staticmesh->SetAtKey("name", "NULL");
    staticmesh->SetAtKey("uuid", "NULL");

    newEntity.SetAtKey("transform", vjson::Object{});
    vjson::Value* transform = newEntity.ValuePtrAtKey("transform");
    transform->SetAtKey("matrix", vjson::Object{});
    vjson::Value* matrix = transform->ValuePtrAtKey("matrix");
    Matrix4f defaultMat;

    SetMatrixRows(matrix, defaultMat);

    // There is no resource connected to this static mesh entity yet
    m_resourcePendingStaticMeshEntities.emplace_back(uuid, staticmesh->AtKey("name").AsString("NOTASTRING"));
    RegisterEntity(uuid, entityName, EntityType::StaticMesh, newEntityIndex);
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

    // Update the entry in the json
    UUID resUUID = GResourceDB->GetResUUID(resourceName);
    vjson::Array& entities = jsonWorld.ValuePtrAtKey("root")->ValuePtrAtKey("entities")->GetArray();
    vjson::Value* staticmesh = entities[m_uuid_to_entity_node_index.at(entityUUID)].ValuePtrAtKey("resources")->ValuePtrAtKey("staticmesh");
    staticmesh->SetAtKey("name", resourceName);
    staticmesh->SetAtKey("uuid", resUUID.ToString());

    return true;
}

void World::UnregisterEntity(UUID uuid)
{
    EntityType type = m_uuid_to_type.at(uuid);
    m_uuids.erase(uuid);
    m_uuid_to_name.erase(uuid);
    m_uuid_to_type.erase(uuid);
    m_uuid_to_entity_node_index.erase(uuid);
    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_entity_node_index.size()
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

void World::RegisterEntity(UUID uuid, const std::string &name, const EntityType type, std::size_t entity_i)
{
    m_entityCount++;
    m_uuids.insert(uuid);
    m_uuid_to_name.insert({uuid, name});
    m_uuid_to_type.insert({uuid, type});
    m_uuid_to_entity_node_index.insert({uuid, entity_i});

    const std::size_t s = m_uuids.size();
    assert(s == m_uuid_to_name.size()
        && s == m_uuid_to_type.size()
        && s == m_uuid_to_entity_node_index.size()
        );
}

void World::Clear()
{
    m_entityCount = 0;
    m_uuids.clear();
    m_uuid_to_name.clear();
    m_uuid_to_type.clear();
    m_uuid_to_entity_node_index.clear();
    {
        m_resourcePendingStaticMeshEntities.clear();
    }
    m_uuid_to_meshEntity.clear();
}
}
