#pragma once
#include <vector>
#include "Components/RenderableMeshComponent.h"
#include "Allocators.h"

namespace Magic
{
class Camera;
struct RenderingInfo
{
    const Camera* const pCamera;
    RenderableMeshAllocator::Payload meshesToRender;
};
}