#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "../../MantraxRender/include/MantraxGFX_Timer.h"
#include "../../MantraxRender/include/MainWinPlug.h"
#include "../../MantraxECS/include/SkyBox.h"
#include "../../MantraxECS/include/ServiceLocator.h"
#include "../../MantraxECS/include/InputSystem.h"
#include "../../MantraxECS/include/ModelManager.h"
#include "../../MantraxECS/include/SceneRenderer.h"
#include "../../MantraxECS/include/TextureLoader.h"
#include "../../MantraxECS/include/SceneManager.h"
#include "../../MantraxECS/include/Scene.h"
#include "../../MantraxECS/include/LuaScript.h"
#include "../../MantraxECS/include/MaterialManager.h"
#include "../../MantraxECS/include/FPSCamera.h"
#include "../includes/Components.h"
#include "../includes/LuaEditor.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../includes/ui/Inspector.h"
#include "../includes/ui/MenuBar.h"
#include "../includes/ImGuiManager.h"
#include <iostream>
#include <vector>

Mantrax::InputSystem *g_InputSystem = nullptr;
Mantrax::FPSCamera *g_Camera = nullptr;
SceneView *g_SceneView = nullptr;
bool g_SkyboxEnabled = true;

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
// SISTEMAS (ahora reciben Scene* en lugar de Registry&)
// ============================================================================

void UpdateRotationSystem(Scene *scene, float deltaTime)
{
    auto &world = scene->GetWorld();

    world.forEach<Transform, Rotator>([deltaTime](ecs::Entity entity, Transform &transform, Rotator &rotator)
                                      {
        // Rotar usando el eje y velocidad del rotator
        transform.RotateAxisAngle(rotator.axis, rotator.speed * deltaTime); });
}

void UpdatePhysicsSystem(Scene *scene, float deltaTime)
{
    auto &world = scene->GetWorld();

    world.forEach<Transform, Velocity>([deltaTime](ecs::Entity entity, Transform &transform, Velocity &velocity)
                                       {
        transform.Translate(velocity.linear * deltaTime);
        transform.RotateEuler(velocity.angular * deltaTime); });
}

void SyncRenderSystem(Scene *scene)
{
    auto &world = scene->GetWorld();

    world.forEach<Transform, RenderComponent>([](ecs::Entity entity, Transform &transform, RenderComponent &render)
                                              {
        transform.UpdateWorldMatrix();
        if (render.renderObject)
        {
            render.renderObject->modelMatrix = transform.GetWorldMatrix();
        } });
}

// Función helper para configurar vista isométrica
void SetupIsometricCamera(Mantrax::FPSCamera &camera, float distance = 15.0f)
{
    float yaw = -45.0f;
    float pitch = -35.264f;

    float radYaw = glm::radians(yaw);
    float radPitch = glm::radians(pitch);

    float x = distance * cos(radYaw) * cos(radPitch);
    float y = distance * sin(radPitch);
    float z = distance * sin(radYaw) * cos(radPitch);

    camera = Mantrax::FPSCamera(glm::vec3(x, y, z), 60.0f, 0.1f, 1000.0f);
    camera.SetProjectionMode(Mantrax::ProjectionMode::Orthographic);
    camera.SetOrthoSize(10.0f);
    camera.SetYaw(yaw);
    camera.SetPitch(pitch);
}

glm::vec3 TileToWorld(int tileX, int tileY, float tileSize = 1.0f)
{
    return glm::vec3(
        tileX * tileSize,
        0.0f,
        tileY * tileSize);
}

// ============================================================================
// FUNCIÓN PARA CREAR LA ESCENA INICIAL CON MATERIALMANAGER
// ============================================================================
void SetupGameScene(EngineLoader *loader, ModelManager *modelManager, SceneRenderer *sceneRenderer)
{
    Scene *scene = SceneManager::GetActiveScene();
    if (!scene)
    {
        throw std::runtime_error("No active scene!");
    }

    std::cout << "\n🎨 Initializing MaterialManager...\n";

    // ========================================================================
    // CREAR MATERIALMANAGER
    // ========================================================================
    ServiceLocator::instance().registerService("MaterialManager",
                                               std::make_shared<Mantrax::MaterialManager>(loader->gfx.get()));
    auto materialManager = ServiceLocator::instance().get<Mantrax::MaterialManager>("MaterialManager");

    std::cout << "✅ MaterialManager initialized!\n";
    std::cout << "\n🚀 Loading PBR textures...\n";

    // ========================================================================
    // CREAR MATERIAL DEL SUELO CON MATERIALMANAGER
    // ========================================================================
    Mantrax::MaterialProperties groundProps;
    groundProps.baseColor = glm::vec3(1.0f);
    groundProps.metallicFactor = 0.1f;
    groundProps.roughnessFactor = 0.8f;
    groundProps.normalScale = 1.0f;

    auto groundMaterial = materialManager->CreatePBRMaterialFromFiles(
        "GroundMaterial",
        loader->normalShader,
        "Ground A2_E.png",
        "",
        "",
        "",
        "",
        groundProps);

    std::cout << "✅ PBR textures loaded successfully!\n";
    std::cout << "\n🚀 Creating isometric ground plane...\n";

    // ========================================================================
    // CREAR GROUND PLANE CON EL MATERIAL
    // ========================================================================
    auto groundPlane = modelManager->CreateModelOnly("Plane.fbx", "GroundPlane");

    if (!groundPlane)
    {
        throw std::runtime_error("Failed to create ground plane");
    }

    // ✅ CAMBIO CRÍTICO: Asignar material Y crear descriptor set
    groundPlane->material = groundMaterial;
    groundPlane->renderObj.material = groundMaterial;

    // ✅ NUEVO: Crear descriptor set para el mesh con el material
    if (groundPlane->renderObj.mesh && groundMaterial)
    {
        loader->gfx->CreateMeshDescriptorSet(groundPlane->renderObj.mesh, groundMaterial);
        std::cout << "✅ Descriptor set created for ground plane\n";
    }

    sceneRenderer->AddObject(groundPlane);

    // Crear EntityObject
    EntityObject groundEntity = scene->CreateEntityObject("GroundPlane");

    Transform &groundTransform = groundEntity.AddComponent<Transform>();
    groundTransform.SetPosition(0.0f, 0.0f, 0.0f);
    groundTransform.SetRotationFromEulerDegrees(0.0f, 0.0f, 0.0f);
    groundTransform.SetScale(20.0f, 1.0f, 20.0f);

    RenderComponent &groundRender = groundEntity.AddComponent<RenderComponent>();
    groundRender.renderObject = groundPlane;

    std::cout << "✅ Isometric ground plane created!\n";

    // ========================================================================
    // CREAR MATERIALES ADICIONALES DE EJEMPLO
    // ========================================================================

    // Material metálico dorado
    auto goldMaterial = materialManager->CreateSolidColorMaterial(
        "Gold",
        loader->normalShader,
        glm::vec3(1.0f, 0.84f, 0.0f),
        0.95f,
        0.2f);

    // Material de plástico rojo
    auto redPlasticMaterial = materialManager->CreateSolidColorMaterial(
        "RedPlastic",
        loader->normalShader,
        glm::vec3(0.8f, 0.1f, 0.1f),
        0.0f,
        0.6f);

    // Material de plástico verde
    auto greenPlasticMaterial = materialManager->CreateSolidColorMaterial(
        "GreenPlastic",
        loader->normalShader,
        glm::vec3(0.1f, 0.8f, 0.1f),
        0.0f,
        0.5f);

    std::cout << "✅ Additional materials created!\n";

    // ========================================================================
    // SKYBOX
    // ========================================================================
    Mantrax::SkyBox skybox(loader->gfx.get(), 1.0f, 32, 16);

    auto skyboxTextureData = Mantrax::TextureLoader::LoadFromFile(
        "textures/skybox.jpg",
        true);
    auto skyboxTexture = loader->gfx->CreateTexture(
        skyboxTextureData->pixels,
        skyboxTextureData->width,
        skyboxTextureData->height);

    auto skyboxMaterial = loader->gfx->CreateMaterial(loader->skyboxShader);
    skyboxMaterial->SetBaseColor(2.0f, 2.0f, 2.0f);

    // ✅ CAMBIO: Ya no usar SetMaterialPBRTextures, usar el nuevo flujo
    skyboxMaterial->SetAlbedoTexture(skyboxTexture);

    auto skyboxRenderObj = skybox.GetRenderObject();
    skyboxRenderObj.material = skyboxMaterial;

    // ✅ NUEVO: Crear descriptor set para skybox
    if (skyboxRenderObj.mesh && skyboxMaterial)
    {
        loader->gfx->CreateMeshDescriptorSet(skyboxRenderObj.mesh, skyboxMaterial);
        std::cout << "✅ Descriptor set created for skybox\n";
    }

    // Crear EntityObject para el skybox
    EntityObject skyboxEntity = scene->CreateEntityObject("Skybox");

    Transform &skyboxTransform = skyboxEntity.AddComponent<Transform>();
    skyboxTransform.SetPosition(0.0f, 0.0f, 0.0f);
    skyboxTransform.SetRotationFromEulerDegrees(0.0f, 0.0f, 0.0f);
    skyboxTransform.SetScale(1.0f, 1.0f, 1.0f);

    RenderComponent &skyboxRender = skyboxEntity.AddComponent<RenderComponent>();

    // Crear RenderableObject para skybox
    RenderableObject *skyboxObj = new RenderableObject();
    skyboxObj->renderObj = skyboxRenderObj;
    skyboxObj->material = skyboxMaterial;
    skyboxObj->name = "Skybox";
    skyboxObj->modelMatrix = glm::mat4(1.0f);

    skyboxRender.renderObject = skyboxObj;
    sceneRenderer->AddObject(skyboxObj);

    // ========================================================================
    // LISTAR TODOS LOS MATERIALES CREADOS
    // ========================================================================
    auto allMaterials = materialManager->GetAllMaterialNames();
    std::cout << "\n📋 Materials loaded (" << allMaterials.size() << "):\n";
    for (const auto &matName : allMaterials)
    {
        std::cout << "  - " << matName << "\n";
    }

    std::cout << "\n✅ Scene setup complete!\n";
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
        // ====================================================================
        // INICIALIZACIÓN DE SERVICIOS
        // ====================================================================
        ServiceLocator::instance().registerService("EngineLoader", std::make_shared<EngineLoader>());
        auto loader = ServiceLocator::instance().get<EngineLoader>("EngineLoader");
        loader->Start(hInst, CustomWndProc);

        Mantrax::ImGuiManager imgui(loader->window->GetHWND(), loader->gfx.get());

        ServiceLocator::instance().registerService("InputSystem", std::make_shared<Mantrax::InputSystem>());
        auto inputLoader = ServiceLocator::instance().get<Mantrax::InputSystem>("InputSystem");
        g_InputSystem = inputLoader.get();

        ServiceLocator::instance().registerService("SceneRenderer", std::make_shared<SceneRenderer>(loader->gfx.get()));
        auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");

        ServiceLocator::instance().registerService("ModelManager", std::make_shared<ModelManager>(loader->gfx.get()));
        std::shared_ptr<ModelManager> modelManager = ServiceLocator::instance().get<ModelManager>("ModelManager");

        // ====================================================================
        // CREAR ESCENA CON EL NUEVO SISTEMA
        // ====================================================================
        SceneManager::CreateScene("MainScene");
        Scene *activeScene = SceneManager::GetActiveScene();

        if (!activeScene)
        {
            throw std::runtime_error("Failed to create scene!");
        }

        std::cout << "✅ Scene created: " << activeScene->GetName() << "\n";

        // Setup de la escena (incluye MaterialManager)
        SetupGameScene(loader.get(), modelManager.get(), sceneRenderer.get());

        // Obtener MaterialManager para uso posterior
        auto materialManager = ServiceLocator::instance().get<Mantrax::MaterialManager>("MaterialManager");

        // ====================================================================
        // CONFIGURAR CÁMARA
        // ====================================================================
        Mantrax::FPSCamera camera;
        SetupIsometricCamera(camera, 20.0f);
        camera.SetAspectRatio(1540.0f / 948.0f);
        camera.SetMovementSpeed(5.0f);
        camera.SetMouseSensitivity(0.1f);
        g_Camera = &camera;

        sceneRenderer->camera = &camera;
        // ====================================================================
        // UI SETUP
        // ====================================================================
        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);
        ServiceLocator::instance().registerService("UIRender", std::make_shared<UIRender>());
        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        g_SceneView = renderView;
        uiRender->Set(renderView);
        uiRender->Set(new MenuBar());

        uiRender->Set(new Inspector());
        uiRender->GetByType<SceneView>()->renderID = offscreen->renderID;

        Mantrax::Timer gameTimer;
        bool running = true;

        std::cout << "\n=== ISOMETRIC GAME ENGINE ===\n";
        std::cout << "Right Click + WASD - Pan camera\n";
        std::cout << "Shift - Fast pan\n";
        std::cout << "Mouse Wheel - Zoom in/out\n";
        std::cout << "P - Toggle Perspective/Orthographic\n";
        std::cout << "M - Print material info\n";
        std::cout << "ESC - Exit\n";
        std::cout << "============================\n\n";

        // ====================================================================
        // LUA SCRIPT TEST
        // ====================================================================
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

        // ====================================================================
        // GAME LOOP
        // ====================================================================
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

            // Input
            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::Escape))
            {
                running = false;
                break;
            }

            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::P))
            {
                camera.ToggleProjectionMode();
                std::cout << "Projection: "
                          << (camera.IsPerspective() ? "Perspective" : "Orthographic")
                          << "\n";
            }

            // Tecla M para imprimir información de materiales
            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::M))
            {
                std::cout << "\n📋 Material Information:\n";
                auto allMats = materialManager->GetAllMaterialNames();
                for (const auto &matName : allMats)
                {
                    auto props = materialManager->GetMaterialProperties(matName);
                    std::cout << "  🎨 " << matName << ":\n";
                    std::cout << "    Color: (" << props.baseColor.r << ", "
                              << props.baseColor.g << ", " << props.baseColor.b << ")\n";
                    std::cout << "    Metallic: " << props.metallicFactor << "\n";
                    std::cout << "    Roughness: " << props.roughnessFactor << "\n";
                }
                std::cout << "\n";
            }

            // ================================================================
            // UPDATE SYSTEMS (ahora usan Scene*)
            // ================================================================
            activeScene->OnUpdate(delta);

            UpdateRotationSystem(activeScene, delta);
            UpdatePhysicsSystem(activeScene, delta);
            SyncRenderSystem(activeScene);

            ProcessCameraInput(camera, *g_InputSystem, delta);
            g_InputSystem->Update();

            // ================================================================
            // RENDER
            // ================================================================
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

            // ================================================================
            // UI RENDER
            // ================================================================
            imgui.BeginFrame();

            luae.Render();
            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }

        // ====================================================================
        // CLEANUP
        // ====================================================================
        g_SceneView = nullptr;
        g_Camera = nullptr;
        g_InputSystem = nullptr;

        delete renderView;

        SceneManager::DestroyActiveScene();

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