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
    DestroyAllGPUResidentMeshes();
    DestroyAllGPUResidentTextures();
}

void AssetManager::DestroyAllGPUResidentMeshes()
{
    Logger::Info("AssetManager: Destroying all meshes");
    for (const RenderableMesh& renderable : m_renderableMeshes)
    {
        m_rctx->DestroyBuffer(renderable.vertexBuffer);
        m_rctx->DestroyBuffer(renderable.indexBuffer);
    }
}

void AssetManager::DestroyAllGPUResidentTextures()
{
    Logger::Info("AssetManager: Destroying all textures");
    for (const AllocatedImage& image : m_renderableImages)
    {
        m_rctx->DestroyImage(image);
    }
}
}
