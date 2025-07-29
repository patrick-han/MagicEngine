#pragma once
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/TransformComponent.h"

namespace Magic
{

class TestSystem : public System
{
public:
    TestSystem()
    {
        RequireComponent<TransformComponent>();
    }

    void Update()
    {
        std::vector<Entity> entities = GetSystemEntities();
        for (Entity& entity : entities)
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.m_position.x = 1;
            transform.m_position.y = 2;
            // 0
        }
    }
};

}
