#include "../render/include/MainWinPlug.h"
#include "../render/include/MantraxGFX_API.h"
#include <cmath>
#include <iostream>

// ============================================================================
// HELPERS MATEMÁTICOS BÁSICOS
// ============================================================================

void MatrixIdentity(float *mat)
{
    for (int i = 0; i < 16; i++)
        mat[i] = 0.0f;
    mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
}

void MatrixPerspective(float *mat, float fov, float aspect, float znear, float zfar)
{
    for (int i = 0; i < 16; ++i)
        mat[i] = 0.0f;

    float f = 1.0f / tanf(fov * 0.5f);
    mat[0] = f / aspect;
    mat[5] = -f;
    mat[10] = zfar / (zfar - znear);
    mat[11] = 1.0f;
    mat[14] = -(zfar * znear) / (zfar - znear);
}

void MatrixOrthographic(float *mat, float left, float right, float bottom, float top, float znear, float zfar)
{
    for (int i = 0; i < 16; ++i)
        mat[i] = 0.0f;

    mat[0] = 2.0f / (right - left);
    mat[5] = -2.0f / (top - bottom);
    mat[10] = 1.0f / (zfar - znear);
    mat[12] = -(right + left) / (right - left);
    mat[13] = -(top + bottom) / (top - bottom);
    mat[14] = -znear / (zfar - znear);
    mat[15] = 1.0f;
}

void MatrixLookAt(float *mat, float eyeX, float eyeY, float eyeZ,
                  float centerX, float centerY, float centerZ,
                  float upX, float upY, float upZ)
{
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    float len = sqrtf(fx * fx + fy * fy + fz * fz);
    if (len > 0.0001f)
    {
        fx /= len;
        fy /= len;
        fz /= len;
    }

    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;
    len = sqrtf(sx * sx + sy * sy + sz * sz);
    if (len > 0.0001f)
    {
        sx /= len;
        sy /= len;
        sz /= len;
    }

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    for (int i = 0; i < 16; i++)
        mat[i] = 0.0f;

    mat[0] = sx;
    mat[1] = ux;
    mat[2] = -fx;
    mat[3] = 0.0f;

    mat[4] = sy;
    mat[5] = uy;
    mat[6] = -fy;
    mat[7] = 0.0f;

    mat[8] = sz;
    mat[9] = uz;
    mat[10] = -fz;
    mat[11] = 0.0f;

    mat[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
    mat[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    mat[14] = (fx * eyeX + fy * eyeY + fz * eyeZ);
    mat[15] = 1.0f;
}

void PrintMatrix(const char *name, const float *mat)
{
    std::cout << name << ":\n";
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            std::cout << mat[i * 4 + j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ============================================================================
// MAIN - MULTIPLATAFORMA
// ============================================================================

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    // Crear consola para debug en Windows
    AllocConsole();
    FILE *dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
#else
int main()
{
    void *hInst = nullptr; // En Linux no necesitamos HINSTANCE
#endif

    try
    {
        std::cout << "=== Mantrax Engine Test ===\n\n";

        // ==================================================
        // 1. CREAR VENTANA USANDO WINDOWMAINPLUG
        // ==================================================
        std::cout << "Creando ventana...\n";
        Mantrax::WindowConfig windowConfig;
        windowConfig.width = 1280;
        windowConfig.height = 720;
        windowConfig.title = "Mantrax Engine - Modular Window Test";
        windowConfig.resizable = true;

        Mantrax::WindowMainPlug window(hInst, windowConfig);
        std::cout << "Ventana creada correctamente\n";

#ifdef _WIN32
        std::cout << "  HWND: " << window.GetHWND() << "\n\n";
#else
        std::cout << "  Display: " << window.GetDisplay() << "\n";
        std::cout << "  Window: " << window.GetWindow() << "\n\n";
#endif

        // ==================================================
        // 2. INICIALIZAR GFX CON LA VENTANA
        // ==================================================
        std::cout << "Inicializando GFX...\n";
        Mantrax::GFXConfig gfxConfig;
        gfxConfig.clearColor = {0.1f, 0.2f, 0.3f, 1.0f};

#ifdef _WIN32
        Mantrax::GFX gfx(hInst, window.GetHWND(), gfxConfig);
#else
        Mantrax::GFX gfx(window.GetDisplay(), window.GetWindow(), gfxConfig);
#endif
        std::cout << "GFX inicializado correctamente\n\n";

        // ==================================================
        // 3. CREAR SHADER BÁSICO
        // ==================================================
        std::cout << "Creando shader...\n";
        Mantrax::ShaderConfig shaderConfig;
        shaderConfig.vertexShaderPath = "shaders/basic.vert.spv";
        shaderConfig.fragmentShaderPath = "shaders/basic.frag.spv";
        shaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
        shaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();
        shaderConfig.cullMode = VK_CULL_MODE_NONE;
        shaderConfig.depthTestEnable = false;

        auto shader = gfx.CreateShader(shaderConfig);
        std::cout << "Shader creado\n\n";

        // ==================================================
        // 4. CREAR TRIÁNGULO
        // ==================================================
        std::cout << "Creando geometría...\n";
        std::vector<Mantrax::Vertex> vertices = {
            {{-0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Rojo
            {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},  // Verde
            {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}}  // Azul
        };

        std::vector<uint32_t> indices = {0, 1, 2};

        auto mesh = gfx.CreateMesh(vertices, indices);
        auto material = gfx.CreateMaterial(shader);
        std::cout << "Mesh y material creados\n\n";

        // ==================================================
        // 5. CONFIGURAR MATRICES MVP
        // ==================================================
        std::cout << "Configurando matrices MVP...\n";
        Mantrax::UniformBufferObject ubo{};

        MatrixIdentity(ubo.model);
        MatrixIdentity(ubo.view);
        MatrixIdentity(ubo.projection);

        gfx.UpdateMaterialUBO(material.get(), ubo);
        std::cout << "UBO actualizado\n\n";

        // ==================================================
        // 6. CREAR OBJETO DE RENDER
        // ==================================================
        Mantrax::RenderObject obj(mesh, material);
        gfx.AddRenderObject(obj);
        std::cout << "Objeto agregado a la escena\n\n";

        // ==================================================
        // 7. GAME LOOP PRINCIPAL
        // ==================================================
        std::cout << "Iniciando loop de renderizado...\n\n";

#ifdef _WIN32
        DWORD lastTime = GetTickCount();
#else
        auto lastTime = std::chrono::high_resolution_clock::now();
#endif
        bool running = true;

        while (running)
        {
            // Procesar mensajes de ventana
            if (!window.ProcessMessages())
            {
                running = false;
                break;
            }

            // Calcular deltaTime
#ifdef _WIN32
            DWORD now = GetTickCount();
            float delta = (now - lastTime) / 1000.0f;
            lastTime = now;
#else
            auto now = std::chrono::high_resolution_clock::now();
            float delta = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
#endif

            // Notificar a GFX si el framebuffer cambió de tamaño
            if (window.WasFramebufferResized())
            {
                gfx.NotifyFramebufferResized();
                window.ResetFramebufferResizedFlag();
            }

            // Dibujar el frame
            gfx.DrawFrame();
        }

        std::cout << "\nCerrando aplicación...\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "\n!!! ERROR !!!\n"
                  << e.what() << "\n";
#ifdef _WIN32
        MessageBoxA(nullptr, e.what(), "ERROR", MB_OK | MB_ICONERROR);
#endif
        return -1;
    }

    return 0;
}