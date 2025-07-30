#pragma once
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "PlayerComponent.h"
#include "../EngineCode/Common/Math/Vector3f.h"

namespace Magic
{

class PlayerMovementSystem : public System
{
public:
    PlayerMovementSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<PlayerComponent>();
    }

    void Update(Vector3f movementVector, float deltaTime)
    {
        std::vector<Entity> entities = GetSystemEntities();
        for (Entity& entity : entities)
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& player = entity.GetComponent<PlayerComponent>();
            transform.m_transform.m03 += movementVector.x * player.m_playerSpeed * deltaTime;
            transform.m_transform.m13 += movementVector.y * player.m_playerSpeed * deltaTime;
            transform.m_transform.m23 += movementVector.z * player.m_playerSpeed * deltaTime;
        }
    }
};

}
