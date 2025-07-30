#pragma once
#include "../Common/Math/Matrix4f.h"

namespace Magic
{
struct TransformComponent
{
    Matrix4f m_transform;
    TransformComponent(const Matrix4f& _transform) : m_transform(_transform)
    {

    }
};
}