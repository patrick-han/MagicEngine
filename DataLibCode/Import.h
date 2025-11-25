#pragma once
#include <vector>
#include "../EngineCode/Common/Math/Vector3f.h"
#include "../EngineCode/Common/Math/Matrix4f.h"
#include "DataSerialization.h"
#include <assimp/scene.h>
#include "../EngineCode/Common/Log.h"
#include <filesystem>
#include <iostream>

#include "stb_image.h"


namespace Magic::Data
{

// Assimp uses row major matrices as well
inline Matrix4f ConvertFromAssimpMatrix(const aiMatrix4x4 &aiMat) {
    return {
        aiMat.a1, aiMat.a2, aiMat.a3, aiMat.a4,
        aiMat.b1, aiMat.b2, aiMat.b3, aiMat.b4,
        aiMat.c1, aiMat.c2, aiMat.c3, aiMat.c4,
        aiMat.d1, aiMat.d2, aiMat.d3, aiMat.d4
    };
}

inline const Vector3f defaultVertexColors[] = {
    {1.0f, 0.0f, 0.0f}
    , {0.0f, 1.0f, 0.0f}
    , {0.0f, 0.0f, 1.0f}
};

inline bool DoDaPointersSameData(const unsigned char* ptr1, const unsigned char* ptr2, size_t bytes)
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

inline std::string GetTextureNameFromAssimpType(aiMaterial* material, aiTextureType textureType) {
    aiString str;
    material->GetTexture(textureType, 0, &str);
    const std::string textureName(str.C_Str());
    return textureName;
}

inline void LoadTextureData(const std::filesystem::path& texturePath, TextureData& textureData)
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

inline void ProcessAssimpMesh(SubMeshData& meshData, aiMesh *mesh, const aiScene *scene, const std::filesystem::path& modelpath)
{
    // Vertex colors temp workaround, only allows vertex colors per mesh and not vertex
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    aiColor4D aiColor;
    bool hasVertexColors = true;
    if (material->Get(AI_MATKEY_BASE_COLOR, aiColor) == AI_SUCCESS)
    {
        hasVertexColors = true;
    }


    // Vertices
    for (std::size_t i = 0; i < mesh->mNumVertices; i++)
    {
        SimpleVertex vertex;
        // Vector4f transformedVertex = transformMatrix * Vector4f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        // vertex.position = Vector3f(transformedVertex.x, transformedVertex.y, transformedVertex.z);

        vertex.position = Vector3f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        
        // if (mesh->HasVertexColors(0))
        // {
        //     vertex.color.x = mesh->mColors[0][i].r;
        //     vertex.color.y = mesh->mColors[0][i].g;
        //     vertex.color.z = mesh->mColors[0][i].b;
        // }
        if (hasVertexColors)
        {
            vertex.color = Vector3f(aiColor.r, aiColor.g, aiColor.b);
        }



        if (mesh->HasTextureCoords(0))
        {
            vertex.uv_x = mesh->mTextureCoords[0][i].x;
            vertex.uv_y = mesh->mTextureCoords[0][i].y;
        }

        if (mesh->HasNormals())
        {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }

        meshData.m_vertices.push_back(vertex);
    }
    // Indices
    for (std::size_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(std::size_t j = 0; j < face.mNumIndices; j++)
        {
            uint32_t index = face.mIndices[j];
            meshData.m_indices.push_back(index);
        }
    }
    if (mesh->mMaterialIndex >= 0) // TODO: this is always true? How do you tell if a mesh has no material
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        const bool hasDiffuseMap = material->GetTextureCount(aiTextureType_BASE_COLOR) > 0;
        // const bool hasMetallicRoughnessMap = material->GetTextureCount(aiTextureType_METALNESS) > 0;
        // const bool hasNormalMap = material->GetTextureCount(aiTextureType_NORMALS) > 0;
        // const bool hasEmissiveMap = material->GetTextureCount(aiTextureType_EMISSIVE) > 0;

        MaterialData materialData;
        if (hasDiffuseMap)
        {
            const std::string textureName = GetTextureNameFromAssimpType(material, aiTextureType_BASE_COLOR);
            std::filesystem::path texturePath = modelpath / std::filesystem::path(textureName);
            TextureData textureData;
            LoadTextureData(texturePath, textureData);
            materialData.diffuseData = std::move(textureData);
        }
        meshData.materialData = std::move(materialData);
    }
}

inline void ProcessAssimpNode(ModelData& modelData, aiNode *node, const aiScene *scene, const Matrix4f& parentTransform, const std::filesystem::path& modelpath)
{
    Matrix4f localTransform = ConvertFromAssimpMatrix(node->mTransformation);
    Matrix4f worldTransform = parentTransform * localTransform;

    // Some models like Sponza are flattened under a single node, whereas A Beautiful Game have an actual node hiearchy

    // Process all of this node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        SubMeshData meshData;
        ProcessAssimpMesh(meshData, mesh, scene, modelpath);
        modelData.m_subMeshes.push_back(std::move(meshData));
        modelData.m_transforms.push_back(worldTransform);
    }
    // Recursively process this node's child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessAssimpNode(modelData, node->mChildren[i], scene, worldTransform, modelpath);
    }
}


}