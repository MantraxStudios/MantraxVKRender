#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxAddons/include/imgui/imgui.h"
#include "../MantraxAddons/include/imgui/imgui_impl_win32.h"
#include "../MantraxAddons/include/imgui/imgui_impl_vulkan.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <commdlg.h>

class LuaEditor
{
private:
    char textBuffer[1024 * 64];
    char lineNumbersBuffer[1024 * 16];
    std::string currentFilePath;
    bool isModified = false;

    // IntelliSense
    bool showAutocomplete = false;
    std::vector<std::string> suggestions;
    int selectedSuggestion = 0;
    std::string currentWord;
    ImVec2 autocompletePos;
    int cursorPosForAutocomplete = 0;

    // Palabras clave de Lua
    std::vector<std::string> luaKeywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
        "if", "in", "local", "nil", "not", "or", "repeat", "return", "then",
        "true", "until", "while"};

    // Funciones built-in de Lua
    std::vector<std::string> luaBuiltins = {
        "print", "pairs", "ipairs", "type", "tonumber", "tostring", "require",
        "assert", "error", "pcall", "xpcall", "next", "select", "getmetatable",
        "setmetatable", "rawget", "rawset", "rawequal", "collectgarbage",
        "math", "string", "table", "abs", "acos", "asin", "atan", "ceil", "cos",
        "deg", "exp", "floor", "log", "max", "min", "rad", "random", "sin",
        "sqrt", "tan", "byte", "char", "find", "format", "gsub", "len", "lower",
        "match", "rep", "reverse", "sub", "upper", "concat", "insert", "remove", "sort"};

    // Colores para syntax highlighting
    ImVec4 colorKeyword = ImVec4(0.86f, 0.47f, 0.89f, 1.0f);  // Púrpura para keywords
    ImVec4 colorBuiltin = ImVec4(0.33f, 0.78f, 0.98f, 1.0f);  // Azul para builtins
    ImVec4 colorString = ImVec4(0.80f, 0.73f, 0.46f, 1.0f);   // Amarillo para strings
    ImVec4 colorNumber = ImVec4(0.68f, 0.85f, 0.57f, 1.0f);   // Verde claro para números
    ImVec4 colorComment = ImVec4(0.45f, 0.53f, 0.45f, 1.0f);  // Gris verde para comentarios
    ImVec4 colorOperator = ImVec4(0.86f, 0.86f, 0.86f, 1.0f); // Blanco para operadores
    ImVec4 colorDefault = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);  // Blanco para texto normal

    void UpdateLineNumbers()
    {
        int lineCount = CountLines();
        memset(lineNumbersBuffer, 0, sizeof(lineNumbersBuffer));

        char temp[32];
        for (int i = 1; i <= lineCount; i++)
        {
            snprintf(temp, sizeof(temp), "%d\n", i);
            strcat_s(lineNumbersBuffer, sizeof(lineNumbersBuffer), temp);
        }
    }

    bool IsKeyword(const std::string &word)
    {
        return std::find(luaKeywords.begin(), luaKeywords.end(), word) != luaKeywords.end();
    }

    bool IsBuiltin(const std::string &word)
    {
        return std::find(luaBuiltins.begin(), luaBuiltins.end(), word) != luaBuiltins.end();
    }

    std::string GetWordAtCursor(const char *text, int cursorPos)
    {
        int start = cursorPos;
        int end = cursorPos;

        while (start > 0 && (isalnum(text[start - 1]) || text[start - 1] == '_' || text[start - 1] == '.'))
            start--;

        while (text[end] != '\0' && (isalnum(text[end]) || text[end] == '_' || text[end] == '.'))
            end++;

        return std::string(text + start, end - start);
    }

    void UpdateAutocomplete(const std::string &word)
    {
        suggestions.clear();

        if (word.length() < 1)
        {
            showAutocomplete = false;
            return;
        }

        for (const auto &keyword : luaKeywords)
        {
            if (keyword.find(word) == 0 && keyword != word)
                suggestions.push_back(keyword);
        }

        for (const auto &builtin : luaBuiltins)
        {
            if (builtin.find(word) == 0 && builtin != word)
                suggestions.push_back(builtin);
        }

        showAutocomplete = !suggestions.empty();
        selectedSuggestion = 0;
    }

    void InsertSuggestion(ImGuiInputTextCallbackData *data)
    {
        if (suggestions.empty())
            return;

        std::string selected = suggestions[selectedSuggestion];

        // Encontrar el inicio de la palabra actual
        int wordStart = data->CursorPos;
        while (wordStart > 0 && (isalnum(data->Buf[wordStart - 1]) ||
                                 data->Buf[wordStart - 1] == '_' || data->Buf[wordStart - 1] == '.'))
            wordStart--;

        // Eliminar la palabra actual
        data->DeleteChars(wordStart, data->CursorPos - wordStart);

        // Insertar la sugerencia
        data->InsertChars(data->CursorPos, selected.c_str());

        showAutocomplete = false;
        isModified = true;
    }

    ImVec2 CalculateAutocompletePosition(const char *buffer, int cursorPos, ImVec2 textInputPos, float scrollY)
    {
        int currentLine = 0;
        int currentColumn = 0;

        for (int i = 0; i < cursorPos; i++)
        {
            if (buffer[i] == '\n')
            {
                currentLine++;
                currentColumn = 0;
            }
            else
            {
                currentColumn++;
            }
        }

        float charWidth = ImGui::CalcTextSize("A").x;
        float lineHeight = ImGui::GetTextLineHeight();

        float paddingX = 8.0f;
        float paddingY = 4.0f;

        ImVec2 pos;
        pos.x = textInputPos.x + paddingX + (currentColumn * charWidth);
        pos.y = textInputPos.y + paddingY + (currentLine * lineHeight) - scrollY + lineHeight;

        return pos;
    }

    std::string OpenFileDialog()
    {
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Archivos Lua\0*.lua\0Todos los archivos\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return std::string(ofn.lpstrFile);
        }

        return "";
    }

    std::string SaveFileDialog()
    {
        OPENFILENAMEA ofn;
        char szFile[260] = {0};

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Archivos Lua\0*.lua\0Todos los archivos\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrDefExt = "lua";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            return std::string(ofn.lpstrFile);
        }

        return "";
    }

    bool LoadFile(const std::string &filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        if (content.size() >= sizeof(textBuffer))
        {
            return false;
        }

        strncpy_s(textBuffer, sizeof(textBuffer), content.c_str(), _TRUNCATE);
        currentFilePath = filepath;
        isModified = false;
        UpdateLineNumbers();

        return true;
    }

    bool SaveFile(const std::string &filepath)
    {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(textBuffer, strlen(textBuffer));
        file.close();

        currentFilePath = filepath;
        isModified = false;

        return true;
    }

    std::string GetFileName() const
    {
        if (currentFilePath.empty())
        {
            return "Sin título";
        }

        size_t pos = currentFilePath.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            return currentFilePath.substr(pos + 1);
        }

        return currentFilePath;
    }

public:
    LuaEditor()
    {
        memset(textBuffer, 0, sizeof(textBuffer));
        memset(lineNumbersBuffer, 0, sizeof(lineNumbersBuffer));
        strcpy_s(textBuffer, sizeof(textBuffer),
                 "function suma(a, b)\n    return a + b\nend\n\nlocal resultado = suma(5, 3)\nprint(\"Resultado: \" .. resultado)");
        UpdateLineNumbers();
    }

    int CountLines()
    {
        int count = 1;
        for (int i = 0; textBuffer[i] != '\0'; i++)
            if (textBuffer[i] == '\n')
                count++;
        return count;
    }

    void Render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.13f, 1.0f));

        std::string windowTitle = "Editor Lua - " + GetFileName();
        if (isModified)
        {
            windowTitle += " *";
        }

        ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Archivo"))
            {
                if (ImGui::MenuItem("Nuevo", "Ctrl+N"))
                {
                    memset(textBuffer, 0, sizeof(textBuffer));
                    currentFilePath.clear();
                    isModified = false;
                    UpdateLineNumbers();
                }

                if (ImGui::MenuItem("Abrir", "Ctrl+O"))
                {
                    std::string filepath = OpenFileDialog();
                    if (!filepath.empty())
                    {
                        if (!LoadFile(filepath))
                        {
                            // Error al cargar el archivo
                            ImGui::OpenPopup("Error");
                        }
                    }
                }

                if (ImGui::MenuItem("Guardar", "Ctrl+S"))
                {
                    if (currentFilePath.empty())
                    {
                        std::string filepath = SaveFileDialog();
                        if (!filepath.empty())
                        {
                            if (!SaveFile(filepath))
                            {
                                ImGui::OpenPopup("Error");
                            }
                        }
                    }
                    else
                    {
                        if (!SaveFile(currentFilePath))
                        {
                            ImGui::OpenPopup("Error");
                        }
                    }
                }

                if (ImGui::MenuItem("Guardar como...", "Ctrl+Shift+S"))
                {
                    std::string filepath = SaveFileDialog();
                    if (!filepath.empty())
                    {
                        if (!SaveFile(filepath))
                        {
                            ImGui::OpenPopup("Error");
                        }
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Salir", "Alt+F4"))
                {
                    // Implementar lógica de salida
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Editar"))
            {
                if (ImGui::MenuItem("Ejecutar", "F5"))
                {
                    // Implementar ejecución de código
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // Popup de error
        if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Error al abrir/guardar el archivo.");
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        int lineCount = CountLines();

        // Info Bar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.16f, 1.0f));
        ImGui::BeginChild("InfoBar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("  Archivo: %s  |  Lineas: %d  |  Caracteres: %d  |  %s",
                    GetFileName().c_str(), lineCount, (int)strlen(textBuffer),
                    isModified ? "Modificado" : "Guardado");
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Editor Area
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.095f, 0.095f, 0.11f, 1.0f));
        ImGui::BeginChild("EditorArea", ImVec2(0, -35), true, ImGuiWindowFlags_NoScrollbar);

        float lineNumWidth = 60.0f;
        static float sharedScrollY = 0.0f;

        // Line Numbers
        ImGui::BeginChild("LineNumbersArea", ImVec2(lineNumWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.078f, 0.078f, 0.098f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

        ImGui::InputTextMultiline("##lineNumbers", lineNumbersBuffer, sizeof(lineNumbersBuffer),
                                  ImVec2(lineNumWidth, -1),
                                  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        if (ImGui::IsItemActive())
            sharedScrollY = ImGui::GetScrollY();
        else
            ImGui::SetScrollY(sharedScrollY);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        ImGui::EndChild();

        // Separator
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 p1 = ImVec2(ImGui::GetItemRectMin().x + lineNumWidth, ImGui::GetItemRectMin().y);
        ImVec2 p2 = ImVec2(ImGui::GetItemRectMin().x + lineNumWidth, ImGui::GetItemRectMax().y);
        drawList->AddLine(p1, p2, IM_COL32(60, 80, 120, 150), 2.0f);

        ImGui::SameLine();

        // Code Area
        ImGui::BeginChild("CodeArea", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.095f, 0.095f, 0.11f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.86f, 0.86f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

        ImVec2 textInputPos = ImGui::GetCursorScreenPos();

        // Capturar Tab ANTES del InputText cuando hay autocompletado
        bool tabPressed = false;
        if (showAutocomplete && !suggestions.empty())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Tab))
            {
                tabPressed = true;
            }
        }

        // Callback para manejar el input
        auto inputCallback = [](ImGuiInputTextCallbackData *data) -> int
        {
            LuaEditor *editor = (LuaEditor *)data->UserData;

            // Manejar Tab para autocompletado
            if (editor->showAutocomplete && !editor->suggestions.empty() &&
                data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
            {
                editor->InsertSuggestion(data);
                return 0;
            }

            // Manejar otras teclas cuando hay autocompletado activo
            if (editor->showAutocomplete && !editor->suggestions.empty())
            {
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    editor->selectedSuggestion = (editor->selectedSuggestion - 1 + editor->suggestions.size()) % editor->suggestions.size();
                    return 1; // Bloquear el evento
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    editor->selectedSuggestion = (editor->selectedSuggestion + 1) % editor->suggestions.size();
                    return 1; // Bloquear el evento
                }
                else if (data->EventKey == ImGuiKey_Enter || data->EventKey == ImGuiKey_KeypadEnter)
                {
                    editor->InsertSuggestion(data);
                    return 1; // Bloquear el evento
                }
                else if (data->EventKey == ImGuiKey_Escape)
                {
                    editor->showAutocomplete = false;
                    return 1; // Bloquear el evento
                }
            }

            // Actualizar palabra actual
            if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
            {
                std::string word = editor->GetWordAtCursor(data->Buf, data->CursorPos);
                editor->currentWord = word;
                editor->cursorPosForAutocomplete = data->CursorPos;
                editor->UpdateAutocomplete(word);
            }

            return 0;
        };

        // Renderizar el InputText con flags adicionales
        bool textChanged = ImGui::InputTextMultiline("##code", textBuffer, sizeof(textBuffer),
                                                     ImVec2(-1, -1),
                                                     ImGuiInputTextFlags_CallbackAlways |
                                                         ImGuiInputTextFlags_CallbackCompletion,
                                                     inputCallback, this);

        // Procesar Tab manual si fue presionado
        if (tabPressed && showAutocomplete && !suggestions.empty())
        {
            std::string selected = suggestions[selectedSuggestion];

            int wordStart = cursorPosForAutocomplete;
            while (wordStart > 0 && (isalnum(textBuffer[wordStart - 1]) ||
                                     textBuffer[wordStart - 1] == '_' || textBuffer[wordStart - 1] == '.'))
                wordStart--;

            std::string before(textBuffer, wordStart);
            std::string after(textBuffer + cursorPosForAutocomplete);
            std::string newText = before + selected + after;

            strncpy_s(textBuffer, sizeof(textBuffer), newText.c_str(), _TRUNCATE);
            showAutocomplete = false;
            isModified = true;
            UpdateLineNumbers();
        }

        // Si se debe insertar sugerencia, hacerlo DESPUÉS del InputText
        if (textChanged)
        {
            isModified = true;
            UpdateLineNumbers();
        }

        // Calcular posición del autocompletado
        if (showAutocomplete && !suggestions.empty())
        {
            autocompletePos = CalculateAutocompletePosition(textBuffer, cursorPosForAutocomplete, textInputPos, sharedScrollY);
        }

        if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            sharedScrollY = ImGui::GetScrollY();
        else
            ImGui::SetScrollY(sharedScrollY);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
        ImGui::EndChild();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Status Bar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.16f, 1.0f));
        ImGui::BeginChild("StatusBar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar);

        ImVec4 statusColor = isModified ? ImVec4(1.0f, 0.8f, 0.4f, 1.0f) : ImVec4(0.6f, 0.8f, 0.6f, 1.0f);
        ImGui::TextColored(statusColor, "  %s  |  IntelliSense: %s",
                           isModified ? "Modificado" : "Listo",
                           showAutocomplete ? "Activo" : "Listo");

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // Render autocomplete popup
        if (showAutocomplete && !suggestions.empty())
        {
            float itemHeight = ImGui::GetTextLineHeightWithSpacing();
            float headerHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
            float separatorHeight = 10.0f;
            float totalHeight = headerHeight + separatorHeight + (suggestions.size() * itemHeight) + 16.0f;

            ImGui::SetNextWindowPos(autocompletePos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(250, totalHeight));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.25f, 0.98f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.7f, 0.9f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

            ImGui::Begin("IntelliSense", nullptr,
                         ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing);

            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sugerencias:");
            ImGui::Separator();

            for (size_t i = 0; i < suggestions.size(); i++)
            {
                bool isSelected = (i == selectedSuggestion);

                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
                    ImGui::Text("> %s", suggestions[i].c_str());
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::Text("  %s", suggestions[i].c_str());
                }
            }

            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }
    }

    const char *GetText() const { return textBuffer; }

    void SetText(const char *text)
    {
        strncpy_s(textBuffer, sizeof(textBuffer), text, _TRUNCATE);
        isModified = true;
        UpdateLineNumbers();
    }
};