#include "Game.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Camera.h"
#include "../EngineCode/Input.h"
#include "../EngineCode/World.h"

#include <SDL3/SDL_scancode.h> // Only for SCANCODES, TODO: Make a translation layer thingy

#include "PlayerComponent.h"
#include "../EngineCode/ResourceManager.h"
#include "../EngineCode/ECS.h"
#include "../EngineCode/Components/TransformComponent.h"
#include "../EngineCode/Components/RenderableMeshComponent.h"
#include "PlayerMovementSystem.h"
#include "RenderSystem.h"
#include "../EngineCode/JobSystem.h"
#include <vector>
#include <cassert>

namespace Magic
{

Game::Game() { }
Game::~Game() { }

static std::uint64_t errorModelHandle;

void Game::Initialize(Renderer* pRenderer)
{
    m_pWorld = new World;
    m_ecs = std::make_unique<ECS::Registry>();
    m_resourceManager = std::make_unique<ResourceManager>(pRenderer, m_ecs.get(), m_pWorld);
    m_ecs->AddSystem<ECS::PlayerMovementSystem>();
    m_ecs->AddSystem<ECS::RenderSystem>();
    // m_resourceManager->LoadModelFromDisk("../DataLibCode/debug/errorOut.bin", "error");
    // std::vector<Entity> errorMeshEntities = m_resourceManager->UploadModel("error");
}

void Game::Shutdown()
{
    delete m_pWorld;
}

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 10.0f, -500.0f), Vector3f(0.0f, 0.0f, 1.0f));

    JobSystem::Execute([&]()
    {
        m_resourceManager->LoadModelFromDisk("../DataLibCode/scene.bin", "scene");
    });
    JobSystem::Execute([&]()
    {
        m_resourceManager->LoadModelFromDisk("../DataLibCode/debug/debugSphereOut.bin", "debugSphere");
    });
    JobSystem::Wait(); // Artificially test super-sponza load

    JobSystem::Execute([&]()
    {
        m_resourceManager->LoadModelFromDisk("../DataLibCode/sponza.bin", "player");
    });

    m_resourceManager->EnqueueUploadModel("player");
    m_resourceManager->EnqueueUploadModel("scene");
    m_resourceManager->EnqueueUploadModel("debugSphere");

    // Entity player = m_ecs->EnqueueCreateEntity();
    // {
    //     player.AddComponent<TransformComponent>(Matrix4f::MakeScale(0.25f));
    //     player.AddComponent<PlayerComponent>(50.0f);
    // }

    // Entity scene = m_ecs->EnqueueCreateEntity();
    // {
    //     Matrix4f t;
    //     scene.AddComponent<TransformComponent>(t);
    // }

    // Entity debugSphere = m_ecs->EnqueueCreateEntity();
    // {
    //     debugSphere.AddComponent<TransformComponent>(Matrix4f::MakeScale(3.0));
    // }
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    m_resourceManager->DestroyAllAssets();
}

[[nodiscard]] RenderingInfo Game::Update(const InputState& inputState, float deltaTime)
{
    m_ecs->Update();

    m_resourceManager->ProcessModelUploadJobs();
    m_resourceManager->ProcessBufferUploadJobs();
    m_resourceManager->ProcessImageUploadJobs();
    m_resourceManager->PollImageUploadJobsFinishedAndUpdateRenderables();


    auto meshesToRender = m_ecs->GetSystem<ECS::RenderSystem>().Update();

    if (inputState.shouldFreezeCamera)
    {
        m_camera->Freeze();
    }
    else
    {
        m_camera->UnFreeze();
    }

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

    if (inputState.keyState[SDL_SCANCODE_U]) {
        JobSystem::Execute([this](){
            m_resourceManager->DestroyAllLoadedModels();
        });
    }


    m_ecs->GetSystem<ECS::PlayerMovementSystem>().Update(playerMovementVector, deltaTime);

    GameStats stats = 
    {
        .entityCount = m_ecs->GetNumberOfEntities()
        , .ramResidentModelCount = m_resourceManager->GetRAMResidentModelCount()
        , .meshCount = m_resourceManager->GetMeshCount()
        , .textureCount = m_resourceManager->GetTextureCount()
        , .pendingModelUploadCount = m_resourceManager->GetPendingModelUploadJobCount()
        , .pendingBufferUploadCount = m_resourceManager->GetPendingBufferUploadJobCount()
        , .pendingImageUploadCount = m_resourceManager->GetPendingImageUploadJobCount()
    };

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = meshesToRender
      , .gameStats = stats
    };
    return renderingInfo;
}

}
