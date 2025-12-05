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

void UpdateRotationSystem(ecs::Registry &registry, float deltaTime)
{
    for (auto [entity, transform, rotator] : registry.view<Transform, Rotator>())
    {
        transform.rotation += rotator.axis * rotator.speed * deltaTime;
    }
}

void UpdatePhysicsSystem(ecs::Registry &registry, float deltaTime)
{
    for (auto [entity, transform, velocity] : registry.view<Transform, Velocity>())
    {
        transform.position += velocity.linear * deltaTime;
        transform.rotation += velocity.angular * deltaTime;
    }
}

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
        std::vector<std::shared_ptr<Mantrax::Material>> individualMaterials; // ✅ Para mantener los materiales vivos

        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> heightDist(0.0f, 30.0f);
        std::uniform_real_distribution<float> speedDist(0.3f, 1.5f);
        std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);

        std::cout << "\n🚀 Loading SHARED PBR textures...\n";

        // ============================================================================
        // ✅ CARGAR TEXTURAS PBR UNA SOLA VEZ (COMPARTIDAS)
        // ============================================================================
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char *colorData = stbi_load("textures/DiamondPlate008A_1K-PNG_Color.png",
                                             &width, &height, &channels, STBI_rgb_alpha);
        auto colorTexture = loader->gfx->CreateTexture(colorData, width, height);
        stbi_image_free(colorData);

        unsigned char *normalData = stbi_load("textures/DiamondPlate008A_1K-PNG_NormalGL.png",
                                              &width, &height, &channels, STBI_rgb_alpha);
        auto normalTexture = loader->gfx->CreateTexture(normalData, width, height);
        stbi_image_free(normalData);

        unsigned char *metallicData = stbi_load("textures/DiamondPlate008A_1K-PNG_Metalness.png",
                                                &width, &height, &channels, STBI_rgb_alpha);
        auto metallicTexture = loader->gfx->CreateTexture(metallicData, width, height);
        stbi_image_free(metallicData);

        unsigned char *roughnessData = stbi_load("textures/DiamondPlate008A_1K-PNG_Roughness.png",
                                                 &width, &height, &channels, STBI_rgb_alpha);
        auto roughnessTexture = loader->gfx->CreateTexture(roughnessData, width, height);
        stbi_image_free(roughnessData);

        unsigned char *aoData = stbi_load("textures/DiamondPlate008A_1K-PNG_AmbientOcclusion.png",
                                          &width, &height, &channels, STBI_rgb_alpha);
        auto aoTexture = loader->gfx->CreateTexture(aoData, width, height);
        stbi_image_free(aoData);

        std::cout << "✅ Shared PBR textures loaded successfully!\n";
        std::cout << "\n🚀 Creating 300 objects with SHARED textures...\n";

        auto startTime = std::chrono::high_resolution_clock::now();

        const int OBJECT_COUNT = 300;

        for (int i = 0; i < OBJECT_COUNT; i++)
        {
            if (i % 50 == 0)
            {
                std::cout << "  Progress: " << i << "/" << OBJECT_COUNT << " objects created...\n";
            }

            // ✅ Crear el modelo con geometría
            auto cubeObj = modelManager.CreateModelOnly(
                "Cube.fbx",
                "Cube_" + std::to_string(i));

            if (!cubeObj)
            {
                std::cerr << "❌ Failed to create object " << i << std::endl;
                continue;
            }

            // ============================================================================
            // ✅ CREAR MATERIAL INDIVIDUAL PARA CADA OBJETO (con su propio UBO)
            // Pero compartiendo el SHADER y las TEXTURAS
            // ============================================================================
            auto individualMaterial = loader->gfx->CreateMaterial(loader->normalShader);
            individualMaterial->SetMetallicFactor(1.0f);
            individualMaterial->SetRoughnessFactor(2.0f);
            individualMaterial->SetNormalScale(5.0f);

            // ✅ Asignar las TEXTURAS COMPARTIDAS al material individual
            loader->gfx->SetMaterialPBRTextures(
                individualMaterial,
                colorTexture,     // ✅ Textura compartida
                normalTexture,    // ✅ Textura compartida
                metallicTexture,  // ✅ Textura compartida
                roughnessTexture, // ✅ Textura compartida
                aoTexture         // ✅ Textura compartida
            );

            // ✅ Asignar el material individual al objeto
            cubeObj->material = individualMaterial;
            cubeObj->renderObj.material = individualMaterial;

            // ✅ Guardar referencia al material para que no se destruya
            individualMaterials.push_back(individualMaterial);

            renderObjects.push_back(cubeObj);
            sceneRenderer->AddObject(cubeObj);

            // Crear entidad en el ECS
            auto entity = registry.createEntity();

            Transform &transform = registry.addComponent<Transform>(entity);

            float angle = (i * 137.5f) * (3.14159f / 180.0f);
            float radius = sqrtf(static_cast<float>(i)) * 3.0f;

            transform.position = glm::vec3(
                cos(angle) * radius + posDist(gen) * 0.1f,
                heightDist(gen),
                sin(angle) * radius + posDist(gen) * 0.1f);

            transform.scale = glm::vec3(scaleDist(gen));

            Rotator &rotator = registry.addComponent<Rotator>(entity);
            rotator.speed = speedDist(gen);
            rotator.axis = glm::normalize(glm::vec3(
                posDist(gen),
                posDist(gen),
                posDist(gen)));

            RenderComponent &render = registry.addComponent<RenderComponent>(entity);
            render.renderObject = cubeObj;

            if (i % 5 == 0)
            {
                Velocity &velocity = registry.addComponent<Velocity>(entity);
                velocity.linear = glm::vec3(
                    posDist(gen) * 0.2f,
                    posDist(gen) * 0.1f,
                    posDist(gen) * 0.2f);
                velocity.angular = glm::vec3(0.0f, speedDist(gen) * 0.5f, 0.0f);
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "\n✅ Created " << renderObjects.size() << " objects in " << duration.count() << "ms\n";
        std::cout << "💡 Optimization: 1 shader + 5 shared textures for all " << OBJECT_COUNT << " objects!\n";
        std::cout << "   (Each object has its own UBO for transforms)\n";

        // Crear Skybox
        Mantrax::SkyBox skybox(loader->gfx.get(), 1.0f, 32, 16);

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

        Mantrax::FPSCamera camera(glm::vec3(0.0f, 30.0f, 80.0f), 60.0f, 0.1f, 1000.0f);
        camera.SetAspectRatio(1540.0f / 948.0f);
        camera.SetMovementSpeed(15.0f);
        camera.SetMouseSensitivity(0.1f);
        g_Camera = &camera;

        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);
        ServiceLocator::instance().registerService("UIRender", std::make_shared<UIRender>());
        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        g_SceneView = renderView;
        uiRender->Set(renderView);
        uiRender->GetByType<SceneView>()->renderID = offscreen->renderID;

        Mantrax::Timer gameTimer;
        bool running = true;

        std::cout << "\n=== OPTIMIZED ECS VIEWER - 300 OBJECTS ===\n";
        std::cout << "Optimization: Shared shader + textures\n";
        std::cout << "Right Click + WASD - Move camera\n";
        std::cout << "Shift - Sprint\n";
        std::cout << "Space/Ctrl - Up/Down\n";
        std::cout << "Mouse Wheel - Zoom\n";
        std::cout << "ESC - Exit\n";
        std::cout << "=========================================\n\n";

        try
        {
            LuaScript script("script.lua");
            script.setGlobal("x", 10);
            int resultado = script.callIntFunction("doblar", 5);
            std::cout << "Lua result: " << resultado << "\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "Lua error: " << e.what() << "\n";
        }

        LuaEditor luae;

        float frameTimeAccumulator = 0.0f;
        int frameCount = 0;
        float avgFrameTime = 0.0f;
        int minFPS = 9999;
        int maxFPS = 0;

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

            frameTimeAccumulator += delta;
            frameCount++;
            if (frameCount >= 60)
            {
                avgFrameTime = frameTimeAccumulator / frameCount;
                frameTimeAccumulator = 0.0f;
                frameCount = 0;
            }
            minFPS = std::min(minFPS, fps);
            maxFPS = std::max(maxFPS, fps);

            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::Escape))
            {
                running = false;
                break;
            }

            UpdateRotationSystem(registry, delta);
            UpdatePhysicsSystem(registry, delta);
            SyncRenderSystem(registry);

            ProcessCameraInput(camera, *g_InputSystem, delta);
            g_InputSystem->Update();

            if (renderView->CheckResize())
            {
                loader->gfx->ResizeOffscreenFramebuffer(
                    offscreen,
                    static_cast<uint32_t>(renderView->width),
                    static_cast<uint32_t>(renderView->height));

                offscreen->renderID = ImGui_ImplVulkan_AddTexture(
                    offscreen->sampler,
                    offscreen->colorImageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                renderView->renderID = offscreen->renderID;

                camera.SetAspectRatio(
                    static_cast<float>(renderView->width) /
                    static_cast<float>(renderView->height));

                renderView->ResetResizeFlag();
            }

            sceneRenderer->UpdateUBOs(&camera);
            sceneRenderer->RenderScene(offscreen);

            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            imgui.BeginFrame();

            luae.Render();

            ImGui::Begin("ECS Scene - OPTIMIZED", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "PERFORMANCE");
            ImGui::Separator();
            ImGui::Text("FPS: %d", fps);
            ImGui::Text("Frame Time: %.2f ms", delta * 1000.0f);
            ImGui::Text("Avg Frame Time: %.2f ms", avgFrameTime * 1000.0f);
            ImGui::Text("Min/Max FPS: %d / %d", minFPS, maxFPS);

            float frameTimeBudget = 16.67f;
            float frameTimePercentage = (delta * 1000.0f) / frameTimeBudget;
            ImGui::ProgressBar(frameTimePercentage, ImVec2(0.0f, 0.0f), "");
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("Frame Budget (16.67ms)");

            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Optimization Stats", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "MEMORY OPTIMIZATION");
                ImGui::Text("Total Objects: %d", OBJECT_COUNT);
                ImGui::Text("Shaders: 1 (SHARED)");
                ImGui::Text("Texture Sets: 1 (SHARED)");
                ImGui::Text("  - 5 PBR textures shared by all objects");
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                                   "Memory saved: ~%d texture copies!", (OBJECT_COUNT - 1) * 5);
                ImGui::Separator();
                ImGui::Text("Render Objects: %zu", renderObjects.size());

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
                ImGui::Text("Entities with Velocity: %d (%.1f%%)",
                            velocityCount, (velocityCount * 100.0f) / OBJECT_COUNT);
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 pos = camera.GetPosition();
                ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                ImGui::Text("Yaw: %.1f | Pitch: %.1f", camera.GetYaw(), camera.GetPitch());

                float speed = camera.GetMovementSpeed();
                if (ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 50.0f))
                {
                    camera.SetMovementSpeed(speed);
                }

                float fov = camera.GetFOV();
                if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
                {
                    camera.SetFOV(fov);
                }

                if (ImGui::Button("Reset Camera", ImVec2(200, 0)))
                {
                    camera = Mantrax::FPSCamera(glm::vec3(0.0f, 30.0f, 80.0f), 60.0f, 0.1f, 1000.0f);
                    camera.SetAspectRatio(static_cast<float>(renderView->width) / static_cast<float>(renderView->height));
                    camera.SetMovementSpeed(15.0f);
                    camera.SetMouseSensitivity(0.1f);
                }
            }

            if (ImGui::CollapsingHeader("Input"))
            {
                ImGui::Text("Right Mouse: %s",
                            g_InputSystem->IsMouseButtonDown(Mantrax::MouseButton::Right) ? "DOWN" : "UP");
                ImGui::Text("Sprint (Shift): %s",
                            g_InputSystem->IsKeyDown(Mantrax::KeyCode::Shift) ? "ACTIVE" : "INACTIVE");
            }

            ImGui::End();

            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }
        // ==================== FIN GAME LOOP ====================

        std::cout << "\n📊 Final Statistics:\n";
        std::cout << "  Total Objects Rendered: " << renderObjects.size() << "\n";
        std::cout << "  Shaders Used: 1 (SHARED)\n";
        std::cout << "  Texture Sets: 1 (5 PBR textures SHARED)\n";
        std::cout << "  Min FPS: " << minFPS << "\n";
        std::cout << "  Max FPS: " << maxFPS << "\n";

        g_SceneView = nullptr;
        g_Camera = nullptr;
        g_InputSystem = nullptr;

        delete renderView;

        std::cout << "\n✅ Shutting down cleanly...\n";
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