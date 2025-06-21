#include "EngineCode/Application.h"
#include "GameCode/MyGame.h"

int main()
{
    Magic::Application app;
    app.Startup();
    std::unique_ptr<MyGame> pGame = std::make_unique<MyGame>();
    app.Run(*pGame);
    app.Shutdown();
    return 0;
}