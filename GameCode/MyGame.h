#pragma once
#include "../EngineCode/Game.h"
#include "../EngineCode/Log.h"
#include "../EngineCode/EventArgs.h"

class MyGame final : public Magic::Game
{

    using KeyEventArgs = Magic::KeyEventArgs;
public:
    MyGame() = default;
    ~MyGame() override = default;

    void Initialize() override;
    void LoadContent() override;
    void UnloadContent() override;
    void Shutdown() override;
    void OnUpdate() override;
    void OnKeyPressed(KeyEventArgs keyEventArgs) override;


private:
};



