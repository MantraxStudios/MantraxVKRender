// ========================================
// MenuBar.h
// ========================================
#pragma once
#include "../imgui/imgui.h"
#include "../UIBehaviour.h"

class MenuBar : public UIBehaviour
{
private:
    bool *showContentBrowser = nullptr;
    bool *showWorldOutliner = nullptr;
    bool *showDetails = nullptr;
    bool *showToolbar = nullptr;
    bool *isRunning = nullptr;

public:
    MenuBar();
    ~MenuBar() = default;

    void OnRender() override;

    // Setters para control de ventanas
    void SetContentBrowserToggle(bool *toggle) { showContentBrowser = toggle; }
    void SetWorldOutlinerToggle(bool *toggle) { showWorldOutliner = toggle; }
    void SetDetailsToggle(bool *toggle) { showDetails = toggle; }
    void SetToolbarToggle(bool *toggle) { showToolbar = toggle; }
    void SetRunningFlag(bool *running) { isRunning = running; }

private:
    void RenderFileMenu();
    void RenderEditMenu();
    void RenderWindowMenu();
    void RenderHelpMenu();
};