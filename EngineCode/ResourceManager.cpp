#include "ResourceManager.h"
#include "Common/Log.h"

namespace Magic
{
ResourceManager::ResourceManager(Renderer* pRenderer)
{
    Logger::Info("Initializing AssetManager");
    m_rctx = pRenderer;
}

ResourceManager::~ResourceManager()
{
    Logger::Info("Destroying AssetManager");
}

void ResourceManager::DestroyAllAssets()
{
    DestroyAllLoadedModels();
    m_rctx->WaitIdle();
    DestroyAllGPUResidentMeshes();
    DestroyAllGPUResidentTextures();
}

void ResourceManager::DestroyAllGPUResidentMeshes()
{
    Logger::Info("AssetManager: Destroying all meshes");
    for (const RenderableMesh& renderable : m_renderableMeshes)
    {
        m_rctx->DestroyBuffer(renderable.vertexBuffer);
        m_rctx->DestroyBuffer(renderable.indexBuffer);
    }
}

void ResourceManager::DestroyAllGPUResidentTextures()
{
    Logger::Info("AssetManager: Destroying all textures");
    for (const AllocatedImage& image : m_renderableImages)
    {
        m_rctx->DestroyImage(image);
    }
}
}
