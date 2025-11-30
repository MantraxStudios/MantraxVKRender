#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../includes/FPSCamera.h"
#include "../includes/EngineLoader.h"
#include "../includes/UIRender.h"
#include "../includes/ui/SceneView.h"
#include "../includes/ServiceLocator.h"
#include "../includes/MinecraftChunk.h"
#include "../includes/ImGuiManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
bool g_OutlineEnabled = true;

void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
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

        auto startTime = std::chrono::high_resolution_clock::now();

        Mantrax::ChunkConfig chunkConfig;
        chunkConfig.chunkSizeX = 64;
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

        auto mesh = loader->gfx->CreateMesh(vertices, indices);

        auto normalMaterial = loader->gfx->CreateMaterial(loader->normalShader);
        auto outlineMaterial = loader->gfx->CreateMaterial(loader->outlineShader);

        std::cout << "Configurando camara FPS...\n";
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 40.0f, 10.0f), 60.0f, 0.1f, 500.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
        camera.SetMovementSpeed(15.0f);
        camera.SetMouseSensitivity(0.1f);

        g_Camera = &camera;
        std::cout << "Camara FPS configurada\n\n";

        Mantrax::UniformBufferObject ubo{};
        glm::mat4 model = glm::mat4(1.0f);

        CopyMat4(ubo.model, model);
        CopyMat4(ubo.view, camera.GetViewMatrix());
        CopyMat4(ubo.projection, camera.GetProjectionMatrix());

        loader->gfx->UpdateMaterialUBO(normalMaterial.get(), ubo);
        loader->gfx->UpdateMaterialUBO(outlineMaterial.get(), ubo);

        Mantrax::RenderObject outlineObj(mesh, outlineMaterial);
        Mantrax::RenderObject normalObj(mesh, normalMaterial);

        loader->gfx->AddRenderObject(outlineObj);
        loader->gfx->AddRenderObject(normalObj);

        DWORD lastTime = GetTickCount();
        int frameCount = 0;
        float totalTime = 0.0f;
        float lastTimeForFPS = 0.0f;
        int fps = 0;
        bool running = true;
        int currentSeed = chunkConfig.seed;
        bool escPressed = false;
        bool oPressed = false;

        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);

        std::vector<Mantrax::RenderObject> objects;
        objects.push_back(normalObj);
        objects.push_back(outlineObj);
        bool offscreenRegistered = false;

        ServiceLocator::instance().registerService(
            "UIRender",
            std::make_shared<UIRender>());

        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");
        uiRender->Set(new SceneView());

        while (running)
        {
            if (!loader->window->ProcessMessages())
            {
                running = false;
                break;
            }

            DWORD now = GetTickCount();
            float delta = (now - lastTime) / 1000.0f;
            lastTime = now;

            if (delta > 0.1f)
                delta = 0.1f;

            totalTime += delta;
            frameCount++;

            if (totalTime - lastTimeForFPS >= 1.0f)
            {
                fps = frameCount;
                frameCount = 0;
                lastTimeForFPS = totalTime;
            }

            if (GetAsyncKeyState('O') & 0x8000)
            {
                if (!oPressed)
                {
                    oPressed = true;
                    g_OutlineEnabled = !g_OutlineEnabled;

                    loader->gfx->ClearRenderObjects();

                    if (g_OutlineEnabled)
                    {
                        loader->gfx->AddRenderObject(outlineObj);
                        loader->gfx->AddRenderObject(normalObj);
                        std::cout << "Outline ACTIVADO\n";
                    }
                    else
                    {
                        loader->gfx->AddRenderObject(normalObj);
                        std::cout << "Outline DESACTIVADO\n";
                    }
                }
            }
            else
            {
                oPressed = false;
            }

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

                    std::cout << "Terreno regenerado en " << regenDuration.count() << "ms\n\n";

                    loader->gfx->ClearRenderObjects();
                    mesh = loader->gfx->CreateMesh(vertices, indices);

                    outlineObj.mesh = mesh;
                    normalObj.mesh = mesh;

                    if (g_OutlineEnabled)
                    {
                        loader->gfx->AddRenderObject(outlineObj);
                        loader->gfx->AddRenderObject(normalObj);
                    }
                    else
                    {
                        loader->gfx->AddRenderObject(normalObj);
                    }
                }
            }
            else
            {
                static bool rPressed = true;
                rPressed = false;
            }

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

            CopyMat4(ubo.model, model);
            CopyMat4(ubo.view, camera.GetViewMatrix());
            CopyMat4(ubo.projection, camera.GetProjectionMatrix());

            loader->gfx->RenderToOffscreenFramebuffer(offscreen, objects);

            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                float newAspect = (float)w / (float)h;

                camera.SetAspectRatio(newAspect);
                CopyMat4(ubo.projection, camera.GetProjectionMatrix());

                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            loader->gfx->UpdateMaterialUBO(normalMaterial.get(), ubo);
            loader->gfx->UpdateMaterialUBO(outlineMaterial.get(), ubo);

            if (!offscreenRegistered)
            {
                offscreen->imguiTextureID = ImGui_ImplVulkan_AddTexture(
                    offscreen->sampler,
                    offscreen->colorImageView,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                offscreenRegistered = true;
            }

            imgui.BeginFrame();

            uiRender->RenderAll();

            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("FPS: ");
            ImGui::SameLine();
            if (fps >= 60)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d", fps);
            else if (fps >= 30)
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%d", fps);
            else
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", fps);

            ImGui::Separator();

            ImGui::Text("Outline: ");
            ImGui::SameLine();
            if (g_OutlineEnabled)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ON");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OFF");

            ImGui::Text("Press 'O' to toggle");
            ImGui::Separator();

            ImGui::Text("Position: (%d, %d, %d)",
                        (int)camera.GetPosition().x,
                        (int)camera.GetPosition().y,
                        (int)camera.GetPosition().z);

            ImGui::End();

            // ───────────────────────────────────────────────
            // PANEL: SCENE VIEW (Render del Offscreen)
            // ───────────────────────────────────────────────
            ImGui::Begin("Viewport");

            bool sceneHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            if (sceneHovered && g_Input.mouseCapture)
            {
                ImGui::GetIO().WantCaptureMouse = false;
            }

            ImVec2 size = ImGui::GetContentRegionAvail();
            ImGui::Image(offscreen->imguiTextureID, size);
            ImGui::End();

            ImGui::Render();

            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
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