#pragma once

#include <pxr/usd/usd/prim.h>

namespace Magic
{

struct StaticMeshData;

class USDImporter
{
public:
    void ImportUSDPrimAsStaticMesh(
        const pxr::UsdPrim& entityPrim,
        StaticMeshData& staticMeshDataData);
};

}
