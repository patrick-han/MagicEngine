#pragma once
#include "../Entity.h"

namespace Magic
{

// ONLY ACCESS WITH A SINGLE THREAD!!!
struct Editor
{
    static constexpr int defaultMaxTextLength = 128;

    char loadWorldTextBoxNameBuffer[defaultMaxTextLength] = "GameCode/world.json";
    char newResourceNameBuffer[defaultMaxTextLength] = "NewResource";
    char newResourcePathBuffer[defaultMaxTextLength*4] = "Path";
    IEntity* sceneOutlineSelectedEntity = nullptr;
    bool isWorldLoaded = false;
    char loadedWorldNameBuffer[defaultMaxTextLength] = "NONE";

    // Static Mesh
    char assignStaticMeshResourceNameBuffer[defaultMaxTextLength] = "ResourceName";

    // Inspector
    float transformAmount = 1.0f;
};


extern Editor* GEditor;

}