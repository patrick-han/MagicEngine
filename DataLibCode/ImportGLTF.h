#pragma once
#include <string>
#include <filesystem>
struct cgltf_node;

namespace Magic
{
struct ModelData;
void ProcessNode(cgltf_node* node, ModelData& modelData, const std::filesystem::path& filePath);
void ImportGLTF(const std::string& filepathStr, ModelData& modelData);

}