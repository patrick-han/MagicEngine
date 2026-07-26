#include "World.h"
#include "Renderer.h"
#include "TextureCache.h"
#include "Threading.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/sdf/assetPath.h>

#include <mutex>
namespace Magic
{

namespace
{

struct EntityLoadPayloadUSD
{
    pxr::UsdPrim entity;
    EntityType entityType = EntityType::Unknown;
};

}

void World::Load(const char* worldPath)
{
    // Upload default texture
    {
        constexpr VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        constexpr VkExtent3D extent { .width = 128 , .height = 128 , .depth = 1 };
        const VkImageCreateInfo imci = DefaultImageCreateInfo(imageFormat, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
        constexpr size_t bytesPerChannel = 1;
        constexpr size_t numChannels = 4;
        constexpr size_t dataSize = extent.width * extent.height * numChannels * bytesPerChannel;
        AllocatedBuffer stagingBuffer = GRenderer->UploadBuffer(dataSize, DefaultTexture::g_DefaultTexture.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        DefaultTexture::g_defaultTextureImage = GRenderer->UploadImage(DefaultTexture::g_DefaultTexture.data(), 4, imci);
        auto imageViewCreateInfo = DefaultImageViewCreateInfo(DefaultTexture::g_defaultTextureImage.image, imageFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        DefaultTexture::g_defaultTextureImage.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
        DefaultTexture::g_defaultTextureImageBindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(DefaultTexture::g_defaultTextureImage);
        GRenderer->DestroyBuffer(stagingBuffer);
    }

    const pxr::UsdStageRefPtr usdStage = pxr::UsdStage::Open(worldPath);
    if (!usdStage)
    {
        Logger::Err(std::format("Could not load the OpenUSD stage: {}", worldPath));
        exit(1);
    }
    pxr::SdfPath path("/root");
    auto rootprim = usdStage->GetPrimAtPath(path);


    std::vector<EntityLoadPayloadUSD> pendingEntities;

    std::size_t entityCount = 0;
    for (auto prim : rootprim.GetChildren())
    {
        if (prim.IsA<pxr::UsdGeomXform>())
        {
            const std::string primName = prim.GetName().GetString();
            Logger::Info(std::format("Xform: {}", primName));
            if (primName.starts_with("sm_"))
            {
                entityCount++;
                EntityLoadPayloadUSD payload = {
                    .entity = prim
                    , .entityType = EntityType::StaticMesh
                };
                pendingEntities.push_back(payload);
            }
        }
        else
        {
            Logger::Info(std::format("Other: {}", prim.GetName().GetString()));
        }
    }
    std::mutex loadingMutex;
    std::vector<IEntity*> loadedEntities;

    auto pendingEntityLoad = [this, &pendingEntities, &loadingMutex, &loadedEntities](std::size_t i){
        EntityLoadPayloadUSD payload = pendingEntities.at(i);
        switch(payload.entityType)
        {
            case EntityType::StaticMesh:
            {
                StaticMeshEntity* staticMeshEntity = GMemoryManager->New<StaticMeshEntity>();
                if (staticMeshEntity->Load(payload.entity))
                {
                    std::lock_guard lock(loadingMutex);
                    loadedEntities.push_back(staticMeshEntity);
                }
                else
                {
                    GMemoryManager->Delete(staticMeshEntity);
                }
                break;
            }
            default:
            {
                Logger::Err("Tried to load unknown entity type");
            }
        }
    };

    auto start = std::chrono::steady_clock::now();
    auto futures = Job::Pool.submit_loop(0, pendingEntities.size(), pendingEntityLoad, 0);
    futures.wait();
    m_entities = std::move(loadedEntities);
    Logger::Info(std::format("Entity loading took: {}", Timing::SinceMS(start)));
}

void World::Destroy()
{
    GRenderer->WaitIdle();
    for (IEntity* entity : m_entities)
    {
        assert(entity->Unload());
        GMemoryManager->Delete(entity);
    }

    m_entities.clear();

    GTextureCache->Destroy();
    GRenderer->DestroyImage(DefaultTexture::g_defaultTextureImage);
    GRenderer->m_bindlessManager.Reset();
}

std::size_t World::GetEntityCount(EntityType entityType) const
{
    std::size_t count = 0;
    for (const IEntity* entity : m_entities)
    {
        if (entity->GetEntityType() == entityType)
        {
            count++;
        }
    }
    return count;
}

std::vector<const IEntity *> World::GetEntitiesOfType(EntityType entityType) const
{
    std::vector<const IEntity *> result;

    for (const IEntity* entity : m_entities)
    {
        if (entity->GetEntityType() == entityType)
        {
            result.push_back(entity);
        }
    }
    return result;
}

}
