#pragma once
#include <optional>
#include "../CommonCode/StaticMeshData.h"
#include "../CommonCode/BinaryBlob.h"


namespace Magic::Data
{

inline void SerializeStaticMeshDataBlob(const StaticMeshData& staticMeshData, const std::string& filename) 
{
    BinaryBlob blob;
    blob.AddU64(staticMeshData.m_subMeshes.size());
    for (const SubMeshData& mesh : staticMeshData.m_subMeshes) {
        blob.AddU64(mesh.m_vertices.size());
        blob.AddSimpleVertexArr(mesh.m_vertices.data(), mesh.m_vertices.size());
        blob.AddU64(mesh.m_indices.size());
        blob.AddU32Array(mesh.m_indices.data(), mesh.m_indices.size());
        blob.AddI32(mesh.materialData.diffuseData.width);
        blob.AddI32(mesh.materialData.diffuseData.height);
        blob.AddI32(mesh.materialData.diffuseData.numChannels);
        blob.AddI32(mesh.materialData.diffuseData.baseTextureDataOffset);
    }
    blob.AddMatrix4fArr(staticMeshData.m_transforms.data(), staticMeshData.m_transforms.size());
    blob.AddSizeT(staticMeshData.textureData.size());
    blob.AddUCharArr(staticMeshData.textureData.data(), staticMeshData.textureData.size());
    blob.SaveToFile(filename);
}

inline std::optional<StaticMeshData> DeserializeStaticMeshDataBlob(const std::string& filename)
{
    BinaryBlob blob;
    blob.LoadFromFile(filename);
    StaticMeshData staticMeshData;
    uint64_t subMeshCount = blob.GetU64();
    staticMeshData.m_subMeshes.resize(subMeshCount);

    for (SubMeshData& mesh : staticMeshData.m_subMeshes) {
        uint64_t vertexCount = blob.GetU64();
        mesh.m_vertices.resize(vertexCount);
        blob.GetSimpleVertexArr(mesh.m_vertices.data(), vertexCount);
        uint64_t indexCount = blob.GetU64();
        mesh.m_indices.resize(indexCount);
        blob.GetU32Array(mesh.m_indices.data(), indexCount);
        mesh.materialData.diffuseData.width = blob.GetI32();
        mesh.materialData.diffuseData.height = blob.GetI32();
        mesh.materialData.diffuseData.numChannels = blob.GetI32();
        mesh.materialData.diffuseData.baseTextureDataOffset = blob.GetI32();
    }
    staticMeshData.m_transforms.resize(staticMeshData.m_subMeshes.size());
    blob.GetMatrix4fArr(staticMeshData.m_transforms.data(), staticMeshData.m_transforms.size());
    std::size_t textureDataSize = blob.GetSizeT();
    staticMeshData.textureData.resize(textureDataSize);
    blob.GetUCharArr(staticMeshData.textureData.data(), staticMeshData.textureData.size());
    return staticMeshData;
}

}
