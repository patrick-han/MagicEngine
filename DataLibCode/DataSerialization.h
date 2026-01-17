#pragma once
#include <optional>
#include "../EngineCode/Model.h"
#include "../EngineCode/BinaryBlob.h"


namespace Magic::Data
{

inline void SerializeModelDataBlob(const ModelData& model, const std::string& filename) 
{
    BinaryBlob blob;
    blob.AddU64(model.m_subMeshes.size());
    for (const SubMeshData& mesh : model.m_subMeshes) {
        blob.AddU64(mesh.m_vertices.size());
        blob.AddSimpleVertexArr(mesh.m_vertices.data(), mesh.m_vertices.size());
        blob.AddU64(mesh.m_indices.size());
        blob.AddU32Array(mesh.m_indices.data(), mesh.m_indices.size());
        blob.AddSizeT(mesh.materialData.diffuseData.data.size());
        blob.AddUCharArr(mesh.materialData.diffuseData.data.data(), mesh.materialData.diffuseData.data.size());
        blob.AddI32(mesh.materialData.diffuseData.width);
        blob.AddI32(mesh.materialData.diffuseData.height);
        blob.AddI32(mesh.materialData.diffuseData.numChannels);
    }
    blob.AddMatrix4fArr(model.m_transforms.data(), model.m_transforms.size());
    blob.SaveToFile(filename);
}

inline std::optional<ModelData> DeserializeModelDataBlob(const std::string& filename)
{
    BinaryBlob blob;
    blob.LoadFromFile(filename);
    ModelData model;
    uint64_t subMeshCount = blob.GetU64();
    model.m_subMeshes.resize(subMeshCount);

    for (SubMeshData& mesh : model.m_subMeshes) {
        uint64_t vertexCount = blob.GetU64();
        mesh.m_vertices.resize(vertexCount);
        blob.GetSimpleVertexArr(mesh.m_vertices.data(), vertexCount);
        uint64_t indexCount = blob.GetU64();
        mesh.m_indices.resize(indexCount);
        blob.GetU32Array(mesh.m_indices.data(), indexCount);
        std::size_t diffuseTextureByteCount = blob.GetSizeT();
        mesh.materialData.diffuseData.data.resize(diffuseTextureByteCount);
        blob.GetUCharArr(mesh.materialData.diffuseData.data.data(), diffuseTextureByteCount);
        mesh.materialData.diffuseData.width = blob.GetI32();
        mesh.materialData.diffuseData.height = blob.GetI32();
        mesh.materialData.diffuseData.numChannels = blob.GetI32();
    }
    model.m_transforms.resize(model.m_subMeshes.size());
    blob.GetMatrix4fArr(model.m_transforms.data(), model.m_transforms.size());
    return model;
}

}
