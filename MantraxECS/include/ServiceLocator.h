// ServiceLocator.h
#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "IService.h"
#include "EngineLoaderDLL.h"

class MANTRAX_API ServiceLocator
{
private:
    std::unordered_map<std::string, std::shared_ptr<IService>> services;
    ServiceLocator() {}

public:
    static ServiceLocator &instance();
    void registerService(const std::string &name, std::shared_ptr<IService> service);

    template <typename T>
    std::shared_ptr<T> get(const std::string &name)
    {
        auto it = services.find(name);
        if (it != services.end())
            return std::dynamic_pointer_cast<T>(it->second);
        return nullptr;
    }
};