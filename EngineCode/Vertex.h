#pragma once
#include "Common/Math/Vector3f.h"

namespace Magic
{

struct SimpleVertex
{
    Vector3f position;
    float uv_x;
    Vector3f color;
    float uv_y;
    Vector3f normal;
};

}