#pragma once
#include <memory>
#include "../EngineCode/RenderingInfo.h"

namespace Magic
{
class Registry;
class Camera;
class ResourceManager;
struct InputState;
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
    void LoadContent();
    void UnloadContent();
    [[nodiscard]] RenderingInfo Update(const InputState& inputState, float deltaTime);
private:
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Registry> m_ecs;
    std::unique_ptr<ResourceManager> m_resourceManager;
};

}
