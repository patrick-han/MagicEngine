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

    std::uint64_t h1;
    std::uint64_t h2;
    std::uint64_t h3;




    auto t1 = std::thread([&]()
    {
        h1 = m_assetManager->LoadModelFromDisk("../DataLibCode/helmet.bin", "helmet1");
    });
    auto t2 = std::thread([&]()
    {
        h2 = m_assetManager->LoadModelFromDisk("../DataLibCode/scene.bin", "scene");
    });
    auto t3 = std::thread([&]()
    {
        h3 = m_assetManager->LoadModelFromDisk("../DataLibCode/debug/debugSphereOut.bin", "debugSphere");
    });

    t1.join();
    t2.join();
    t3.join();



    Entity player = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        // t.m03 = 20;
        player.AddComponent<TransformComponent>(t);
        m_assetManager->UploadModel("helmet1");
        player.AddComponent<RenderableComponent>(h1);
        player.AddComponent<PlayerComponent>(50.0f);
    }


    Entity scene = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        scene.AddComponent<TransformComponent>(t);
        m_assetManager->UploadModel("scene");
        scene.AddComponent<RenderableComponent>(h2);
    }

    Entity debugSphere = m_ecs->EnqueueCreateEntity();
    {
        debugSphere.AddComponent<TransformComponent>(Matrix4f::MakeScale(3.0));
        m_assetManager->UploadModel("debugSphere");
        debugSphere.AddComponent<RenderableComponent>(h3, RenderableFlags::DrawDebug);
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
