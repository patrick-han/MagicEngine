#include "Game.h"
#include "../CommonCode/Log.h"
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
#include "../CommonCode/BinaryBlob.h"
#include "../EngineCode/Timing.h"
#include <filesystem>
#include <unordered_map>
namespace Magic
{

Game::Game() { }
Game::~Game() { }

void Game::Initialize(Renderer* pRenderer)
{

    /*
    // TeST
    Matrix4f m0;
    m0.m22 = 1337;
    Matrix4f m1;
    m1.m00 = 9999;

    SimpleVertex vtx;
    vtx.position = Vector3f(1337, 0, 1337);
    vtx.uv_x = 0.23;
    vtx.color = Vector3f(255, 0, 255);
    vtx.uv_y = 0.32;
    vtx.normal = Vector3f(0.5,0.6,0.7);

    std::vector<std::uint32_t> u32v {1,3,9998};

    BinaryBlob blob;
    blob.InitializeAndAlloc();

    blob.AddChar('a');
    blob.AddChar('B');
    blob.AddChar('c');
    blob.AddChar('D');
    blob.AddU32Array(u32v.data(), u32v.size());
    blob.AddVector3f(Vector3f(1.321, 2.123, 3.123));
    blob.AddMatrix4f(m0);
    blob.AddSimpleVertex(vtx);
    blob.AddF32(3.14);
    blob.AddMatrix4f(m1);
    blob.SetPos(0);
    char a = blob.GetChar();
    char B = blob.GetChar();
    char c = blob.GetChar();
    char D = blob.GetChar();
    std::vector<std::uint32_t> u32v1;
    u32v1.resize(3);
    blob.GetU32Array(u32v1.data(), 3);
    Vector3f v = blob.GetVector3f();
    Matrix4f M0 = blob.GetMatrix4f();
    SimpleVertex vtx0 = blob.GetSimpleVertex();
    float f = blob.GetF32();
    Matrix4f M1 = blob.GetMatrix4f();

    blob.SaveToFile("test.bin");
    
    BinaryBlob blob2;
    blob2.LoadFromFile("test.bin");
    blob.SetPos(0);
    char a2 = blob.GetChar();
    char B2 = blob.GetChar();
    char c2 = blob.GetChar();
    char D2 = blob.GetChar();
    std::vector<std::uint32_t> u32v2;
    u32v2.resize(3);
    blob.GetU32Array(u32v2.data(), 3);
    Vector3f v2 = blob.GetVector3f();
    Matrix4f M02 = blob.GetMatrix4f();
    SimpleVertex vtx02 = blob.GetSimpleVertex();
    float f2 = blob.GetF32();
    Matrix4f M12 = blob.GetMatrix4f();

    blob.Clear();
    */

    GResourceDB = GMemoryManager->New<ResourceDatabase>();
    GResourceDB->Init("GameCode/magic.db");
    m_pWorld = GMemoryManager->New<World>();
    GResourceManager = GMemoryManager->New<ResourceManager>();

    Logger::Info(std::format("Game working directory: {}", std::filesystem::current_path().string()));
    GResourceManager->UploadDefaultTexture();

    // Make a free camera pointing down +Z with +X left and +Y up
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, 1.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f));
}

void Game::Shutdown()
{
    GResourceDB->Save();
    GMemoryManager->Delete(GResourceDB);
    GMemoryManager->Delete(m_pWorld);
    GResourceManager->Shutdown();
    GMemoryManager->Delete(GResourceManager);
}

void Game::LoadContent()
{
    Logger::Info("Load MyGame content");
    const std::unordered_set<UUID>& uuids = m_pWorld->GetAllUUIDs();
    for (const UUID& uuid : uuids)
    {
        std::optional<UUID> res_uuid = m_pWorld->GetStaticMeshEntityResourceUUID(uuid);
        if (res_uuid)
        {
            const char* resPath = GResourceDB->GetResPath(*res_uuid);
            const char* resName = GResourceDB->GetResName(*res_uuid);
            GResourceManager->SetStaticMeshDataLoadingFromDiskStatus(resName);
            Job::Pool.detach_task([=]() {
                if (!GResourceManager->IsStaticMeshDataLoadedFromDisk(resName))
                {
                    GResourceManager->LoadStaticMeshDataFromDisk(resPath, resName);
                }
            });
            if (!GResourceManager->IsStaticMeshDataGPUResident(resName))
            {
                GResourceManager->EnqueueUploadStaticMeshData(resName);
            }
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
    auto start = std::chrono::steady_clock::now();
    GResourceManager->ProcessStaticMeshDataUploadJobs();
    GResourceManager->ProcessBufferUploadJobs();
    GResourceManager->ProcessImageUploadJobs();
    GResourceManager->PollImageUploadJobsFinishedAndUpdateRenderables();

    // The World's static mesh entities are looking for certain named resources which the ResourceManager _should_ have
    auto& pending = m_pWorld->m_resourcePendingStaticMeshEntities;
    for (auto it = pending.begin(); it != pending.end(); )
    {
        if (!GResourceManager->IsStaticMeshDataGPUResident(it->resourceName)) // If the mesh is not gpu resident, it's not ready to be assigned
        {
            // Maybe the mesh is not yet loaded into the manager, but we need to check to make sure
            // its also not currently being loaded from disk or already loaded from disk
            // This path hits when we are reassigning a static mesh entity to a different static mesh resource
            if (
                !GResourceManager->IsStaticMeshDataLoadedFromDisk(it->resourceName.c_str())
                && !GResourceManager->IsStaticMeshDataLoadingFromDisk(it->resourceName.c_str())
            )
            {
                const char* resPath = GResourceDB->GetResPath(it->resourceName.c_str());
                assert(resPath != nullptr);
                GResourceManager->SetStaticMeshDataLoadingFromDiskStatus(it->resourceName.c_str());
                Job::Pool.detach_task([=]() {
                    GResourceManager->LoadStaticMeshDataFromDisk(resPath, it->resourceName.c_str());
                });
                GResourceManager->EnqueueUploadStaticMeshData(it->resourceName);
            }
            ++it; // the iterator does not advance on its own if we hit this and continue, so we manually increment
            continue;
        }
        std::size_t meshEntityIndex = GResourceManager->GetGPUResidentStaticMeshDataIndex(it->resourceName);
        StaticMeshEntity* staticMeshEntity = GResourceManager->m_meshEntities.at(meshEntityIndex);
        m_pWorld->m_uuid_to_pMeshEntity.insert_or_assign(it->entityUUID, staticMeshEntity);
        it = pending.erase(it); // Remove from m_resourcePendingStaticMeshEntities
    }

    std::vector<SubMesh*> meshesToRender;
    {
        for (auto staticMeshEntity : m_pWorld->m_uuid_to_pMeshEntity)
        {
            StaticMeshEntity* pMeshEntity = staticMeshEntity.second;
            for (SubMesh* pSubMesh : pMeshEntity->GetSubMeshes())
            {
                if (!ShouldCull(pSubMesh))
                {
                    meshesToRender.push_back(pSubMesh); // We don't necessarily want to wait for the entire static mesh to be ready, submeshes are okay
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


    // TEMP: count submeshes
    int subMeshCount = 0;
    for (StaticMeshEntity* pMeshEntity : GResourceManager->m_meshEntities)
    {
        subMeshCount += pMeshEntity->GetSubMeshes().size();
    }
    GameStats stats = 
    {
        .entityCount = m_pWorld->GetEntityCount()
        , .ramResidentStaticMeshDataCount = GResourceManager->GetRAMResidentStaticMeshDataCount()
        , .loadingFromDiskStaticMeshCount = GResourceManager->GetDiskLoadingStaticMeshDataCount()
        , .meshCount = static_cast<int>(GResourceManager->m_meshEntities.size())
        , .subMeshCount = subMeshCount
        , .textureCount = GResourceManager->GetTextureCount()
        , .pendingStaticMeshDataUploadCount = GResourceManager->GetPendingStaticMeshDataUploadJobCount()
        , .pendingBufferUploadCount = GResourceManager->GetPendingBufferUploadJobCount()
        , .pendingImageUploadCount = GResourceManager->GetPendingImageUploadJobCount()
        , .pendingStaticMeshEntities = static_cast<int>(m_pWorld->m_resourcePendingStaticMeshEntities.size())
        , .readyStaticMeshEntities = static_cast<int>(m_pWorld->m_uuid_to_pMeshEntity.size())
    };

    RenderingInfo renderingInfo = {
        .pCamera = m_camera.get()
      , .meshesToRender = std::move(meshesToRender)
      , .gameStats = stats
      , .pWorld = m_pWorld
      , .pGame = this
      , .updateLoopTimingUS = Timing::SinceUS(start)
    };
    return renderingInfo;
}

}
