// ServiceLocator.h
#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "IService.h"

class ServiceLocator
{
private:
    std::unordered_map<std::string, std::shared_ptr<IService>> services;

    ServiceLocator() = default;

public:
    static ServiceLocator &instance()
    {
        static ServiceLocator inst;
        return inst;
    }

    void registerService(const std::string &name, std::shared_ptr<IService> service)
    {
        services[name] = service;
    }

    template <typename T>
    std::shared_ptr<T> get(const std::string &name)
    {
        auto it = services.find(name);
        if (it != services.end())
            return std::dynamic_pointer_cast<T>(it->second);
        return nullptr;
    }
};
