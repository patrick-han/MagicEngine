#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Math/Matrix4f.h"
#include "Vertex.h"

namespace Magic
{

struct TextureData
{
    int width = 0;
    int height = 0;
    int numChannels = 0;
    int baseTextureDataOffset = 0; // into textureData in model data
    std::string sourcePath; // Runtime source identity used to deduplicate loaded textures.
};

struct MaterialData
{
    TextureData diffuseData;
    TextureData normalData;
    TextureData metallicRoughnessData;
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
    float normalScale = 1.0f;
    float normalYSign = 1.0f;
    // TextureData emissiveData;
};

struct SubMeshData
{
    MaterialData materialData;
    std::vector<SimpleVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct StaticMeshData
{
    std::vector<SubMeshData> m_subMeshes;
    std::vector<Matrix4f> m_transforms; // per submesh
    std::vector<unsigned char> textureData;
};

}
