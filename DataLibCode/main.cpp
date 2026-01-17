#include "ImportGLTF.h"
#include "DataSerialization.h"
#include "../CommonCode/Log.h"
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
    std::string outputPath = "helmet.bin";
    if (argc > 3)
    {
         outputPath = argv[3];
    }
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
    StaticMeshData modelData;
    GLTFImporter importer;
    if (assetType == Data::AssetType::StaticMesh)
    {
        importer.ImportGLTF(filepathStr, modelData);
    }

    Data::SerializeStaticMeshDataBlob(modelData, outputPath);

    Logger::Info(std::format("Processed model: {}", filepathStr));
}