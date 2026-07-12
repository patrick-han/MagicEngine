#include "StaticMeshEntity.h"
#include "vjson_header.h"
#include "MemoryManager.h"
#include "../CommonCode/StaticMeshData.h"
#include "Renderer.h"
#include "DefaultTexture.h"
#include "../DataLibCode/DataSerialization.h"
#include "../DataLibCode/ImportGLTF.h"
#include <filesystem>
namespace Magic
{
StaticMeshEntity::StaticMeshEntity() 
    : IEntity()
{

}

StaticMeshEntity::~StaticMeshEntity()
{

}
static constexpr char not_a_string[] = "NOTASTRING";
static const VkFormat g_defaultTextureFormat = VK_FORMAT_R8G8B8A8_UNORM; // TODO: hardcoded default format

bool StaticMeshEntity::Load(vjson::Value *entity)
{
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

    SetName(entityName);
    SetUUID(entityUUID);
    m_transform = mtx;


    vjson::Value* resources = entity->ValuePtrAtKey("resources");
    std::string resourceFilePath = resources->ValuePtrAtKey("staticmesh")->ValuePtrAtKey("path")->AsString(not_a_string);
    std::string resourceName = resources->ValuePtrAtKey("staticmesh")->ValuePtrAtKey("name")->AsString(not_a_string);

    StaticMeshData* staticMeshData = nullptr;


    if (std::filesystem::path(resourceFilePath).extension() == ".gltf")
    {
        GLTFImporter ctx;
        staticMeshData = GMemoryManager->New<StaticMeshData>();
        ctx.ImportGLTF(resourceFilePath, *staticMeshData);
    }
    else
    {
        std::optional<StaticMeshData> staticMeshDataOpt = Data::DeserializeStaticMeshDataBlob(resourceFilePath);
        staticMeshData = GMemoryManager->New<StaticMeshData>(std::move(*staticMeshDataOpt));

        if (!staticMeshData)
        {
            Logger::Err(std::format("DeserializeStaticMeshDataBlob({}): FAILED (could not load '{}')", resourceFilePath, resourceName));
            Logger::Err(std::format("Skipping loading entity: {}", entityName));
            GMemoryManager->Delete(staticMeshData);
            return false;
        }
    }

    std::unordered_map<int, int> m_diffuseBaseTextureDataOffsetToBindlessIndex; // Used to deduplicate textures

    std::size_t subMesh_i = 0;
    for (const SubMeshData& subMeshData : staticMeshData->m_subMeshes)
    {
        SubMesh* pSubMesh = GMemoryManager->New<SubMesh>();
        pSubMesh->indexCount = static_cast<uint32_t>(subMeshData.m_indices.size());
        pSubMesh->m_transform = staticMeshData->m_transforms[subMesh_i];
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

        auto tryUploadTexture = [&m_diffuseBaseTextureDataOffsetToBindlessIndex](
            TextureData subMeshTextureData
            , SubMesh* pSubMesh
            , StaticMeshData* pStaticMeshData) 
        {
            const bool validTexture = subMeshTextureData.width != 0;
            pSubMesh->hasTexture = validTexture; // TODO: this assumes only a single texture!
            if (validTexture)
            {
                const bool textureAlreadyLoaded = m_diffuseBaseTextureDataOffsetToBindlessIndex.find(subMeshTextureData.baseTextureDataOffset) != m_diffuseBaseTextureDataOffsetToBindlessIndex.end();
                if (textureAlreadyLoaded)
                {
                    pSubMesh->diffuseTextureBindlessArraySlot = m_diffuseBaseTextureDataOffsetToBindlessIndex.at(subMeshTextureData.baseTextureDataOffset);
                    Logger::Info("Found duplicate texture, skipping upload");
                }
                else
                {
                    VkExtent3D extent
                    {
                        .width = static_cast<uint32_t>(subMeshTextureData.width)
                        , .height = static_cast<uint32_t>(subMeshTextureData.height)
                        , .depth = 1
                    };
                    const VkImageCreateInfo imci = DefaultImageCreateInfo(g_defaultTextureFormat, extent, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_TYPE_2D);

                    if (!GRenderer->m_bindlessManager.IsBindlessArrayFull())
                    {
                        pSubMesh->diffuseImage = GRenderer->UploadImage(
                            pStaticMeshData->textureData.data() + subMeshTextureData.baseTextureDataOffset
                            , subMeshTextureData.numChannels
                            , imci
                        );
                        auto imageViewCreateInfo = DefaultImageViewCreateInfo(pSubMesh->diffuseImage.image, g_defaultTextureFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
                        pSubMesh->diffuseImage.view = GRenderer->CreateViewForAllocatedImage(imageViewCreateInfo);
                        int bindlessSlot = GRenderer->m_bindlessManager.AddToBindlessTextureArray(pSubMesh->diffuseImage);
                        pSubMesh->diffuseTextureBindlessArraySlot = bindlessSlot;
                        m_diffuseBaseTextureDataOffsetToBindlessIndex[subMeshTextureData.baseTextureDataOffset] = bindlessSlot;
                    }
                    else
                    {
                        Logger::Err("Bindless texture array is full");
                        pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
                    }
                }
            }
            else
            {
                pSubMesh->diffuseTextureBindlessArraySlot = DefaultTexture::g_defaultTextureImageBindlessSlot;
            }
        };


        // Textures
        tryUploadTexture(subMeshData.materialData.diffuseData, pSubMesh, staticMeshData);


        // Finalize
        m_subMeshes.push_back(pSubMesh);
        subMesh_i++;
    }
    
    GMemoryManager->Delete(staticMeshData);
    return true;
}
bool StaticMeshEntity::Unload()
{
    for (SubMesh* pSubMesh : m_subMeshes)
    {
        GRenderer->DestroyBuffer(pSubMesh->vertexBuffer);
        GRenderer->DestroyBuffer(pSubMesh->indexBuffer);
        GRenderer->DestroyImage(pSubMesh->diffuseImage);
        GMemoryManager->Delete<SubMesh>(pSubMesh);
    }
    return true;
}

std::span<SubMesh* const> StaticMeshEntity::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}

}