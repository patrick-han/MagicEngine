#include "AssetManager.h"
#include "Common/Log.h"

namespace Magic
{
AssetManager::AssetManager(Renderer* pRenderer)
{
    Logger::Info("Initializing AssetManager");
    m_rctx = pRenderer;
}

AssetManager::~AssetManager()
{
    Logger::Info("Destroying AssetManager");
}

void AssetManager::DestroyAllAssets()
{
    m_rctx->WaitIdle(); // TODO: This is probably too heavy handed, we need some way to distinguish which assets are actually GPU resident, but i'll worry about that later
    DestroyAllMeshes();
}

void AssetManager::DestroyAllMeshes()
{
    Logger::Info("AssetManager: Destroying all meshes");
    for (const RenderableMesh& renderable : m_renderableMeshes)
    {
        m_rctx->DestroyBuffer(renderable.vertexBuffer);
        m_rctx->DestroyBuffer(renderable.indexBuffer);
    }
}
}
