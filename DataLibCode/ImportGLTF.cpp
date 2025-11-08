#include "ImportGLTF.h"
#define CGLTF_IMPLEMENTATION
#include "ThirdParty/cgltf.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Model.h"
#include "stb_image.h"

static int i_node_count = 0;

namespace Magic
{

static bool DoDaPointersSameData(const unsigned char* ptr1, const unsigned char* ptr2, size_t bytes)
{
    for (size_t i = 0; i < bytes; i++)
    {
        if (!(ptr1[i] == ptr2[i]))
        {
            return false;
        }
    }
    return true;
}

static void LoadTextureData(const std::filesystem::path& texturePath, TextureData& textureData)
{
    int textureWidth = 0;
    int textureHeight = 0;
    int numChannels = 0;
    int desiredChannels = 4; // TODO: non-zero if we want to force a certain number of channels
    stbi_uc* textureData_stbi = stbi_load(reinterpret_cast<char const *>(texturePath.u8string().c_str()), &textureWidth, &textureHeight, &numChannels, desiredChannels);
    std::vector<unsigned char> pixels;
    if (!textureData_stbi)
    {
        Logger::Err(std::format("Could not load texture file: {}", texturePath.string()));
        std::exit(1);
    }
    else
    {
        auto textureDataSizeBytes = textureWidth * textureHeight * desiredChannels * sizeof(stbi_uc);
        pixels.assign(textureData_stbi, textureData_stbi + textureDataSizeBytes);
        // test
        if (!DoDaPointersSameData(pixels.data(), textureData_stbi, textureDataSizeBytes)) { Logger::Err("uh oh"); std::abort(); }
        stbi_image_free(textureData_stbi);
    }
    textureData.width = textureWidth;
    textureData.height = textureHeight;
    textureData.numChannels = desiredChannels;
    textureData.data = std::move(pixels);
}

/*
The global transformation matrix of a node is the product of the global transformation matrix of its parent node and its own local transformation matrix. 

When the node has no parent node, its global transformation matrix is identical to its local transformation matrix.
*/

static Matrix4f ConvertCGLTFMatrix(const cgltf_float* const in)
{
    // gltf matrices are column major
    Matrix4f out = Matrix4f(
        in[0], in[1], in[2], in[3]
        , in[4], in[5], in[6], in[7]
        , in[8], in[9], in[10], in[11]
        , in[12], in[13], in[14], in[15]
    ).Transposed();
    return out;
}


void ProcessNode(cgltf_node* node, ModelData& modelData, const std::filesystem::path& filePath)
{
    cgltf_float worldTransformCGLTF[16];
    cgltf_node_transform_world(node, worldTransformCGLTF);
    Matrix4f worldTransform = ConvertCGLTFMatrix(worldTransformCGLTF);



    if (node->camera)
    {
        Logger::Info("Camera node encountered, skipping");
    }
    else if (node->mesh)
    {
        cgltf_mesh* mesh = node->mesh;
        if (mesh->primitives_count > 1) { Logger::Warn("Encountered mesh with more than 1 primitive!"); }
        for (cgltf_size primitive_i = 0; primitive_i < mesh->primitives_count; ++primitive_i) // gltf primitive ~= a mesh in my mind
        {
            // All meshes and primitives of a node share the same transform? Since transforms are defined at the node level
            modelData.m_transforms.push_back(worldTransform);
            //

            MeshData meshData;
            // Typically a mesh will have a single primitive, but in some cases primitives are used for:
            // - Multiple materials per "object", allowing vertex data re-use
            // - Splitting big meshes if bitsize of indices is not enough
            // https://github.com/KhronosGroup/glTF/issues/821#issuecomment-277036938
            const cgltf_primitive* primitive = &mesh->primitives[primitive_i];
            if (!primitive->attributes) { Logger::Info("Encountered primitive has no attributes, skipping it..."); }
            else
            {
                // Find the position attribute and resize vertices
                cgltf_size vertexCount = 0;
                for (cgltf_size attribute_i = 0; attribute_i < primitive->attributes_count; ++attribute_i)
                {
                    const cgltf_attribute& attribute = primitive->attributes[attribute_i];
                    if (attribute.type == cgltf_attribute_type_position)
                    {
                        Logger::Info(std::format("Position attribute count: {}", attribute.data->count));
                        vertexCount = attribute.data->count;
                    }
                }
                if (vertexCount == 0) 
                { 
                    Logger::Err("Primitive has no position attribute! Skipping this node entirely..."); 
                    return;
                };

                cgltf_accessor* indices_accessor = primitive->indices;
                cgltf_size indexCount = indices_accessor->count;
                if (indexCount <= 0) { Logger::Warn("Primitive has no indices!"); }
                meshData.m_indices.resize(indexCount);
                for (cgltf_size index_i = 0; index_i < indexCount; ++index_i)
                {
                    meshData.m_indices.at(index_i) = (uint32_t)cgltf_accessor_read_index(indices_accessor, index_i);
                }
                meshData.m_vertices.resize(vertexCount);
                for (cgltf_size attribute_i = 0; attribute_i < primitive->attributes_count; ++attribute_i)
                {
                    const cgltf_attribute& attribute = primitive->attributes[attribute_i];
                    const cgltf_accessor* accessor = attribute.data;

                    switch (attribute.type)
                    {
                        case cgltf_attribute_type_position:
                        {
                            if (accessor->type != cgltf_type_vec3)
                            {
                                Logger::Err("Position is not vec3! Skipping to next attribute...");
                                continue;
                            }
                            cgltf_float vec[3];
                            for (cgltf_size i = 0; i < accessor->count; ++i)
                            {
                                cgltf_accessor_read_float(accessor, i, vec, 3);
                                meshData.m_vertices.at(i).position = { (float)vec[0], (float)vec[1], (float)vec[2] };
                            }
                            break;
                        }
                        case cgltf_attribute_type_texcoord:
                        {
                            if (accessor->type != cgltf_type_vec2)
                            {
                                Logger::Err("Texcoord is not vec2! Skipping to next attribute...");
                                continue;
                            }
                            cgltf_float vec[2];
                            for (cgltf_size i = 0; i < accessor->count; ++i)
                            {
                                cgltf_accessor_read_float(accessor, i, vec, 2);
                                meshData.m_vertices.at(i).uv_x = (float)vec[0];
                                meshData.m_vertices.at(i).uv_y = (float)vec[1];
                            }
                            break;
                        }
                        case cgltf_attribute_type_normal:
                        {
                            if (accessor->type != cgltf_type_vec3)
                            {
                                Logger::Err("Normal is not vec3! Skipping to next attribute...");
                                continue;
                            }
                            cgltf_float vec[3];
                            for (cgltf_size i = 0; i < accessor->count; ++i)
                            {
                                cgltf_accessor_read_float(accessor, i, vec, 3);
                                meshData.m_vertices.at(i).normal = { (float)vec[0], (float)vec[1], (float)vec[2] };
                            }
                            break;
                        }
                        default:
                        {
                            Logger::Info("Encountered unsupported attribute type, skipping...");
                            break;
                        }
                    }
                }
            }

            // Load materials
            cgltf_material* material = primitive->material;
            Vector3f baseColorFactor; // "Vertex colors" not quite, since there is an attribute for that, but for now
            if (material)
            {
                if (material->has_pbr_metallic_roughness)
                {
                    const cgltf_texture_view baseColor = material->pbr_metallic_roughness.base_color_texture;
                    baseColorFactor.x = material->pbr_metallic_roughness.base_color_factor[0];
                    baseColorFactor.y = material->pbr_metallic_roughness.base_color_factor[1];
                    baseColorFactor.z = material->pbr_metallic_roughness.base_color_factor[2];
                    if (!baseColor.texture)
                    {
                        Logger::Warn("This primitive has no base color texture!");
                    }
                    else
                    {
                        std::vector<uint8_t> baseColorTextureBytes;
                        const cgltf_image* image = baseColor.texture->image;
                        if (image->buffer_view) // Case 1: Image is embedded in the buffer view
                        {
                            // TODO:
                            assert(false);
                        }
                        else // Case 2: Load from file
                        {
                            std::filesystem::path textureFilePath = filePath.parent_path() / std::filesystem::path(image->uri);
                            TextureData textureData;
                            LoadTextureData(textureFilePath, textureData);
                            meshData.materialData.diffuseData = std::move(textureData);
                        }
                    }
                }
            }
            for (auto& vertex : meshData.m_vertices) // fixup vertex colors after material parsing
            {
                vertex.color = baseColorFactor;
            }
            modelData.m_meshes.push_back(std::move(meshData));
        }
    }

    assert(modelData.m_meshes.size() == modelData.m_transforms.size());

    i_node_count++;
    if (node->children_count > 0)
    {
        for (cgltf_size i = 0; i < node->children_count; i++)
        {
            ProcessNode(node->children[i], modelData, filePath);
        }
    }
}

void ImportGLTF(const std::string& filepathStr, ModelData& modelData)
{
    cgltf_options options = {};
    cgltf_data* gltfData = nullptr;

    if (cgltf_parse_file(&options, filepathStr.c_str(), &gltfData) != cgltf_result_success) { Logger::Err("Failed to load gltf file"); exit(1); }

    if (cgltf_load_buffers(&options, gltfData, filepathStr.c_str()) != cgltf_result_success) { Logger::Err("Failed to load gltf buffers"); exit(1); }

    std::filesystem::path filepath(filepathStr);

    assert(gltfData->scenes_count == 1);
    Logger::Info(std::format("Total node count: {}", gltfData->nodes_count));
    Logger::Info(std::format("Scene (top-level) node count: {}", gltfData->scenes[0].nodes_count));

    cgltf_scene scene = gltfData->scenes[0];

    for (cgltf_size i = 0; i < scene.nodes_count; ++i)
    {
        cgltf_node* node = scene.nodes[i];
        ProcessNode(node, modelData, filepath);
    }

    Logger::Info(std::format("Total nodes processed: {}", i_node_count ));
    i_node_count = 0;
    cgltf_free(gltfData);
}


}