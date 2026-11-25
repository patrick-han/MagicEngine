#include "World.h"
#include "Renderer.h"
#include "TextureCache.h"
#include "Threading.h"
#include "Entity.h"
#include "vjson_header.h"

#include <unordered_map>
#include <mutex>
namespace Magic
{

void World::LoadJSON(const char* worldPath)
{
    // Every material texture slot is always valid. Missing base-color and
    // metallic-roughness maps use white; missing normal maps use a flat normal.
    {        
        auto uploadDefaultTexture = [](VkFormat format, const DefaultTexture::TextureArray& pixels, AllocatedImage& image, int& bindlessSlot)
        {
            constexpr VkExtent3D extent { .width = 128 , .height = 128 , .depth = 1 };
            const VkImageCreateInfo imci = DefaultImageCreateInfo(format, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
            image = GRenderer->UploadImage(pixels.data(), DefaultTexture::CHANNELS, imci);
            const VkImageViewCreateInfo imageViewCreateInfo = DefaultImageViewCreateInfo(
                image.image,
                imci.format,
                VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                VK_IMAGE_ASPECT_COLOR_BIT);
            image.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
            bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(image);
        };

        uploadDefaultTexture(VK_FORMAT_R8G8B8A8_SRGB, DefaultTexture::g_DefaultTexture, DefaultTexture::g_defaultTextureImage, DefaultTexture::g_defaultTextureImageBindlessSlot);
        uploadDefaultTexture(VK_FORMAT_R8G8B8A8_SRGB, DefaultTexture::g_WhiteTexture, DefaultTexture::g_whiteTextureImage, DefaultTexture::g_whiteTextureImageBindlessSlot);
        uploadDefaultTexture(VK_FORMAT_R8G8B8A8_UNORM, DefaultTexture::g_FlatNormalTexture, DefaultTexture::g_flatNormalTextureImage, DefaultTexture::g_flatNormalTextureImageBindlessSlot);
    }

    std::ifstream file(worldPath, std::ios::binary);
    if (!file)
    {
        Logger::Err("Failed to open file stream");
        return;
    }
    const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>{});
    if (file.bad())
    {
        Logger::Err("Bad file");
        return;
    }

    vjson::ParseContext ctx;
    vjson::Object doc;
    if (!doc.ParseJSON(contents, &ctx))
    {
        Logger::Err(std::format("JSON error on line: {} with message: {}", ctx.error_line, ctx.error_message));
        return;
    }
    
    std::unordered_map<std::size_t, IEntity*> idToEntity;
    std::unordered_map<IEntity*, std::size_t> entityToParentId;
    {
        struct PendingEntityRecord
        {
            IEntity* entity = nullptr;
            int parentId = -1;
        };
        // Parse all entities and queue them for loading
        std::vector<PendingEntityRecord> pendingEntities;
        for (const vjson::Object &e : doc.ArrayAtKeyOrEmpty("entities").Iter<vjson::Object>())
        {
            std::string entityName = e.StringAtKey("name", "?");
            std::string entityType = e.StringAtKey("type", "?");
            Logger::Info(std::format("Entity Name: {}", entityName));
            Logger::Info(std::format("Entity Type: {}", entityType));
            int id = e.IntAtKey("id", 0);
            int parentId = e.IntAtKey("parent_entity_id", 0);
            assert(id > 0);
            assert(parentId >= 0);
            const vjson::Object& transform = e.ObjectAtKeyOrEmpty("transform");
            auto GetMatrix4fFromTransformObject = [](const vjson::Object& transform){
                const vjson::Array& position = transform.ArrayAtKeyOrEmpty("position");
                const vjson::Array& rotationEuler = transform.ArrayAtKeyOrEmpty("rotation_euler");
                const vjson::Array& scale = transform.ArrayAtKeyOrEmpty("scale");
                Matrix4f translate = Matrix4f::MakeTranslate(
                    position.DoubleAtIndex(0, 0.0)
                    , position.DoubleAtIndex(1, 0.0)
                    , position.DoubleAtIndex(2, 0.0)
                );
                Matrix4f scaleMtx = Matrix4f::MakeScale(
                    scale.DoubleAtIndex(0, 0.0)
                    , scale.DoubleAtIndex(1, 0.0)
                    , scale.DoubleAtIndex(2, 0.0)
                );
                Matrix4f rotX = Matrix4f::MakeRotateX(rotationEuler.DoubleAtIndex(0, 0.0));
                Matrix4f rotY = Matrix4f::MakeRotateY(rotationEuler.DoubleAtIndex(1, 0.0));
                Matrix4f rotZ = Matrix4f::MakeRotateZ(rotationEuler.DoubleAtIndex(2, 0.0));
                return rotX * rotY * rotZ * scaleMtx * translate;
            };

            if (entityType == "StaticMesh")
            {
                const vjson::Object* meshComponent = e.ObjectPtrAtKey("mesh");
                if (meshComponent)
                {
                    StaticMeshEntity* staticMeshEntity = GMemoryManager->New<StaticMeshEntity>();
                    staticMeshEntity->SetName(std::move(entityName));
                    staticMeshEntity->SetUUID(id);
                    staticMeshEntity->m_transform = GetMatrix4fFromTransformObject(transform);
                    staticMeshEntity->meshComponent = meshComponent;
                    pendingEntities.push_back({staticMeshEntity, parentId});
                }
                else
                {
                    Logger::Warn(std::format("StaticMesh entity with id {} missing mesh component, check the world file", id));
                }
            }
            else if (entityType == "DirectionalLight")
            {
                if (m_pDirLight)
                {
                    Logger::Warn("Directional light already exists, skipping...");
                    continue;
                }
                const vjson::Object* params = e.ObjectPtrAtKey("params");
                if (params)
                {
                    m_pDirLight = GMemoryManager->New<DirectionalLightEntity>();
                    m_pDirLight->SetName(std::move(entityName));
                    m_pDirLight->SetUUID(id);
                    m_pDirLight->m_transform = GetMatrix4fFromTransformObject(transform);
                    m_pDirLight->params = params;
                    pendingEntities.push_back({m_pDirLight, parentId});
                }
                else
                {
                    Logger::Warn(std::format("DirectionalLight entity with id {} missing params component, check the world file", id));
                }
            }
            else
            {
                Logger::Warn(std::format("Unknown entity type encountered for entity: {}", entityName));
            }
        }

        // Do the loading
        std::mutex loadingMutex;
        std::vector<IEntity*> loadedEntities;

        auto pendingEntityLoad = [this, &pendingEntities, &loadingMutex, &loadedEntities, &idToEntity, &entityToParentId](std::size_t i){
            IEntity* entity = pendingEntities.at(i).entity;
            int parentId = pendingEntities.at(i).parentId;
            switch(entity->GetEntityType())
            {
                case EntityType::StaticMesh:
                {
                    StaticMeshEntity* staticMeshEntity = static_cast<StaticMeshEntity*>(entity);
                    if (staticMeshEntity->Load(staticMeshEntity->meshComponent))
                    {
                        std::lock_guard lock(loadingMutex);
                        staticMeshEntity->meshComponent = nullptr;
                        loadedEntities.push_back(staticMeshEntity);
                        auto res = idToEntity.insert({entity->GetUUID().Value(), entity});
                        assert (res.second); // Make sure there are no duplicate keys
                        entityToParentId.insert({entity, parentId});
                    }
                    else
                    {
                        Logger::Warn(std::format("StaticMeshEntity {} failed to load", staticMeshEntity->GetUUID().Value()));
                        GMemoryManager->Delete(staticMeshEntity);
                    }
                    break;
                }
                case EntityType::DirectionalLight:
                {
                    DirectionalLightEntity* directionalLightEntity = static_cast<DirectionalLightEntity*>(entity);
                    if (directionalLightEntity->Load(directionalLightEntity->params))
                    {
                        std::lock_guard lock(loadingMutex);
                        directionalLightEntity->params = nullptr;
                        loadedEntities.push_back(directionalLightEntity);
                        auto res = idToEntity.insert({entity->GetUUID().Value(), entity});
                        assert (res.second); // Make sure there are no duplicate keys
                        entityToParentId.insert({entity, parentId});
                    }
                    else
                    {
                        Logger::Warn(std::format("DirectionalLightEntity {} failed to load", directionalLightEntity->GetUUID().Value()));
                        GMemoryManager->Delete(directionalLightEntity);
                        m_pDirLight = nullptr;
                    }
                    break;
                }
            }
        };

        auto start = std::chrono::steady_clock::now();
        auto futures = Job::Pool.submit_loop(0, pendingEntities.size(), pendingEntityLoad, 0);
        futures.wait();
        m_entities = std::move(loadedEntities);
        Logger::Info(std::format("Entity loading took: {}", Timing::SinceMS(start)));
    }

    // Fix up relationships
    auto startFixup = std::chrono::steady_clock::now();
    for (IEntity* entity : m_entities)
    {
        std::size_t parentId = entityToParentId.at(entity);
        assert(entity->GetParent() == nullptr);
        if (parentId == 0) // Root entities
        {
            continue;
        }
        const auto it = idToEntity.find(parentId);
        if (it != idToEntity.end())
        {
            IEntity* parent = it->second;
            assert(entity != parent && "Entity is trying to reference itself!");
#ifndef NDEBUG
            // check for cycles where B is a parent of A, but somehow A is a parent of B (or any depth of that)
            for(IEntity* ancestor = parent; ancestor; ancestor = ancestor->GetParent())
            {
                assert(ancestor != entity && "Entity hierarchy contains a cycle");
            }
#endif
            parent->AddChild(entity);
        }
        else
        {
            Logger::Warn("Entity parentId is invalid or parent failed to load");
        }
    }
    Logger::Info(std::format("Entity fixup took: {}", Timing::SinceMS(startFixup)));
}

void World::Destroy()
{
    GRenderer->WaitIdle();
    for (IEntity* entity : m_entities)
    {
        assert(entity->Unload());
        GMemoryManager->Delete(entity);
    }
#if MAGIC_USE_SIMPLE_UUID
    UUID::ResetUUIDs();
#endif
    m_entities.clear();
    m_pDirLight = nullptr;

    GTextureCache->Destroy();
    GRenderer->DestroyImage(DefaultTexture::g_defaultTextureImage);
    GRenderer->DestroyImage(DefaultTexture::g_whiteTextureImage);
    GRenderer->DestroyImage(DefaultTexture::g_flatNormalTextureImage);
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
