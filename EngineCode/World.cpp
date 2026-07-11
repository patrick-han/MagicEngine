#include "World.h"

namespace Magic
{

void World::Load(const char* worldPath)
{
    // Upload default texture
    {
        constexpr VkExtent3D extent { .width = 128 , .height = 128 , .depth = 1 };
        const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);
        constexpr size_t bytesPerChannel = 1;
        constexpr size_t numChannels = 4;
        constexpr size_t dataSize = extent.width * extent.height * numChannels * bytesPerChannel;
        AllocatedBuffer stagingBuffer = GRenderer->UploadBuffer(dataSize, DefaultTexture::g_DefaultTexture.data(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        DefaultTexture::g_defaultTextureImage = GRenderer->UploadImage(DefaultTexture::g_DefaultTexture.data(), 4, imci);
        auto imageViewCreateInfo = DefaultImageViewCreateInfo(DefaultTexture::g_defaultTextureImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
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
        const char* entityName = entity->AtKey("name").AsCString(not_a_string);
        UUID entityUUID;
        assert(UUID::TryParse(entity->AtKey("uuid").AsString(not_a_string), entityUUID));
        const char* entityTypeString = entity->AtKey("type").AsCString(not_a_string);

        vjson::Value* entityTransformMatrixJson = entity->ValuePtrAtKey("transform")->ValuePtrAtKey("matrix");
        Matrix4f mtx;
        {
            auto ReadMatrix4fJson = [](vjson::Value* inTransformMatrix) -> Matrix4f {
                auto transformMatrixRows = inTransformMatrix->GetArray();

                Matrix4f result;

                auto ReadJsonMatrixRow = [&transformMatrixRows, &result](std::size_t row_i){
                    auto rowi = transformMatrixRows.ArrayAtIndexOrEmpty(row_i);
                    result.m[row_i * 4 + 0] = rowi.AtIndex(0).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 1] = rowi.AtIndex(1).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 2] = rowi.AtIndex(2).InterpretAsDouble(1337.0);
                    result.m[row_i * 4 + 3] = rowi.AtIndex(3).InterpretAsDouble(1337.0);
                };

                ReadJsonMatrixRow(0);
                ReadJsonMatrixRow(1);
                ReadJsonMatrixRow(2);
                ReadJsonMatrixRow(3);

                return result;
            };
            mtx = ReadMatrix4fJson(entityTransformMatrixJson);
        }

        vjson::Value* resources = entity->ValuePtrAtKey("resources");
        EntityType entityType = Entity::StrToEntityType(entityTypeString);

        
        switch(entityType)
        {
            case EntityType::StaticMesh:
            {
                StaticMeshEntity staticMeshEntity;
                staticMeshEntity.SetName(entityName);
                staticMeshEntity.SetUUID(entityUUID);
                staticMeshEntity.m_transform = mtx;
                staticMeshEntity.m_staticMesh = GMemoryManager->New<StaticMesh>();
                StaticMesh* pStaticMesh = staticMeshEntity.m_staticMesh;

                std::string resourceFilePath = resources->ValuePtrAtKey("staticmesh")->ValuePtrAtKey("path")->AsString(not_a_string);
                std::string resourceName = resources->ValuePtrAtKey("staticmesh")->ValuePtrAtKey("name")->AsString(not_a_string);

                StaticMeshData* pStaticMeshData = nullptr;
                {
                    std::optional<StaticMeshData> staticMeshData = Data::DeserializeStaticMeshDataBlob(resourceFilePath);
                    pStaticMeshData = GMemoryManager->New<StaticMeshData>(std::move(*staticMeshData));

                    if (!staticMeshData)
                    {
                        Logger::Err(std::format("DeserializeStaticMeshDataBlob({}): FAILED (could not load '{}')", resourceFilePath, resourceName));
                        Logger::Err(std::format("Skipping loading entity: {}", entityName));
                        break;
                    }
                }

                std::size_t subMesh_i = 0;
                for (const SubMeshData& subMeshData : pStaticMeshData->m_subMeshes)
                {
                    SubMesh* pSubMesh = GMemoryManager->New<SubMesh>();
                    pSubMesh->indexCount = static_cast<uint32_t>(subMeshData.m_indices.size());
                    pSubMesh->m_transform = pStaticMeshData->m_transforms[subMesh_i];
                    // calculate aabb, TODO: this can be spun off into a separate job, or better yet done in the cooker
                    for (const auto& vertex : subMeshData.m_vertices)
                    {
                        pSubMesh->aabb.Update(vertex.position);
                    }
                    // Vertex and Index buffers
                    AllocatedBuffer vertexBuffer = GRenderer->UploadBuffer(
                        sizeof(SimpleVertex) * subMeshData.m_vertices.size()
                        , static_cast<const void*>(subMeshData.m_vertices.data())
                        , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

                    AllocatedBuffer indexBuffer = GRenderer->UploadBuffer(
                        sizeof(uint32_t) * subMeshData.m_indices.size()
                        , static_cast<const void*>(subMeshData.m_indices.data())
                        , VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                    
                    pSubMesh->vertexBuffer = vertexBuffer;
                    pSubMesh->indexBuffer = indexBuffer;


                    // Textures
                    const bool hasDiffuseTexture = subMeshData.materialData.diffuseData.width != 0;
                    if (hasDiffuseTexture) // TODO:
                    {
                        VkExtent3D extent
                        {
                            .width = static_cast<uint32_t>(subMeshData.materialData.diffuseData.width)
                            , .height = static_cast<uint32_t>(subMeshData.materialData.diffuseData.height)
                            , .depth = 1
                        };
                        const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);

                        pSubMesh->diffuseImage = GRenderer->UploadImage(
                            pStaticMeshData->textureData.data() + subMeshData.materialData.diffuseData.baseTextureDataOffset
                            , subMeshData.materialData.diffuseData.numChannels
                            , imci
                        );
                        auto imageViewCreateInfo = DefaultImageViewCreateInfo(pSubMesh->diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                        pSubMesh->diffuseImage.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
                        int bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(pSubMesh->diffuseImage);
                        if (bindlessSlot < 0) // If we run out of bindless slots
                        {
                            pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
                        }
                        else
                        {
                            pSubMesh->diffuseTextureBindlessArraySlot = bindlessSlot;
                        }
                    }
                    else
                    {
                        pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
                    }

                    // Finalize
                    pStaticMesh->AddSubMesh(pSubMesh);
                    subMesh_i++;
                }
                m_entities.push_back(staticMeshEntity);
                GMemoryManager->Delete(pStaticMeshData);
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
    for (StaticMeshEntity& entity : m_entities)
    {
        StaticMesh* pStaticMesh = entity.m_staticMesh;
        for (SubMesh* pSubMesh : pStaticMesh->GetSubMeshes())
        {
            GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
            GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
            GRenderer->DestroyImage(pSubMesh->diffuseImage);
            GMemoryManager->Delete<SubMesh>(pSubMesh);
        }
        GMemoryManager->Delete<StaticMesh>(pStaticMesh);
    }

    m_entities.clear();

    GRenderer->DestroyImage(DefaultTexture::g_defaultTextureImage);
    GRenderer->m_bindlessManager.Reset();
}

std::size_t World::GetEntityCount(EntityType entityType) const
{
    std::size_t count = 0;
    for (const StaticMeshEntity& entity : m_entities)
    {
        if (entity.GetEntityType() == entityType)
        {
            count++;
        }
    }
    return count;
}

std::vector<const IEntity *> World::GetEntitiesOfType(EntityType entityType) const
{
    std::vector<const IEntity *> result;

    for (const StaticMeshEntity& entity : m_entities)
    {
        if (entity.GetEntityType() == entityType)
        {
            result.push_back(&entity);
        }
    }
    return result;
}

}