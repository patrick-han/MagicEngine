#pragma once
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
};

struct MaterialData
{
    TextureData diffuseData;
    // TextureData metallicRoughnessData;
    // TextureData normalData;
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
    std::vector<Matrix4f> m_transforms;
    std::vector<unsigned char> textureData;
};

}