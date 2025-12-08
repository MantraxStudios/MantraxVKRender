#include "../include/ServiceLocator.h"

ServiceLocator &ServiceLocator::instance()
{
    static ServiceLocator inst;
    return inst;
}

void ServiceLocator::registerService(const std::string &name, std::shared_ptr<IService> service)
{
    services[name] = service;
}