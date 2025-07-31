#include "Game.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Camera.h"
#include "../EngineCode/Input.h"

#include <SDL3/SDL_scancode.h> // Only for SCANCODES, TODO: Make a translation layer thingy

#include "PlayerComponent.h"
#include "../EngineCode/ResourceManager.h"
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "../EngineCode/Components/RenderableComponent.h"
#include "PlayerMovementSystem.h"
#include "RenderSystem.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }

void Game::Initialize(Renderer* pRenderer)
{
    m_assetManager = std::make_unique<ResourceManager>(pRenderer);
    m_ecs = std::make_unique<Registry>();
    m_ecs->AddSystem<PlayerMovementSystem>();
    m_ecs->AddSystem<RenderSystem>();
}


void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f));


    Entity player = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        // t.m03 = 20;
        player.AddComponent<TransformComponent>(t);
        std::vector<int> renderableIndices = m_assetManager->LoadModel("../DataLibCode/helmet.bin");
        player.AddComponent<RenderableComponent>(renderableIndices);
        player.AddComponent<PlayerComponent>(50.0f);
    }


    Entity enemy = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        enemy.AddComponent<TransformComponent>(t);
        std::vector<int> renderableIndices = m_assetManager->LoadModel("../DataLibCode/helmet.bin");
        enemy.AddComponent<RenderableComponent>(renderableIndices);
    }
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    m_assetManager->DestroyAllAssets();
}

[[nodiscard]] RenderingInfo Game::Update(const InputState& inputState, float deltaTime)
{
    m_ecs->Update();

    auto meshesToRender = m_ecs->GetSystem<RenderSystem>().Update(m_assetManager.get());

    m_camera->Rotate(inputState.mouseXOffset, inputState.mouseYOffset, true);
    float cameraSpeed = 20.0f;
    if (inputState.keyState[SDL_SCANCODE_LSHIFT]) {
        cameraSpeed = 60.0f;
    }
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



    Vector3f playerMovementVector = Vector3f(0.0f, 0.0f, 0.0f);
    if (inputState.keyState[SDL_SCANCODE_UP]) {
        playerMovementVector.z = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_DOWN]) {
        playerMovementVector.z = -1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_LEFT]) {
        playerMovementVector.x = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_RIGHT]) {
        playerMovementVector.x = -1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_PAGEUP]) {
        playerMovementVector.y = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_PAGEDOWN]) {
        playerMovementVector.y = -1.0f;
    }


    m_ecs->GetSystem<PlayerMovementSystem>().Update(playerMovementVector, deltaTime);

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = meshesToRender
    };
    return renderingInfo;
}

}
