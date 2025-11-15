#include "Game.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Camera.h"
#include "../EngineCode/Input.h"
#include "../EngineCode/World.h"

#include <SDL3/SDL_scancode.h> // Only for SCANCODES, TODO: Make a translation layer thingy
#include "../EngineCode/ResourceDatabase.h"
#include "../EngineCode/ResourceManager.h"
#include "../EngineCode/MemoryManager.h"
#include <vector>
#include <cassert>
#include "../EngineCode/Threading.h"

#include <filesystem>
namespace Magic
{

Game::Game() { }
Game::~Game() { }

void Game::Initialize(Renderer* pRenderer)
{
    m_resourceDB = std::make_unique<ResourceDatabase>();
    m_resourceDB->Init("GameCode/magic.db");

    m_memoryManager = std::make_unique<MemoryManager>();
    m_pWorld = new World(m_memoryManager.get());
    m_memoryManager->Initialize();
    m_resourceManager = std::make_unique<ResourceManager>(pRenderer, m_pWorld, m_memoryManager.get());

    Logger::Info(std::format("Game working directory: {}", std::filesystem::current_path().string()));
    m_resourceManager->UploadDefaultTexture();
}

void Game::Shutdown()
{
    m_resourceDB->Save();
    m_pWorld->Destroy();
    delete m_pWorld;
    m_memoryManager->Shutdown();
    
}

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 10.0f, -100.0f), Vector3f(0.0f, 0.0f, 1.0f));

    const std::unordered_set<UUID>& uuids = m_resourceDB->GetAllUUIDs();

    for (const UUID& uuid : uuids)
    {
        Job::Pool.detach_task([&]() {
            m_resourceManager->LoadModelFromDisk(m_resourceDB->GetResPath(uuid), m_resourceDB->GetResName(uuid));
        });
        m_resourceManager->EnqueueUploadModel(m_resourceDB->GetResName(uuid));
    }
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    m_resourceManager->DestroyAllAssets();
}


// TEMP
bool ShouldCull(SubMesh* subMesh)
{
    if (!subMesh->vertexBufferReady || !subMesh->indexBufferReady || !subMesh->texturesReady)
    {
        return true;
    }
    return false;
}

bool a = true;

[[nodiscard]] RenderingInfo Game::Update(const InputState& inputState, float deltaTime)
{

    m_resourceManager->ProcessModelUploadJobs();
    m_resourceManager->ProcessBufferUploadJobs();
    m_resourceManager->ProcessImageUploadJobs();
    m_resourceManager->PollImageUploadJobsFinishedAndUpdateRenderables();

    std::vector<SubMesh*> meshesToRender;
    {
        for (MeshEntity* pMeshEntity : m_pWorld->GetMeshEntities())
        {
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
            {
                if (!ShouldCull(pSubMesh))
                {
                    meshesToRender.push_back(pSubMesh);
                }
            }
        }
    }

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
    if (inputState.keyState[SDL_SCANCODE_LALT]) {
        cameraSpeed = 5.0f;
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
        if (a)
        {
            auto m = m_pWorld->GetMeshEntities();
            m_pWorld->RemoveMeshEntity(m[0]);
            a = false;
        }
        
    }

    GameStats stats = 
    {
        .entityCount = m_pWorld->GetEntityCount()
        , .ramResidentModelCount = m_resourceManager->GetRAMResidentModelCount()
        , .meshCount = static_cast<int>(m_pWorld->GetMeshEntities().size())
        , .subMeshCount = m_pWorld->GetSubMeshCount()
        , .textureCount = m_resourceManager->GetTextureCount()
        , .pendingModelUploadCount = m_resourceManager->GetPendingModelUploadJobCount()
        , .pendingBufferUploadCount = m_resourceManager->GetPendingBufferUploadJobCount()
        , .pendingImageUploadCount = m_resourceManager->GetPendingImageUploadJobCount()
    };

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = meshesToRender
      , .gameStats = stats
      , .pResourceDB = m_resourceDB.get()
    };
    return renderingInfo;
}

}
