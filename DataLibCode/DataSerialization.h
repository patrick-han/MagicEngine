#pragma once
#include <fstream>
#include "../EngineCode/Vertex.h"
#include "../EngineCode/Model.h"


namespace Magic::Data
{

inline void SerializeModelData(const ModelData& model, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Failed to open file for writing");

    uint64_t subMeshCount = model.m_subMeshes.size();
    out.write(reinterpret_cast<const char*>(&subMeshCount), sizeof(subMeshCount));

    for (const SubMeshData& mesh : model.m_subMeshes) {
        uint64_t vertexCount = mesh.m_vertices.size();
        out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
        out.write(reinterpret_cast<const char*>(mesh.m_vertices.data()), vertexCount * sizeof(SimpleVertex));

        uint64_t indexCount = mesh.m_indices.size();
        out.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
        out.write(reinterpret_cast<const char*>(mesh.m_indices.data()), indexCount * sizeof(uint32_t));

        size_t diffuseTextureByteCount = mesh.materialData.diffuseData.data.size();
        out.write(reinterpret_cast<const char*>(&diffuseTextureByteCount), sizeof(size_t));
        out.write(reinterpret_cast<const char*>(mesh.materialData.diffuseData.data.data()), diffuseTextureByteCount * sizeof(unsigned char));
        out.write(reinterpret_cast<const char*>(&mesh.materialData.diffuseData.width), sizeof(int));
        out.write(reinterpret_cast<const char*>(&mesh.materialData.diffuseData.height), sizeof(int));
        out.write(reinterpret_cast<const char*>(&mesh.materialData.diffuseData.numChannels), sizeof(int));
    }

    for (auto& transform : model.m_transforms)
    {
        out.write(reinterpret_cast<const char*>(&transform), sizeof(Matrix4f));
    }

}

inline std::optional<ModelData> DeserializeModelData(const std::string& filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        return std::nullopt;
    }

    ModelData model;
    uint64_t subMeshCount;
    in.read(reinterpret_cast<char*>(&subMeshCount), sizeof(subMeshCount));
    model.m_subMeshes.resize(subMeshCount);

    for (SubMeshData& mesh : model.m_subMeshes) {
        uint64_t vertexCount;
        in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
        mesh.m_vertices.resize(vertexCount);
        in.read(reinterpret_cast<char*>(mesh.m_vertices.data()), vertexCount * sizeof(SimpleVertex));

        uint64_t indexCount;
        in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
        mesh.m_indices.resize(indexCount);
        in.read(reinterpret_cast<char*>(mesh.m_indices.data()), indexCount * sizeof(uint32_t));

        size_t diffuseTextureByteCount;
        in.read(reinterpret_cast<char*>(&diffuseTextureByteCount), sizeof(size_t));
        mesh.materialData.diffuseData.data.resize(diffuseTextureByteCount);
        in.read(reinterpret_cast<char*>(mesh.materialData.diffuseData.data.data()), diffuseTextureByteCount * sizeof(unsigned char));
        in.read(reinterpret_cast<char*>(&mesh.materialData.diffuseData.width), sizeof(int));
        in.read(reinterpret_cast<char*>(&mesh.materialData.diffuseData.height), sizeof(int));
        in.read(reinterpret_cast<char*>(&mesh.materialData.diffuseData.numChannels), sizeof(int));
    }

    model.m_transforms.resize(model.m_subMeshes.size());
    for (size_t i = 0; i < model.m_transforms.size(); i++)
    {
        in.read(reinterpret_cast<char*>(&model.m_transforms[i]), sizeof(Matrix4f));
    }

    return model; // TODO: RVO?
}
}
