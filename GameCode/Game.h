#pragma once
#include "../EngineCode/EventArgs.h"

namespace Magic
{
class Renderer;
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
    void Shutdown();

    void OnUpdate();
    void Render(Renderer& rctx);
    void OnKeyPressed(KeyEventArgs keyEventArgs);

private:
    int m_frameNumber = 0;
};

}
