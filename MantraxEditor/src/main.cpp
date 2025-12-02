#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxRender/include/MantraxGFX_Timer.h"
#include "../MantraxPhysics/include/UPhysics.h"
#include "../MantraxPhysics/include/UBody.h"
#include "../MantraxPhysics/include/UCollisionBehaviour.h"
#include "../includes/FPSCamera.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../includes/ServiceLocator.h"
#include "../includes/ImGuiManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

struct InputState
{
    bool firstMouse = true;
    POINT lastMousePos = {0, 0};
    bool rightMouseDown = false;

    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;
    bool keySpace = false;
    bool keyShift = false;
    bool keyCtrl = false;
    bool keyR = false;
};

struct PhysicsObject
{
    UBody *body;
    UCollisionBehaviour *collider;
    Mantrax::RenderObject renderObj;
    std::shared_ptr<Mantrax::Material> material;
    Mantrax::UniformBufferObject ubo;
    Vec3 initialPos;
    bool isSphere;
    glm::vec3 scale;
};

InputState g_Input;
Mantrax::FPSCamera *g_Camera = nullptr;
SceneView *g_SceneView = nullptr;

void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

glm::vec3 Vec3ToGlm(const Vec3 &v)
{
    return glm::vec3(v.x, v.y, v.z);
}

Vec3 GlmToVec3(const glm::vec3 &v)
{
    return Vec3(v.x, v.y, v.z);
}

void CreateSphereVertices(std::vector<Mantrax::Vertex> &vertices, std::vector<uint32_t> &indices,
                          float radius = 1.0f, int sectors = 36, int stacks = 18)
{
    vertices.clear();
    indices.clear();

    const float PI = 3.14159265359f;
    float sectorStep = 2 * PI / sectors;
    float stackStep = PI / stacks;

    for (int i = 0; i <= stacks; ++i)
    {
        float stackAngle = PI / 2 - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * sectorStep;

            Mantrax::Vertex vertex;

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            vertex.position[0] = x;
            vertex.position[1] = y;
            vertex.position[2] = z;

            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;
            vertex.normal[0] = nx;
            vertex.normal[1] = ny;
            vertex.normal[2] = nz;

            vertex.color[0] = nx * 0.5f + 0.5f;
            vertex.color[1] = ny * 0.5f + 0.5f;
            vertex.color[2] = nz * 0.5f + 0.5f;

            vertex.texCoord[0] = (float)j / sectors;
            vertex.texCoord[1] = (float)i / stacks;

            vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < stacks; ++i)
    {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void CreateCubeVertices(std::vector<Mantrax::Vertex> &vertices, std::vector<uint32_t> &indices)
{
    vertices.clear();
    indices.clear();

    float positions[][3] = {
        {-0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},
        {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f}};

    float normals[][3] = {
        {0.0f, 0.0f, -1.0f},
        {0.0f, 0.0f, 1.0f},
        {-1.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}};

    float colors[][3] = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}};

    uint32_t faceIndices[][4] = {
        {3, 2, 1, 0},
        {4, 5, 6, 7},
        {7, 3, 0, 4},
        {2, 6, 5, 1},
        {0, 1, 5, 4},
        {7, 6, 2, 3}};

    for (int face = 0; face < 6; face++)
    {
        int triangleIndices[6] = {0, 1, 2, 0, 2, 3};

        for (int i = 0; i < 6; i++)
        {
            Mantrax::Vertex v;

            uint32_t posIdx = faceIndices[face][triangleIndices[i]];
            memcpy(v.position, positions[posIdx], 3 * sizeof(float));
            memcpy(v.normal, normals[face], 3 * sizeof(float));
            memcpy(v.color, colors[face], 3 * sizeof(float));

            v.texCoord[0] = 0.0f;
            v.texCoord[1] = 0.0f;

            vertices.push_back(v);
        }

        uint32_t baseIdx = face * 6;
        for (uint32_t i = 0; i < 6; i++)
        {
            indices.push_back(baseIdx + i);
        }
    }
}

LRESULT CustomWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;

    switch (msg)
    {
    case WM_RBUTTONDOWN:
        if (g_SceneView && g_SceneView->IsHovered())
        {
            g_Input.rightMouseDown = true;
            g_Input.firstMouse = true;
        }
        return 0;

    case WM_RBUTTONUP:
        g_Input.rightMouseDown = false;
        g_Input.firstMouse = true;
        return 0;

    case WM_KEYDOWN:
        switch (wp)
        {
        case 'W':
            g_Input.keyW = true;
            break;
        case 'A':
            g_Input.keyA = true;
            break;
        case 'S':
            g_Input.keyS = true;
            break;
        case 'D':
            g_Input.keyD = true;
            break;
        case 'R':
            g_Input.keyR = true;
            break;
        case VK_SPACE:
            g_Input.keySpace = true;
            break;
        case VK_SHIFT:
            g_Input.keyShift = true;
            break;
        case VK_CONTROL:
            g_Input.keyCtrl = true;
            break;
        }
        return 0;

    case WM_KEYUP:
        switch (wp)
        {
        case 'W':
            g_Input.keyW = false;
            break;
        case 'A':
            g_Input.keyA = false;
            break;
        case 'S':
            g_Input.keyS = false;
            break;
        case 'D':
            g_Input.keyD = false;
            break;
        case 'R':
            g_Input.keyR = false;
            break;
        case VK_SPACE:
            g_Input.keySpace = false;
            break;
        case VK_SHIFT:
            g_Input.keyShift = false;
            break;
        case VK_CONTROL:
            g_Input.keyCtrl = false;
            break;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_Input.rightMouseDown && g_Camera && g_SceneView && g_SceneView->IsHovered())
        {
            POINT currentPos;
            GetCursorPos(&currentPos);
            ScreenToClient(hwnd, &currentPos);

            if (g_Input.firstMouse)
            {
                g_Input.lastMousePos = currentPos;
                g_Input.firstMouse = false;
                return 0;
            }

            float xoffset = static_cast<float>(currentPos.x - g_Input.lastMousePos.x);
            float yoffset = static_cast<float>(g_Input.lastMousePos.y - currentPos.y);

            g_Input.lastMousePos = currentPos;
            g_Camera->ProcessMouseMovement(xoffset, yoffset);
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (g_Camera && g_SceneView && g_SceneView->IsHovered())
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wp) / 120.0f;
            g_Camera->ProcessMouseScroll(delta * 2.0f);
        }
        return 0;
    }

    return 0;
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
        ServiceLocator::instance().registerService(
            "EngineLoader",
            std::make_shared<EngineLoader>());

        auto loader = ServiceLocator::instance().get<EngineLoader>("EngineLoader");
        loader->Start(hInst, CustomWndProc);

        Mantrax::ImGuiManager imgui(loader->window->GetHWND(), loader->gfx.get());

        // Crear meshes
        std::vector<Mantrax::Vertex> sphereVertices;
        std::vector<uint32_t> sphereIndices;
        CreateSphereVertices(sphereVertices, sphereIndices, 1.0f, 36, 18);

        std::vector<Mantrax::Vertex> cubeVertices;
        std::vector<uint32_t> cubeIndices;
        CreateCubeVertices(cubeVertices, cubeIndices);

        auto sphereMesh = loader->gfx->CreateMesh(sphereVertices, sphereIndices);
        auto cubeMesh = loader->gfx->CreateMesh(cubeVertices, cubeIndices);

        Mantrax::FPSCamera camera(glm::vec3(0.0f, 5.0f, 20.0f), 60.0f, 0.1f, 500.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
        camera.SetMovementSpeed(10.0f);
        camera.SetMouseSensitivity(0.1f);

        g_Camera = &camera;

        // Sistema de física
        UPhysics physics;
        std::vector<PhysicsObject> physicsObjects;

        // Configuración de objetos: posición, masa, isSphere, escala
        struct ObjectConfig
        {
            Vec3 pos;
            float mass;
            bool isSphere;
            glm::vec3 scale;
        };

        std::vector<ObjectConfig> configs = {
            // Esferas cayendo
            {Vec3(-4.0f, 10.0f, 0.0f), 1.0f, true, glm::vec3(0.8f)},
            {Vec3(-2.0f, 12.0f, 1.0f), 1.5f, true, glm::vec3(1.0f)},
            {Vec3(0.0f, 14.0f, -1.0f), 0.8f, true, glm::vec3(0.6f)},
            {Vec3(2.0f, 11.0f, 0.5f), 1.2f, true, glm::vec3(0.9f)},
            {Vec3(4.0f, 13.0f, -0.5f), 1.0f, true, glm::vec3(0.7f)},

            // Cubos cayendo
            {Vec3(-3.0f, 15.0f, 2.0f), 2.0f, false, glm::vec3(1.0f, 1.0f, 1.0f)},
            {Vec3(1.0f, 16.0f, -2.0f), 1.5f, false, glm::vec3(0.8f, 1.2f, 0.8f)},
            {Vec3(3.0f, 17.0f, 1.5f), 1.8f, false, glm::vec3(1.2f, 0.7f, 1.2f)},

            // Suelos estáticos
            {Vec3(0.0f, -2.0f, 0.0f), 0.0f, false, glm::vec3(15.0f, 1.0f, 15.0f)},
            {Vec3(0.0f, 3.0f, -8.0f), 0.0f, false, glm::vec3(8.0f, 1.0f, 3.0f)},
        };

        // Crear objetos físicos
        for (size_t i = 0; i < configs.size(); i++)
        {
            PhysicsObject obj;
            obj.initialPos = configs[i].pos;
            obj.isSphere = configs[i].isSphere;
            obj.scale = configs[i].scale;

            // Crear body
            obj.body = new UBody(configs[i].pos, configs[i].mass);

            // Configurar como estático si es suelo (últimos 2 objetos)
            bool isStatic = (i >= 8);
            if (isStatic)
            {
                obj.body->SetStatic(true);
                obj.body->SetGravity(false);
            }

            // Crear collider apropiado
            if (obj.isSphere)
            {
                float radius = obj.scale.x; // Usar el componente x como radio
                obj.collider = new USphereCollision(radius);
            }
            else
            {
                Vec3 halfExtents(obj.scale.x * 0.5f, obj.scale.y * 0.5f, obj.scale.z * 0.5f);
                obj.collider = new UBoxCollision(halfExtents);
            }

            obj.body->SetCollisionBehaviour(obj.collider);
            physics.AddBody(obj.body);

            // Crear material y render object
            obj.material = loader->gfx->CreateMaterial(loader->normalShader);

            if (obj.isSphere)
            {
                obj.renderObj = Mantrax::RenderObject(sphereMesh, obj.material);
            }
            else
            {
                obj.renderObj = Mantrax::RenderObject(cubeMesh, obj.material);
            }

            loader->gfx->AddRenderObject(obj.renderObj);

            physicsObjects.push_back(obj);
        }

        Mantrax::Timer gameTimer;

        bool running = true;
        bool escPressed = false;
        bool rKeyPressed = false;

        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);

        std::vector<Mantrax::RenderObject> renderObjects;
        for (auto &obj : physicsObjects)
        {
            renderObjects.push_back(obj.renderObj);
        }

        ServiceLocator::instance().registerService(
            "UIRender",
            std::make_shared<UIRender>());

        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        g_SceneView = renderView;

        uiRender->Set(renderView);

        auto uiRenderGet = ServiceLocator::instance().get<UIRender>("UIRender");
        uiRenderGet->GetByType<SceneView>()->renderID = offscreen->renderID;

        std::cout << "=== SIMULACION CON 10 OBJETOS ===\n";
        std::cout << "5 Esferas + 3 Cubos cayendo\n";
        std::cout << "2 Plataformas estaticas\n";
        std::cout << "R - Reiniciar simulacion\n";
        std::cout << "Click derecho + WASD - Mover camara\n";
        std::cout << "================================\n\n";

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

            // Actualizar física
            physics.Update(delta);

            // Reset física con tecla R
            if (g_Input.keyR && !rKeyPressed)
            {
                rKeyPressed = true;
                for (auto &obj : physicsObjects)
                {
                    obj.body->SetPosition(obj.initialPos);
                    obj.body->Stop();
                }
                std::cout << "Fisica reiniciada!\n";
            }
            else if (!g_Input.keyR)
            {
                rKeyPressed = false;
            }

            // Actualizar matrices de transformación
            for (auto &obj : physicsObjects)
            {
                Vec3 pos = obj.body->GetPosition();
                glm::mat4 model = glm::translate(glm::mat4(1.0f), Vec3ToGlm(pos));
                model = glm::scale(model, obj.scale);

                CopyMat4(obj.ubo.model, model);
                CopyMat4(obj.ubo.view, camera.GetViewMatrix());
                CopyMat4(obj.ubo.projection, camera.GetProjectionMatrix());
            }

            // Cámara FPS
            if (g_Input.rightMouseDown && g_SceneView && g_SceneView->IsHovered())
            {
                bool sprint = g_Input.keyShift;
                if (g_Input.keyW)
                    camera.ProcessKeyboard(Mantrax::FORWARD, delta, sprint);
                if (g_Input.keyS)
                    camera.ProcessKeyboard(Mantrax::BACKWARD, delta, sprint);
                if (g_Input.keyA)
                    camera.ProcessKeyboard(Mantrax::LEFT, delta, sprint);
                if (g_Input.keyD)
                    camera.ProcessKeyboard(Mantrax::RIGHT, delta, sprint);
                if (g_Input.keySpace)
                    camera.ProcessKeyboard(Mantrax::UP, delta, sprint);
                if (g_Input.keyCtrl)
                    camera.ProcessKeyboard(Mantrax::DOWN, delta, sprint);
            }

            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            {
                if (!escPressed)
                {
                    escPressed = true;
                    running = false;
                    break;
                }
            }
            else
            {
                escPressed = false;
            }

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

                float newAspect = static_cast<float>(renderView->width) /
                                  static_cast<float>(renderView->height);
                camera.SetAspectRatio(newAspect);

                renderView->ResetResizeFlag();
            }

            loader->gfx->RenderToOffscreenFramebuffer(offscreen, renderObjects);

            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            // Actualizar UBOs
            for (auto &obj : physicsObjects)
            {
                loader->gfx->UpdateMaterialUBO(obj.material.get(), obj.ubo);
            }

            // Reemplaza el panel ImGui en el loop principal (después de imgui.BeginFrame())

            imgui.BeginFrame();

            // ===== PANEL PRINCIPAL DE FÍSICA =====
            ImGui::Begin("Physics Editor", nullptr, ImGuiWindowFlags_MenuBar);

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Actions"))
                {
                    if (ImGui::MenuItem("Reset All (R)"))
                    {
                        for (auto &obj : physicsObjects)
                        {
                            obj.body->SetPosition(obj.initialPos);
                            obj.body->Stop();
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Text("FPS: %d | Delta: %.4f", fps, delta);
            ImGui::Separator();

            // ===== TABS PARA ORGANIZAR =====
            if (ImGui::BeginTabBar("PhysicsTabs"))
            {
                // TAB 1: OBJECT EDITOR
                if (ImGui::BeginTabItem("Object Editor"))
                {
                    ImGui::Text("Select and modify physics objects:");
                    ImGui::Separator();

                    static int selectedObject = 0;

                    // Lista de objetos
                    ImGui::BeginChild("ObjectList", ImVec2(200, 0), true);
                    for (size_t i = 0; i < physicsObjects.size(); i++)
                    {
                        auto &obj = physicsObjects[i];
                        std::string label = obj.isSphere ? "Sphere " : "Box ";
                        label += std::to_string(i);
                        if (obj.body->IsStatic())
                            label += " (Static)";

                        if (ImGui::Selectable(label.c_str(), selectedObject == (int)i))
                        {
                            selectedObject = (int)i;
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    // Editor del objeto seleccionado
                    ImGui::BeginGroup();
                    ImGui::BeginChild("ObjectEditor", ImVec2(0, 0), true);

                    if (selectedObject >= 0 && selectedObject < (int)physicsObjects.size())
                    {
                        auto &obj = physicsObjects[selectedObject];

                        ImGui::Text("Editing: %s %d", obj.isSphere ? "Sphere" : "Box", selectedObject);
                        ImGui::Separator();

                        // POSICIÓN
                        Vec3 pos = obj.body->GetPosition();
                        float posArray[3] = {pos.x, pos.y, pos.z};
                        if (ImGui::DragFloat3("Position", posArray, 0.1f))
                        {
                            obj.body->SetPosition(Vec3(posArray[0], posArray[1], posArray[2]));
                        }

                        // VELOCIDAD
                        Vec3 vel = obj.body->GetVelocity();
                        float velArray[3] = {vel.x, vel.y, vel.z};
                        if (ImGui::DragFloat3("Velocity", velArray, 0.1f))
                        {
                            obj.body->SetVelocity(Vec3(velArray[0], velArray[1], velArray[2]));
                        }

                        ImGui::Separator();

                        // PROPIEDADES FÍSICAS
                        float mass = obj.body->GetMass();
                        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f))
                        {
                            obj.body->SetMass(mass);
                        }

                        bool isStatic = obj.body->IsStatic();
                        if (ImGui::Checkbox("Static", &isStatic))
                        {
                            obj.body->SetStatic(isStatic);
                            obj.body->SetGravity(!isStatic);
                        }

                        bool useGravity = !obj.body->IsStatic(); // Simplificado
                        if (!isStatic)
                        {
                            ImGui::Text("Gravity: Enabled");
                        }
                        else
                        {
                            ImGui::Text("Gravity: Disabled (Static)");
                        }

                        ImGui::Separator();

                        // ESCALA (afecta collider)
                        float scaleArray[3] = {obj.scale.x, obj.scale.y, obj.scale.z};
                        if (ImGui::DragFloat3("Scale", scaleArray, 0.05f, 0.1f, 10.0f))
                        {
                            obj.scale = glm::vec3(scaleArray[0], scaleArray[1], scaleArray[2]);

                            // Actualizar collider
                            delete obj.collider;
                            if (obj.isSphere)
                            {
                                obj.collider = new USphereCollision(obj.scale.x);
                            }
                            else
                            {
                                Vec3 halfExtents(obj.scale.x * 0.5f, obj.scale.y * 0.5f, obj.scale.z * 0.5f);
                                obj.collider = new UBoxCollision(halfExtents);
                            }
                            obj.body->SetCollisionBehaviour(obj.collider);
                        }

                        ImGui::Separator();

                        // INFORMACIÓN DE COLISIÓN
                        if (obj.isSphere)
                        {
                            USphereCollision *sphere = dynamic_cast<USphereCollision *>(obj.collider);
                            if (sphere)
                            {
                                ImGui::Text("Collision Type: Sphere");
                                ImGui::Text("Radius: %.2f", sphere->GetRadius());
                            }
                        }
                        else
                        {
                            UBoxCollision *box = dynamic_cast<UBoxCollision *>(obj.collider);
                            if (box)
                            {
                                Vec3 he = box->GetHalfExtents();
                                ImGui::Text("Collision Type: Box");
                                ImGui::Text("Half Extents: (%.2f, %.2f, %.2f)", he.x, he.y, he.z);
                            }
                        }

                        ImGui::Separator();

                        // ESTADÍSTICAS
                        float speed = vel.Length();
                        float ke = obj.body->GetKineticEnergy();
                        ImGui::Text("Speed: %.2f", speed);
                        ImGui::Text("Kinetic Energy: %.2f", ke);

                        ImGui::Separator();

                        // ACCIONES RÁPIDAS
                        if (ImGui::Button("Stop Motion"))
                        {
                            obj.body->Stop();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Reset Position"))
                        {
                            obj.body->SetPosition(obj.initialPos);
                            obj.body->Stop();
                        }

                        if (ImGui::Button("Apply Upward Force"))
                        {
                            obj.body->ApplyForce(Vec3(0, 500, 0));
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Apply Random Force"))
                        {
                            float rx = ((rand() % 200) - 100) * 2.0f;
                            float ry = ((rand() % 100) + 50) * 2.0f;
                            float rz = ((rand() % 200) - 100) * 2.0f;
                            obj.body->ApplyForce(Vec3(rx, ry, rz));
                        }
                    }

                    ImGui::EndChild();
                    ImGui::EndGroup();

                    ImGui::EndTabItem();
                }

                // TAB 2: GLOBAL SETTINGS
                if (ImGui::BeginTabItem("Global Settings"))
                {
                    ImGui::Text("Physics World Settings");
                    ImGui::Separator();

                    // GRAVEDAD GLOBAL
                    Vec3 gravity = physics.GetGlobalGravity();
                    float gravityArray[3] = {gravity.x, gravity.y, gravity.z};
                    if (ImGui::DragFloat3("Global Gravity", gravityArray, 0.1f, -50.0f, 50.0f))
                    {
                        physics.SetGlobalGravity(Vec3(gravityArray[0], gravityArray[1], gravityArray[2]));
                    }

                    if (ImGui::Button("Reset Gravity (Earth)"))
                    {
                        physics.SetGlobalGravity(Vec3(0, -9.81f, 0));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Zero Gravity"))
                    {
                        physics.SetGlobalGravity(Vec3(0, 0, 0));
                    }

                    ImGui::Separator();

                    // ESTADÍSTICAS GLOBALES
                    float totalKE = 0.0f;
                    int activeObjects = 0;
                    int staticObjects = 0;

                    for (auto &obj : physicsObjects)
                    {
                        totalKE += obj.body->GetKineticEnergy();
                        if (obj.body->GetVelocity().Length() > 0.01f)
                            activeObjects++;
                        if (obj.body->IsStatic())
                            staticObjects++;
                    }

                    ImGui::Text("Total Objects: %zu", physicsObjects.size());
                    ImGui::Text("Active Objects: %d", activeObjects);
                    ImGui::Text("Static Objects: %d", staticObjects);
                    ImGui::Text("Total Kinetic Energy: %.2f", totalKE);

                    ImGui::Separator();

                    // ACCIONES GLOBALES
                    if (ImGui::Button("Stop All Objects"))
                    {
                        for (auto &obj : physicsObjects)
                        {
                            obj.body->Stop();
                        }
                    }

                    if (ImGui::Button("Reset All to Initial"))
                    {
                        for (auto &obj : physicsObjects)
                        {
                            obj.body->SetPosition(obj.initialPos);
                            obj.body->Stop();
                        }
                    }

                    if (ImGui::Button("Randomize All Positions"))
                    {
                        for (auto &obj : physicsObjects)
                        {
                            if (!obj.body->IsStatic())
                            {
                                float rx = ((rand() % 100) - 50) * 0.1f;
                                float ry = 10.0f + (rand() % 100) * 0.1f;
                                float rz = ((rand() % 100) - 50) * 0.1f;
                                obj.body->SetPosition(Vec3(rx, ry, rz));
                                obj.body->Stop();
                            }
                        }
                    }

                    ImGui::EndTabItem();
                }

                // TAB 3: COLLISION MATRIX
                if (ImGui::BeginTabItem("Collision Info"))
                {
                    ImGui::Text("Collision Detection Matrix");
                    ImGui::Separator();

                    ImGui::BeginChild("CollisionMatrix");

                    // Tabla de colisiones
                    if (ImGui::BeginTable("Collisions", physicsObjects.size() + 1,
                                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        // Header
                        ImGui::TableSetupColumn("");
                        for (size_t i = 0; i < physicsObjects.size(); i++)
                        {
                            ImGui::TableSetupColumn(std::to_string(i).c_str());
                        }
                        ImGui::TableHeadersRow();

                        // Filas
                        for (size_t i = 0; i < physicsObjects.size(); i++)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%zu", i);

                            for (size_t j = 0; j < physicsObjects.size(); j++)
                            {
                                ImGui::TableNextColumn();

                                if (i == j)
                                {
                                    ImGui::TextDisabled("-");
                                }
                                else if (i < j)
                                {
                                    bool colliding = physics.CheckCollision((int)i, (int)j);
                                    if (colliding)
                                    {
                                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
                                    }
                                    else
                                    {
                                        ImGui::TextDisabled(".");
                                    }
                                }
                            }
                        }

                        ImGui::EndTable();
                    }

                    ImGui::Separator();
                    ImGui::Text("Legend: X = Colliding, . = Not colliding");

                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();

            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }

        // Cleanup
        for (auto &obj : physicsObjects)
        {
            delete obj.body;
            delete obj.collider;
        }

        g_SceneView = nullptr;
        g_Camera = nullptr;
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