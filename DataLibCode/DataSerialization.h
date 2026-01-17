#pragma once
#include <optional>
#include "../CommonCode/Model.h"
#include "../CommonCode/BinaryBlob.h"


namespace Magic::Data
{

inline void SerializeStaticMeshDataBlob(const StaticMeshData& model, const std::string& filename) 
{
    BinaryBlob blob;
    blob.AddU64(model.m_subMeshes.size());
    for (const SubMeshData& mesh : model.m_subMeshes) {
        blob.AddU64(mesh.m_vertices.size());
        blob.AddSimpleVertexArr(mesh.m_vertices.data(), mesh.m_vertices.size());
        blob.AddU64(mesh.m_indices.size());
        blob.AddU32Array(mesh.m_indices.data(), mesh.m_indices.size());
        blob.AddI32(mesh.materialData.diffuseData.width);
        blob.AddI32(mesh.materialData.diffuseData.height);
        blob.AddI32(mesh.materialData.diffuseData.numChannels);
        blob.AddI32(mesh.materialData.diffuseData.baseTextureDataOffset);
    }
    blob.AddMatrix4fArr(model.m_transforms.data(), model.m_transforms.size());
    blob.AddSizeT(model.textureData.size());
    blob.AddUCharArr(model.textureData.data(), model.textureData.size());
    blob.SaveToFile(filename);
}

inline std::optional<StaticMeshData> DeserializeStaticMeshDataBlob(const std::string& filename)
{
    BinaryBlob blob;
    blob.LoadFromFile(filename);
    StaticMeshData model;
    uint64_t subMeshCount = blob.GetU64();
    model.m_subMeshes.resize(subMeshCount);

    for (SubMeshData& mesh : model.m_subMeshes) {
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
    model.m_transforms.resize(model.m_subMeshes.size());
    blob.GetMatrix4fArr(model.m_transforms.data(), model.m_transforms.size());
    std::size_t textureDataSize = blob.GetSizeT();
    model.textureData.resize(textureDataSize);
    blob.GetUCharArr(model.textureData.data(), model.textureData.size());
    return model;
}

}
