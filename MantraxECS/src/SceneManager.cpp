#include "../include/SceneManager.h"
#include <iostream>

std::unique_ptr<Scene> SceneManager::activeScene;

// ==========================
void SceneManager::CreateScene(const std::string &name)
{
    // Verificar que no se esté intentando crear una escena con nombre vacío
    if (name.empty())
    {
        std::cerr << "Error: Cannot create scene with empty name" << std::endl;
        return;
    }

    // Solo destruir si hay una escena activa Y no es la escena principal
    // O si quieres permitir destruir la principal, asegúrate de notificar a los listeners
    DestroyActiveScene();

    activeScene = std::make_unique<Scene>(name);

    // Verificar que la escena se creó correctamente
    if (!activeScene)
    {
        std::cerr << "Error: Failed to create scene '" << name << "'" << std::endl;
        return;
    }

    activeScene->OnCreate();

    std::cout << "Scene '" << name << "' created successfully" << std::endl;
}

// ==========================
void SceneManager::LoadScene(const std::string &name)
{
    CreateScene(name);
}

// ==========================
void SceneManager::DestroyActiveScene()
{
    if (activeScene)
    {
        std::cout << "Destroying scene: " << activeScene->GetName() << std::endl;
        activeScene->OnDestroy();
        activeScene.reset();
    }
}

// ==========================
Scene *SceneManager::GetActiveScene()
{
    return activeScene.get();
}

// ==========================
bool SceneManager::HasActiveScene()
{
    return activeScene != nullptr;
}

// ==========================
std::string SceneManager::GetActiveSceneName()
{
    if (activeScene)
    {
        return activeScene->GetName();
    }
    return "";
}