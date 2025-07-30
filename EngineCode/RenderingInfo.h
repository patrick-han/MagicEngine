#pragma once
#include <vector>
#include "Renderable.h"

namespace Magic
{
class Camera;
struct RenderingInfo
{
    const Camera* const pCamera;
    std::vector<RenderableMesh> meshesToRender;
};
}