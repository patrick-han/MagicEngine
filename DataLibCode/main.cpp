#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "TestHeader.h"

int main()
{
    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate
                            | aiProcess_FlipUVs
                            | aiProcess_CalcTangentSpace
                            | aiProcess_PreTransformVertices // Flattens all nodes and their relative transforms into a single node with "frozen: transforms
            // TODO: don't really understand how this is working tbh
                            ;
    const aiScene* scene = importer.ReadFile("test", flags);
    if (!scene)
    {
        std::cout << "Failed to load scene" << std::endl;
    }
    std::cout << "Hello Data Cooker!: " << Magic::DataLib::kMax << std::endl;
}