#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>
#include "../CommonCode/Model.h"
struct cgltf_node;

namespace Magic
{
struct ModelData;


class GLTFImporter
{
    int m_i_node_count = 0;
    std::unordered_map<std::string, TextureData> m_texturesSeen;
    void ProcessNode(cgltf_node* node, ModelData& modelData, const std::filesystem::path& filePath);
public:
    void ImportGLTF(const std::string& filepathStr, ModelData& modelData);
};

}