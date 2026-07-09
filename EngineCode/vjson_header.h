#pragma once
#include "ThirdParty/vjson/vjson.h"
#include <fstream>
#include <sstream>

namespace Magic
{

[[nodiscard]] inline bool SaveJsonToFile(const std::string& fileName, const std::string& jsonString)
{
    std::ofstream outFile(fileName);
    if (outFile.is_open()) 
    {
        outFile << jsonString;
        outFile.close();
        return true;
    } 
    else 
    {
        return false;
    }
}

[[nodiscard]] inline bool LoadJsonToString(const std::string& fileName, std::string& jsonString)
{
    std::ifstream inFile(fileName);

    if (!inFile.is_open())
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << inFile.rdbuf();

    jsonString = buffer.str();

    return true;
}

}
