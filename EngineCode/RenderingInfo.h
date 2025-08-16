#pragma once
#include <vector>
#include "Components/RenderableMeshComponent.h"

namespace Magic
{
class Camera;
struct RenderingInfo
{
    const Camera* const pCamera;
    std::vector<RenderableMeshComponent> meshesToRender;
};
}