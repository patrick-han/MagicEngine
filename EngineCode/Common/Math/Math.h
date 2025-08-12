#pragma once
#include <numbers>
namespace Magic
{
    inline float deg2rad(float degrees)
    {
        return static_cast<float>(degrees * (std::numbers::pi / 180.0f));
    }

    inline float clamp(float x, float min, float max)
    {
        if (x < min)
        {
            return min;
        }
        if (x > max)
        {
            return max;
        }
    }
}