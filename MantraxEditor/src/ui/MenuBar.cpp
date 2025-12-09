// ========================================
// MenuBar.cpp
// ========================================
#include "../../includes/ui/MenuBar.h"
#include <iostream>

MenuBar::MenuBar()
{
}

void MenuBar::OnRender()
{
    if (ImGui::BeginMainMenuBar())
    {
        RenderFileMenu();
        RenderEditMenu();
        RenderWindowMenu();
        RenderHelpMenu();

        ImGui::EndMainMenuBar();
    }
}

void MenuBar::RenderFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New Level", "Ctrl+N"))
        {
            std::cout << "New Level clicked\n";
        }

        if (ImGui::MenuItem("Open Level", "Ctrl+O"))
        {
            std::cout << "Open Level clicked\n";
        }

        if (ImGui::MenuItem("Save", "Ctrl+S"))
        {
            std::cout << "Save clicked\n";
        }

        if (ImGui::MenuItem("Save As..."))
        {
            std::cout << "Save As clicked\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
            if (isRunning)
                *isRunning = false;
        }

        ImGui::EndMenu();
    }
}

void MenuBar::RenderEditMenu()
{
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z"))
        {
            std::cout << "Undo clicked\n";
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Y"))
        {
            std::cout << "Redo clicked\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Cut", "Ctrl+X"))
        {
            std::cout << "Cut clicked\n";
        }

        if (ImGui::MenuItem("Copy", "Ctrl+C"))
        {
            std::cout << "Copy clicked\n";
        }

        if (ImGui::MenuItem("Paste", "Ctrl+V"))
        {
            std::cout << "Paste clicked\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete", "Del"))
        {
            std::cout << "Delete clicked\n";
        }

        ImGui::EndMenu();
    }
}

void MenuBar::RenderWindowMenu()
{
    if (ImGui::BeginMenu("Window"))
    {
        if (showContentBrowser)
            ImGui::MenuItem("Content Browser", nullptr, showContentBrowser);

        if (showWorldOutliner)
            ImGui::MenuItem("World Outliner", nullptr, showWorldOutliner);

        if (showDetails)
            ImGui::MenuItem("Details", nullptr, showDetails);

        if (showToolbar)
            ImGui::MenuItem("Toolbar", nullptr, showToolbar);

        ImGui::EndMenu();
    }
}

void MenuBar::RenderHelpMenu()
{
    if (ImGui::BeginMenu("Help"))
    {
        if (ImGui::MenuItem("Documentation"))
        {
            std::cout << "Documentation clicked\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("About"))
        {
            std::cout << "About clicked\n";
        }

        ImGui::EndMenu();
    }
}