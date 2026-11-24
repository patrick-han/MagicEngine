#pragma once

#include <filesystem>

struct cgltf_node;

namespace Magic
{

struct StaticMeshData;

class GLTFImporterNew
{
    int m_i_node_count = 0;
    void ProcessNode(cgltf_node* node, StaticMeshData& staticMeshDataData, const std::filesystem::path& filePath);
public:
    void ImportGLTFAsStaticMesh(const std::filesystem::path& filePath, StaticMeshData& staticMeshDataData);
};

}
