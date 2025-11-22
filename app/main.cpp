#include "../render/include/MainWinPlug.h"
#include "../render/include/MantraxGFX_API.h"
#include "FPSCamera.h"
#include "MinecraftChunk.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>

// ============================================================================
// ESTADO DE INPUT
// ============================================================================
struct InputState
{
    bool firstMouse = true;
    POINT lastMousePos = {0, 0};
    bool mouseCapture = false;

    // Teclas de movimiento
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

// ============================================================================
// HELPERS
// ============================================================================
void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

// ============================================================================
// WINDOW PROCEDURE PERSONALIZADO PARA CAPTURAR INPUT FPS
// ============================================================================
LRESULT CustomWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
        // Capturar mouse cuando se hace click
        if (!g_Input.mouseCapture)
        {
            g_Input.mouseCapture = true;
            g_Input.firstMouse = true;
            SetCapture(hwnd);
            ShowCursor(FALSE); // Ocultar cursor

            // Centrar cursor
            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT center = {rect.right / 2, rect.bottom / 2};
            ClientToScreen(hwnd, &center);
            SetCursorPos(center.x, center.y);
        }
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
            // Liberar mouse con ESC
            if (g_Input.mouseCapture)
            {
                g_Input.mouseCapture = false;
                ReleaseCapture();
                ShowCursor(TRUE);
            }
            break;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (g_Input.mouseCapture && g_Camera)
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
            float yoffset = static_cast<float>(g_Input.lastMousePos.y - currentPos.y); // Invertido

            g_Input.lastMousePos = currentPos;

            g_Camera->ProcessMouseMovement(xoffset, yoffset);

            // Recentrar cursor para evitar que salga de la ventana
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
        if (g_Camera)
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wp) / 120.0f;
            g_Camera->ProcessMouseScroll(delta * 2.0f);
        }
        return 0;
    }

    return 0;
}

// ============================================================================
// MAIN
// ============================================================================
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
        std::cout << "=== Mantrax Engine - Minecraft FPS Camera ===\n\n";

        std::cout << "Creando ventana...\n";
        Mantrax::WindowConfig windowConfig;
        windowConfig.width = 1920;
        windowConfig.height = 1080;
        windowConfig.title = "Mantrax Engine - Minecraft FPS";
        windowConfig.resizable = true;

        Mantrax::WindowMainPlug window(hInst, windowConfig);
        window.SetCustomWndProc(CustomWndProc);
        std::cout << "Ventana creada\n\n";

        std::cout << "Inicializando GFX...\n";
        Mantrax::GFXConfig gfxConfig;
        gfxConfig.clearColor = {0.53f, 0.81f, 0.92f, 1.0f}; // Cielo azul

        Mantrax::GFX gfx(hInst, window.GetHWND(), gfxConfig);
        std::cout << "GFX inicializado\n\n";

        std::cout << "Creando shader...\n";
        Mantrax::ShaderConfig shaderConfig;
        shaderConfig.vertexShaderPath = "shaders/pbr.vert.spv";
        shaderConfig.fragmentShaderPath = "shaders/pbr.frag.spv";
        shaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
        shaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();
        shaderConfig.cullMode = VK_CULL_MODE_BACK_BIT;
        shaderConfig.depthTestEnable = true;

        auto shader = gfx.CreateShader(shaderConfig);
        std::cout << "Shader creado\n\n";

        // =====================================================================
        // GENERAR CHUNK DE MINECRAFT
        // =====================================================================
        std::cout << "Generando terreno Minecraft...\n";
        auto startTime = std::chrono::high_resolution_clock::now();

        Mantrax::ChunkConfig chunkConfig;
        chunkConfig.chunkSizeX = 64; // Más grande para explorar
        chunkConfig.chunkSizeY = 64;
        chunkConfig.chunkSizeZ = 64;
        chunkConfig.seed = 42;
        chunkConfig.noiseScale = 0.06f;
        chunkConfig.terrainHeight = 32.0f;
        chunkConfig.terrainAmplitude = 14.0f;

        Mantrax::MinecraftChunk chunk(chunkConfig);
        chunk.GenerateTerrain();

        std::vector<Mantrax::Vertex> vertices;
        std::vector<uint32_t> indices;
        chunk.BuildMesh(vertices, indices);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "Terreno generado en " << duration.count() << "ms\n";
        std::cout << "  - Vertices: " << vertices.size() << "\n";
        std::cout << "  - Indices: " << indices.size() << "\n";
        std::cout << "  - Triangulos: " << indices.size() / 3 << "\n\n";

        if (vertices.empty() || indices.empty())
        {
            throw std::runtime_error("Error: No se generaron vertices o indices");
        }

        auto mesh = gfx.CreateMesh(vertices, indices);
        auto material = gfx.CreateMaterial(shader);

        // =====================================================================
        // CONFIGURAR CÁMARA FPS
        // =====================================================================
        std::cout << "Configurando camara FPS...\n";
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 40.0f, 10.0f), 60.0f, 0.1f, 500.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
        camera.SetMovementSpeed(15.0f);
        camera.SetMouseSensitivity(0.1f);

        g_Camera = &camera; // Para acceso desde WndProc

        std::cout << "Camara FPS configurada\n";
        std::cout << "  - Posicion: ("
                  << camera.GetPosition().x << ", "
                  << camera.GetPosition().y << ", "
                  << camera.GetPosition().z << ")\n";
        std::cout << "  - Velocidad: " << camera.GetMovementSpeed() << " unidades/seg\n\n";

        // Configurar UBO inicial
        Mantrax::UniformBufferObject ubo{};
        glm::mat4 model = glm::mat4(1.0f);

        CopyMat4(ubo.model, model);
        CopyMat4(ubo.view, camera.GetViewMatrix());
        CopyMat4(ubo.projection, camera.GetProjectionMatrix());

        gfx.UpdateMaterialUBO(material.get(), ubo);

        Mantrax::RenderObject obj(mesh, material);
        gfx.AddRenderObject(obj);

        std::cout << "=== CONTROLES FPS ===\n";
        std::cout << "  - Click Izquierdo: Capturar mouse (modo FPS)\n";
        std::cout << "  - W/A/S/D: Movimiento horizontal\n";
        std::cout << "  - Espacio: Subir\n";
        std::cout << "  - Ctrl: Bajar\n";
        std::cout << "  - Shift: Sprint (correr mas rapido)\n";
        std::cout << "  - Mouse: Mirar alrededor\n";
        std::cout << "  - Scroll: Zoom (FOV)\n";
        std::cout << "  - ESC: Liberar mouse / Salir (presionar 2 veces)\n";
        std::cout << "  - R: Regenerar terreno\n\n";

        std::cout << "Iniciando loop de renderizado...\n\n";

        DWORD lastTime = GetTickCount();
        int frameCount = 0;
        float totalTime = 0.0f;
        float lastTimeForFPS = 0.0f;
        int fps = 0;
        bool running = true;
        int currentSeed = chunkConfig.seed;
        bool escPressed = false;

        while (running)
        {
            if (!window.ProcessMessages())
            {
                running = false;
                break;
            }

            DWORD now = GetTickCount();
            float delta = (now - lastTime) / 1000.0f;
            lastTime = now;

            // Limitar delta para evitar saltos grandes
            if (delta > 0.1f)
                delta = 0.1f;

            totalTime += delta;
            frameCount++;

            // Calcular FPS
            if (totalTime - lastTimeForFPS >= 1.0f)
            {
                fps = frameCount;
                frameCount = 0;
                lastTimeForFPS = totalTime;
            }

            // =================================================================
            // PROCESAR MOVIMIENTO FPS
            // =================================================================

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

            // =================================================================
            // REGENERAR TERRENO
            // =================================================================

            if (GetAsyncKeyState('R') & 0x8000)
            {
                static bool rPressed = false;
                if (!rPressed)
                {
                    rPressed = true;
                    currentSeed++;

                    std::cout << "\n=== REGENERANDO TERRENO (Seed: " << currentSeed << ") ===\n";
                    auto regenStart = std::chrono::high_resolution_clock::now();

                    chunkConfig.seed = currentSeed;
                    chunk = Mantrax::MinecraftChunk(chunkConfig);
                    chunk.GenerateTerrain();

                    vertices.clear();
                    indices.clear();
                    chunk.BuildMesh(vertices, indices);

                    auto regenEnd = std::chrono::high_resolution_clock::now();
                    auto regenDuration = std::chrono::duration_cast<std::chrono::milliseconds>(regenEnd - regenStart);

                    std::cout << "Terreno regenerado en " << regenDuration.count() << "ms\n";
                    std::cout << "Vertices: " << vertices.size() << " | Triangulos: " << indices.size() / 3 << "\n\n";

                    // Recrear mesh
                    gfx.ClearRenderObjects();
                    mesh = gfx.CreateMesh(vertices, indices);
                    obj.mesh = mesh;
                    gfx.AddRenderObject(obj);
                }
            }
            else
            {
                static bool rPressed = true;
                rPressed = false;
            }

            // Salir con ESC (doble presión)
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            {
                if (!g_Input.mouseCapture)
                {
                    if (!escPressed)
                    {
                        escPressed = true;
                    }
                    else
                    {
                        running = false;
                        break;
                    }
                }
            }
            else
            {
                escPressed = false;
            }

            // =================================================================
            // ACTUALIZAR MATRICES
            // =================================================================

            CopyMat4(ubo.model, model);
            CopyMat4(ubo.view, camera.GetViewMatrix());
            CopyMat4(ubo.projection, camera.GetProjectionMatrix());

            // Manejar resize
            if (window.WasFramebufferResized())
            {
                uint32_t w, h;
                window.GetWindowSize(w, h);
                float newAspect = (float)w / (float)h;

                camera.SetAspectRatio(newAspect);
                CopyMat4(ubo.projection, camera.GetProjectionMatrix());

                gfx.NotifyFramebufferResized();
                window.ResetFramebufferResizedFlag();

                std::cout << "Ventana redimensionada: " << w << "x" << h << "\n";
            }

            // Actualizar UBO
            gfx.UpdateMaterialUBO(material.get(), ubo);

            // Dibujar
            gfx.DrawFrame();

            // Debug info cada 2 segundos
            static float lastDebug = 0.0f;
            if (totalTime - lastDebug >= 2.0f)
            {
                lastDebug = totalTime;
                glm::vec3 pos = camera.GetPosition();
                std::cout << "FPS: " << fps
                          << " | Pos: (" << (int)pos.x << ", " << (int)pos.y << ", " << (int)pos.z << ")"
                          << " | FOV: " << (int)camera.GetFOV()
                          << " | " << (sprint ? "SPRINT" : "Walk")
                          << " | Mouse: " << (g_Input.mouseCapture ? "CAPTURED" : "Free") << "\n";
            }
        }

        std::cout << "\nCerrando aplicacion...\n";
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