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
#include "../EngineCode/JobSystem.h"

namespace Magic
{

Game::Game() { }
Game::~Game() { }

static std::uint64_t errorModelHandle;

void Game::Initialize(Renderer* pRenderer)
{
    JobSystem::Initialize();

    m_resouceManager = std::make_unique<ResourceManager>(pRenderer);
    m_ecs = std::make_unique<Registry>();
    m_ecs->AddSystem<PlayerMovementSystem>();
    m_ecs->AddSystem<RenderSystem>();


    // errorModelHandle = m_assetManager->LoadModelFromDisk("../DataLibCode/debug/errorOut.bin", "error");
    // m_assetManager->UploadModel(errorModelHandle);
    // pRenderer->m_errorModelMeshIndices = m_assetManager->GetRenderableMeshIndices("errorS");
}

// testing
static ResourceHandle h1;
static ResourceHandle h2;
static ResourceHandle h3;

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 10.0f, -500.0f), Vector3f(0.0f, 0.0f, 1.0f));

    JobSystem::Execute([&]()
    {
        h1 = m_resouceManager->LoadModelFromDisk("../DataLibCode/helmet.bin", "player");
    });
    JobSystem::Execute([&]()
    {
        h2 = m_resouceManager->LoadModelFromDisk("../DataLibCode/scene.bin", "scene");
    });
    JobSystem::Execute([&]()
    {
        h3 = m_resouceManager->LoadModelFromDisk("../DataLibCode/debug/debugSphereOut.bin", "debugSphere");
    });
    JobSystem::Wait();

    m_resouceManager->UploadModel(h1);
    m_resouceManager->UploadModel(h2);
    m_resouceManager->UploadModel(h3);

    Entity player = m_ecs->EnqueueCreateEntity();
    {
        player.AddComponent<TransformComponent>(Matrix4f::MakeScale(0.25f));
        if (auto handle = m_resouceManager->GetResourceHandleByName("player"))
        {
            player.AddComponent<RenderableComponent>(*handle);
        }
        player.AddComponent<PlayerComponent>(50.0f);
    }

    Entity scene = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        scene.AddComponent<TransformComponent>(t);
        if (auto handle = m_resouceManager->GetResourceHandleByName("scene"))
        {
            scene.AddComponent<RenderableComponent>(*handle);
        }
    }

    Entity debugSphere = m_ecs->EnqueueCreateEntity();
    {
        debugSphere.AddComponent<TransformComponent>(Matrix4f::MakeScale(3.0));
        if (auto handle = m_resouceManager->GetResourceHandleByName("debugSphere"))
        {
            debugSphere.AddComponent<RenderableComponent>(*handle, RenderableFlags::DrawDebug);
        }
    }
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    m_resouceManager->DestroyAllAssets();
}

[[nodiscard]] RenderingInfo Game::Update(const InputState& inputState, float deltaTime)
{
    m_ecs->Update();

    auto meshesToRender = m_ecs->GetSystem<RenderSystem>().Update(m_resouceManager.get(), errorModelHandle);

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
