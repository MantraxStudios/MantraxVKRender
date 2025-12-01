#pragma once
#include "../UIBehaviour.h"
#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "../../MantraxAddons/include/imgui/imgui.h"
#include "../../MantraxAddons/include/imgui/imgui_impl_win32.h"
#include "../../MantraxAddons/include/imgui/imgui_impl_vulkan.h"
#include <memory>

class SceneView : public UIBehaviour
{
public:
    // Vulkan resources
    VkDescriptorSet renderID = VK_NULL_HANDLE;
    std::shared_ptr<Mantrax::OffscreenFramebuffer> framebuffer = nullptr;

    // Dimensiones actuales
    int width = 1024;
    int height = 768;

private:
    // Control de resize
    int lastWidth = 0;
    int lastHeight = 0;
    bool needsResize = false;

    // Constantes
    static const int MIN_WIDTH = 64;
    static const int MIN_HEIGHT = 64;
    static const int RESIZE_THRESHOLD = 1;

public:
    // Constructor
    SceneView();

    // Destructor
    ~SceneView() = default;

    // Override de UIBehaviour
    void OnRender() override;

    // Getters de dimensiones
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    ImVec2 GetSize() const;
    float GetAspectRatio() const;

    // Control de resize
    bool CheckResize();
    bool NeedsResize() const { return needsResize; }
    void ResetResizeFlag() { needsResize = false; }

    // Validación
    bool IsValid() const;
    bool HasValidRenderTarget() const { return renderID != VK_NULL_HANDLE; }
    bool HasValidFramebuffer() const { return framebuffer != nullptr; }
};