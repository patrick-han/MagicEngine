#pragma once
#include "EventArgs.h"

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
    void virtual Initialize() = 0;
    void virtual LoadContent() = 0;
    void virtual UnloadContent() = 0;
    void virtual Shutdown() = 0;

    void virtual OnUpdate();
    void Render(Renderer& rctx);
    void virtual OnKeyPressed(KeyEventArgs keyEventArgs) = 0;

private:
    int m_frameNumber = 0;
};

}
