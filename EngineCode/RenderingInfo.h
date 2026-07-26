#pragma once
#include <vector>
#include "StaticMeshEntity.h"
#include "Timing.h"
#include "GPUStats.h"
namespace Magic
{

struct GameStats
{
    std::size_t entityCount = 0;
    std::size_t meshCount = 0;
    std::size_t subMeshCount = 0;
    std::size_t textureCount = 0;
#if MAGIC_TRACK_GPU_STATS
    std::size_t bufferBytesUploaded = 0;
    std::size_t imageBytesUploaded = 0;
#endif
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