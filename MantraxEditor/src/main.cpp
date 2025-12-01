#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxRender/include/MantraxGFX_Timer.h"
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

struct InputState
{
    bool firstMouse = true;
    POINT lastMousePos = {0, 0};
    bool mouseCapture = false;

    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;
    bool keySpace = false;
    bool keyShift = false;
    bool keyCtrl = false;
};

InputState g_Input;
Mantrax::FPSCamera *g_Camera = nullptr;

void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

// Función para crear vértices de un cubo
void CreateCubeVertices(std::vector<Mantrax::Vertex> &vertices, std::vector<uint32_t> &indices)
{
    vertices.clear();
    indices.clear();

    // Vértices del cubo (8 vértices)
    float positions[][3] = {
        {-0.5f, -0.5f, -0.5f}, // 0
        {0.5f, -0.5f, -0.5f},  // 1
        {0.5f, 0.5f, -0.5f},   // 2
        {-0.5f, 0.5f, -0.5f},  // 3
        {-0.5f, -0.5f, 0.5f},  // 4
        {0.5f, -0.5f, 0.5f},   // 5
        {0.5f, 0.5f, 0.5f},    // 6
        {-0.5f, 0.5f, 0.5f}    // 7
    };

    float normals[][3] = {
        {0.0f, 0.0f, -1.0f}, // Frente
        {0.0f, 0.0f, 1.0f},  // Atrás
        {-1.0f, 0.0f, 0.0f}, // Izquierda
        {1.0f, 0.0f, 0.0f},  // Derecha
        {0.0f, -1.0f, 0.0f}, // Abajo
        {0.0f, 1.0f, 0.0f}   // Arriba
    };

    float colors[][3] = {
        {1.0f, 0.0f, 0.0f}, // Rojo
        {0.0f, 1.0f, 0.0f}, // Verde
        {0.0f, 0.0f, 1.0f}, // Azul
        {1.0f, 1.0f, 0.0f}, // Amarillo
        {1.0f, 0.0f, 1.0f}, // Magenta
        {0.0f, 1.0f, 1.0f}  // Cyan
    };

    // Definir las 6 caras del cubo (cada cara con 4 vértices)
    // IMPORTANTE: Orden corregido para que las normales apunten HACIA AFUERA
    uint32_t faceIndices[][4] = {
        {3, 2, 1, 0}, // Frente (invertido)
        {4, 5, 6, 7}, // Atrás (invertido)
        {7, 3, 0, 4}, // Izquierda (invertido)
        {2, 6, 5, 1}, // Derecha (invertido)
        {0, 1, 5, 4}, // Abajo (invertido)
        {7, 6, 2, 3}  // Arriba (invertido)
    };

    // Coordenadas baricéntricas para los 3 vértices de un triángulo
    float baryCoords[3][3] = {
        {1.0f, 0.0f, 0.0f}, // Primer vértice
        {0.0f, 1.0f, 0.0f}, // Segundo vértice
        {0.0f, 0.0f, 1.0f}  // Tercer vértice
    };

    // Crear vértices para cada cara
    for (int face = 0; face < 6; face++)
    {
        // Cada cara tiene 2 triángulos, necesitamos 6 vértices (no 4)
        // Triángulo 1: vértices 0, 1, 2
        // Triángulo 2: vértices 0, 2, 3

        int triangleIndices[6] = {0, 1, 2, 0, 2, 3};
        int baryIndices[6] = {0, 1, 2, 0, 1, 2}; // Coordenadas baricéntricas para cada vértice

        for (int i = 0; i < 6; i++)
        {
            Mantrax::Vertex v;

            // Copiar position
            uint32_t posIdx = faceIndices[face][triangleIndices[i]];
            memcpy(v.position, positions[posIdx], 3 * sizeof(float));

            // Copiar normal
            memcpy(v.normal, normals[face], 3 * sizeof(float));

            // Copiar color
            memcpy(v.color, colors[face], 3 * sizeof(float));

            // Copiar texCoord
            v.texCoord[0] = 0.0f;
            v.texCoord[1] = 0.0f;

            // Asignar coordenadas baricéntricas correctas
            memcpy(v.barycentric, baryCoords[baryIndices[i]], 3 * sizeof(float));

            vertices.push_back(v);
        }

        // Índices para dos triángulos por cara (ahora son secuenciales)
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

    ImGuiIO &io = ImGui::GetIO();

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        if (!io.WantCaptureMouse && !g_Input.mouseCapture)
        {
            g_Input.mouseCapture = true;
            g_Input.firstMouse = true;
            SetCapture(hwnd);
            ShowCursor(FALSE);

            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT center = {rect.right / 2, rect.bottom / 2};
            ClientToScreen(hwnd, &center);
            SetCursorPos(center.x, center.y);
        }
        return 0;

    case WM_KEYDOWN:
        if (!io.WantCaptureKeyboard)
        {
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
        }
        return 0;

    case WM_KEYUP:
        if (!io.WantCaptureKeyboard)
        {
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
            case VK_SPACE:
                g_Input.keySpace = false;
                break;
            case VK_SHIFT:
                g_Input.keyShift = false;
                break;
            case VK_CONTROL:
                g_Input.keyCtrl = false;
                break;
            case VK_ESCAPE:
                if (g_Input.mouseCapture)
                {
                    g_Input.mouseCapture = false;
                    ReleaseCapture();
                    ShowCursor(TRUE);
                }
                break;
            }
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_Input.mouseCapture && g_Camera && !io.WantCaptureMouse)
        {
            POINT currentPos;
            GetCursorPos(&currentPos);
            ScreenToClient(hwnd, &currentPos);

            if (g_Input.firstMouse)
            {
                g_Input.lastMousePos = currentPos;
                g_Input.firstMouse = false;
            }

            float xoffset = static_cast<float>(currentPos.x - g_Input.lastMousePos.x);
            float yoffset = static_cast<float>(g_Input.lastMousePos.y - currentPos.y);

            g_Input.lastMousePos = currentPos;
            g_Camera->ProcessMouseMovement(xoffset, yoffset);

            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT center = {rect.right / 2, rect.bottom / 2};
            ClientToScreen(hwnd, &center);
            SetCursorPos(center.x, center.y);

            GetCursorPos(&currentPos);
            ScreenToClient(hwnd, &currentPos);
            g_Input.lastMousePos = currentPos;
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (g_Camera && !io.WantCaptureMouse)
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

        // ============================================
        // TIMER: Medir tiempo de generación del cubo
        // ============================================
        Mantrax::Timer generationTimer;

        // Crear geometría del cubo
        std::vector<Mantrax::Vertex> vertices;
        std::vector<uint32_t> indices;
        CreateCubeVertices(vertices, indices);

        generationTimer.Update();
        float generationTime = generationTimer.GetTotalTime() * 1000.0f; // En milisegundos

        std::cout << "Cubo generado en " << generationTime << "ms\n";
        std::cout << "Vertices: " << vertices.size() << ", Indices: " << indices.size() << "\n\n";

        auto mesh = loader->gfx->CreateMesh(vertices, indices);
        auto normalMaterial = loader->gfx->CreateMaterial(loader->normalShader);

        std::cout << "Configurando camara FPS...\n";
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 0.0f, 5.0f), 60.0f, 0.1f, 500.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
        camera.SetMovementSpeed(5.0f);
        camera.SetMouseSensitivity(0.1f);

        g_Camera = &camera;
        std::cout << "Camara FPS configurada\n\n";

        // Crear dos cubos con diferentes posiciones
        glm::mat4 modelCubeArriba = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
        glm::mat4 modelCubeAbajo = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));

        Mantrax::UniformBufferObject uboArriba{};
        Mantrax::UniformBufferObject uboAbajo{};

        // Escalar los cubos para verlos mejor
        modelCubeArriba = glm::scale(modelCubeArriba, glm::vec3(2.0f));
        modelCubeAbajo = glm::scale(modelCubeAbajo, glm::vec3(2.0f));

        CopyMat4(uboArriba.model, modelCubeArriba);
        CopyMat4(uboArriba.view, camera.GetViewMatrix());
        CopyMat4(uboArriba.projection, camera.GetProjectionMatrix());

        CopyMat4(uboAbajo.model, modelCubeAbajo);
        CopyMat4(uboAbajo.view, camera.GetViewMatrix());
        CopyMat4(uboAbajo.projection, camera.GetProjectionMatrix());

        // Materiales para cada cubo
        auto normalMaterialArriba = loader->gfx->CreateMaterial(loader->normalShader);
        auto normalMaterialAbajo = loader->gfx->CreateMaterial(loader->normalShader);

        loader->gfx->UpdateMaterialUBO(normalMaterialArriba.get(), uboArriba);
        loader->gfx->UpdateMaterialUBO(normalMaterialAbajo.get(), uboAbajo);

        // Objetos de render
        Mantrax::RenderObject normalObjArriba(mesh, normalMaterialArriba);
        Mantrax::RenderObject normalObjAbajo(mesh, normalMaterialAbajo);

        loader->gfx->AddRenderObject(normalObjArriba);
        loader->gfx->AddRenderObject(normalObjAbajo);

        // ============================================
        // TIMER: Para el game loop
        // ============================================
        Mantrax::Timer gameTimer;

        bool running = true;
        bool escPressed = false;

        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);

        std::vector<Mantrax::RenderObject> objects;
        objects.push_back(normalObjArriba);
        objects.push_back(normalObjAbajo);

        ServiceLocator::instance().registerService(
            "UIRender",
            std::make_shared<UIRender>());

        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        uiRender->Set(renderView);

        auto uiRenderGet = ServiceLocator::instance().get<UIRender>("UIRender");
        uiRenderGet->GetByType<SceneView>()->renderID = offscreen->renderID;

        // ============================================
        // GAME LOOP
        // ============================================
        while (running)
        {
            if (!loader->window->ProcessMessages())
            {
                running = false;
                break;
            }

            // ============================================
            // TIMER: Actualizar y obtener delta time + FPS
            // ============================================
            gameTimer.Update();
            float delta = gameTimer.GetDeltaTime();
            int fps = gameTimer.GetFPS();

            // Input de cámara
            if (g_Input.mouseCapture)
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

            // ESC handling
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            {
                if (!g_Input.mouseCapture && !escPressed)
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
                std::cout << "SceneView redimensionado: " << renderView->width << "x" << renderView->height << "\n";

                loader->gfx->ResizeOffscreenFramebuffer(
                    offscreen,
                    static_cast<uint32_t>(renderView->width),
                    static_cast<uint32_t>(renderView->height));

                // Actualizar el descriptor set
                offscreen->renderID = ImGui_ImplVulkan_AddTexture(
                    offscreen->sampler,
                    offscreen->colorImageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                renderView->renderID = offscreen->renderID;

                // Actualizar aspect ratio de la cámara
                float newAspect = static_cast<float>(renderView->width) /
                                  static_cast<float>(renderView->height);
                camera.SetAspectRatio(newAspect);

                renderView->ResetResizeFlag();
            }

            // Actualizar matrices de cámara
            CopyMat4(uboArriba.model, modelCubeArriba);
            CopyMat4(uboArriba.view, camera.GetViewMatrix());
            CopyMat4(uboArriba.projection, camera.GetProjectionMatrix());

            CopyMat4(uboAbajo.model, modelCubeAbajo);
            CopyMat4(uboAbajo.view, camera.GetViewMatrix());
            CopyMat4(uboAbajo.projection, camera.GetProjectionMatrix());

            // Renderizar a offscreen framebuffer
            loader->gfx->RenderToOffscreenFramebuffer(offscreen, objects);

            // Framebuffer resize de ventana principal
            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            // Actualizar UBOs
            loader->gfx->UpdateMaterialUBO(normalMaterialArriba.get(), uboArriba);
            loader->gfx->UpdateMaterialUBO(normalMaterialAbajo.get(), uboAbajo);

            // Renderizar ImGui
            imgui.BeginFrame();
            uiRender->RenderAll();
            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
        }

        std::cout << "\nCerrando aplicacion...\n";
        std::cout << "Total frames renderizados: " << gameTimer.GetFrameCount() << "\n";
        std::cout << "Tiempo total de ejecucion: " << gameTimer.GetTotalTime() << " segundos\n";

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