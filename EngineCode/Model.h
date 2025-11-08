#pragma once
#include <vector>
#include "Common/Math/Matrix4f.h"
#include "Vertex.h"

namespace Magic
{

struct TextureData
{
    int width = 0;
    int height = 0;
    int numChannels = 0;
    std::vector<unsigned char> data;
};

struct MaterialData
{
    TextureData diffuseData;
    // TextureData metallicRoughnessData;
    // TextureData normalData;
    // TextureData emissiveData;
};

struct MeshData
{
    MaterialData materialData;
    std::vector<SimpleVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct ModelData
{
    std::vector<MeshData> m_meshes;
    std::vector<Matrix4f> m_transforms;
};

}