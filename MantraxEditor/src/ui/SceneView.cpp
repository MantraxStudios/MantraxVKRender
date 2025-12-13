#include "../../includes/ui/SceneView.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/type_ptr.hpp>

#include <SceneRenderer.h>
#include <ServiceLocator.h>

#include "../../includes/Components.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
SceneView::SceneView()
    : lastWidth(width),
      lastHeight(height),
      isHovered(false),
      isFocused(false),
      gizmoOperation(ImGuizmo::TRANSLATE),
      gizmoMode(ImGuizmo::WORLD)
{
    isOpen = true;
}

// ------------------------------------------------------------
// Render del Viewport
// ------------------------------------------------------------
void SceneView::OnRender()
{
    ImGui::Begin("View Port", &isOpen);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist();

    // Tamaño disponible
    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    int newWidth = std::max(static_cast<int>(availableSize.x), MIN_WIDTH);
    int newHeight = std::max(static_cast<int>(availableSize.y), MIN_HEIGHT);

    if (std::abs(newWidth - width) > RESIZE_THRESHOLD ||
        std::abs(newHeight - height) > RESIZE_THRESHOLD)
    {
        width = newWidth;
        height = newHeight;
    }

    // --------------------------------------------------------
    // Render Target
    // --------------------------------------------------------
    if (HasValidRenderTarget())
    {
        ImVec2 imageSize((float)width, (float)height);
        ImGui::Image(renderID, imageSize);

        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageMax = ImGui::GetItemRectMax();

        auto sceneRenderer =
            ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");

        if (sceneRenderer && sceneRenderer->camera)
        {
            RenderGizmo(
                sceneRenderer->camera->GetViewMatrix(),
                sceneRenderer->camera->GetProjectionMatrixLegacy(),
                imageMin,
                imageMax);
        }

        isHovered = ImGui::IsItemHovered();
        isFocused = ImGui::IsWindowFocused();
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                           "No render target available");

        isHovered = false;
        isFocused = false;
    }

    ImGui::End();
}

// ------------------------------------------------------------
// Render Gizmo (ImGuizmo)
// ------------------------------------------------------------
void SceneView::RenderGizmo(
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const ImVec2 &imageMin,
    const ImVec2 &imageMax)
{
    if (!HasValidRenderTarget())
        return;

    // ✔ SOLO USAR SELECTION
    if (!Selection::hasSelection || !Selection::objectSelected->IsValid())
        return;

    EntityObject &selectedObj = *Selection::objectSelected;

    Transform *transform = selectedObj.GetComponent<Transform>();
    if (!transform)
        return;

    ImGuizmo::SetOrthographic(false);

    glm::mat4 worldMatrix = transform->GetWorldMatrix();

    ImGuizmo::SetRect(
        imageMin.x,
        imageMin.y,
        imageMax.x - imageMin.x,
        imageMax.y - imageMin.y);

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        gizmoOperation,
        gizmoMode,
        glm::value_ptr(worldMatrix));

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 position, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(worldMatrix),
            glm::value_ptr(position),
            glm::value_ptr(rotation),
            glm::value_ptr(scale));

        transform->SetPosition(position);
        transform->SetRotationFromEulerDegrees(rotation);
        transform->SetScale(scale);
        transform->UpdateWorldMatrix();
    }
}

// ------------------------------------------------------------
// Gizmo API
// ------------------------------------------------------------
void SceneView::SetGizmoOperation(ImGuizmo::OPERATION operation)
{
    gizmoOperation = operation;
}

void SceneView::SetGizmoMode(ImGuizmo::MODE mode)
{
    gizmoMode = mode;
}

ImGuizmo::OPERATION SceneView::GetGizmoOperation() const
{
    return gizmoOperation;
}

ImGuizmo::MODE SceneView::GetGizmoMode() const
{
    return gizmoMode;
}

bool SceneView::IsUsingGizmo() const
{
    return ImGuizmo::IsUsing();
}

// ------------------------------------------------------------
// Viewport utils
// ------------------------------------------------------------
ImVec2 SceneView::GetSize() const
{
    return ImVec2((float)width, (float)height);
}

float SceneView::GetAspectRatio() const
{
    return (height > 0) ? (float)width / (float)height : 1.0f;
}

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

bool SceneView::IsValid() const
{
    return HasValidRenderTarget() && HasValidFramebuffer();
}
