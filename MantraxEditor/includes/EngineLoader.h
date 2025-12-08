#pragma once
#include "../../MantraxRender/include/MainWinPlug.h"
#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "../../MantraxECS/include/IService.h"
#include <memory>

class EngineLoader : public IService
{
public:
    Mantrax::WindowConfig windowConfig;

    std::unique_ptr<Mantrax::WindowMainPlug> window;
    std::unique_ptr<Mantrax::GFX> gfx;

    Mantrax::ShaderConfig normalShaderConfig;
    Mantrax::ShaderConfig outlineShaderConfig;

    Mantrax::ShaderConfig skyboxShaderConfig;
    std::shared_ptr<Mantrax::Shader> normalShader;
    std::shared_ptr<Mantrax::Shader> skyboxShader;
    std::shared_ptr<Mantrax::Shader> outlineShader;

    std::string getName() override { return "EngineLoader"; }

    void Start(HINSTANCE hInst, const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> &proc);
    void Render(const std::function<void()> &renderLambda);
};
