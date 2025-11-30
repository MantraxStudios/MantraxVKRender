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
    gfxConfig.clearColor = {0.53f, 0.81f, 0.92f, 1.0f};

    // Crear GFX
    gfx = std::make_unique<Mantrax::GFX>(hInst, window->GetHWND(), gfxConfig);

    // --- Normal shader ---
    normalShaderConfig.vertexShaderPath = "shaders/pbr.vert.spv";
    normalShaderConfig.fragmentShaderPath = "shaders/pbr.frag.spv";
    normalShaderConfig.vertexBinding = Mantrax::Vertex::GetBindingDescription();
    normalShaderConfig.vertexAttributes = Mantrax::Vertex::GetAttributeDescriptions();
    normalShaderConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    normalShaderConfig.depthTestEnable = true;
    normalShader = gfx->CreateShader(normalShaderConfig);

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
