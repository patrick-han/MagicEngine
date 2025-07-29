#pragma once
#include "../Common/Math/Vector3f.h"

namespace Magic
{
struct TransformComponent
{
    Vector3f m_position;
    TransformComponent(Vector3f _position = Vector3f(0.0f, 0.0f, 0.0f)) : m_position(_position)
    {

    }
};
}