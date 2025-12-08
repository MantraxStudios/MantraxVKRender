#include "../includes/EngineLoader.h"

void EngineLoader::Start(HINSTANCE hInst, const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> &proc)
{
    windowConfig.width = 1920;
    windowConfig.height = 1080;
    windowConfig.title = "Mantrax Engine - Toon Outline";
    windowConfig.resizable = true;

    // Crear ventana
    window = std::make_unique<Mantrax::WindowMainPlug>(hInst, windowConfig);
    window->SetCustomWndProc(proc);

    // Configuración GFX
    Mantrax::GFXConfig gfxConfig;
    gfxConfig.clearColor = {0.1f, 0.1f, 0.1f, 1.0f};

    // Crear GFX
    gfx = std::make_unique<Mantrax::GFX>(hInst, window->GetHWND(), gfxConfig);

    // SHADER PARA OBJETOS OPACOS
    normalShaderConfig.vertexShaderPath = "shaders/simple.vert.spv";
    normalShaderConfig.fragmentShaderPath = "shaders/simple.frag.spv";
    normalShaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
    normalShaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();

    normalShaderConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    normalShaderConfig.polygonMode = VK_POLYGON_MODE_FILL;
    normalShaderConfig.cullMode = VK_CULL_MODE_BACK_BIT;    // ✅ O NONE si ves caras faltantes
    normalShaderConfig.frontFace = VK_FRONT_FACE_CLOCKWISE; // ✅ Probar con CLOCKWISE si se ve al revés

    normalShaderConfig.depthTestEnable = true;
    normalShaderConfig.depthWriteEnable = false; // ✅ TRUE para opacos
    normalShaderConfig.depthCompareOp = VK_COMPARE_OP_LESS;
    normalShaderConfig.blendEnable = true; // ✅ FALSE para opacos

    normalShader = gfx->CreateShader(normalShaderConfig);

    skyboxShaderConfig.vertexShaderPath = "shaders/skybox.vert.spv";
    skyboxShaderConfig.fragmentShaderPath = "shaders/skybox.frag.spv";
    skyboxShaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
    skyboxShaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();

    skyboxShaderConfig.depthTestEnable = true;                       // Testear depth
    skyboxShaderConfig.depthWriteEnable = false;                     // ❌ NO escribir en depth
    skyboxShaderConfig.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Permitir depth = 1.0
    skyboxShaderConfig.cullMode = VK_CULL_MODE_FRONT_BIT;            // Cull frontal (interior de esfera)
    skyboxShaderConfig.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    skyboxShaderConfig.polygonMode = VK_POLYGON_MODE_FILL;
    skyboxShaderConfig.blendEnable = false;

    skyboxShader = gfx->CreateShader(skyboxShaderConfig);

    // --- Outline shader ---
    outlineShaderConfig.vertexShaderPath = "shaders/outline.vert.spv";
    outlineShaderConfig.fragmentShaderPath = "shaders/outline.frag.spv";
    outlineShaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
    outlineShaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();
    outlineShaderConfig.cullMode = VK_CULL_MODE_FRONT_BIT;
    outlineShaderConfig.depthTestEnable = true;
    outlineShader = gfx->CreateShader(outlineShaderConfig);
}

void EngineLoader::Render(const std::function<void()> &renderLambda)
{
    renderLambda();
}
