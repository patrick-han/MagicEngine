#pragma once
#include <numbers>

inline float deg2rad(float degrees) {
    return static_cast<float>(degrees * (std::numbers::pi / 180.0f));
}