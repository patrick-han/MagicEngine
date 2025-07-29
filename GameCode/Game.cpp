#include "Game.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Camera.h"
#include "../EngineCode/Input.h"

#include <SDL3/SDL_scancode.h> // Only for SCANCODES, TODO: Make a translation layer thingy


#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "TestSystem.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }

void Game::Initialize()
{
    m_ecs = std::make_unique<Registry>();
    m_ecs->AddSystem<TestSystem>();
}


void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    Entity player = m_ecs->EnqueueCreateEntity();
    player.AddComponent<TransformComponent>(Vector3f(0.0f, 0.0f, 0.0f));
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
}

void Game::Update(const InputState& inputState, float deltaTime)
{
    m_ecs->Update();
    m_ecs->GetSystem<TestSystem>().Update();

    m_camera->Rotate(inputState.mouseXOffset, inputState.mouseYOffset, true);
    float cameraSpeed = 20.0f;
    if (inputState.keyState[SDL_SCANCODE_W]) {
        m_camera->Move(Camera::CameraMovementDirection::FORWARD, cameraSpeed * deltaTime);
    }
    if (inputState.keyState[SDL_SCANCODE_S]) {
        m_camera->Move(Camera::CameraMovementDirection::BACKWARD, cameraSpeed * deltaTime);
    }
    if (inputState.keyState[SDL_SCANCODE_A]) {
        m_camera->Move(Camera::CameraMovementDirection::LEFT, cameraSpeed * deltaTime);
    }
    if (inputState.keyState[SDL_SCANCODE_D]) {
        m_camera->Move(Camera::CameraMovementDirection::RIGHT, cameraSpeed * deltaTime);
    }
    if (inputState.keyState[SDL_SCANCODE_SPACE]) {
        m_camera->Move(Camera::CameraMovementDirection::UP, cameraSpeed * deltaTime);
    }
    if (inputState.keyState[SDL_SCANCODE_LCTRL]) {
        m_camera->Move(Camera::CameraMovementDirection::DOWN, cameraSpeed * deltaTime);
    }
}

RenderingInfo Game::GetRenderingInfo()
{
    RenderingInfo renderingInfo = {.pCamera = m_camera.get()};
    return renderingInfo;
}
}
