#include "Game.h"
#include "../CommonCode/Log.h"
#include "../EngineCode/Camera.h"
#include "../EngineCode/Input.h"
#include "../EngineCode/World.h"
#include "../EngineCode/Renderer.h"
#include <SDL3/SDL_scancode.h> // Only for SCANCODES, TODO: Make a translation layer thingy
#include "../EngineCode/MemoryManager.h"
#include <vector>
#include <cassert>
#include "../EngineCode/Threading.h"
#include "../CommonCode/BinaryBlob.h"
#include "../EngineCode/Timing.h"
#include "../EngineCode/GPUStats.h"
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

    // vjson::Object doc;
    // doc["key"] = 3;
    // std::string p = doc.PrintJSON();
    // assert(SaveJsonToFile("output.json", p));
    // exit(1);

    m_pWorld = GMemoryManager->New<World>();

    Logger::Info(std::format("Game working directory: {}", std::filesystem::current_path().string()));

    // Make a free camera pointing down +Y with +X right and +Z up.
    m_camera = std::make_unique<Camera>(Vector3f(0.0f, -5.0f, 1.0f), Vector3f(0.0f, 1.0f, 0.0f));
}

void Game::Shutdown()
{
    GMemoryManager->Delete(m_pWorld);
}

void Game::LoadContent(const char* worldPath)
{
    Logger::Info("Load MyGame content");
    m_pWorld->Load(worldPath);
}

void Game::UnloadContent()
{
    Logger::Info("Unload MyGame content");
    m_pWorld->Destroy();
}


// TEMP
bool ShouldCull(SubMesh* subMesh)
{
    return false;
}

bool a = true;

[[nodiscard]] RenderingInfo Game::Update(const InputState& inputState, float deltaTime)
{
    auto start = std::chrono::steady_clock::now();

    GMemoryManager->ResetFrameTransformLinearAllocator();
    std::vector<SubMesh*> meshesToRender;
    {
        std::vector<const IEntity*> staticMeshEntities = m_pWorld->GetEntitiesOfType(EntityType::StaticMesh);
        for (const IEntity* entity : staticMeshEntities)
        {
            const StaticMeshEntity* staticMeshEntity = static_cast<const StaticMeshEntity*>(entity);
            for (SubMesh* pSubMesh : staticMeshEntity->GetSubMeshes())
            {
                if (!ShouldCull(pSubMesh))
                {
                    meshesToRender.push_back(pSubMesh); // We don't necessarily want to wait for the entire static mesh to be ready, submeshes are okay
                    Matrix4f* allocTransform = GMemoryManager->AllocateFrameTransform();
                    Matrix4f worldTransform = staticMeshEntity->m_transform * pSubMesh->m_transform;
                    *allocTransform = worldTransform;
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
        playerMovementVector.y = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_DOWN]) {
        playerMovementVector.y = -1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_LEFT]) {
        playerMovementVector.x = -1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_RIGHT]) {
        playerMovementVector.x = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_PAGEUP]) {
        playerMovementVector.z = 1.0f;
    }
    if (inputState.keyState[SDL_SCANCODE_PAGEDOWN]) {
        playerMovementVector.z = -1.0f;
    }


    std::size_t subMeshCount = 0;
    std::vector<const IEntity*> staticMeshEntities = m_pWorld->GetEntitiesOfType(EntityType::StaticMesh);
    for (const IEntity* pStaticMesh : staticMeshEntities)
    {
        subMeshCount += ((const StaticMeshEntity*)pStaticMesh)->GetSubMeshCount();
    }
    GameStats stats = 
    {
        .entityCount = m_pWorld->m_entities.size()
        , .meshCount = staticMeshEntities.size()
        , .subMeshCount = subMeshCount
        , .textureCount = GRenderer->m_bindlessManager.GetNumberOfGPUTextures()
        , .bufferBytesUploaded = GGpuStats.ReadBufferBytes()
        , .imageBytesUploaded = GGpuStats.ReadImageBytes()

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
