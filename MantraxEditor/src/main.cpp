#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxRender/include/MantraxGFX_Timer.h"
#include "../MantraxECS/include/SkyBox.h"
#include "../includes/FPSCamera.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../../MantraxECS/include/ServiceLocator.h"
#include "../includes/ImGuiManager.h"
#include "../MantraxECS/include/InputSystem.h"
#include "../MantraxECS/include/ModelManager.h"
#include "../MantraxECS/include/SceneRenderer.h"
#include "../MantraxECS/include/TextureLoader.h"
#include "../MantraxECS/include/ECS.h"
#include "../MantraxECS/include/LuaScript.h"
#include "../includes/LuaEditor.h"
#include <iostream>
#include <random>

// ============================================================================
// COMPONENTES DEL JUEGO
// ============================================================================
struct Transform
{
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct Velocity
{
    glm::vec3 linear{0.0f};
    glm::vec3 angular{0.0f};
};

struct RenderComponent
{
    RenderableObject *renderObject = nullptr;
};

struct Rotator
{
    float speed = 1.0f;
    glm::vec3 axis{0.0f, 1.0f, 0.0f};
};

// Variables globales
Mantrax::InputSystem *g_InputSystem = nullptr;
Mantrax::FPSCamera *g_Camera = nullptr;
SceneView *g_SceneView = nullptr;

LRESULT CustomWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;

    if (g_InputSystem)
    {
        g_InputSystem->ProcessMessage(msg, wp, lp, hwnd);
    }

    return 0;
}

void ProcessCameraInput(Mantrax::FPSCamera &camera, Mantrax::InputSystem &input, float deltaTime)
{
    if (!input.IsMouseButtonDown(Mantrax::MouseButton::Right))
        return;

    bool sprint = input.IsKeyDown(Mantrax::KeyCode::Shift);

    if (input.IsKeyDown(Mantrax::KeyCode::W))
        camera.ProcessKeyboard(Mantrax::FORWARD, deltaTime, sprint);

    if (input.IsKeyDown(Mantrax::KeyCode::S))
        camera.ProcessKeyboard(Mantrax::BACKWARD, deltaTime, sprint);

    if (input.IsKeyDown(Mantrax::KeyCode::A))
        camera.ProcessKeyboard(Mantrax::LEFT, deltaTime, sprint);

    if (input.IsKeyDown(Mantrax::KeyCode::D))
        camera.ProcessKeyboard(Mantrax::RIGHT, deltaTime, sprint);

    if (input.IsKeyDown(Mantrax::KeyCode::Space))
        camera.ProcessKeyboard(Mantrax::UP, deltaTime, sprint);

    if (input.IsKeyDown(Mantrax::KeyCode::Ctrl))
        camera.ProcessKeyboard(Mantrax::DOWN, deltaTime, sprint);

    POINT delta = input.GetMouseDelta();
    if (delta.x != 0 || delta.y != 0)
    {
        camera.ProcessMouseMovement(
            static_cast<float>(delta.x),
            static_cast<float>(delta.y));
    }

    float wheelDelta = input.GetMouseWheelDelta();
    if (wheelDelta != 0.0f)
    {
        camera.ProcessMouseScroll(wheelDelta * 2.0f);
    }
}

// ============================================================================
// SISTEMAS DEL JUEGO
// ============================================================================

// Sistema que actualiza rotaciones
void UpdateRotationSystem(ecs::Registry &registry, float deltaTime)
{
    for (auto [entity, transform, rotator] : registry.view<Transform, Rotator>())
    {
        transform.rotation += rotator.axis * rotator.speed * deltaTime;
    }
}

// Sistema que aplica velocidades
void UpdatePhysicsSystem(ecs::Registry &registry, float deltaTime)
{
    for (auto [entity, transform, velocity] : registry.view<Transform, Velocity>())
    {
        transform.position += velocity.linear * deltaTime;
        transform.rotation += velocity.angular * deltaTime;
    }
}

// Sistema que sincroniza Transform con RenderableObject
void SyncRenderSystem(ecs::Registry &registry)
{
    for (auto [entity, transform, render] : registry.view<Transform, RenderComponent>())
    {
        if (render.renderObject)
        {
            render.renderObject->position = transform.position;
            render.renderObject->rotation = transform.rotation;
            render.renderObject->scale = transform.scale;
        }
    }
}

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    AllocConsole();
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
#else
int main()
{
    void *hInst = nullptr;
#endif

    try
    {
        // Inicializar servicios
        ServiceLocator::instance().registerService("EngineLoader", std::make_shared<EngineLoader>());
        auto loader = ServiceLocator::instance().get<EngineLoader>("EngineLoader");
        loader->Start(hInst, CustomWndProc);

        Mantrax::ImGuiManager imgui(loader->window->GetHWND(), loader->gfx.get());

        ServiceLocator::instance().registerService("InputSystem", std::make_shared<Mantrax::InputSystem>());
        auto inputLoader = ServiceLocator::instance().get<Mantrax::InputSystem>("InputSystem");
        g_InputSystem = inputLoader.get();

        ServiceLocator::instance().registerService("SceneRenderer", std::make_shared<SceneRenderer>(loader->gfx.get()));
        auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");

        ModelManager modelManager(loader->gfx.get());

        ecs::Registry registry;

        std::vector<RenderableObject *> renderObjects;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDist(-10.0f, 10.0f);
        std::uniform_real_distribution<float> speedDist(0.5f, 2.0f);

        // Crear 10 cubos con diferentes posiciones y velocidades
        for (int i = 0; i < 10; i++)
        {
            // Crear el modelo
            auto cubeObj = modelManager.CreateModelWithPBR(
                "Cube.fbx",
                "Cube_" + std::to_string(i),
                "textures/DiamondPlate008A_1K-PNG_Color.png",
                "textures/DiamondPlate008A_1K-PNG_NormalGL.png",
                "textures/DiamondPlate008A_1K-PNG_Metalness.png",
                "textures/DiamondPlate008A_1K-PNG_Roughness.png",
                "textures/DiamondPlate008A_1K-PNG_AmbientOcclusion.png",
                loader->normalShader);

            if (!cubeObj)
                continue;

            renderObjects.push_back(cubeObj);
            sceneRenderer->AddObject(cubeObj);

            // Crear entidad en el ECS
            auto entity = registry.createEntity();

            // Agregar componente Transform con posición aleatoria
            Transform &transform = registry.addComponent<Transform>(entity);
            transform.position = glm::vec3(posDist(gen), posDist(gen), posDist(gen));
            transform.scale = glm::vec3(1.0f + (i * 0.2f)); // Escala variable

            // Agregar componente Rotator con velocidad aleatoria
            Rotator &rotator = registry.addComponent<Rotator>(entity);
            rotator.speed = speedDist(gen);
            rotator.axis = glm::normalize(glm::vec3(
                posDist(gen),
                posDist(gen),
                posDist(gen)));

            // Agregar componente RenderComponent
            RenderComponent &render = registry.addComponent<RenderComponent>(entity);
            render.renderObject = cubeObj;

            // Algunos objetos tendrán velocidad lineal
            if (i % 3 == 0)
            {
                Velocity &velocity = registry.addComponent<Velocity>(entity);
                velocity.linear = glm::vec3(
                    posDist(gen) * 0.5f,
                    posDist(gen) * 0.5f,
                    posDist(gen) * 0.5f);
                velocity.angular = glm::vec3(0.0f, speedDist(gen), 0.0f);
            }
        }

        std::cout << "\n✅ Created " << renderObjects.size() << " entities with ECS\n";

        // Crear Skybox
        Mantrax::SkyBox skybox(loader->gfx.get(), 1.0f, 32, 16);

        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *skyboxData = stbi_load("textures/skybox.jpg", &width, &height, &channels, STBI_rgb_alpha);
        auto skyboxTexture = loader->gfx->CreateTexture(skyboxData, width, height);
        stbi_image_free(skyboxData);

        auto skyboxMaterial = loader->gfx->CreateMaterial(loader->skyboxShader);
        skyboxMaterial->SetBaseColor(2.0f, 2.0f, 2.0f);
        loader->gfx->SetMaterialPBRTextures(skyboxMaterial, skyboxTexture, nullptr, nullptr, nullptr, nullptr);

        auto skyboxRenderObj = skybox.GetRenderObject();
        skyboxRenderObj.material = skyboxMaterial;

        RenderableObject skyboxObj;
        skyboxObj.renderObj = skyboxRenderObj;
        skyboxObj.material = skyboxMaterial;
        skyboxObj.name = "Skybox";
        skyboxObj.position = glm::vec3(0.0f);
        skyboxObj.rotation = glm::vec3(0.0f);
        skyboxObj.scale = glm::vec3(1.0f);

        sceneRenderer->AddObject(&skyboxObj);

        // Configurar cámara
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 5.0f, 20.0f), 60.0f, 0.1f, 1000.0f);
        camera.SetAspectRatio(1540.0f / 948.0f);
        camera.SetMovementSpeed(5.0f);
        camera.SetMouseSensitivity(0.1f);
        g_Camera = &camera;

        // Setup UI
        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);
        ServiceLocator::instance().registerService("UIRender", std::make_shared<UIRender>());
        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        g_SceneView = renderView;
        uiRender->Set(renderView);
        uiRender->GetByType<SceneView>()->renderID = offscreen->renderID;

        Mantrax::Timer gameTimer;
        bool running = true;

        std::cout << "\n=== ECS PBR MODEL VIEWER ===\n";
        std::cout << "Right Click + WASD - Move camera\n";
        std::cout << "Mouse Wheel - Zoom\n";
        std::cout << "ESC - Exit\n";
        std::cout << "===========================\n\n";

        try
        {
            LuaScript script("script.lua");

            // Variable global que Lua podrá leer
            script.setGlobal("x", 10);

            int resultado = script.callIntFunction("doblar", 5);
            std::cout << "Resultado desde Lua: " << resultado << "\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "Excepción: " << e.what() << "\n";
            return 1;
        }

        LuaEditor luae;

        // ==================== GAME LOOP ====================
        while (running)
        {
            if (!loader->window->ProcessMessages())
            {
                running = false;
                break;
            }

            gameTimer.Update();
            float delta = gameTimer.GetDeltaTime();
            int fps = gameTimer.GetFPS();

            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::Escape))
            {
                running = false;
                break;
            }

            // ✅ ACTUALIZAR SISTEMAS DEL ECS
            UpdateRotationSystem(registry, delta);
            UpdatePhysicsSystem(registry, delta);
            SyncRenderSystem(registry);

            // Procesar input de cámara
            ProcessCameraInput(camera, *g_InputSystem, delta);
            g_InputSystem->Update();

            // ✅ Orden correcto en el game loop
            if (renderView->CheckResize())
            {
                // 1. Resize del framebuffer
                loader->gfx->ResizeOffscreenFramebuffer(
                    offscreen,
                    static_cast<uint32_t>(renderView->width),
                    static_cast<uint32_t>(renderView->height));

                // 2. Actualizar descriptor de ImGui
                offscreen->renderID = ImGui_ImplVulkan_AddTexture(
                    offscreen->sampler,
                    offscreen->colorImageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                renderView->renderID = offscreen->renderID;

                // 3. ✅ ACTUALIZAR ASPECT RATIO INMEDIATAMENTE
                camera.SetAspectRatio(
                    static_cast<float>(renderView->width) /
                    static_cast<float>(renderView->height));

                // 4. Reset flag
                renderView->ResetResizeFlag();
            }

            // Actualizar y renderizar escena
            sceneRenderer->UpdateUBOs(&camera);
            sceneRenderer->RenderScene(offscreen);

            // Window resize handling
            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            // ImGui UI
            imgui.BeginFrame();

            luae.Render();

            ImGui::Begin("ECS Scene Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("FPS: %d | Delta: %.4f ms", fps, delta * 1000.0f);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("ECS Statistics", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Total Objects: %zu", renderObjects.size());

                // Contar entidades con cada componente
                int transformCount = 0;
                int rotatorCount = 0;
                int velocityCount = 0;

                for (auto [entity, transform] : registry.view<Transform>())
                    transformCount++;

                for (auto [entity, rotator] : registry.view<Rotator>())
                    rotatorCount++;

                for (auto [entity, velocity] : registry.view<Velocity>())
                    velocityCount++;

                ImGui::Text("Entities with Transform: %d", transformCount);
                ImGui::Text("Entities with Rotator: %d", rotatorCount);
                ImGui::Text("Entities with Velocity: %d", velocityCount);
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 pos = camera.GetPosition();
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

                float speed = camera.GetMovementSpeed();
                if (ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 20.0f))
                {
                    camera.SetMovementSpeed(speed);
                }

                if (ImGui::Button("Reset Camera", ImVec2(200, 0)))
                {
                    camera = Mantrax::FPSCamera(glm::vec3(0.0f, 5.0f, 20.0f), 60.0f, 0.1f, 1000.0f);
                    camera.SetAspectRatio(static_cast<float>(renderView->width) / static_cast<float>(renderView->height));
                    camera.SetMovementSpeed(5.0f);
                    camera.SetMouseSensitivity(0.1f);
                }
            }

            if (ImGui::CollapsingHeader("Performance"))
            {
                ImGui::Text("Frame Time: %.2f ms", delta * 1000.0f);
                ImGui::Text("FPS: %d", fps);
                ImGui::Text("Right Mouse: %s",
                            g_InputSystem->IsMouseButtonDown(Mantrax::MouseButton::Right) ? "DOWN" : "UP");
            }

            ImGui::End();

            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }
        // ==================== FIN GAME LOOP ====================

        // Cleanup
        g_SceneView = nullptr;
        g_Camera = nullptr;
        g_InputSystem = nullptr;

        delete renderView;

        std::cout << "\nShutting down cleanly...\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n!!! ERROR !!!\n"
                  << e.what() << "\n";
        MessageBoxA(nullptr, e.what(), "ERROR", MB_OK | MB_ICONERROR);
        return -1;
    }

    return 0;
}