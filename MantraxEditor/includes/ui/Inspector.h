// ========================================
// Inspector.h - Totalmente Automático
// ========================================
#pragma once
#include "../../../MantraxECS/include/ecs.h"
#include "../../../MantraxECS/include/SceneRenderer.h"
#include "../includes/Components.h"
#include "../imgui/imgui.h"
#include "../UIBehaviour.h"

class Inspector : public UIBehaviour
{
public:
    Inspector();
    void OnRender() override;

private:
    // Solo necesitamos el índice de selección, todo lo demás viene de SceneManager
    int selectedObjectIndex = -1;
};