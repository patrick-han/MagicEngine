#pragma once
#include <vector>
#include "../EngineCode/Common/Math/Vector3f.h"
#include "../EngineCode/Common/Math/Matrix4f.h"
#include "Data.h"
#include <assimp/scene.h>


namespace Magic::Data
{

// Assimp uses row major matrices as well
Matrix4f ConvertFromAssimpMatrix(const aiMatrix4x4 &aiMat) {
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

inline void ProcessAssimpMesh(MeshData& meshData, aiMesh *mesh, const aiScene *scene, const Matrix4f& transformMatrix)
{
    // Vertices
    for (std::size_t i = 0; i < mesh->mNumVertices; i++)
    {
        SimpleVertex vertex;
        Vector4f transformedVertex = transformMatrix * Vector4f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.position = Vector3f(transformedVertex.x, transformedVertex.y, transformedVertex.z);

        size_t color_i = i%3;
        vertex.color = defaultVertexColors[color_i];

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
    // TODO: materials/textures
}

inline void ProcessAssimpNode(ModelData& modelData, aiNode *node, const aiScene *scene, const Matrix4f& accumulateMatrix)
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
        ProcessAssimpMesh(meshData, mesh, scene, relativeToRootTransform);
        modelData.m_meshes.push_back(meshData);
        modelData.m_transforms.push_back(relativeToRootTransform); // ??
    }
    // Recursively process this node's child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessAssimpNode(modelData, node->mChildren[i], scene, relativeToRootTransform);
    }
}


}