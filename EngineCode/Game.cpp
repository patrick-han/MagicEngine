#include "Game.h"
#include "Renderer.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }


void Game::OnUpdate()
{
    m_frameNumber++;
}

void Game::Render(Renderer& rctx)
{
    rctx.DoWork(m_frameNumber);
}

}
