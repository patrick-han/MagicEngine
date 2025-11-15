#pragma once
#include <memory>
#include "../EngineCode/RenderingInfo.h"

namespace Magic
{
namespace ECS
{
class Registry;
}

class Camera;
class ResourceDatabase;
class ResourceManager;
struct InputState;
class World;
class Game
{
public:
    Game();
    virtual ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

protected:
    friend class Application;
    void Initialize(class Renderer* pRenderer);
    void Shutdown();
    void LoadContent();
    void UnloadContent();
    [[nodiscard]] RenderingInfo Update(const InputState& inputState, float deltaTime);
private:
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<ResourceDatabase> m_resourceDB;
    std::unique_ptr<ResourceManager> m_resourceManager;
    World* m_pWorld;
};

}
