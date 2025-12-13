#pragma once

#include <string>
#include "ecs.h"
#include "EngineLoaderDLL.h"

class MANTRAX_API EntityObject
{
public:
    std::string name = "EntityObject";

    EntityObject() : world(nullptr), entity() {}
    EntityObject(ecs::World *w, ecs::Entity ent)
        : world(w), entity(ent) {}

    // ========================================================================
    // COMPONENT MANAGEMENT (Estilo Unity)
    // ========================================================================

    // AddComponent - Agrega un componente con valores por defecto o personalizados
    template <typename T, typename... Args>
    T &AddComponent(Args &&...args)
    {
        if (!world)
            throw std::runtime_error("EntityObject has no valid world");

        return world->addComponent<T>(entity, std::forward<Args>(args)...);
    }

    // GetComponent - Obtiene un componente (nullptr si no existe)
    template <typename T>
    T *GetComponent()
    {
        if (!world)
            return nullptr;
        return world->getComponent<T>(entity);
    }

    // HasComponent - Verifica si tiene un componente
    template <typename T>
    bool HasComponent() const
    {
        if (!world)
            return false;
        return world->hasComponent<T>(entity);
    }

    // RemoveComponent - Remueve un componente
    template <typename T>
    void RemoveComponent()
    {
        if (world)
            world->removeComponent<T>(entity);
    }

    // ========================================================================
    // ENTITY INFO
    // ========================================================================

    ecs::Entity GetEntity() const { return entity; }
    bool IsValid() const { return world && world->isAlive(entity); }

    bool operator==(const EntityObject &other) const
    {
        return entity == other.entity;
    }

private:
    ecs::World *world;
    ecs::Entity entity;
};