#include "../render/include/MainWinPlug.h"
#include "../render/include/MantraxGFX_API.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// ============================================================================
// CREAR GEOMETRÍA DE CUBO
// ============================================================================

void CreateCube(std::vector<Mantrax::Vertex> &vertices, std::vector<uint32_t> &indices)
{
    float s = 0.5f;

    std::vector<Mantrax::Vertex> cubeVertices = {
        // Cara frontal (Z+)
        {{-s, -s, s}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{s, -s, s}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{s, s, s}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-s, s, s}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

        // Cara trasera (Z-)
        {{s, -s, -s}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{-s, -s, -s}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{-s, s, -s}, {1.0f, 0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
        {{s, s, -s}, {0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

        // Cara superior (Y+)
        {{-s, s, s}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{s, s, s}, {0.8f, 0.8f, 0.2f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{s, s, -s}, {0.2f, 0.8f, 0.8f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-s, s, -s}, {0.8f, 0.2f, 0.8f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},

        // Cara inferior (Y-)
        {{-s, -s, -s}, {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{s, -s, -s}, {0.3f, 0.3f, 0.7f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
        {{s, -s, s}, {0.7f, 0.3f, 0.3f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
        {{-s, -s, s}, {0.3f, 0.7f, 0.3f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},

        // Cara derecha (X+)
        {{s, -s, s}, {1.0f, 0.5f, 0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{s, -s, -s}, {0.5f, 1.0f, 0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{s, s, -s}, {0.5f, 0.5f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
        {{s, s, s}, {1.0f, 1.0f, 0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

        // Cara izquierda (X-)
        {{-s, -s, -s}, {0.5f, 1.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-s, -s, s}, {1.0f, 0.5f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-s, s, s}, {1.0f, 1.0f, 0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
        {{-s, s, -s}, {0.5f, 1.0f, 0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
    };

    std::vector<uint32_t> cubeIndices = {
        0, 1, 2, 2, 3, 0,       // Frontal
        4, 5, 6, 6, 7, 4,       // Trasera
        8, 9, 10, 10, 11, 8,    // Superior
        12, 13, 14, 14, 15, 12, // Inferior
        16, 17, 18, 18, 19, 16, // Derecha
        20, 21, 22, 22, 23, 20  // Izquierda
    };

    vertices = cubeVertices;
    indices = cubeIndices;
}

// Helper para copiar glm::mat4 a float array
void CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

// Helper para imprimir matrices GLM
void PrintMatrix(const char *name, const glm::mat4 &mat)
{
    std::cout << name << ":\n";
    for (int i = 0; i < 4; i++)
    {
        std::cout << "  ";
        for (int j = 0; j < 4; j++)
        {
            printf("%8.3f ", mat[j][i]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
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
        std::cout << "=== Mantrax Engine - Cubo Giratorio PBR ===\n\n";

        std::cout << "Creando ventana...\n";
        Mantrax::WindowConfig windowConfig;
        windowConfig.width = 1280;
        windowConfig.height = 720;
        windowConfig.title = "Mantrax Engine - Rotating Cube PBR";
        windowConfig.resizable = true;

        Mantrax::WindowMainPlug window(hInst, windowConfig);
        std::cout << "Ventana creada\n\n";

        std::cout << "Inicializando GFX...\n";
        Mantrax::GFXConfig gfxConfig;
        gfxConfig.clearColor = {0.05f, 0.05f, 0.1f, 1.0f};

#ifdef _WIN32
        Mantrax::GFX gfx(hInst, window.GetHWND(), gfxConfig);
#else
        Mantrax::GFX gfx(window.GetDisplay(), window.GetWindow(), gfxConfig);
#endif
        std::cout << "GFX inicializado\n\n";

        std::cout << "Creando shader PBR...\n";
        Mantrax::ShaderConfig shaderConfig;
        shaderConfig.vertexShaderPath = "shaders/pbr.vert.spv";
        shaderConfig.fragmentShaderPath = "shaders/pbr.frag.spv";
        shaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
        shaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();
        shaderConfig.cullMode = VK_CULL_MODE_NONE; // Desactivado para debug
        shaderConfig.depthTestEnable = false;      // Desactivado temporalmente

        auto shader = gfx.CreateShader(shaderConfig);
        std::cout << "Shader PBR creado\n\n";

        std::cout << "Creando cubo...\n";
        std::vector<Mantrax::Vertex> vertices;
        std::vector<uint32_t> indices;
        CreateCube(vertices, indices);

        auto mesh = gfx.CreateMesh(vertices, indices);
        auto material = gfx.CreateMaterial(shader);
        std::cout << "Cubo creado con " << vertices.size() << " vértices y "
                  << indices.size() << " índices\n\n";

        std::cout << "Configurando cámara con GLM...\n";
        Mantrax::UniformBufferObject ubo{};

        // Usar GLM para las matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f), // Posición de la cámara
            glm::vec3(0.0f, 0.0f, 0.0f), // Punto al que mira
            glm::vec3(0.0f, 1.0f, 0.0f)  // Vector UP
        );

        float aspect = 1280.0f / 720.0f;
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), // FOV
            aspect,              // Aspect ratio
            0.1f,                // Near plane
            100.0f               // Far plane
        );

        // Corrección para Vulkan (invertir Y del clip space)
        projection[1][1] *= -1;

        // Copiar a UBO
        CopyMat4(ubo.model, model);
        CopyMat4(ubo.view, view);
        CopyMat4(ubo.projection, projection);

        // DEBUG: Imprimir matrices
        PrintMatrix("Model (GLM)", model);
        PrintMatrix("View (GLM)", view);
        PrintMatrix("Projection (GLM)", projection);

        gfx.UpdateMaterialUBO(material.get(), ubo);
        std::cout << "UBO inicial actualizado\n\n";

        Mantrax::RenderObject obj(mesh, material);
        gfx.AddRenderObject(obj);

        std::cout << "Objeto añadido a la escena\n";
        std::cout << "Iniciando loop de renderizado...\n\n";

#ifdef _WIN32
        DWORD lastTime = GetTickCount();
#else
        auto lastTime = std::chrono::high_resolution_clock::now();
#endif

        float totalTime = 0.0f;
        bool running = true;
        int frameCount = 0;

        while (running)
        {
            if (!window.ProcessMessages())
            {
                running = false;
                break;
            }

#ifdef _WIN32
            DWORD now = GetTickCount();
            float delta = (now - lastTime) / 1000.0f;
            lastTime = now;
#else
            auto now = std::chrono::high_resolution_clock::now();
            float delta = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
#endif

            totalTime += delta;
            frameCount++;

            // DEBUG: Imprimir info cada 60 frames
            if (frameCount % 60 == 0)
            {
                std::cout << "Frame " << frameCount << " - Time: " << totalTime << "s\n";
            }

            // Rotar el cubo usando GLM
            model = glm::rotate(glm::mat4(1.0f), totalTime * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, totalTime * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

            CopyMat4(ubo.model, model);

            // Actualizar aspecto si cambió el tamaño
            if (window.WasFramebufferResized())
            {
                uint32_t w, h;
                window.GetWindowSize(w, h);
                float newAspect = (float)w / (float)h;

                projection = glm::perspective(glm::radians(45.0f), newAspect, 0.1f, 100.0f);
                projection[1][1] *= -1; // Corrección Vulkan

                CopyMat4(ubo.projection, projection);

                gfx.NotifyFramebufferResized();
                window.ResetFramebufferResizedFlag();
                std::cout << "Ventana redimensionada: " << w << "x" << h << "\n";
            }

            // Actualizar UBO cada frame
            gfx.UpdateMaterialUBO(material.get(), ubo);

            // Dibujar
            gfx.DrawFrame();
        }

        std::cout << "\nTotal de frames renderizados: " << frameCount << "\n";
        std::cout << "Cerrando aplicación...\n";
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