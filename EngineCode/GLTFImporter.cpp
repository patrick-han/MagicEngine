#include "GLTFImporter.h"
#include "StaticMeshData.h"
#include "Log.h"
#define CGLTF_IMPLEMENTATION
#include "ThirdParty/cgltf.h"
#include "vjson_header.h"
#include "Math/Matrix4f.h"

#include <filesystem>
#include <format>
#include <string>

static Matrix4f ConvertCGLTFMatrix(const cgltf_float* const in)
{
    // gltf matrices are column major
    Matrix4f out = Matrix4f(
        in[0], in[1], in[2], in[3]
        , in[4], in[5], in[6], in[7]
        , in[8], in[9], in[10], in[11   ]
        , in[12], in[13], in[14], in[15]
    ).Transposed();
    return out;
}
namespace Magic
{
void GLTFImporterNew::ProcessNode(cgltf_node* node, StaticMeshData& staticMeshDataData, const std::filesystem::path& filePath)
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
            staticMeshDataData.m_transforms.push_back(worldTransform);
            //

            SubMeshData meshData;
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
                        // Logger::Info(std::format("Position attribute count: {}", attribute.data->count));
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
                bool hasTangents = false;
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
                            if (attribute.index != 0)
                            {
                                // ignore TEXCOORD_1+ for now
                                break;
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
                        case cgltf_attribute_type_tangent:
                        {
                            hasTangents = true;
                            if (accessor->type != cgltf_type_vec4)
                            {
                                Logger::Err("Tangent is not vec4! Skipping to next attribute...");
                                continue;
                            }
                            cgltf_float vec[4];
                            for (cgltf_size i = 0; i < accessor->count; ++i)
                            {
                                cgltf_accessor_read_float(accessor, i, vec, 3);
                                meshData.m_vertices.at(i).tangentW = Vector4f((float)vec[0], (float)vec[1], (float)vec[2], (float)vec[3]);
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
                if (!hasTangents)
                {
                    Logger::Warn("GLTF primitive is missing tangents!");
                }
            }

            // Load materials
            // cgltf_material* material = primitive->material;
            // Vector3f baseColorFactor; // "Vertex colors" not quite, since there is an attribute for that, but for now
            // if (material)
            // {
            //     auto loadTextureView = [this, &filePath, &staticMeshDataData](const cgltf_texture_view& textureView, TextureData& destination) -> bool
            //     {
            //         if (!textureView.texture)
            //         {
            //             return false;
            //         }

            //         assert(textureView.texcoord == 0); // Only TEXCOORD_0 is supported for now.
            //         const cgltf_image* image = textureView.texture->image;
            //         if (!image)
            //         {
            //             Logger::Warn("Texture has no image");
            //             return false;
            //         }
            //         if (image->buffer_view)
            //         {
            //             Logger::Warn("Embedded glTF images are not supported yet");
            //             return false;
            //         }
            //         if (!image->uri)
            //         {
            //             Logger::Warn("External glTF image has no URI");
            //             return false;
            //         }

            //         const std::filesystem::path textureFilePath = (filePath.parent_path() / std::filesystem::path(image->uri)).lexically_normal();
            //         const std::string textureKey = textureFilePath.string();
            //         const auto previouslyLoaded = m_texturesSeen.find(textureKey);
            //         if (previouslyLoaded != m_texturesSeen.end())
            //         {
            //             Logger::Info(std::format("Reusing texture '{}'", textureKey));
            //             destination = previouslyLoaded->second;
            //             return true;
            //         }

            //         TextureData textureData;
            //         LoadTextureData(textureFilePath, textureData, staticMeshDataData);
            //         destination = textureData;
            //         m_texturesSeen.emplace(textureKey, std::move(textureData));
            //         return true;
            //     };

            //     if (material->has_pbr_metallic_roughness)
            //     {
            //         const cgltf_texture_view baseColor = material->pbr_metallic_roughness.base_color_texture;
            //         baseColorFactor.x = material->pbr_metallic_roughness.base_color_factor[0];
            //         baseColorFactor.y = material->pbr_metallic_roughness.base_color_factor[1];
            //         baseColorFactor.z = material->pbr_metallic_roughness.base_color_factor[2];
            //         meshData.materialData.metallicFactor = material->pbr_metallic_roughness.metallic_factor;
            //         meshData.materialData.roughnessFactor = material->pbr_metallic_roughness.roughness_factor;

            //         if (!loadTextureView(baseColor, meshData.materialData.diffuseData))
            //         {
            //             Logger::Warn("This primitive has no base color texture!");
            //         }

            //         loadTextureView(material->pbr_metallic_roughness.metallic_roughness_texture, meshData.materialData.metallicRoughnessData);
            //     }
            // }
            for (auto& vertex : meshData.m_vertices) // fixup vertex colors after material parsing
            {
                vertex.color = Vector3f(1.0f);
            }
            staticMeshDataData.m_subMeshes.push_back(std::move(meshData));
        }
    }

    assert(staticMeshDataData.m_subMeshes.size() == staticMeshDataData.m_transforms.size());

    m_i_node_count++;
    if (node->children_count > 0)
    {
        for (cgltf_size i = 0; i < node->children_count; i++)
        {
            ProcessNode(node->children[i], staticMeshDataData, filePath);
        }
    }
}


void GLTFImporterNew::ImportGLTFAsStaticMesh(const std::filesystem::path& filePath, StaticMeshData &staticMeshDataData)
{
    std::string filePathStr = filePath.string();
    cgltf_options options = {};
    cgltf_data* gltfData = nullptr;

    if (cgltf_parse_file(&options, filePathStr.c_str(), &gltfData) != cgltf_result_success) { Logger::Err("Failed to load gltf file"); exit(1); }

    if (cgltf_load_buffers(&options, gltfData, filePathStr.c_str()) != cgltf_result_success) { Logger::Err("Failed to load gltf buffers"); exit(1); }

    assert(gltfData->scenes_count == 1);
    Logger::Info(std::format("Total node count: {}", gltfData->nodes_count));
    Logger::Info(std::format("Scene (top-level) node count: {}", gltfData->scenes[0].nodes_count));

    cgltf_scene scene = gltfData->scenes[0];

    for (cgltf_size i = 0; i < scene.nodes_count; ++i)
    {
        cgltf_node* node = scene.nodes[i];
        ProcessNode(node, staticMeshDataData, filePath);
    }

    Logger::Info(std::format("Total nodes processed: {}", m_i_node_count ));
    m_i_node_count = 0;
    cgltf_free(gltfData);

}
}
