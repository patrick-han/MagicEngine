#include "ImportGLTF.h"
#include "DataSerialization.h"
#include "../EngineCode/Common/Log.h"
#include "../EngineCode/Common/Math/Matrix4f.h"
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
    ModelData modelData;
    if (assetType == Data::AssetType::StaticMesh)
    {
        ImportGLTF(filepathStr, modelData);
    }

    Data::SerializeModelDataBlob(modelData, outputPath);

    Logger::Info(std::format("Processed model: {}", filepathStr));
}