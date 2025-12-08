#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "../../MantraxRender/include/MantraxGFX_Timer.h"
#include "../../MantraxRender/include/MainWinPlug.h"
#include "../../MantraxECS/include/SkyBox.h"
#include "../../MantraxECS/include/ServiceLocator.h"
#include "../../MantraxECS/include/InputSystem.h"
#include "../../MantraxECS/include/ModelManager.h"
#include "../../MantraxECS/include/SceneRenderer.h"
#include "../../MantraxECS/include/TextureLoader.h"
#include "../../MantraxECS/include/ecs.h"
#include "../../MantraxECS/include/LuaScript.h"
#include "../includes/LuaEditor.h"
#include "../includes/FPSCamera.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../includes/ImGuiManager.h"
#include <iostream>
#include <vector>

// Removed: #define STB_IMAGE_IMPLEMENTATION
// Removed: #include "../../MantraxECS/include/stb_image.h"

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

struct ObjectInfo
{
    std::string name;
    ecs::Entity entity;
    RenderableObject *renderObject;
    bool isActive = true;
};

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

        // Vector para gestionar objetos
        std::vector<ObjectInfo> sceneObjects;
        int objectCounter = 0;

        std::cout << "\n🚀 Loading PBR textures...\n";

        // Usar TextureLoader en lugar de stbi_load directamente
        auto colorTextureData = Mantrax::TextureLoader::LoadFromFile(
            "Ground A2_E.png");
        auto colorTexture = loader->gfx->CreateTexture(
            colorTextureData->pixels,
            colorTextureData->width,
            colorTextureData->height);

        std::cout << "✅ PBR textures loaded successfully!\n";
        std::cout << "\n🚀 Creating initial object...\n";

        auto cubeObj = modelManager.CreateModelOnly("Cube.fbx", "InitialCube");

        if (!cubeObj)
        {
            throw std::runtime_error("Failed to create cube object");
        }

        auto cubeMaterial = loader->gfx->CreateMaterial(loader->normalShader);
        cubeMaterial->SetMetallicFactor(1.0f);
        cubeMaterial->SetRoughnessFactor(2.0f);
        cubeMaterial->SetNormalScale(5.0f);

        loader->gfx->SetMaterialPBRTextures(
            cubeMaterial,
            colorTexture,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        cubeObj->material = cubeMaterial;
        cubeObj->renderObj.material = cubeMaterial;

        sceneRenderer->AddObject(cubeObj);

        auto entity = registry.createEntity();

        Transform &transform = registry.addComponent<Transform>(entity);
        transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        transform.scale = glm::vec3(1.0f);

        Rotator &rotator = registry.addComponent<Rotator>(entity);
        rotator.speed = 1.0f;
        rotator.axis = glm::vec3(0.0f, 1.0f, 0.0f);

        RenderComponent &render = registry.addComponent<RenderComponent>(entity);
        render.renderObject = cubeObj;

        // Agregar al vector de objetos
        ObjectInfo initialObj;
        initialObj.name = "InitialCube";
        initialObj.entity = entity;
        initialObj.renderObject = cubeObj;
        initialObj.isActive = true;
        sceneObjects.push_back(initialObj);

        std::cout << "✅ Initial object created successfully!\n";

        Mantrax::SkyBox skybox(loader->gfx.get(), 1.0f, 32, 16);

        // Usar TextureLoader para el skybox
        auto skyboxTextureData = Mantrax::TextureLoader::LoadFromFile(
            "textures/skybox.jpg",
            true);
        auto skyboxTexture = loader->gfx->CreateTexture(
            skyboxTextureData->pixels,
            skyboxTextureData->width,
            skyboxTextureData->height);

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

        Mantrax::FPSCamera camera(glm::vec3(0.0f, 0.0f, 15.0f), 60.0f, 0.1f, 1000.0f);
        camera.SetAspectRatio(1540.0f / 948.0f);
        camera.SetMovementSpeed(5.0f);
        camera.SetMouseSensitivity(0.1f);
        camera.SetProjectionMode(Mantrax::ProjectionMode::Orthographic);
        camera.SetOrthoSize(10.0f);
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

        std::cout << "\n=== OBJECT MANAGER VIEWER ===\n";
        std::cout << "Right Click + WASD - Move camera\n";
        std::cout << "Shift - Sprint\n";
        std::cout << "Space/Ctrl - Up/Down\n";
        std::cout << "Mouse Wheel - Zoom\n";
        std::cout << "ESC - Exit\n";
        std::cout << "============================\n\n";

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

        // Variables para la ventana de Object Manager
        char modelPathBuffer[256] = "Cube.fbx";
        char nameBuffer[128] = "";
        int selectedObjectIndex = -1;

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

            if (g_InputSystem->IsKeyPressed(Mantrax::KeyCode::P))
            {
                camera.ToggleProjectionMode();
                std::cout << "Projection: "
                          << (camera.IsPerspective() ? "Perspective" : "Orthographic")
                          << "\n";
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

            // ============ VENTANA DE OBJECT MANAGER ============
            ImGui::Begin("Object Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "ADD NEW OBJECT");
            ImGui::Separator();

            ImGui::InputText("Model Path", modelPathBuffer, 256);
            ImGui::InputText("Object Name", nameBuffer, 128);

            if (ImGui::Button("Add Object", ImVec2(200, 30)))
            {
                try
                {
                    std::string objectName = strlen(nameBuffer) > 0 ? nameBuffer : ("Object_" + std::to_string(objectCounter++));

                    auto newObj = modelManager.CreateModelOnly(modelPathBuffer, objectName);
                    if (newObj)
                    {
                        auto newMaterial = loader->gfx->CreateMaterial(loader->normalShader);
                        newMaterial->SetMetallicFactor(1.0f);
                        newMaterial->SetRoughnessFactor(2.0f);
                        newMaterial->SetNormalScale(5.0f);

                        loader->gfx->SetMaterialPBRTextures(
                            newMaterial,
                            colorTexture,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr);

                        newObj->material = newMaterial;
                        newObj->renderObj.material = newMaterial;

                        sceneRenderer->AddObject(newObj);

                        auto newEntity = registry.createEntity();
                        Transform &newTransform = registry.addComponent<Transform>(newEntity);
                        newTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
                        newTransform.scale = glm::vec3(1.0f);

                        Rotator &newRotator = registry.addComponent<Rotator>(newEntity);
                        newRotator.speed = 1.0f;
                        newRotator.axis = glm::vec3(0.0f, 1.0f, 0.0f);

                        RenderComponent &newRender = registry.addComponent<RenderComponent>(newEntity);
                        newRender.renderObject = newObj;

                        ObjectInfo info;
                        info.name = objectName;
                        info.entity = newEntity;
                        info.renderObject = newObj;
                        info.isActive = true;
                        sceneObjects.push_back(info);

                        std::cout << "✅ Object added: " << objectName << "\n";

                        // Limpiar buffers
                        nameBuffer[0] = '\0';
                    }
                    else
                    {
                        std::cerr << "❌ Failed to create object from: " << modelPathBuffer << "\n";
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "❌ Error adding object: " << e.what() << "\n";
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "SCENE OBJECTS");
            ImGui::Separator();
            ImGui::Text("Total Objects: %zu", sceneObjects.size());

            ImGui::Spacing();

            // Lista de objetos
            for (size_t i = 0; i < sceneObjects.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));

                bool isSelected = (selectedObjectIndex == static_cast<int>(i));
                if (ImGui::Selectable(sceneObjects[i].name.c_str(), isSelected))
                {
                    selectedObjectIndex = static_cast<int>(i);
                }

                ImGui::SameLine(250);

                // Toggle active/inactive
                bool active = sceneObjects[i].isActive;
                if (ImGui::Checkbox("##active", &active))
                {
                    sceneObjects[i].isActive = active;
                    if (active)
                    {
                        sceneRenderer->AddObject(sceneObjects[i].renderObject);
                        std::cout << "✅ Object activated: " << sceneObjects[i].name << "\n";
                    }
                    else
                    {
                        sceneRenderer->RemoveObject(sceneObjects[i].renderObject);
                        std::cout << "⏸ Object deactivated: " << sceneObjects[i].name << "\n";
                    }
                }

                ImGui::SameLine();

                // Botón eliminar
                if (ImGui::Button("Delete"))
                {
                    sceneRenderer->RemoveObject(sceneObjects[i].renderObject);
                    registry.destroyEntity(sceneObjects[i].entity);

                    std::cout << "🗑️ Object deleted: " << sceneObjects[i].name << "\n";

                    sceneObjects.erase(sceneObjects.begin() + i);
                    if (selectedObjectIndex == static_cast<int>(i))
                    {
                        selectedObjectIndex = -1;
                    }
                    else if (selectedObjectIndex > static_cast<int>(i))
                    {
                        selectedObjectIndex--;
                    }

                    ImGui::PopID();
                    break;
                }

                ImGui::PopID();
            }

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::Button("Clear All Objects", ImVec2(200, 30)))
            {
                for (auto &obj : sceneObjects)
                {
                    sceneRenderer->RemoveObject(obj.renderObject);
                    registry.destroyEntity(obj.entity);
                }
                sceneObjects.clear();
                selectedObjectIndex = -1;
                std::cout << "🗑️ All objects cleared!\n";
            }

            ImGui::End();

            // ============ VENTANA DE SCENE INFO ============
            ImGui::Begin("Scene Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "PERFORMANCE");
            ImGui::Separator();
            ImGui::Text("FPS: %d", fps);
            ImGui::Text("Frame Time: %.2f ms", delta * 1000.0f);

            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Scene Info", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Total Objects: %zu", sceneObjects.size());
                ImGui::Text("Shaders: 1");
                ImGui::Text("PBR Textures: 5");

                ImGui::Spacing();
                ImGui::Separator();

                if (ImGui::Checkbox("Enable Skybox", &g_SkyboxEnabled))
                {
                    if (g_SkyboxEnabled)
                    {
                        sceneRenderer->AddObject(&skyboxObj);
                        std::cout << "Skybox enabled\n";
                    }
                    else
                    {
                        sceneRenderer->RemoveObject(&skyboxObj);
                        std::cout << "Skybox disabled\n";
                    }
                }
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                glm::vec3 pos = camera.GetPosition();
                ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                ImGui::Text("Yaw: %.1f | Pitch: %.1f", camera.GetYaw(), camera.GetPitch());

                float speed = camera.GetMovementSpeed();
                if (ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 20.0f))
                {
                    camera.SetMovementSpeed(speed);
                }

                if (camera.IsPerspective())
                {
                    float fov = camera.GetFOV();
                    if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
                        camera.SetFOV(fov);
                }
                else
                {
                    float orthoSize = camera.GetOrthoSize();
                    if (ImGui::SliderFloat("Ortho Size", &orthoSize, 0.5f, 100.0f))
                        camera.SetOrthoSize(orthoSize);
                }

                if (ImGui::Button("Reset Camera", ImVec2(200, 0)))
                {
                    camera = Mantrax::FPSCamera(glm::vec3(0.0f, 2.0f, 5.0f), 60.0f, 0.1f, 1000.0f);
                    camera.SetAspectRatio(static_cast<float>(renderView->width) / static_cast<float>(renderView->height));
                    camera.SetMovementSpeed(5.0f);
                    camera.SetMouseSensitivity(0.1f);
                }

                const char *modes[] = {"Perspective", "Orthographic"};
                int mode = static_cast<int>(camera.GetProjectionMode());

                if (ImGui::Combo("Projection", &mode, modes, 2))
                {
                    camera.SetProjectionMode(static_cast<Mantrax::ProjectionMode>(mode));
                }
            }

            if (ImGui::CollapsingHeader("Selected Object") && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
            {
                auto &selectedObj = sceneObjects[selectedObjectIndex];

                // Usar getComponent en lugar de hasComponent + getComponent
                Transform *objTransform = registry.getComponent<Transform>(selectedObj.entity);
                if (objTransform)
                {
                    ImGui::Text("Name: %s", selectedObj.name.c_str());
                    ImGui::Separator();

                    ImGui::DragFloat3("Position", &objTransform->position.x, 0.1f);
                    ImGui::DragFloat3("Rotation", &objTransform->rotation.x, 0.1f);
                    ImGui::DragFloat3("Scale", &objTransform->scale.x, 0.01f, 0.01f, 10.0f);

                    Rotator *objRotator = registry.getComponent<Rotator>(selectedObj.entity);
                    if (objRotator)
                    {
                        ImGui::Separator();
                        ImGui::Text("Rotation System");
                        ImGui::SliderFloat("Speed", &objRotator->speed, 0.0f, 5.0f);
                        ImGui::DragFloat3("Axis", &objRotator->axis.x, 0.01f, -1.0f, 1.0f);
                    }
                }
            }

            ImGui::End();

            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }

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