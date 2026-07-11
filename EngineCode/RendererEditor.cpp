#include <random>
#include "../CommonCode/Log.h"
#include "Renderer.h"
#include "ThirdParty/imgui/imgui.h"
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "ThirdParty/imgui/imgui_impl_vulkan.h"
#include "Editor/Editor.h"
#include "World.h"
#include "../GameCode/Game.h"
#include "Camera.h"
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
    ImGui::Text("Entity Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%zu", renderingInfo.gameStats.entityCount);
    ImGui::Text("Mesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%zu", renderingInfo.gameStats.meshCount);
    ImGui::Text("SubMesh Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%zu", renderingInfo.gameStats.subMeshCount);
    ImGui::Text("Texture Count:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%zu", renderingInfo.gameStats.textureCount);
    ImGui::Text("Camera Axes:");
    ImGui::TextColored(ImVec4(1,0.3,0.3,1), "X: [%f %f %f]", pGame->m_camera->GetLeft().x, pGame->m_camera->GetLeft().y, pGame->m_camera->GetLeft().z);
    ImGui::TextColored(ImVec4(0.3,1,0.3,1), "Y: [%f %f %f]", pGame->m_camera->GetUp().x, pGame->m_camera->GetUp().y, pGame->m_camera->GetUp().z);
    ImGui::TextColored(ImVec4(0.3,0.3,1,1), "Z: [%f %f %f]", pGame->m_camera->GetForward().x, pGame->m_camera->GetForward().y, pGame->m_camera->GetForward().z);
    ImGui::Checkbox("Show Bounding Boxes", &m_renderBoundingBoxes);
    if (!GEditor->isWorldLoaded)
    {
        ImGui::InputText("##WorldNameTextBox", GEditor->loadWorldTextBoxNameBuffer, IM_ARRAYSIZE(GEditor->loadWorldTextBoxNameBuffer));

        if (ImGui::Button("Load World", ImVec2(150, 30)))
        {
            if (!GEditor->isWorldLoaded)
            {
                // world->Init(GEditor->loadWorldTextBoxNameBuffer);
                pGame->LoadContent();
                // const std::unordered_set<UUID>& uuids = world->GetAllUUIDs();
                // if (!uuids.empty())
                // {
                //     GEditor->sceneOutlineSelectedEntityUUID = *uuids.begin();
                //     GEditor->isSceneOutlineSelectedEntityUUIDValid = true;
                // }
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
    }
    ImGui::Text("World Loaded:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%s", GEditor->loadedWorldNameBuffer);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y * 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, displaySize.y * 0.5f));

    // if (ImGui::Begin("Scene Outline", nullptr, flags))
    // {
    //     // Make a scrollable region that fills the window's content area
    //     ImVec2 avail = ImGui::GetContentRegionAvail();
    //     ImGui::BeginChild("SceneOutlineList", avail, ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    //     for (const auto& uuid : world->GetAllUUIDs()) // std::set
    //     {
    //         const std::string& name = world->GetEntityName(uuid);
    //         const EntityType entityType = world->GetEntityType(uuid);

    //         bool isSelected = (uuid == GEditor->sceneOutlineSelectedEntityUUID);
    //         if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
    //         {
    //             GEditor->sceneOutlineSelectedEntityUUID = uuid;
    //         }
    //     }

    //     ImGui::EndChild();
    // }
    // ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(displaySize.x - 400.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, displaySize.y));


    //     // Entity: Display transform
    //     std::optional<Matrix4f> pEntityTransform = world->GetStaticMeshEntityTransform(selectedEntityUUID);
    //     if (pEntityTransform)
    //     {
    //         const Matrix4f& t = *pEntityTransform;
    //         ImGui::TextWrapped("Transform Matrix");
    //         ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[0], t.m[1], t.m[2], t.m[3]);
    //         ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[4], t.m[5], t.m[6], t.m[7]);
    //         ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[8], t.m[9], t.m[10], t.m[11]);
    //         ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[12], t.m[13], t.m[14], t.m[15]);

    //         ImGui::InputFloat("Transform Amount", &GEditor->transformAmount, 0.01f, 1.0f, "%.3f");
    //         if (ImGui::Button("+X", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //         if (ImGui::Button("-X", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(-1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //         if (ImGui::Button("+Y", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 1.0f, 0.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //         if (ImGui::Button("-Y", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, -1.0f, 0.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //         if (ImGui::Button("+Z", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, 1.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //         if (ImGui::Button("-Z", ImVec2(50, 30)))
    //         {
    //             Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, -1.0f) * GEditor->transformAmount);
    //             world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
    //         }
    //     }
    // }
    // ImGui::End();
}
}