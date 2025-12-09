#include "../include/Scene.h"
#include <algorithm>
#include <iostream>

Scene::Scene(const std::string &name)
    : sceneName(name)
{
}

void Scene::OnCreate()
{
    std::cout << "Scene created: " << sceneName << std::endl;
}

void Scene::OnDestroy()
{
    for (auto &obj : entityObjects)
        world.destroyEntity(obj.GetEntity()); // Cambiado registry a world

    entityObjects.clear();
}

void Scene::OnUpdate(float /*dt*/)
{
    // Aquí después metes sistemas
}

EntityObject Scene::CreateEntityObject(const std::string &name)
{
    ecs::Entity entity = world.createEntity(); // Cambiado registry a world
    EntityObject obj(&world, entity);          // Cambiado registry a world
    obj.name = name;

    entityObjects.push_back(obj);
    return obj;
}

void Scene::DestroyEntityObject(EntityObject &obj)
{
    world.destroyEntity(obj.GetEntity()); // Cambiado registry a world

    entityObjects.erase(
        std::remove_if(entityObjects.begin(), entityObjects.end(),
                       [&](const EntityObject &o)
                       {
                           return o.GetEntity() == obj.GetEntity();
                       }),
        entityObjects.end());
}