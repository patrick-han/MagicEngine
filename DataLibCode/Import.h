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

inline void ProcessAssimpMesh(MeshData& meshData, aiMesh *mesh, const aiScene *scene, const Matrix4f& transformMatrix, const std::filesystem::path& modelpath)
{
    // Vertices
    for (std::size_t i = 0; i < mesh->mNumVertices; i++)
    {
        SimpleVertex vertex;
        Vector4f transformedVertex = transformMatrix * Vector4f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.position = Vector3f(transformedVertex.x, transformedVertex.y, transformedVertex.z);

        size_t color_i = i%3;
        vertex.color = defaultVertexColors[color_i];

        if (mesh->HasTextureCoords(0))
        {
            vertex.uv_x = mesh->mTextureCoords[0][i].x;
            vertex.uv_y = mesh->mTextureCoords[0][i].y;
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
            int textureWidth = 0;
            int textureHeight = 0;
            int numChannels = 0;
            int desiredChannels = 0; // non-zero if we want to force a certain number of channels
            stbi_uc* textureData_stbi = stbi_load(texturePath.c_str(), &textureWidth, &textureHeight, &numChannels, desiredChannels);
            std::vector<unsigned char> pixels;
            if (textureData_stbi)
            {
                auto textureDataSizeBytes = textureWidth * textureHeight * numChannels * sizeof(stbi_uc);
                pixels.assign(textureData_stbi, textureData_stbi + textureDataSizeBytes);
                // test
                if (!DoDaPointersSameData(pixels.data(), textureData_stbi, textureDataSizeBytes)) { Logger::Err("uh oh"); std::abort(); }
                stbi_image_free(textureData_stbi);
            }
            TextureData textureData =
            {
                .width = textureWidth
                , .height = textureHeight
                , .numChannels = numChannels
                , .data = std::move(pixels)
            };
            materialData.diffuseData = std::move(textureData);
        }




        meshData.materialData = std::move(materialData);
    }
}

inline void ProcessAssimpNode(ModelData& modelData, aiNode *node, const aiScene *scene, const Matrix4f& accumulateMatrix, const std::filesystem::path& modelpath)
{
    Matrix4f relativeToParentTransform = ConvertFromAssimpMatrix(node->mTransformation);
    Matrix4f relativeToRootTransform = relativeToParentTransform * accumulateMatrix;
    // TODO: Actually apply transforms?

    // Some models like Sponza are flattened under a single node, whereas A Beautiful Game have an actual node hiearchy
    // Either way, we want all meshes across all nodes to be separate, maintaining no hiearchy whatsoever.

    // Process all of this node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        MeshData meshData;
        ProcessAssimpMesh(meshData, mesh, scene, relativeToRootTransform, modelpath);
        modelData.m_meshes.push_back(meshData);
        modelData.m_transforms.push_back(relativeToRootTransform); // ??
    }
    // Recursively process this node's child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessAssimpNode(modelData, node->mChildren[i], scene, relativeToRootTransform, modelpath);
    }
}


}