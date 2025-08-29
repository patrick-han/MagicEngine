#pragma once
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/RenderableMeshComponent.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "../EngineCode/Allocators.h"
#include "../EngineCode/Limits.h"

namespace Magic
{

inline std::unique_ptr<RenderableMeshAllocator> g_renderablePool;

class RenderSystem : public System
{
public:
    RenderSystem()
    {
        RequireComponent<RenderableMeshComponent>();
        RequireComponent<TransformComponent>();
        g_renderablePool = std::make_unique<RenderableMeshAllocator>(g_maxRenderablesPerFrame);
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

    [[nodiscard]] RenderableMeshAllocator::Payload Update()
    {
        g_renderablePool->Reset();
        std::vector<Entity> renderableEntities = GetSystemEntities();
        for (const Entity& entity : renderableEntities)
        {
            auto& renderable = entity.GetComponent<RenderableMeshComponent>();
            auto& transform = entity.GetComponent<TransformComponent>();
            if (!ShouldCull(renderable))
            {
                TransformMesh(renderable, transform);
                RenderableMeshComponent* alloc = g_renderablePool->Allocate();
                if (alloc == nullptr)
                {
                    Logger::Err("Renderable pool ran out of space!");
                    break;
                }
                *alloc = renderable;
            }
        }
        return g_renderablePool->GetState();
    }
};

}