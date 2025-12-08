#pragma once
#include <string>
#include "EngineLoaderDLL.h"

class MANTRAX_API IService
{
public:
    virtual ~IService() {}
    virtual std::string getName() = 0;
};
