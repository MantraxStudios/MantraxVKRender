#include "../../includes/ui/Inspector.h"
#include "../../MantraxECS/include/SceneManager.h"
#include "../../MantraxECS/include/ServiceLocator.h"
#include "../../MantraxECS/include/SceneRenderer.h"
#include "../../MantraxECS/include/MaterialManager.h"
#include "../../MantraxECS/include/ModelManager.h"
#include "../../includes/EngineLoader.h"

Inspector::Inspector() {}

void Inspector::OnRender()
{
    ImGui::Begin("Inspector");

    // ------------------------------------------------------------
    // NO SELECCIÓN
    // ------------------------------------------------------------
    if (!Selection::hasSelection || !Selection::objectSelected->IsValid())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No object selected");
        ImGui::Text("Select an object to edit its properties");
        ImGui::End();
        return;
    }

    EntityObject &selectedObj = *Selection::objectSelected;
    Scene *activeScene = SceneManager::GetActiveScene();

    if (!activeScene)
    {
        ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "No active scene");
        ImGui::End();
        return;
    }

    auto &world = activeScene->GetWorld();
    ecs::Entity entity = selectedObj.GetEntity();

    // ------------------------------------------------------------
    // HEADER
    // ------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
    ImGui::Text("%s", selectedObj.name.c_str());
    ImGui::PopStyleColor();

    ImGui::Text("Entity ID: %u", entity.id);
    ImGui::Separator();

    // ------------------------------------------------------------
    // DELETE OBJECT
    // ------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Delete Object", ImVec2(-1, 0)))
        ImGui::OpenPopup("DeleteObjectPopup");
    ImGui::PopStyleColor();

    if (ImGui::BeginPopupModal("DeleteObjectPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Delete '%s' ?", selectedObj.name.c_str());
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            activeScene->DestroyEntityObject(selectedObj);
            Selection::hasSelection = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Components");
    ImGui::Separator();

    // ------------------------------------------------------------
    // TRANSFORM
    // ------------------------------------------------------------
    if (auto *transform = world.getComponent<Transform>(entity))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 pos = transform->GetPosition();
            if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                transform->SetPosition(pos);

            glm::vec3 rot = transform->GetEulerAnglesDegrees();
            if (ImGui::DragFloat3("Rotation", &rot.x, 1.0f))
                transform->SetRotationFromEulerDegrees(rot);

            glm::vec3 scale = transform->GetScale();
            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
                transform->SetScale(scale);
        }
    }
    else
    {
        if (ImGui::Button("Add Transform"))
            world.addComponent<Transform>(entity);
    }

    // ------------------------------------------------------------
    // ROTATOR
    // ------------------------------------------------------------
    if (auto *rotator = world.getComponent<Rotator>(entity))
    {
        if (ImGui::CollapsingHeader("Rotator"))
        {
            ImGui::DragFloat("Speed", &rotator->speed, 0.1f);
            ImGui::DragFloat3("Axis", &rotator->axis.x, 0.01f);

            if (ImGui::Button("Remove Rotator"))
                world.removeComponent<Rotator>(entity);
        }
    }
    else
    {
        if (ImGui::Button("Add Rotator"))
            world.addComponent<Rotator>(entity);
    }

    // ------------------------------------------------------------
    // VELOCITY
    // ------------------------------------------------------------
    if (auto *velocity = world.getComponent<Velocity>(entity))
    {
        if (ImGui::CollapsingHeader("Velocity"))
        {
            ImGui::DragFloat3("Linear", &velocity->linear.x, 0.1f);
            ImGui::DragFloat3("Angular", &velocity->angular.x, 0.1f);

            if (ImGui::Button("Remove Velocity"))
                world.removeComponent<Velocity>(entity);
        }
    }
    else
    {
        if (ImGui::Button("Add Velocity"))
            world.addComponent<Velocity>(entity);
    }

    // ------------------------------------------------------------
    // RENDER COMPONENT
    // ------------------------------------------------------------
    if (auto *renderComp = world.getComponent<RenderComponent>(entity))
    {
        if (ImGui::CollapsingHeader("Render Component"))
        {
            ImGui::Text("Render Object: %s",
                        renderComp->renderObject
                            ? renderComp->renderObject->name.c_str()
                            : "None");

            if (ImGui::Button("Remove Render Component"))
            {
                auto sceneRenderer = ServiceLocator::instance()
                                         .get<SceneRenderer>("SceneRenderer");

                if (sceneRenderer && renderComp->renderObject)
                    sceneRenderer->RemoveObject(renderComp->renderObject);

                world.removeComponent<RenderComponent>(entity);
            }
        }
    }
    else
    {
        if (ImGui::Button("Add Render Component"))
        {
            auto loader = ServiceLocator::instance().get<EngineLoader>("EngineLoader");
            auto modelManager = ServiceLocator::instance().get<ModelManager>("ModelManager");
            auto materialManager = ServiceLocator::instance().get<Mantrax::MaterialManager>("MaterialManager");
            auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");

            if (!loader || !modelManager || !materialManager || !sceneRenderer)
                return;

            EntityObject &entityObj = selectedObj;

            if (entityObj.HasComponent<RenderComponent>())
                return;

            auto renderObject =
                modelManager->CreateModelOnly("Cube.fbx", entityObj.name + "_Model");

            if (!renderObject)
                return;

            auto material = materialManager->GetMaterial("GroundMaterial");
            if (!material)
            {
                std::cerr << "❌ GroundMaterial not found\n";
                return;
            }

            renderObject->material = material;
            renderObject->renderObj.material = material;

            if (renderObject->renderObj.mesh)
            {
                loader->gfx->CreateMeshDescriptorSet(
                    renderObject->renderObj.mesh,
                    material);
            }

            sceneRenderer->AddObject(renderObject);

            RenderComponent &renderComp =
                entityObj.AddComponent<RenderComponent>();

            renderComp.renderObject = renderObject;
        }
    }

    ImGui::End();
}
