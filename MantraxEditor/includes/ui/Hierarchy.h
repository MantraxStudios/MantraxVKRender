#pragma once
#include "../../../MantraxECS/include/ecs.h"
#include "../../../MantraxECS/include/SceneRenderer.h"
#include "../includes/Components.h"
#include "../imgui/imgui.h"
#include "../UIBehaviour.h"
#include "../Selection.h"

class Hierarchy : public UIBehaviour
{
public:
    Hierarchy();
    void OnRender() override;
};