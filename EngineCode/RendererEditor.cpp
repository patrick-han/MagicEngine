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
#include "GPUStats.h"
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

#if MAGIC_TRACK_GPU_STATS
    auto bytesToMB = [](std::size_t bytes) -> double { return static_cast<double>(bytes) / (1024.0 * 1024.0); };\
    ImGui::Text("Buffer MB Uploaded:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%lf", bytesToMB(renderingInfo.gameStats.bufferBytesUploaded));
    ImGui::Text("Image MB Uploaded:"); ImGui::SameLine(); ImGui::TextColored(ImVec4(0,1,0,1), "%lf", bytesToMB(renderingInfo.gameStats.imageBytesUploaded));
#endif
    ImGui::Text("Camera Axes:");
    ImGui::TextColored(ImVec4(1,0.3,0.3,1), "X (right): [%f %f %f]", pGame->m_camera->GetRight().x, pGame->m_camera->GetRight().y, pGame->m_camera->GetRight().z);
    ImGui::TextColored(ImVec4(0.3,1,0.3,1), "Y (forward): [%f %f %f]", pGame->m_camera->GetForward().x, pGame->m_camera->GetForward().y, pGame->m_camera->GetForward().z);
    ImGui::TextColored(ImVec4(0.3,0.3,1,1), "Z (up): [%f %f %f]", pGame->m_camera->GetUp().x, pGame->m_camera->GetUp().y, pGame->m_camera->GetUp().z);
    ImGui::Checkbox("Show Bounding Boxes", &m_renderBoundingBoxes);
    if (!GEditor->isWorldLoaded)
    {
        ImGui::InputText("##WorldNameTextBox", GEditor->loadWorldTextBoxNameBuffer, IM_ARRAYSIZE(GEditor->loadWorldTextBoxNameBuffer));

        if (ImGui::Button("Load World", ImVec2(150, 30)))
        {
            if (!GEditor->isWorldLoaded)
            {
                pGame->LoadContent(GEditor->loadWorldTextBoxNameBuffer);
                auto entities = world->GetAllEntities();
                if (!entities.empty())
                {
                    GEditor->sceneOutlineSelectedEntity = entities[0];
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
                pGame->UnloadContent();
                renderingInfo.meshesToRender.clear(); // Invalidate all queued up items
                GEditor->isWorldLoaded = false;
                GEditor->sceneOutlineSelectedEntity = nullptr;
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

    if (ImGui::Begin("Scene Outline", nullptr, flags))
    {
        // Make a scrollable region that fills the window's content area
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("SceneOutlineList", avail, ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        int i = 0;
        for (auto entity : world->GetAllEntities())
        {
            const EntityType entityType = entity->GetEntityType();
            bool isSelected = (entity == GEditor->sceneOutlineSelectedEntity);
            std::string label = entity->GetName() + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
            {
                GEditor->sceneOutlineSelectedEntity = entity;
            }
            i++;
        }

        ImGui::EndChild();
    }
    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(displaySize.x - 400.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, displaySize.y));

    if (ImGui::Begin("Inspector", nullptr, flags))
    {
        // Entity: Display transform
        if (GEditor->sceneOutlineSelectedEntity)
        {
            Matrix4f t = GEditor->sceneOutlineSelectedEntity->m_transform;
  
            ImGui::TextWrapped("Transform Matrix");
            ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[0], t.m[1], t.m[2], t.m[3]);
            ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[4], t.m[5], t.m[6], t.m[7]);
            ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[8], t.m[9], t.m[10], t.m[11]);
            ImGui::TextWrapped("[%f, %f, %f, %f]", t.m[12], t.m[13], t.m[14], t.m[15]);

            // ImGui::InputFloat("Transform Amount", &GEditor->transformAmount, 0.01f, 1.0f, "%.3f");
            // if (ImGui::Button("+X", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
            //     GEditor->sceneOutlineSelectedEntity->m_transform, translate * t);
            // }
            // if (ImGui::Button("-X", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(-1.0f, 0.0f, 0.0f) * GEditor->transformAmount);
            //     world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
            // }
            // if (ImGui::Button("+Y", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 1.0f, 0.0f) * GEditor->transformAmount);
            //     world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
            // }
            // if (ImGui::Button("-Y", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, -1.0f, 0.0f) * GEditor->transformAmount);
            //     world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
            // }
            // if (ImGui::Button("+Z", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, 1.0f) * GEditor->transformAmount);
            //     world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
            // }
            // if (ImGui::Button("-Z", ImVec2(50, 30)))
            // {
            //     Matrix4f translate = Matrix4f::MakeTranslate(Vector3f(0.0f, 0.0f, -1.0f) * GEditor->transformAmount);
            //     world->SetStaticMeshEntityTransform(selectedEntityUUID, translate * t);
            // }
        
        }
    }
    ImGui::End();
}
}
