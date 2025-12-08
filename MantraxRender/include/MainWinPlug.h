#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <functional>
#include <string>
#include "../../MantraxECS/include/EngineLoaderDLL.h"

namespace Mantrax
{
    // ========================================================================
    // WINDOW CONFIG
    // ========================================================================
    struct MANTRAX_API WindowConfig
    {
        uint32_t width = 1920;
        uint32_t height = 1080;
        const char *title = "Mantrax Window";
        bool resizable = true;
    };

    // ========================================================================
    // WINDOW MAIN PLUG - GESTIÓN DE VENTANA WIN32
    // ========================================================================
    class MANTRAX_API WindowMainPlug
    {
    public:
        WindowMainPlug(HINSTANCE hInstance, const WindowConfig &config = WindowConfig{});
        ~WindowMainPlug();

        // Getters
        HINSTANCE GetHInstance() const { return m_hInstance; }
        HWND GetHWND() const { return m_hWnd; }
        bool WasFramebufferResized() const { return m_FramebufferResized; }
        void ResetFramebufferResizedFlag() { m_FramebufferResized = false; }

        // Obtener dimensiones actuales de la ventana
        void GetWindowSize(uint32_t &width, uint32_t &height) const;

        // Configurar callback personalizado para mensajes de ventana
        void SetCustomWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> &proc);

        // Procesar mensajes de Windows (llamar en el game loop)
        bool ProcessMessages();

        WindowMainPlug(WindowMainPlug &&) noexcept;
        WindowMainPlug &operator=(WindowMainPlug &&) noexcept;

    private:
        HINSTANCE m_hInstance;
        WindowConfig m_Config;
        HWND m_hWnd;
        bool m_FramebufferResized;
        std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_CustomWndProc;

        // Procedimiento de ventana estático
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

        void CreateWindowClass();
        void CreateWindowInternal();
    };

} // namespace Mantrax