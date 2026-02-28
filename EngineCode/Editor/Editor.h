#pragma once
#include "../UUID.h"


namespace Magic
{

// ONLY ACCESS WITH A SINGLE THREAD!!!
struct Editor
{
    static constexpr int defaultMaxTextLength = 128;

    char loadWorldTextBoxNameBuffer[defaultMaxTextLength] = "GameCode/magic.world";
    char newResourceNameBuffer[defaultMaxTextLength] = "NewResource";
    char newResourcePathBuffer[defaultMaxTextLength*4] = "Path";
    UUID sceneOutlineSelectedUUID = UUID{};
    bool isSceneOutlineSelectedUUIDValid = false;
    bool isWorldLoaded = false;
    char loadedWorldNameBuffer[defaultMaxTextLength] = "NONE";

    // Static Mesh
    char assignStaticMeshResourceNameBuffer[defaultMaxTextLength] = "ResourceName";
};


extern Editor* GEditor;

}