// ========================================
// Inspector.h - Totalmente Automático
// ========================================
#pragma once
#include "../../../MantraxECS/include/ecs.h"
#include "../../../MantraxECS/include/SceneRenderer.h"
#include "../includes/Components.h"
#include "../imgui/imgui.h"
#include "../UIBehaviour.h"
#include "../Selection.h"

class Inspector : public UIBehaviour
{
public:
    Inspector();
    void OnRender() override;
};