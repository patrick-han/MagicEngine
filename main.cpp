#include "EngineCode/Application.h"
#include "GameCode/Game.h"

int main()
{
    Magic::Application app;
    app.Startup();
    std::unique_ptr<Magic::Game> pGame = std::make_unique<Magic::Game>();
    app.Run(*pGame);
    app.Shutdown();
    return 0;
}