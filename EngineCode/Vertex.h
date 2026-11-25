#pragma once
#include "Math/Vector3f.h"
#include "Math/Vector4f.h"

namespace Magic
{

struct SimpleVertex
{
    Vector3f position;
    float uv_x = 0.0f;
    Vector3f color;
    float uv_y = 0.0f;
    Vector3f normal;
    float data = 0.0f;
    Vector4f tangentW;
};

}
