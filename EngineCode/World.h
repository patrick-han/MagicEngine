#pragma once

#include <iostream>
// #include <list>
#include <vector>
#include "Entity.h"

namespace Magic
{

class World
{
public:
    World();
    ~World();

    [[nodiscard]] MeshEntity* CreateMeshEntity()
    {
        MeshEntity* newMeshEntity = new MeshEntity;
        m_meshEntities.push_back(newMeshEntity);
        return newMeshEntity;
    }

    std::vector<MeshEntity*> m_meshEntities;
    std::string m_mapName;
};


}