#pragma once
#include <string>
namespace Magic
{

struct NameComponent
{
    NameComponent(const std::string& _name) : name(_name)
    {

    }
    std::string name;
}
}