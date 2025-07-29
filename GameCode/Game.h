#pragma once
#include <memory>
#include "../EngineCode/RenderingInfo.h"

namespace Magic
{
class Registry;
class Camera;
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
    void Initialize();
    void LoadContent();
    void UnloadContent();
    void Update(const InputState& inputState, float deltaTime);
    [[nodiscard]] RenderingInfo GetRenderingInfo();
private:
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Registry> m_ecs;
};

}
