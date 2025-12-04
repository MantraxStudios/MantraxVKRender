#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxRender/include/MantraxGFX_Timer.h"
#include "../MantraxECS/include/SkyBox.h"
#include "../includes/FPSCamera.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../includes/ServiceLocator.h"
#include "../includes/ImGuiManager.h"
#include "../MantraxECS/include/InputSystem.h"
#include "../MantraxECS/include/ModelManager.h"
#include "../MantraxECS/include/SceneRenderer.h"
#include "../MantraxECS/include/TextureLoader.h"
#include <iostream>

// Variables globales (solo para acceso desde WndProc)
Mantrax::InputSystem *g_InputSystem = nullptr;
Mantrax::FPSCamera *g_Camera = nullptr;
SceneView *g_SceneView = nullptr;

LRESULT CustomWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // 1. Dejar que ImGui procese primero
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;

    // 2. Procesar en nuestro InputSystem
    if (g_InputSystem)
    {
        g_InputSystem->ProcessMessage(msg, wp, lp, hwnd);
    }

    // 3. ✅ CRÍTICO: Llamar a DefWindowProc para los mensajes que no manejamos
    // Esto le dice a Windows que el mensaje fue procesado correctamente
    return DefWindowProc(hwnd, msg, wp, lp);
}

void ProcessCameraInput(Mantrax::FPSCamera &camera, Mantrax::InputSystem &input, float deltaTime)
{
    // Solo procesar input si el botón derecho está presionado
    if (!input.IsMouseButtonDown(Mantrax::KeyCode::RightMouse))
        return;

    bool sprint = input.IsKeyDown(Mantrax::KeyCode::Shift);

    // Movimiento con teclado
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

    // Movimiento del mouse
    POINT delta = input.GetMouseDelta();
    if (delta.x != 0 || delta.y != 0)
    {
        camera.ProcessMouseMovement(
            static_cast<float>(delta.x),
            static_cast<float>(delta.y));
    }

    // Scroll del mouse
    float wheelDelta = input.GetMouseWheelDelta();
    if (wheelDelta != 0.0f)
    {
        camera.ProcessMouseScroll(wheelDelta * 2.0f);
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

        // Crear sistema de input
        Mantrax::InputSystem inputSystem;
        g_InputSystem = &inputSystem;

        // Crear managers
        ModelManager modelManager(loader->gfx.get());
        SceneRenderer sceneRenderer(loader->gfx.get());

        // Cargar modelo con PBR
        auto cubeObj = modelManager.CreateModelWithPBR(
            "Cube.fbx",
            "MainCube",
            "textures/DiamondPlate008A_1K-PNG_Color.png",
            "textures/DiamondPlate008A_1K-PNG_NormalGL.png",
            "textures/DiamondPlate008A_1K-PNG_Metalness.png",
            "textures/DiamondPlate008A_1K-PNG_Roughness.png",
            "textures/DiamondPlate008A_1K-PNG_AmbientOcclusion.png",
            loader->normalShader);

        if (!cubeObj)
        {
            std::cerr << "Failed to load cube model!" << std::endl;
            return -1;
        }

        cubeObj->scale = glm::vec3(2.0f);
        sceneRenderer.AddObject(cubeObj);

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

        sceneRenderer.AddObject(&skyboxObj);

        // Configurar cámara
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 0.0f, 5.0f), 60.0f, 0.1f, 1000.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
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
        float objectRotation = 0.0f;

        std::cout << "\n=== PBR MODEL VIEWER ===\n";
        std::cout << "Right Click + WASD - Move camera\n";
        std::cout << "Mouse Wheel - Zoom\n";
        std::cout << "ESC - Exit\n";
        std::cout << "========================\n\n";

        // ==================== GAME LOOP OPTIMIZADO ====================
        while (running)
        {
            // 1. Procesar TODOS los mensajes de Windows disponibles
            // Esto acumula eventos en el InputSystem
            if (!loader->window->ProcessMessages())
            {
                running = false;
                break;
            }

            // 2. Actualizar timer
            gameTimer.Update();
            float delta = gameTimer.GetDeltaTime();
            int fps = gameTimer.GetFPS();

            // 3. ✅ USAR LOS INPUTS PRIMERO (antes de Update)
            // Los deltas están acumulados desde ProcessMessages

            if (inputSystem.IsKeyPressed(Mantrax::KeyCode::Escape))
            {
                running = false;
                break;
            }

            // Rotar objeto
            objectRotation += delta * 1.0f;
            cubeObj->rotation.y = objectRotation;

            // Procesar input de cámara (USA deltas acumulados)
            ProcessCameraInput(camera, inputSystem, delta);

            // 4. ✅ DESPUÉS actualizar InputSystem
            // Esto actualiza previous state Y resetea deltas
            inputSystem.Update();

            // 5. Resize handling
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
                camera.SetAspectRatio(static_cast<float>(renderView->width) / static_cast<float>(renderView->height));
                renderView->ResetResizeFlag();
            }

            // 6. Actualizar y renderizar escena
            sceneRenderer.UpdateUBOs(&camera);
            sceneRenderer.RenderScene(offscreen);

            // 7. Window resize handling
            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            // 8. ImGui UI
            imgui.BeginFrame();

            ImGui::Begin("PBR Material Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("FPS: %d | Delta: %.4f ms", fps, delta * 1000.0f);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 pos = camera.GetPosition();
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
                ImGui::Text("Object Rotation: %.1f°", objectRotation);

                float speed = camera.GetMovementSpeed();
                if (ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 20.0f))
                {
                    camera.SetMovementSpeed(speed);
                }

                if (ImGui::Button("Reset Camera", ImVec2(200, 0)))
                {
                    camera = Mantrax::FPSCamera(glm::vec3(0.0f, 0.0f, 5.0f), 60.0f, 0.1f, 1000.0f);
                    camera.SetAspectRatio(static_cast<float>(renderView->width) / static_cast<float>(renderView->height));
                    camera.SetMovementSpeed(5.0f);
                    camera.SetMouseSensitivity(0.1f);
                }
            }

            if (ImGui::CollapsingHeader("Performance"))
            {
                ImGui::Text("Frame Time: %.2f ms", delta * 1000.0f);
                ImGui::Text("FPS: %d", fps);

                // NOTA: Estos deltas YA fueron reseteados por Update()
                // Para debug, mover esta sección ANTES de Update()
                ImGui::Text("Right Mouse: %s",
                            inputSystem.IsMouseButtonDown(Mantrax::KeyCode::RightMouse) ? "DOWN" : "UP");
            }

            ImGui::End();

            uiRender->RenderAll();
            ImGui::Render();

            // 9. Draw final frame
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