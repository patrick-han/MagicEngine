#include "ECS.h"
#include <algorithm>
#include <format>
#include "Common/Log.h"

namespace Magic::ECS
{

COMPONENT_ID IComponent::m_nextId = COMPONENT_ID(0);

Entity::Entity(ENTITY_ID id) : m_id(id) {

}

void Entity::EnqueueKill() const {
    m_registry->EnqueueKillEntity(*this);
}

void System::AddEntity(const Entity& entity) {
    m_entities.push_back(entity);
}

bool System::RemoveEntity(const Entity& entity) {
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [&entity](const Entity& current_entity) {
            return current_entity == entity;
        }
    );
    if (it != m_entities.end()) {
        m_entities.erase(it);
        return true;
    }
    return false;
}

Entity Registry::EnqueueCreateEntity() {
    ENTITY_ID entityId;
    if (m_freeIds.empty()) {
        entityId = ENTITY_ID(m_numEntities++);
        if (entityId.GetValue() >= static_cast<int>(m_entityComponentSignatures.size())) {
            m_entityComponentSignatures.resize(entityId.GetValue() + 1);
        }
    } else {
        entityId = m_freeIds.front();
        m_freeIds.pop_front();
    }

    Entity entity(entityId);
    entity.m_registry = this;
    m_entitiesToBeAdded.insert(entity);
    // Logger::Info(std::format("Entity created with id: {}", entityId.GetValue()));
    return entity;
}

void Registry::EnqueueKillEntity(Entity entity) {
    m_entitiesToBeKilled.insert(entity);
    m_numEntities--;
}

void Registry::AddEntityToSystems(Entity entity) {
    const auto entityId = entity.GetId();
    const Signature& entityComponentSignature = m_entityComponentSignatures[entityId.GetValue()];
    for (auto& [systemTypeIndex, system] : m_systems) {
        const Signature& systemComponentSignature = system->GetComponentSignature();
        // The entity may have more bits set in its signature than the system requires, and should still qualify to be added to the system
        bool systemInterestedInEntity = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;
        if (systemInterestedInEntity) {
            system->AddEntity(entity);
            // Logger::Info(std::format("Added Entity {} to System: {}", entity.GetId().GetValue(), std::string(systemTypeIndex.name())));
        }
    }
}

void Registry::RemoveEntityFromSystems(Entity entity) {
    for (auto& [systemTypeIndex, system] : m_systems) {

        const std::string systemName(systemTypeIndex.name());

        if (system->RemoveEntity(entity)) {
            // Logger::Info(std::format("Removed Entity {} from System: {}", entity.GetId().GetValue(), systemName.c_str()));
        }
        else {
            // Logger::Warn(std::format("Tried to remove Entity {} from System: {} but failed, possibly because this System did not have the entity to begin with", entity.GetId().GetValue(), systemName.c_str()));
        }
    }
}

void Registry::Update() {
    // Add entities waiting to be created to active Systems
    for (Entity entity : m_entitiesToBeAdded) {
        AddEntityToSystems(entity);
    }
    m_entitiesToBeAdded.clear();

    // Remove entities waiting to be killed from active Systems
    for (Entity entity : m_entitiesToBeKilled) {
        RemoveEntityFromSystems(entity);
        m_entityComponentSignatures[entity.GetId().GetValue()].reset(); // Reset the entity id's component signature
        m_freeIds.push_back(entity.GetId()); // Free up the entity id for re-use

        // Remove entity from component pools
        for (auto pool : m_componentPools) {
            pool->RemoveEntityFromPool(entity.GetId());
        }
    }
    m_entitiesToBeKilled.clear();
}
}