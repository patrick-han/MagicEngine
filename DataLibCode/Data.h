#pragma once
#include <fstream>
#include "../EngineCode/Vertex.h"
#include "../EngineCode/Model.h"


namespace Magic::Data
{

inline void SerializeModelData(const ModelData& model, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Failed to open file for writing");

    uint64_t meshCount = model.m_meshes.size();
    out.write(reinterpret_cast<const char*>(&meshCount), sizeof(meshCount));

    for (const auto& mesh : model.m_meshes) {
        uint64_t vertexCount = mesh.m_vertices.size();
        out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
        out.write(reinterpret_cast<const char*>(mesh.m_vertices.data()), vertexCount * sizeof(SimpleVertex));

        uint64_t indexCount = mesh.m_indices.size();
        out.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
        out.write(reinterpret_cast<const char*>(mesh.m_indices.data()), indexCount * sizeof(uint32_t));
    }

    for (auto& transform : model.m_transforms)
    {
        out.write(reinterpret_cast<const char*>(&transform), sizeof(Matrix4f));
    }

}

inline ModelData DeserializeModelData(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open file for reading");

    ModelData model;
    uint64_t meshCount;
    in.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount));
    model.m_meshes.resize(meshCount);

    for (auto& mesh : model.m_meshes) {
        uint64_t vertexCount;
        in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
        mesh.m_vertices.resize(vertexCount);
        in.read(reinterpret_cast<char*>(mesh.m_vertices.data()), vertexCount * sizeof(SimpleVertex));

        uint64_t indexCount;
        in.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
        mesh.m_indices.resize(indexCount);
        in.read(reinterpret_cast<char*>(mesh.m_indices.data()), indexCount * sizeof(uint32_t));
    }

    model.m_transforms.resize(model.m_meshes.size());
    for (size_t i = 0; i < model.m_transforms.size(); i++)
    {
        in.read(reinterpret_cast<char*>(&model.m_transforms[i]), sizeof(Matrix4f));
    }

    return model; // TODO: RVO?
}
}
