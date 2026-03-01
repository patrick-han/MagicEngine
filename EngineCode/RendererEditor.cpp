#include <random>
#include "../CommonCode/Log.h"
#include "Renderer.h"
#include "ThirdParty/imgui/imgui.h"
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "ThirdParty/imgui/imgui_impl_vulkan.h"
#include "ResourceDatabase.h"
#include "Editor/Editor.h"
#include "World.h"
#include "../GameCode/Game.h"
namespace Magic
{
void Renderer::DoUIWork(int frameNumber, RenderingInfo& renderingInfo)
{
    const auto world = renderingInfo.pWorld;
    const auto pGame = renderingInfo.pGame;

    ImGui::NewFrame();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Top left
    ImGui::SetNextWindowSize(ImVec2(300, displaySize.y / 2));

    ImGui::Begin("Engine Info", nullptr, flags);
    ImGui::Text("Game::Update() (us):"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%lld", renderingInfo.updateLoopTimingUS.count());
    ImGui::Text("Frame #:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", frameNumber);
    ImGui::Text("Entity Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.entityCount);
    ImGui::Text("RAM Resident StaticMeshData Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.ramResidentStaticMeshDataCount);
    ImGui::Text("Disk Loading StaticMeshData Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.loadingFromDiskStaticMeshCount);
    ImGui::Text("Mesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.meshCount);
    ImGui::Text("SubMesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.subMeshCount);
    ImGui::Text("Texture Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.textureCount);
    ImGui::Text("Pending StaticMeshData Upload Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.pendingStaticMeshDataUploadCount);
    ImGui::Text("Pending Buffer Upload Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.pendingBufferUploadCount);
    ImGui::Text("Pending Image Upload Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.pendingImageUploadCount);
    ImGui::Text("Pending StaticMesh Entities:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.pendingStaticMeshEntities);
    ImGui::Text("Ready StaticMesh Entities:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%d", renderingInfo.gameStats.readyStaticMeshEntities);
    ImGui::Checkbox("Show Bounding Boxes", &m_renderBoundingBoxes);
    if (!GEditor->isWorldLoaded)
    {
        ImGui::InputText("##WorldNameTextBox", GEditor->loadWorldTextBoxNameBuffer, IM_ARRAYSIZE(GEditor->loadWorldTextBoxNameBuffer));

        if (ImGui::Button("Load World", ImVec2(150, 30)))
        {
            if (!GEditor->isWorldLoaded)
            {
                world->Init(GEditor->loadWorldTextBoxNameBuffer);
                pGame->LoadContent();
                const std::unordered_set<UUID>& uuids = world->GetAllUUIDs();
                if (!uuids.empty())
                {
                    GEditor->sceneOutlineSelectedEntityUUID = *uuids.begin();
                    GEditor->isSceneOutlineSelectedEntityUUIDValid = true;
                }
                GEditor->isWorldLoaded = true;
#if PLATFORM_WINDOWS
                strncpy_s(GEditor->loadedWorldNameBuffer, GEditor->loadWorldTextBoxNameBuffer, GEditor->defaultMaxTextLength);
#elif PLATFORM_MACOS
                strlcpy(GEditor->loadedWorldNameBuffer, GEditor->loadWorldTextBoxNameBuffer, GEditor->defaultMaxTextLength);
#endif
            }
            else
            {
                Logger::Warn("World already loaded! Please unload the current world first before loading another!");
            }
        }
    }
    else
    {
        if (ImGui::Button("Unload World", ImVec2(150, 30)))
        {
            if (GEditor->isWorldLoaded)
            {
                world->Clear();
                pGame->UnloadContent();
                renderingInfo.meshesToRender.clear(); // Invalidate all queued up items
                GEditor->isWorldLoaded = false;
                GEditor->isSceneOutlineSelectedEntityUUIDValid = false;
#if PLATFORM_WINDOWS
                strncpy_s(GEditor->loadedWorldNameBuffer, "NULL", 5);
#elif PLATFORM_MACOS
                strlcpy(GEditor->loadedWorldNameBuffer, "NULL", 5);
#endif
            }
        }
        if (ImGui::Button("Save World", ImVec2(150, 30)))
        {
            if (GEditor->isWorldLoaded)
            {
                world->Save(GEditor->loadWorldTextBoxNameBuffer);
            }
        }
        if (ImGui::Button("Add StaticMeshEntity", ImVec2(150, 30)))
        {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(1,610293);
            std::string s = std::string("RandomStaticMesh") + std::to_string(dist(rng));
            world->AddNewStaticMeshEntity(s.c_str());
        }
        if (ImGui::Button("Remove Selected Entity", ImVec2(150, 30)))
        {
            if (GEditor->isSceneOutlineSelectedEntityUUIDValid == true)
            {
                world->RemoveEntity(GEditor->sceneOutlineSelectedEntityUUID);
                const std::unordered_set<UUID>& uuids = world->GetAllUUIDs();
                if (!uuids.empty())
                {
                    GEditor->sceneOutlineSelectedEntityUUID = *uuids.begin();
                    GEditor->isSceneOutlineSelectedEntityUUIDValid = true;
                }
                else
                {
                    GEditor->isSceneOutlineSelectedEntityUUIDValid = false;
                }
            }
        }
    }
    ImGui::Text("World Loaded:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%s", GEditor->loadedWorldNameBuffer);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, displaySize.y * 0.5f));

    if (ImGui::Begin("Scene Outline", nullptr, flags))
    {
        // Make a scrollable region that fills the window's content area
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("SceneOutlineList", avail, ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        for (const auto& uuid : world->GetAllUUIDs()) // std::set
        {
            const std::string& name = world->GetEntityName(uuid);
            const EntityType entityType = world->GetEntityType(uuid);

            bool isSelected = (uuid == GEditor->sceneOutlineSelectedEntityUUID);
            if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
            {
                GEditor->sceneOutlineSelectedEntityUUID = uuid;
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(displaySize.x - 400.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, displaySize.y));
    ImGui::Begin("Inspector", nullptr, flags);
    if (!GEditor->isSceneOutlineSelectedEntityUUIDValid) // Sometimes we may start with an empty world, but later add an entity
    {
        const std::unordered_set<UUID>& uuids = world->GetAllUUIDs();
        if (!uuids.empty())
        {
            GEditor->sceneOutlineSelectedEntityUUID = *uuids.begin();
            GEditor->isSceneOutlineSelectedEntityUUIDValid = true;
        }
    }
    if (GEditor->isSceneOutlineSelectedEntityUUIDValid)
    {
        UUID selectedEntityUUID  = GEditor->sceneOutlineSelectedEntityUUID;
        const EntityType entityType = world->GetEntityType(selectedEntityUUID );
        // Display entity type
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.8f, 1.0f), "%s", World::EntityTypeToStr(entityType));

        // StaticMesh: Display the resource path and name
        if (entityType == EntityType::StaticMesh)
        {
            std::optional<UUID> resourceUUID = world->GetStaticMeshEntityResourceUUID(selectedEntityUUID);
            if (resourceUUID)
            {
                ImGui::TextWrapped( "ResName: %s", GResourceDB->GetResName(*resourceUUID));
                ImGui::TextWrapped( "ResPath: %s", GResourceDB->GetResPath(*resourceUUID));
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "ResName: NULL\nResPath: NULL");
            }
            ImGui::InputText("ResName##AssignNewResNameBox", GEditor->assignStaticMeshResourceNameBuffer, IM_ARRAYSIZE(GEditor->assignStaticMeshResourceNameBuffer));
            if (ImGui::Button("Assign", ImVec2(150, 30)))
            {
                if (!GResourceDB->CheckIfResourceExists(GEditor->assignStaticMeshResourceNameBuffer))
                {
                    Logger::Err("Trying to assign resource to StaticMesh that doesn't exist!");
                }
                else
                {
                    if (!world->UpdateStaticMeshEntityResource(selectedEntityUUID, GEditor->assignStaticMeshResourceNameBuffer))
                    {
                        Logger::Err("Tried to update static mesh entity resource but could not find entity!");
                    }
                }
            }
        }

        // Entity: Display transform
        std::optional<Matrix4f> pEntityTransform = world->GetStaticMeshEntityTransform(selectedEntityUUID);
        if (pEntityTransform)
        {
            Matrix4f entityTransform = *pEntityTransform;
            ImGui::TextWrapped( "Transform Matrix");
            ImGui::TextWrapped( "[%f, %f, %f, %f]", entityTransform.m00, entityTransform.m01, entityTransform.m02, entityTransform.m03);
            ImGui::TextWrapped( "[%f, %f, %f, %f]", entityTransform.m10, entityTransform.m11, entityTransform.m12, entityTransform.m13);
            ImGui::TextWrapped( "[%f, %f, %f, %f]", entityTransform.m20, entityTransform.m21, entityTransform.m22, entityTransform.m23);
            ImGui::TextWrapped( "[%f, %f, %f, %f]", entityTransform.m30, entityTransform.m31, entityTransform.m32, entityTransform.m33);

            ImGui::InputFloat("Transform Amount", &GEditor->transformAmount, 0.01f, 1.0f, "%.3f");
            if (ImGui::Button("+X", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
            if (ImGui::Button("-X", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(-1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
            if (ImGui::Button("+Y", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 1.0f, 0.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
            if (ImGui::Button("-Y", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, -1.0f, 0.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
            if (ImGui::Button("+Z", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, 1.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
            if (ImGui::Button("-Z", ImVec2(50, 30)))
            {
                Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, -1.0f) * GEditor->transformAmount);
                world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * entityTransform);
            }
        }
    }
    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(300, (displaySize.y / 2) + (displaySize.y / 3)));
    ImGui::SetNextWindowSize(ImVec2(500, (displaySize.y / 2) - (displaySize.y / 3)));
    ImGui::Begin("Resource Database", nullptr, flags);

    ImGui::PushItemWidth(200.0f); 
    ImGui::InputText("ResName##NewResourceNameTextBox", GEditor->newResourceNameBuffer, IM_ARRAYSIZE(GEditor->newResourceNameBuffer));
    ImGui::InputText("ResPath##NewResourcePathTextBox", GEditor->newResourcePathBuffer, IM_ARRAYSIZE(GEditor->newResourcePathBuffer));
    ImGui::PopItemWidth();
    if (ImGui::Button("Add New Resource", ImVec2(150, 30)))
    {
        GResourceDB->AddStaticMeshResource(GEditor->newResourceNameBuffer, GEditor->newResourcePathBuffer);
        GResourceDB->Save();
    }
    ImGui::End();
}
}