#include "Import.h"
#include "Data.h"
#include "../CommonCode/Log.h"
#include "../CommonCode/Math/Matrix4f.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace Magic::Data
{
enum class AssetType
{
    Unknown
    , StaticMesh
    , Texture
};
}

using namespace Magic;

int main(int argc, char *argv[])
{
    Logger::Info(std::format("Number of arguments: {}", argc));
    Data::AssetType assetType = Data::AssetType::Unknown;
    std::string typeArg = argv[1];
    std::string filepath = argv[2];
    if (typeArg == "-staticMesh")
    {
        assetType = Data::AssetType::StaticMesh;
    }
    else if (typeArg == "-texture")
    {
        assetType = Data::AssetType::Texture;
    }
    else
    {
        Logger::Err(std::format("Unknown argument: {}", typeArg));
    }


    if (assetType == Data::AssetType::StaticMesh)
    {
        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate
                                // | aiProcess_FlipUVs
                                // | aiProcess_CalcTangentSpace
                                // | aiProcess_PreTransformVertices // Flattens all nodes and their relative transforms into a single node with "frozen: transforms
                // TODO: don't really understand how this is working tbh
                                ;
        const aiScene* scene = importer.ReadFile(filepath, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Logger::Err(std::format("Failed to load model: {}", importer.GetErrorString()));
            std::exit(EXIT_FAILURE);
        }
        else
        {
            Logger::Info(std::format("Loaded model: {}", filepath));
        }

        ModelData modelData;

        Matrix4f dummyTransform;
        Data::ProcessAssimpNode(modelData, scene->mRootNode, scene, dummyTransform);

        Data::SerializeModelData(modelData, "helmet.bin");

        ModelData modelData2 = Data::DeserializeModelData("helmet.bin");


        Logger::Info(std::format("Processed model: {}", filepath));
    }
}