#define STB_IMAGE_IMPLEMENTATION
#include "Import.h"
#include "DataSerialization.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Common/Math/Matrix4f.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>



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
    std::string filepathStr = argv[2];
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
                                // | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes
                // TODO: don't really understand how this is working tbh
                                ;
        const aiScene* scene = importer.ReadFile(filepathStr, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Logger::Err(std::format("Failed to load model: {}", importer.GetErrorString()));
            std::exit(EXIT_FAILURE);
        }
        else
        {
            Logger::Info(std::format("Loaded model: {}", filepathStr));
        }

        ModelData modelData;

        Matrix4f dummyTransform;
        std::filesystem::path filepath = filepathStr;
        filepath = filepath.parent_path();
        Logger::Info(std::format("Loaded model: {}", filepath.string()));
        Data::ProcessAssimpNode(modelData, scene->mRootNode, scene, dummyTransform, filepath);

        Data::SerializeModelData(modelData, "helmet.bin");

        ModelData modelData2 = Data::DeserializeModelData("helmet.bin");


        Logger::Info(std::format("Processed model: {}", filepathStr));
    }
}