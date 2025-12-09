// SceneManager.h
#pragma once
#include "Scene.h"
#include <memory>
#include <string>
#include "EngineLoaderDLL.h"

class MANTRAX_API SceneManager
{
private:
    static std::unique_ptr<Scene> activeScene;

public:
    static void CreateScene(const std::string &name);
    static void LoadScene(const std::string &name);
    static void DestroyActiveScene();
    static Scene *GetActiveScene();

    // Nuevos métodos útiles
    static bool HasActiveScene();
    static std::string GetActiveSceneName();
};