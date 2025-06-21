#include "MyGame.h"

using Logger = Magic::Logger;
using GameBase = Magic::Game;
using KeyEventArgs = Magic::KeyEventArgs;

void MyGame::Initialize()
{
    Logger::Info("Starting MyGame up");
}
void MyGame::LoadContent()
{
    Logger::Info("Load MyGame content");
}
void MyGame::UnloadContent()
{
    Logger::Info("Unload MyGame content");
}
void MyGame::Shutdown()
{
    Logger::Info("Shutting MyGame down");
}
void MyGame::OnUpdate()
{
    GameBase::OnUpdate();
}

void MyGame::OnKeyPressed(KeyEventArgs keyEventArgs)
{
    Logger::Info("OnKeyPressed");
}
