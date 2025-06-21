#include "Game.h"
#include "Renderer.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }


void Game::PreInitialize(Renderer& rctx)
{
    rctx.BuildResources();
}

void Game::PostShutdown(Renderer&  rctx)
{
    rctx.DestroyResources();
}


void Game::OnUpdate()
{
    m_frameNumber++;
}

void Game::Render(Renderer& rctx)
{
    rctx.DoWork(m_frameNumber);
}

}
