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
            if (renderable.isReadyToBeRendered)
            {
                const auto& renderableMeshIndices = pResourceManager->GetRenderableMeshIndices(renderable.name);
                for (const auto& index : renderableMeshIndices)
                {
                    if (!ShouldCull(/*Renderable?*/))
                    {
                        auto renderableMesh = pResourceManager->GetRenderableMeshByIndex(index);
                        TransformMesh(renderableMesh, transform);
                        renderableMesh.renderableFlags = renderable.m_renderableFlags;
                        meshesToRender.push_back(renderableMesh);
                    }
                }
            }
            else
            {
                // if (pResourceManager->UploadModel(renderable.handle))
                if (pResourceManager->UploadModel(renderable.name)) // If the model can be uploaded (already loaded from disk)
                {
                    renderable.isReadyToBeRendered = true;
                }
                else if (renderable.testFlag)
                // else // Else, need to request load from disk
                {
                    if (flag)
                    {
                        auto t1 = std::thread([=]()
                        {
                            pResourceManager->LoadModelFromDisk(renderable.filePath, renderable.name);
                        });
                        t1.detach();
                        flag = false;
                    }

                    // RenderableMesh renderableMesh = pResourceManager->GetRenderableMeshByIndex(errorModelHandle); // TODO: Error model
                    // TransformMesh(renderableMesh, transform);
                    // renderableMesh.renderableFlags = RenderableFlags::DrawDebug;
                    // meshesToRender.push_back(renderableMesh);
                    //
                    // renderable.testFlag = false;
                }
            }
        }
        return meshesToRender;
        
    }
};

}