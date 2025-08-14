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

    m_assetManager = std::make_unique<ResourceManager>(pRenderer);
    m_ecs = std::make_unique<Registry>();
    m_ecs->AddSystem<PlayerMovementSystem>();
    m_ecs->AddSystem<RenderSystem>();


    errorModelHandle = m_assetManager->LoadModelFromDisk("../DataLibCode/debug/errorOut.bin", "error");
    m_assetManager->UploadModel(errorModelHandle);
    pRenderer->m_errorModelMeshIndices = m_assetManager->GetRenderableMeshIndices("errorS");
}

// testing
// static std::uint64_t h1;
static std::uint64_t h2;
static std::uint64_t h3;
static ENTITY_ID playerId;

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 10.0f, -500.0f), Vector3f(0.0f, 0.0f, 1.0f));


    // auto t1 = std::thread([&]()
    // {
    //     h1 = m_assetManager->LoadModelFromDisk("../DataLibCode/helmet.bin", "helmet1");
    // });

    JobSystem::Execute([&]()
    {
        h2 = m_assetManager->LoadModelFromDisk("../DataLibCode/scene.bin", "scene");
    });
    JobSystem::Execute([&]()
    {
        h3 = m_assetManager->LoadModelFromDisk("../DataLibCode/debug/debugSphereOut.bin", "debugSphere");
    });
    JobSystem::Wait();


    Entity player = m_ecs->EnqueueCreateEntity();
    playerId = player.GetId();
    {
        Matrix4f t;
        // t.m03 = 20;
        player.AddComponent<TransformComponent>(Matrix4f::MakeScale(0.25f));
        // player.AddComponent<RenderableComponent>(h1);
        player.AddComponent<RenderableComponent>(10293, "../DataLibCode/super-sponza.bin", "player");
        player.AddComponent<PlayerComponent>(50.0f);
    }


    Entity scene = m_ecs->EnqueueCreateEntity();
    {
        Matrix4f t;
        scene.AddComponent<TransformComponent>(t);
        scene.AddComponent<RenderableComponent>(h2, "../DataLibCode/scene.bin", "scene");
    }

    Entity debugSphere = m_ecs->EnqueueCreateEntity();
    {
        debugSphere.AddComponent<TransformComponent>(Matrix4f::MakeScale(3.0));
        debugSphere.AddComponent<RenderableComponent>(h3, "../DataLibCode/debug/debugSphereOut.bin", "debugSphere", RenderableFlags::DrawDebug);
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

    auto meshesToRender = m_ecs->GetSystem<RenderSystem>().Update(m_assetManager.get(), errorModelHandle);

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

    // Stream test
    Entity player(playerId);
    const auto& transform = m_ecs->GetComponent<TransformComponent>(player);
    Vector3f playerPos = Vector3f(transform.m_transform.m03, transform.m_transform.m13, transform.m_transform.m23);
    Vector3f cameraPos = m_camera->GetPosition();

    if ((playerPos-cameraPos).Length() < 450) {

        auto& renderable = m_ecs->GetComponent<RenderableComponent>(player);
        if (!renderable.testFlag)
        {
            Logger::Info("Start streaming...");
        }
        renderable.testFlag = true;
    }



    m_ecs->GetSystem<PlayerMovementSystem>().Update(playerMovementVector, deltaTime);

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = meshesToRender
    };
    return renderingInfo;
}

}
