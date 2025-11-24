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
    GResourceDB = new ResourceDatabase();
    GResourceDB->Init("GameCode/magic.db");
    GMemoryManager = new MemoryManager();
    m_pWorld = new World();
    m_pWorld->Init("GameCode/magic.world");
    GMemoryManager->Initialize();
    GResourceManager = new ResourceManager(m_pWorld);

    Logger::Info(std::format("Game working directory: {}", std::filesystem::current_path().string()));
    GResourceManager->UploadDefaultTexture();
}

void Game::Shutdown()
{
    m_pWorld->Save();
    GResourceDB->Save();
    delete GResourceDB;
    m_pWorld->Destroy();
    delete m_pWorld;
    GMemoryManager->Shutdown();
    delete GMemoryManager;
    delete GResourceManager;
}

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 1.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f));


    const std::unordered_set<UUID>& uuids = m_pWorld->GetAllUUIDs();
    for (const UUID& uuid : uuids)
    {
        std::optional<UUID> res_uuid = m_pWorld->GetStaticMeshEntityResourceUUID(uuid);
        if (res_uuid)
        {
            const char* resPath = GResourceDB->GetResPath(*res_uuid);
            const char* resName = GResourceDB->GetResName(*res_uuid);
            Job::Pool.detach_task([=]() {
                GResourceManager->LoadModelFromDisk(resPath, resName);
            });
            GResourceManager->EnqueueUploadModel(resName);
        }
    }
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    GResourceManager->DestroyAllAssets();
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

    GResourceManager->ProcessModelUploadJobs();
    GResourceManager->ProcessBufferUploadJobs();
    GResourceManager->ProcessImageUploadJobs();
    GResourceManager->PollImageUploadJobsFinishedAndUpdateRenderables();

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
        , .ramResidentModelCount = GResourceManager->GetRAMResidentModelCount()
        , .meshCount = static_cast<int>(m_pWorld->GetMeshEntities().size())
        , .subMeshCount = m_pWorld->GetSubMeshCount()
        , .textureCount = GResourceManager->GetTextureCount()
        , .pendingModelUploadCount = GResourceManager->GetPendingModelUploadJobCount()
        , .pendingBufferUploadCount = GResourceManager->GetPendingBufferUploadJobCount()
        , .pendingImageUploadCount = GResourceManager->GetPendingImageUploadJobCount()
    };

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = meshesToRender
      , .gameStats = stats
      , .pWorld = m_pWorld
    };
    return renderingInfo;
}

}
