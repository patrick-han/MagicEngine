#pragma once

namespace Magic
{
struct InputState
{
    float mouseXOffset = 0.0f;
    float mouseYOffset = 0.0f;
    const bool* keyState = nullptr; // From SDL
    bool shouldFreezeCamera = false;
};
}