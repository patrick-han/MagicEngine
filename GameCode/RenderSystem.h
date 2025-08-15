#pragma once
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/RenderableComponent.h"
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
        RequireComponent<RenderableComponent>();
        RequireComponent<TransformComponent>();
    }

    bool ShouldCull()
    {
        return false;
    }

    static void TransformMesh(RenderableMesh& mesh, const TransformComponent& transform)
    {
        mesh.transform = transform.m_transform * mesh.transform;
    }

    // TODO: Return a list of RenderableComponents? Basically I want this system to do CPU side culling / visibility and provide a list of things
    // back to the engine to render
    [[nodiscard]] std::vector<RenderableMesh> Update(ResourceManager* pResourceManager, const std::uint64_t errorModelHandle)
    {
        std::vector<Entity> renderableEntities = GetSystemEntities();
        std::vector<RenderableMesh> meshesToRender;
        for (const Entity& entity : renderableEntities)
        {
            auto& renderable = entity.GetComponent<RenderableComponent>();
            auto& transform = entity.GetComponent<TransformComponent>();
            const auto& renderableMeshIndices = pResourceManager->GetRenderableMeshIndicesByHandle(renderable.handle);
            for (const auto& index : renderableMeshIndices)
            {
                if (!ShouldCull(/*Renderable?*/))
                {
                    RenderableMesh renderableMesh = pResourceManager->GetRenderableMeshByIndex(index);
                    TransformMesh(renderableMesh, transform);
                    renderableMesh.renderableFlags = renderable.m_renderableFlags;
                    meshesToRender.push_back(renderableMesh);
                }
            }
        }
        return meshesToRender;
        
    }
};

}