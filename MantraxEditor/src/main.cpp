#include "../MantraxRender/include/MainWinPlug.h"
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxRender/include/MantraxGFX_Timer.h"
#include "../MantraxECS/include/TextureLoader.h"
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

// Incluir para cargar texturas
#define STB_IMAGE_IMPLEMENTATION
#include "../MantraxECS/include/TextureLoader.h"

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
};

struct RenderableObject
{
    Mantrax::RenderObject renderObj;
    std::shared_ptr<Mantrax::Material> material;
    Mantrax::UniformBufferObject ubo;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

InputState g_Input;
Mantrax::FPSCamera *g_Camera = nullptr;
SceneView *g_SceneView = nullptr;

void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

glm::mat4 CreateRotationMatrix(const glm::vec3 &rotation)
{
    glm::mat4 rotMat = glm::mat4(1.0f);
    rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    return rotMat;
}

// Función para cargar texturas PBR
std::shared_ptr<Mantrax::Texture> LoadPBRTexture(
    Mantrax::GFX *gfx,
    const std::string &path,
    const std::string &name)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        std::cerr << "Failed to load " << name << " texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    auto texture = gfx->CreateTexture(data, width, height);
    stbi_image_free(data);

    std::cout << "✓ Loaded " << name << ": " << path << " (" << width << "x" << height << ")\n";
    return texture;
}

void CreateCubeVertices(std::vector<Mantrax::Vertex> &vertices, std::vector<uint32_t> &indices)
{
    vertices.clear();
    indices.clear();

    struct FaceVertex
    {
        float pos[3];
        float normal[3];
        float color[3];
        float uv[2];
    };

    // Cara frontal (Z+)
    FaceVertex frontFace[] = {
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    // Cara trasera (Z-)
    FaceVertex backFace[] = {
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    // Cara izquierda (X-)
    FaceVertex leftFace[] = {
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    // Cara derecha (X+)
    FaceVertex rightFace[] = {
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    // Cara superior (Y+)
    FaceVertex topFace[] = {
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    // Cara inferior (Y-)
    FaceVertex bottomFace[] = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}};

    FaceVertex *faces[] = {frontFace, backFace, leftFace, rightFace, topFace, bottomFace};
    uint32_t faceIndices[] = {0, 1, 2, 0, 2, 3};

    for (int face = 0; face < 6; face++)
    {
        uint32_t baseIdx = vertices.size();

        for (int i = 0; i < 4; i++)
        {
            Mantrax::Vertex v;
            memcpy(v.position, faces[face][i].pos, 3 * sizeof(float));
            memcpy(v.normal, faces[face][i].normal, 3 * sizeof(float));
            memcpy(v.color, faces[face][i].color, 3 * sizeof(float));
            memcpy(v.texCoord, faces[face][i].uv, 2 * sizeof(float));
            vertices.push_back(v);
        }

        for (int i = 0; i < 6; i++)
        {
            indices.push_back(baseIdx + faceIndices[i]);
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

        // ============ CREAR MESH DEL CUBO ============
        std::vector<Mantrax::Vertex> cubeVertices;
        std::vector<uint32_t> cubeIndices;
        CreateCubeVertices(cubeVertices, cubeIndices);
        auto cubeMesh = loader->gfx->CreateMesh(cubeVertices, cubeIndices);

        // ============ CARGAR TEXTURAS PBR ============
        std::cout << "\n=== Loading PBR Textures ===\n";

        auto albedoTex = LoadPBRTexture(loader->gfx.get(), "textures/DiamondPlate008A_1K-PNG_Color.png", "Albedo");
        auto normalTex = LoadPBRTexture(loader->gfx.get(), "textures/DiamondPlate008A_1K-PNG_NormalGL.png", "Normal");
        auto metalTex = LoadPBRTexture(loader->gfx.get(), "textures/DiamondPlate008A_1K-PNG_Metalness.png", "Metallic");
        auto roughnessTex = LoadPBRTexture(loader->gfx.get(), "textures/DiamondPlate008A_1K-PNG_Roughness.png", "Roughness");
        auto aoTex = LoadPBRTexture(loader->gfx.get(), "textures/DiamondPlate008A_1K-PNG_AmbientOcclusion.png", "AO");

        std::cout << "============================\n\n";

        // ============ CREAR MATERIAL PBR ============
        auto cubeMaterial = loader->gfx->CreateMaterial(loader->normalShader);

        // ESTO ES LO CRUCIAL: Configurar propiedades ANTES de asignar texturas
        cubeMaterial->SetBaseColor(1.0f, 1.0f, 1.0f);
        cubeMaterial->SetMetallicFactor(1.0f);  // CAMBIADO: 1.0 para metal brillante
        cubeMaterial->SetRoughnessFactor(0.2f); // CAMBIADO: 0.2 para superficie pulida
        cubeMaterial->SetNormalScale(1.0f);

        // Asignar todas las texturas PBR
        loader->gfx->SetMaterialPBRTextures(
            cubeMaterial,
            albedoTex,    // Albedo
            normalTex,    // Normal
            metalTex,     // Metallic
            roughnessTex, // Roughness
            aoTex         // AO
        );

        // ============ CREAR CUBO ============
        RenderableObject cube;
        cube.position = glm::vec3(0.0f, 0.0f, 0.0f);
        cube.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        cube.scale = glm::vec3(2.0f);
        cube.material = cubeMaterial;
        cube.renderObj = Mantrax::RenderObject(cubeMesh, cube.material);

        // ============ CONFIGURAR CÁMARA ============
        Mantrax::FPSCamera camera(glm::vec3(0.0f, 0.0f, 5.0f), 60.0f, 0.1f, 100.0f);
        camera.SetAspectRatio(1920.0f / 1080.0f);
        camera.SetMovementSpeed(5.0f);
        camera.SetMouseSensitivity(0.1f);
        g_Camera = &camera;

        // ============ SETUP RENDERING ============
        loader->gfx->AddRenderObject(cube.renderObj);

        std::vector<RenderableObject> objects = {cube};
        std::vector<Mantrax::RenderObject> renderObjects = {cube.renderObj};

        Mantrax::Timer gameTimer;

        auto offscreen = loader->gfx->CreateOffscreenFramebuffer(1024, 768);

        ServiceLocator::instance().registerService(
            "UIRender",
            std::make_shared<UIRender>());

        auto uiRender = ServiceLocator::instance().get<UIRender>("UIRender");

        SceneView *renderView = new SceneView();
        g_SceneView = renderView;
        uiRender->Set(renderView);

        auto uiRenderGet = ServiceLocator::instance().get<UIRender>("UIRender");
        uiRenderGet->GetByType<SceneView>()->renderID = offscreen->renderID;

        std::cout << "=== PBR METAL CUBE DEMO ===\n";
        std::cout << "Right Click + WASD - Move camera\n";
        std::cout << "Mouse Wheel - Zoom\n";
        std::cout << "ESC - Exit\n";
        std::cout << "=====================\n\n";

        bool running = true;
        bool escPressed = false;
        float cubeRotation = 0.0f;

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

            // Rotar el cubo automáticamente
            cubeRotation += delta * 20.0f;
            cube.rotation.y = cubeRotation;

            // Actualizar matriz del cubo
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cube.position);
            model = model * CreateRotationMatrix(cube.rotation);
            model = glm::scale(model, cube.scale);

            CopyMat4(cube.ubo.model, model);
            CopyMat4(cube.ubo.view, camera.GetViewMatrix());
            CopyMat4(cube.ubo.projection, camera.GetProjectionMatrix());

            // NUEVO: Copiar posición de cámara
            glm::vec3 camPos = camera.GetPosition();
            cube.ubo.cameraPosition[0] = camPos.x;
            cube.ubo.cameraPosition[1] = camPos.y;
            cube.ubo.cameraPosition[2] = camPos.z;
            cube.ubo.cameraPosition[3] = 0.0f;

            // Control de cámara
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

            // ESC para salir
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

            // Resize handling
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

            // Render to offscreen
            loader->gfx->RenderToOffscreenFramebuffer(offscreen, renderObjects);

            // Window resize
            if (loader->window->WasFramebufferResized())
            {
                uint32_t w, h;
                loader->window->GetWindowSize(w, h);
                loader->gfx->NotifyFramebufferResized();
                loader->window->ResetFramebufferResizedFlag();
            }

            // Update UBO
            loader->gfx->UpdateMaterialUBO(cube.material.get(), cube.ubo);

            imgui.BeginFrame();

            ImGui::Begin("PBR Material Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            // ============ PERFORMANCE ============
            ImGui::Text("FPS: %d | Delta: %.4f", fps, delta);
            ImGui::Separator();

            // ============ CAMERA INFO ============
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                            camera.GetPosition().x,
                            camera.GetPosition().y,
                            camera.GetPosition().z);

                ImGui::Text("Cube Rotation: %.1f°", cubeRotation);

                if (ImGui::Button("Reset Camera", ImVec2(200, 0)))
                {
                    camera = Mantrax::FPSCamera(glm::vec3(0.0f, 0.0f, 5.0f), 60.0f, 0.1f, 100.0f);
                    camera.SetAspectRatio(static_cast<float>(renderView->width) /
                                          static_cast<float>(renderView->height));
                    camera.SetMovementSpeed(5.0f);
                    camera.SetMouseSensitivity(0.1f);
                }
            }

            ImGui::Separator();

            // ============ MATERIAL PROPERTIES ============
            if (ImGui::CollapsingHeader("Material Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static float normalScale = 1.0f;
                ImGui::Text("Normal Map Intensity:");
                if (ImGui::SliderFloat("##normal", &normalScale, 0.0f, 5.0f, "%.2f"))
                {
                    cube.material->SetNormalScale(normalScale);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##normal"))
                {
                    normalScale = 1.0f;
                    cube.material->SetNormalScale(normalScale);
                }

                ImGui::Spacing();

                static float metallic = 1.0f;
                ImGui::Text("Metallic:");
                if (ImGui::SliderFloat("##metallic", &metallic, 0.0f, 1.0f, "%.3f"))
                {
                    cube.material->SetMetallicFactor(metallic);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##metal"))
                {
                    metallic = 1.0f;
                    cube.material->SetMetallicFactor(metallic);
                }

                ImGui::Spacing();

                static float roughness = 0.2f;
                ImGui::Text("Roughness:");
                if (ImGui::SliderFloat("##roughness", &roughness, 0.0f, 1.0f, "%.3f"))
                {
                    cube.material->SetRoughnessFactor(roughness);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##rough"))
                {
                    roughness = 0.2f;
                    cube.material->SetRoughnessFactor(roughness);
                }

                ImGui::Spacing();

                static float colorTint[3] = {1.0f, 1.0f, 1.0f};
                ImGui::Text("Color Tint:");
                if (ImGui::ColorEdit3("##colortint", colorTint, ImGuiColorEditFlags_NoInputs))
                {
                    cube.material->SetBaseColor(colorTint[0], colorTint[1], colorTint[2]);
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##color"))
                {
                    colorTint[0] = colorTint[1] = colorTint[2] = 1.0f;
                    cube.material->SetBaseColor(1.0f, 1.0f, 1.0f);
                }
            }

            ImGui::Separator();

            // ============ MATERIAL PRESETS ============
            if (ImGui::CollapsingHeader("Material Presets"))
            {
                static float normalScale = 1.0f;
                static float metallic = 1.0f;
                static float roughness = 0.2f;

                if (ImGui::Button("Polished Chrome", ImVec2(200, 0)))
                {
                    normalScale = 0.5f;
                    metallic = 1.0f;
                    roughness = 0.1f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(1.0f, 1.0f, 1.0f);
                }

                if (ImGui::Button("Brushed Metal", ImVec2(200, 0)))
                {
                    normalScale = 1.5f;
                    metallic = 1.0f;
                    roughness = 0.4f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(1.0f, 1.0f, 1.0f);
                }

                if (ImGui::Button("Gold", ImVec2(200, 0)))
                {
                    normalScale = 0.8f;
                    metallic = 1.0f;
                    roughness = 0.2f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(1.0f, 0.8f, 0.3f);
                }

                if (ImGui::Button("Copper", ImVec2(200, 0)))
                {
                    normalScale = 1.0f;
                    metallic = 1.0f;
                    roughness = 0.3f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(0.95f, 0.6f, 0.4f);
                }

                if (ImGui::Button("Rusty Metal", ImVec2(200, 0)))
                {
                    normalScale = 2.0f;
                    metallic = 0.7f;
                    roughness = 0.8f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(0.7f, 0.4f, 0.3f);
                }
                if (ImGui::Button("Diamond Plate (Default)", ImVec2(200, 0)))
                {
                    normalScale = 1.0f;
                    metallic = 1.0f;
                    roughness = 0.2f;
                    cube.material->SetNormalScale(normalScale);
                    cube.material->SetMetallicFactor(metallic);
                    cube.material->SetRoughnessFactor(roughness);
                    cube.material->SetBaseColor(1.0f, 1.0f, 1.0f);
                }
            }
            ImGui::Separator(); // ============ TEXTURE INFO ============
            if (ImGui::CollapsingHeader("Active Textures"))
            {
                ImGui::BulletText("Albedo Map");
                ImGui::BulletText("Normal Map");
                ImGui::BulletText("Metallic Map");
                ImGui::BulletText("Roughness Map");
                ImGui::BulletText("AO Map");
            }
            ImGui::Separator(); // ============ HELP ============
            if (ImGui::CollapsingHeader("Controls"))
            {
                ImGui::TextWrapped("Right Click + WASD: Move camera");
                ImGui::TextWrapped("Mouse Wheel: Zoom");
                ImGui::TextWrapped("Shift: Sprint");
                ImGui::TextWrapped("Space/Ctrl: Up/Down");
                ImGui::TextWrapped("ESC: Exit");
            }
            ImGui::End();
            uiRender->RenderAll();
            ImGui::Render();
            loader->gfx->DrawFrame([](VkCommandBuffer cmd)
                                   { ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd); });
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