#include <filesystem>
#include "root.magic"
#include "EngineCode/Application.h"
#include "GameCode/Game.h"
#include "EngineCode/Common/Log.h"
#include "EngineCode/MemoryManager.h"

int main()
{
    Magic::Logger::Info(std::format("Original working directory: {}", std::filesystem::current_path().string()));
    try
    {
        // Change the current working directory
        std::filesystem::current_path(PROJECT_ROOT);
        Magic::Logger::Info(std::format("Working directory changed to: {}", std::filesystem::current_path().string()));
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        Magic::Logger::Err(std::format("Error changing working directory: {}", e.what()));
        exit(1);
    }

    // Double check:
    {
        std::filesystem::path check_root_path = std::filesystem::current_path() / std::string("root.magic");
        if (std::filesystem::exists(check_root_path))
        {
            Magic::Logger::Info("You found the root...");
        }
        else
        {
            Magic::Logger::Info("You failed to find the root, looks like you're not cut out for this after all...");
        }
    }
    Magic::GMemoryManager = new Magic::MemoryManager();
    Magic::GMemoryManager->Initialize();
    Magic::Application app;
    app.Startup();
    std::unique_ptr<Magic::Game> pGame = std::make_unique<Magic::Game>();
    app.Run(*pGame);
    app.Shutdown();
    Magic::GMemoryManager->Shutdown();
    delete Magic::GMemoryManager;
    return 0;
}