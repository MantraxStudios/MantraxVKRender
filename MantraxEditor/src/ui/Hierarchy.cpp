#include "../../includes/ui/Hierarchy.h"

#include "../../MantraxECS/include/SceneManager.h"
#include "../../includes/Selection.h"

Hierarchy::Hierarchy() {}

void Hierarchy::OnRender()
{
    ImGui::Begin("Hierarchy");

    Scene *activeScene = SceneManager::GetActiveScene();
    if (!activeScene)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No active scene");
        ImGui::End();
        return;
    }

    auto &objects = activeScene->GetEntityObjects();

    if (objects.empty())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Scene has no objects");
        ImGui::End();
        return;
    }

    // ------------------------------------------------------------
    // Add Entity Button
    // ------------------------------------------------------------
    if (ImGui::Button("+ Add Entity", ImVec2(-1, 0)))
    {
        Scene *scene = SceneManager::GetActiveScene();
        if (scene)
        {
            static int entityCounter = 0;
            std::string name = "Entity_" + std::to_string(entityCounter++);

            EntityObject newEntity = scene->CreateEntityObject(name);

            Selection::objectSelected = &scene->GetEntityObjects().back();
            Selection::hasSelection = true;
        }
    }

    ImGui::Separator();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    int index = 0;
    for (auto &obj : objects)
    {
        if (!obj.IsValid())
            continue;

        bool isSelected =
            Selection::hasSelection &&
            Selection::objectSelected &&
            Selection::objectSelected->IsValid() &&
            Selection::objectSelected->GetEntity() == obj.GetEntity();

        // Alternating background color for more life
        if (index % 2 == 0)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        else
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_NoTreePushOnOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Fake icon + name
        std::string label = "\uf1b2  " + obj.name; // cube-like icon if using icon font

        ImGui::TreeNodeEx((void *)(intptr_t)obj.GetEntity().id, flags, "%s", label.c_str());

        if (ImGui::IsItemClicked())
        {
            // Click same object again -> clear selection
            if (isSelected)
            {
                Selection::hasSelection = false;
                Selection::objectSelected = nullptr;
            }
            else
            {
                Selection::objectSelected = &obj;
                Selection::hasSelection = true;
            }
        }

        ImGui::PopStyleColor(3);
        index++;
    }

    ImGui::PopStyleVar(2);

    ImGui::End();
}
