#pragma once
#include <vector>
#include "SubMesh.h"
namespace Magic
{

struct GameStats
{
    int entityCount = 0;
    int ramResidentModelCount = 0;
    int meshCount = 0;
    int textureCount = 0;
    int pendingModelUploadCount = 0;
    int pendingBufferUploadCount = 0;
    int pendingImageUploadCount = 0;
};


class Camera;
struct RenderingInfo
{
    const Camera* const pCamera;
    std::vector<SubMesh*> meshesToRender;
    GameStats gameStats;
};
}