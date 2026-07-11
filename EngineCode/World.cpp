#include "World.h"

namespace Magic
{

std::size_t World::GetEntityCount(EntityType entityType) const
{
    std::size_t count = 0;
    for (const StaticMeshEntity& entity : m_entities)
    {
        if (entity.GetEntityType() == entityType)
        {
            count++;
        }
    }
    return count;
}

std::vector<const IEntity *> World::GetEntitiesOfType(EntityType entityType) const
{
    std::vector<const IEntity *> result;

    for (const StaticMeshEntity& entity : m_entities)
    {
        if (entity.GetEntityType() == entityType)
        {
            result.push_back(&entity);
        }
    }
    return result;
}

}