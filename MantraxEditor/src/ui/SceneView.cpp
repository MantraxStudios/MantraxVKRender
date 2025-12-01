#include "../../includes/ui/SceneView.h"
#include <algorithm>
#include <cmath>

// Constructor
SceneView::SceneView()
    : lastWidth(width),
      lastHeight(height)
{
    isOpen = true;
}

// Render del viewport
void SceneView::OnRender()
{
    ImGui::Begin("View Port", &isOpen);

    // Obtener el área disponible en la ventana de ImGui
    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    // Calcular nuevo tamaño con límites mínimos
    int newWidth = static_cast<int>(availableSize.x);
    int newHeight = static_cast<int>(availableSize.y);

    newWidth = std::max(newWidth, MIN_WIDTH);
    newHeight = std::max(newHeight, MIN_HEIGHT);

    // Solo actualizar si el cambio es significativo (evita resize constante)
    if (std::abs(newWidth - width) > RESIZE_THRESHOLD ||
        std::abs(newHeight - height) > RESIZE_THRESHOLD)
    {
        width = newWidth;
        height = newHeight;
    }

    // Mostrar contenido del viewport
    if (HasValidRenderTarget())
    {
        // Renderizar la imagen del framebuffer offscreen
        ImVec2 imageSize(static_cast<float>(width), static_cast<float>(height));
        ImGui::Image(renderID, imageSize);
    }
    else
    {
        // Placeholder cuando no hay render target disponible
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No render target available");
        ImGui::Separator();
        ImGui::Text("Viewport Size: %dx%d", width, height);
        ImGui::Text("Aspect Ratio: %.3f", GetAspectRatio());

        // Mostrar estado de recursos
        ImGui::Spacing();
        ImGui::Text("Resources:");
        ImGui::BulletText("RenderID: %s", HasValidRenderTarget() ? "Valid" : "Invalid");
        ImGui::BulletText("Framebuffer: %s", HasValidFramebuffer() ? "Valid" : "Invalid");
    }

    ImGui::End();
}

// Obtener tamaño como ImVec2
ImVec2 SceneView::GetSize() const
{
    return ImVec2(static_cast<float>(width), static_cast<float>(height));
}

// Calcular aspect ratio
float SceneView::GetAspectRatio() const
{
    if (height <= 0)
        return 1.0f;
    return static_cast<float>(width) / static_cast<float>(height);
}

// Verificar si hubo resize
bool SceneView::CheckResize()
{
    if (width != lastWidth || height != lastHeight)
    {
        lastWidth = width;
        lastHeight = height;
        needsResize = true;
        return true;
    }
    return false;
}

// Verificar si el SceneView está completamente válido
bool SceneView::IsValid() const
{
    return HasValidRenderTarget() && HasValidFramebuffer();
}