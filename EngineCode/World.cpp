#include "World.h"
#include "Renderer.h"

namespace Magic
{

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


    vjson::Object jsonWorld;
    vjson::ParseContext ctx;
    std::string sjson;
    assert(LoadJsonToString(worldPath, sjson));
    if ( !jsonWorld.ParseJSON( sjson, &ctx ) )
    {
        fprintf( stderr, "Parse failed line %d: %s\n",
            ctx.error_line, ctx.error_message.c_str() );

        Logger::Err("Could not open world file");
    }
    vjson::Value* root = jsonWorld.ValuePtrAtKey("root");
    vjson::Array& entities = root->ValuePtrAtKey("entities")->GetArray();
    const std::size_t entityCount = entities.size();

    for (std::size_t entity_i = 0; entity_i < entityCount; entity_i++)
    {
        vjson::Value* entity = entities.ValuePtrAtIndex(entity_i);
        const char* entityTypeString = entity->AtKey("type").AsCString("Unknown");
        EntityType entityType = Entity::StrToEntityType(entityTypeString);
        switch(entityType)
        {
            case EntityType::StaticMesh:
            {
                StaticMeshEntity* staticMeshEntity = GMemoryManager->New<StaticMeshEntity>();
                if (staticMeshEntity->Load(entity))
                {
                    m_entities.push_back(staticMeshEntity);
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
    }
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