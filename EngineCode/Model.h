#pragma once
#include "Mesh.h"

#include "Common/Math/Matrix4f.h"

namespace Magic
{
struct ModelData
{
    std::vector<MeshData> m_meshes;
    std::vector<Matrix4f> m_transforms;
};

}