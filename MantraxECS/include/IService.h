#pragma once
#include <string>

class IService
{
public:
    virtual ~IService() = default;
    virtual std::string getName() = 0;
};
