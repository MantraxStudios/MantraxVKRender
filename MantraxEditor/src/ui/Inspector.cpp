#include "../../includes/ui/Inspector.h"
#include "../../MantraxECS/include/SceneManager.h"
#include "../../MantraxECS/include/ServiceLocator.h"
#include "../../MantraxECS/include/SceneRenderer.h"
#include "../../MantraxECS/include/MaterialManager.h"
#include "../../MantraxECS/include/ModelManager.h"
#include "../../includes/EngineLoader.h"
#include "../../includes/imgui/ImGuizmo.h"

#include <cstring>

Inspector::Inspector()
{
}

void Inspector::OnRender()
{
    ImGui::Begin("Inspector");

    Scene *activeScene = SceneManager::GetActiveScene();

    if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (activeScene)
        {
            ImGui::Text("Active Scene: %s", activeScene->GetName().c_str());

            if (ImGui::Button("New Scene", ImVec2(-1, 0)))
            {
                ImGui::OpenPopup("NewScenePopup");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No active scene");

            if (ImGui::Button("Create Initial Scene", ImVec2(-1, 0)))
            {
                ImGui::OpenPopup("NewScenePopup");
            }
        }

        if (ImGui::BeginPopup("NewScenePopup"))
        {
            static char sceneNameBuffer[128] = "";

            static bool isFirstOpen = true;
            if (isFirstOpen)
            {
#ifdef _WIN32
                strncpy_s(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
#else
                strncpy(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
                sceneNameBuffer[sizeof(sceneNameBuffer) - 1] = '\0';
#endif
                isFirstOpen = false;
            }

            ImGui::InputText("Scene Name", sceneNameBuffer, sizeof(sceneNameBuffer));

            if (ImGui::Button("Create", ImVec2(100, 0)))
            {
                if (strlen(sceneNameBuffer) > 0)
                {
                    Selection::selectedObjectIndex = -1;

                    SceneManager::CreateScene(sceneNameBuffer);
                }

#ifdef _WIN32
                strncpy_s(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
#else
                strncpy(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
                sceneNameBuffer[sizeof(sceneNameBuffer) - 1] = '\0';
#endif
                isFirstOpen = true;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
#ifdef _WIN32
                strncpy_s(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
#else
                strncpy(sceneNameBuffer, "New Scene", sizeof(sceneNameBuffer) - 1);
                sceneNameBuffer[sizeof(sceneNameBuffer) - 1] = '\0';
#endif
                isFirstOpen = true;

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::Separator();

    if (!activeScene)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "No active scene");
        ImGui::Text("Create a scene to get started");
        ImGui::End();
        return;
    }

    const auto &sceneObjects = activeScene->GetEntityObjects();
    auto &world = activeScene->GetWorld();

    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Objects in scene: %zu", sceneObjects.size());

        if (ImGui::Button("Create Object", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("CreateObjectPopup");
        }

        if (ImGui::BeginPopup("CreateObjectPopup"))
        {
            static char objectNameBuffer[128] = "GameObject";
            ImGui::InputText("Object Name", objectNameBuffer, sizeof(objectNameBuffer));

            if (ImGui::Button("Create", ImVec2(100, 0)))
            {
                if (strlen(objectNameBuffer) > 0)
                {
                    EntityObject newObj = activeScene->CreateEntityObject(objectNameBuffer);
                    newObj.AddComponent<Transform>();
                }

#ifdef _WIN32
                strncpy_s(objectNameBuffer, "GameObject", sizeof(objectNameBuffer) - 1);
#else
                strncpy(objectNameBuffer, "GameObject", sizeof(objectNameBuffer) - 1);
                objectNameBuffer[sizeof(objectNameBuffer) - 1] = '\0';
#endif

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (!sceneObjects.empty())
        {
            ImGui::BeginChild("ObjectList", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);

            for (size_t i = 0; i < sceneObjects.size(); i++)
            {
                ImGui::PushID("ObjectHierarchy" + i);
                const auto &obj = sceneObjects[i];

                bool isSelected = (Selection::selectedObjectIndex == static_cast<int>(i));

                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.6f, 0.9f, 1.0f));
                }

                std::string displayName = obj.name;
                if (displayName.empty())
                    displayName = "Unnamed Object";

                if (ImGui::Selectable(displayName.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    Selection::selectedObjectIndex = static_cast<int>(i);
                }

                if (isSelected)
                {
                    ImGui::PopStyleColor(2);
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", displayName.c_str());
                    ImGui::Text("Entity ID: %u", obj.GetEntity().id);
                    ImGui::Text("Index: %u | Gen: %u", obj.GetEntity().index(), obj.GetEntity().generation());

                    ImGui::Separator();
                    ImGui::Text("Components:");
                    if (world.getComponent<Transform>(obj.GetEntity()))
                        ImGui::BulletText("Transform");
                    if (world.getComponent<Rotator>(obj.GetEntity()))
                        ImGui::BulletText("Rotator");
                    if (world.getComponent<Velocity>(obj.GetEntity()))
                        ImGui::BulletText("Velocity");
                    if (world.getComponent<RenderComponent>(obj.GetEntity()))
                        ImGui::BulletText("RenderComponent");

                    ImGui::EndTooltip();
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete", "Del"))
                    {
                        Selection::selectedObjectIndex = static_cast<int>(i);
                        ImGui::OpenPopup("DeleteConfirmPopup");
                    }

                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }

            ImGui::EndChild();

            if (Selection::selectedObjectIndex >= 0)
            {
                if (ImGui::Button("Clear Selection", ImVec2(-1, 0)))
                {
                    Selection::selectedObjectIndex = -1;
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No objects in scene");
            ImGui::Text("Create an object to get started");
        }
    }

    ImGui::Separator();

    if (Selection::selectedObjectIndex >= 0 && Selection::selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        const auto &selectedObj = sceneObjects[Selection::selectedObjectIndex];

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
        ImGui::Text("Selected: %s", selectedObj.name.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

        if (ImGui::Button("Delete", ImVec2(100, 0)))
        {
            ImGui::OpenPopup("DeleteConfirmPopup");
        }

        ImGui::PopStyleColor(2);

        if (ImGui::BeginPopupModal("DeleteConfirmPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete '%s'?", selectedObj.name.c_str());
            ImGui::Separator();

            if (ImGui::Button("Yes, Delete", ImVec2(120, 0)))
            {
                EntityObject objToDelete = selectedObj;
                activeScene->DestroyEntityObject(objToDelete);

                Selection::selectedObjectIndex = -1;

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Components");
        ImGui::Separator();

        ecs::Entity entity = selectedObj.GetEntity();

        if (auto *transform = world.getComponent<Transform>(entity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Position
                glm::vec3 pos = transform->GetPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                {
                    transform->SetPosition(pos);
                }

                // Rotation (mostrar como Euler angles en grados)
                glm::vec3 eulerDegrees = transform->GetEulerAnglesDegrees();
                if (ImGui::DragFloat3("Rotation", &eulerDegrees.x, 1.0f))
                {
                    transform->SetRotationFromEulerDegrees(eulerDegrees);
                }

                // Scale
                glm::vec3 scale = transform->GetScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
                {
                    transform->SetScale(scale);
                }

                // Información adicional (opcional)
                if (ImGui::TreeNode("Advanced"))
                {
                    // Mostrar vectores direccionales (read-only)
                    glm::vec3 forward = transform->GetForward();
                    glm::vec3 right = transform->GetRight();
                    glm::vec3 up = transform->GetUp();

                    ImGui::Text("Forward: (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);
                    ImGui::Text("Right:   (%.2f, %.2f, %.2f)", right.x, right.y, right.z);
                    ImGui::Text("Up:      (%.2f, %.2f, %.2f)", up.x, up.y, up.z);

                    // Mostrar quaternion (read-only)
                    glm::quat rot = transform->GetRotation();
                    ImGui::Text("Quaternion: (%.2f, %.2f, %.2f, %.2f)", rot.w, rot.x, rot.y, rot.z);

                    ImGui::TreePop();
                }
            }
        }
        else
        {
            if (ImGui::Button("Add Transform"))
            {
                world.addComponent<Transform>(entity);
            }
        }

        if (auto *rotator = world.getComponent<Rotator>(entity))
        {
            if (ImGui::CollapsingHeader("Rotator"))
            {
                ImGui::DragFloat("Speed", &rotator->speed, 0.1f);
                ImGui::DragFloat3("Axis", &rotator->axis.x, 0.01f);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Remove Rotator"))
                {
                    world.removeComponent<Rotator>(entity);
                }
                ImGui::PopStyleColor();
            }
        }
        else
        {
            if (ImGui::Button("Add Rotator"))
            {
                world.addComponent<Rotator>(entity, Rotator{});
            }
        }

        if (auto *velocity = world.getComponent<Velocity>(entity))
        {
            if (ImGui::CollapsingHeader("Velocity"))
            {
                ImGui::DragFloat3("Linear", &velocity->linear.x, 0.1f);
                ImGui::DragFloat3("Angular", &velocity->angular.x, 0.1f);

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Remove Velocity"))
                {
                    world.removeComponent<Velocity>(entity);
                }
                ImGui::PopStyleColor();
            }
        }
        else
        {
            if (ImGui::Button("Add Velocity"))
            {
                world.addComponent<Velocity>(entity);
            }
        }

        if (auto *renderComp = world.getComponent<RenderComponent>(entity))
        {
            if (ImGui::CollapsingHeader("Render Component"))
            {
                ImGui::Text("Has Render Object: %s", renderComp->renderObject ? "Yes" : "No");

                if (renderComp->renderObject)
                {
                    ImGui::Text("Render Object Name: %s", renderComp->renderObject->name.c_str());
                }

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Remove Render Component"))
                {
                    auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");
                    sceneRenderer->RemoveObject(world.getComponent<RenderComponent>(entity)->renderObject);

                    world.removeComponent<RenderComponent>(entity);
                }
                ImGui::PopStyleColor();
            }
        }
        else
        {
            if (ImGui::Button("Add Render Component"))
            {
                auto materialManager = ServiceLocator::instance().get<Mantrax::MaterialManager>("MaterialManager");
                auto sceneRenderer = ServiceLocator::instance().get<SceneRenderer>("SceneRenderer");
                auto loader = ServiceLocator::instance().get<EngineLoader>("EngineLoader");
                std::shared_ptr<ModelManager> modelManager = ServiceLocator::instance().get<ModelManager>("ModelManager");

                if (!modelManager)
                {
                    std::cerr << "Error: ModelManager not available" << std::endl;
                }
                else if (!materialManager)
                {
                    std::cerr << "Error: MaterialManager not available" << std::endl;
                }
                else if (!sceneRenderer)
                {
                    std::cerr << "Error: SceneRenderer not available" << std::endl;
                }
                else
                {
                    // Crear el modelo (mesh + renderObject)
                    auto renderObject = modelManager->CreateModelOnly("Plane.fbx", selectedObj.name + "_Model");

                    if (!renderObject)
                    {
                        std::cerr << "Error: Failed to create model" << std::endl;
                    }
                    else
                    {
                        // Obtener el material (puede ser compartido entre múltiples objetos)
                        auto material = materialManager->GetMaterial("GroundMaterial");

                        if (material)
                        {
                            // Asignar el mismo material (esto es válido y eficiente)
                            renderObject->material = material;
                            renderObject->renderObj.material = material;
                        }
                        else
                        {
                            std::cerr << "Warning: Material 'GroundMaterial' not found" << std::endl;
                        }

                        sceneRenderer->AddObject(renderObject);

                        world.addComponent<RenderComponent>(entity);

                        auto *renderComp = world.getComponent<RenderComponent>(entity);
                        if (renderComp)
                        {
                            renderComp->renderObject = renderObject;
                        }
                    }
                }
            }

            // Popup de error (opcional, colócalo después del botón)
            if (ImGui::BeginPopupModal("RenderComponentError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Failed to add Render Component");
                ImGui::Separator();

                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No object selected");
        ImGui::Text("Select an object from the Hierarchy to edit");
    }

    ImGui::End();
}
