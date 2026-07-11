#pragma once
#include <vector>
#include "SubMesh.h"
#include "Timing.h"
namespace Magic
{

struct GameStats
{
    std::size_t entityCount = 0;
    std::size_t meshCount = 0;
    std::size_t subMeshCount = 0;
    std::size_t textureCount = 0;
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