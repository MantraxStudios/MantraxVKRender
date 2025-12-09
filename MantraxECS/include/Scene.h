#pragma once
#include <vector>
#include <string>

#include "EngineLoaderDLL.h"
#include "ecs.h"
#include "EntityObject.h"

class MANTRAX_API Scene
{
public:
    Scene() = default;
    explicit Scene(const std::string &name);

    // Deshabilitar copia (World no es copiable)
    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;

    // Habilitar movimiento
    Scene(Scene &&) noexcept = default;
    Scene &operator=(Scene &&) noexcept = default;

    void OnCreate();
    void OnDestroy();
    void OnUpdate(float dt);

    EntityObject CreateEntityObject(const std::string &name = "EntityObject");
    void DestroyEntityObject(EntityObject &obj);

    std::vector<EntityObject> &GetEntityObjects() { return entityObjects; }
    ecs::World &GetWorld() { return world; }

    const std::string &GetName() const { return sceneName; }

private:
    std::string sceneName = "Scene";
    ecs::World world;
    std::vector<EntityObject> entityObjects;
};