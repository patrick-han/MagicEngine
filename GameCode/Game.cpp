#include "Game.h"
#include "../EngineCode/Renderer.h"
#include "../CommonCode/Log.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }


void Game::Initialize()
{
    Logger::Info("Starting Game up");
}

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
}

void Game::Shutdown()
{
    Logger::Info("Shutting MyGame down");
}

void Game::OnUpdate()
{
    m_frameNumber++;
}

void Game::Render(Renderer& rctx)
{
    rctx.DoWork(m_frameNumber);
}

void Game::OnKeyPressed(KeyEventArgs keyEventArgs)
{
}
}
