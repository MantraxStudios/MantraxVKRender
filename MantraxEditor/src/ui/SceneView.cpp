#include "../../includes/ui/SceneView.h"
#include <algorithm>
#include <cmath>
#include <SceneRenderer.h>
#include <ServiceLocator.h>
#include "../includes/EngineLoader.h"
#include "../../includes/Components.h"

// Constructor
SceneView::SceneView()
    : lastWidth(width),
      lastHeight(height),
      isHovered(false),
      isFocused(false),
      gizmoOperation(ImGuizmo::TRANSLATE), // Inicializar modo del gizmo
      gizmoMode(ImGuizmo::WORLD)           // Inicializar espacio del gizmo
{
    isOpen = true;
}

// Render del viewport
void SceneView::OnRender()
{
    ImGui::Begin("View Port", &isOpen);
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist();

    // Obtener el área disponible en la ventana de ImGui
    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    // Calcular nuevo tamaño con límites mínimos
    int newWidth = static_cast<int>(availableSize.x);
    int newHeight = static_cast<int>(availableSize.y);

    newWidth = std::max(newWidth, MIN_WIDTH);
    newHeight = std::max(newHeight, MIN_HEIGHT);

    // Solo actualizar si el cambio es significativo
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

        // Guardar rect exacto del viewport
        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageMax = ImGui::GetItemRectMax();

        auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");
        RenderGizmo(sceneRenderer->camera->GetViewMatrix(),
                    sceneRenderer->camera->GetProjectionMatrixLegacy(),
                    imageMin,
                    imageMax);

        // IMPORTANTE: Detectar si el mouse está sobre la imagen del viewport
        isHovered = ImGui::IsItemHovered();

        // Detectar si la ventana tiene foco
        isFocused = ImGui::IsWindowFocused();
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

        isHovered = false;
        isFocused = false;
    }

    ImGui::End();
}

// Método para renderizar el gizmo
void SceneView::RenderGizmo(
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const ImVec2 &imageMin,
    const ImVec2 &imageMax)
{
    if (!HasValidRenderTarget())
        return;

    // Verificar si hay un objeto seleccionado
    if (Selection::selectedObjectIndex == -1)
        return;

    Scene *activeScene = SceneManager::GetActiveScene();
    if (!activeScene)
        return;

    // IMPORTANTE: No usar const& aquí
    auto &entityObjects = activeScene->GetEntityObjects();
    if (Selection::selectedObjectIndex >= entityObjects.size())
        return;

    // IMPORTANTE: No usar const& aquí tampoco
    EntityObject &selectedObj = entityObjects[Selection::selectedObjectIndex];

    // Ahora GetComponent funcionará correctamente
    auto transformComponent = selectedObj.GetComponent<Transform>();
    if (!transformComponent)
        return;

    ImGuizmo::SetOrthographic(false);

    glm::mat4 transform = transformComponent->GetWorldMatrix();

    ImGuizmo::SetRect(
        imageMin.x,
        imageMin.y,
        imageMax.x - imageMin.x,
        imageMax.y - imageMin.y);

    // Manipular el gizmo
    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        gizmoOperation,
        gizmoMode,
        glm::value_ptr(transform));

    // Si el gizmo está siendo usado, actualizar el transform del objeto
    if (ImGuizmo::IsUsing())
    {
        // Descomponer la matriz transformada
        glm::vec3 position, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(transform),
            glm::value_ptr(position),
            glm::value_ptr(rotation),
            glm::value_ptr(scale));

        // Actualizar el transform del objeto
        transformComponent->SetPosition(position);
        transformComponent->SetRotationFromEulerDegrees(rotation);
        transformComponent->SetScale(scale);

        // Marcar como dirty y actualizar la matriz world
        transformComponent->UpdateWorldMatrix();
    }
}

// Método para cambiar el modo del gizmo (para llamar desde tu UI o con teclas)
void SceneView::SetGizmoOperation(ImGuizmo::OPERATION operation)
{
    gizmoOperation = operation;
}

// Método para cambiar el espacio del gizmo (World/Local)
void SceneView::SetGizmoMode(ImGuizmo::MODE mode)
{
    gizmoMode = mode;
}

// Método para obtener la operación actual
ImGuizmo::OPERATION SceneView::GetGizmoOperation() const
{
    return gizmoOperation;
}

// Método para obtener el modo actual
ImGuizmo::MODE SceneView::GetGizmoMode() const
{
    return gizmoMode;
}

// Método para verificar si el gizmo está siendo usado
bool SceneView::IsUsingGizmo() const
{
    return ImGuizmo::IsUsing();
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