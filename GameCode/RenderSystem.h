#pragma once
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/RenderableMeshComponent.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "../EngineCode/ResourceManager.h"

namespace Magic
{

static bool flag = true;

class RenderSystem : public System
{
public:
    RenderSystem()
    {
        RequireComponent<RenderableMeshComponent>();
        RequireComponent<TransformComponent>();
    }

    bool ShouldCull(const RenderableMeshComponent& renderable)
    {
        if (!renderable.buffersReady || !renderable.texturesReady)
        {
            return true;
        }
        return false;
    }

    static void TransformMesh(RenderableMeshComponent& mesh, const TransformComponent& transform)
    {
        mesh.transform = transform.m_transform * mesh.transform;
    }

    [[nodiscard]] std::vector<RenderableMeshComponent> Update(ResourceManager* pResourceManager, const std::uint64_t errorModelHandle)
    {
        std::vector<Entity> renderableEntities = GetSystemEntities();
        std::vector<RenderableMeshComponent> meshesToRender;
        for (const Entity& entity : renderableEntities)
        {
            auto& renderable = entity.GetComponent<RenderableMeshComponent>();
            auto& transform = entity.GetComponent<TransformComponent>();
            if (!ShouldCull(renderable))
            {
                TransformMesh(renderable, transform);
                meshesToRender.push_back(renderable);
            }
        }
        return meshesToRender;
        
    }
};

}