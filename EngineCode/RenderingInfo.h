#pragma once
#include <vector>
#include "SubMesh.h"
#include "Timing.h"
namespace Magic
{

struct GameStats
{
    int entityCount = 0;
    int ramResidentStaticMeshDataCount = 0;
    int loadingFromDiskStaticMeshCount = 0;
    int meshCount = 0;
    int subMeshCount = 0;
    int textureCount = 0;
    int pendingStaticMeshDataUploadCount = 0;
    int pendingBufferUploadCount = 0;
    int pendingImageUploadCount = 0;
    int pendingStaticMeshEntities = 0;
    int readyStaticMeshEntities = 0;
};


class Camera;
class World;
class Game;
struct RenderingInfo
{
    const Camera* const pCamera;
    std::vector<SubMesh*> meshesToRender;
    GameStats gameStats;
    World* pWorld;
    Game* pGame;
    std::chrono::microseconds updateLoopTimingUS;
};
}