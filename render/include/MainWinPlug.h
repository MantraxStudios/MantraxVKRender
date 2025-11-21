#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <functional>
#include <stdexcept>
#include <string>

namespace Mantrax
{
    // ========================================================================
    // WINDOW CONFIG
    // ========================================================================
    struct WindowConfig
    {
        uint32_t width = 1920;
        uint32_t height = 1080;
        const char *title = "Mantrax Window";
        bool resizable = true;
    };

    // ========================================================================
    // WINDOW MAIN PLUG - GESTIÓN DE VENTANA WIN32
    // ========================================================================
    class WindowMainPlug
    {
    public:
        WindowMainPlug(HINSTANCE hInstance, const WindowConfig &config = WindowConfig{})
            : m_hInstance(hInstance),
              m_Config(config),
              m_hWnd(nullptr),
              m_FramebufferResized(false)
        {
            CreateWindowClass();
            CreateWindowInternal();
        }

        ~WindowMainPlug()
        {
            if (m_hWnd)
            {
                DestroyWindow(m_hWnd);
                m_hWnd = nullptr;
            }
        }

        // Getters
        HINSTANCE GetHInstance() const { return m_hInstance; }
        HWND GetHWND() const { return m_hWnd; }
        bool WasFramebufferResized() const { return m_FramebufferResized; }
        void ResetFramebufferResizedFlag() { m_FramebufferResized = false; }

        // Obtener dimensiones actuales de la ventana
        void GetWindowSize(uint32_t &width, uint32_t &height) const
        {
            RECT rect;
            GetClientRect(m_hWnd, &rect);
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }

        // Configurar callback personalizado para mensajes de ventana
        void SetCustomWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> &proc)
        {
            m_CustomWndProc = proc;
        }

        // Procesar mensajes de Windows (llamar en el game loop)
        bool ProcessMessages()
        {
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return false;

                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            return true;
        }

        // No permitir copia
        WindowMainPlug(const WindowMainPlug &) = delete;
        WindowMainPlug &operator=(const WindowMainPlug &) = delete;

    private:
        HINSTANCE m_hInstance;
        WindowConfig m_Config;
        HWND m_hWnd;
        bool m_FramebufferResized;
        std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_CustomWndProc;

        // Procedimiento de ventana estático
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
        {
            WindowMainPlug *pThis = nullptr;

            if (msg == WM_NCCREATE)
            {
                auto cs = reinterpret_cast<CREATESTRUCTW *>(lp);
                pThis = reinterpret_cast<WindowMainPlug *>(cs->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
                pThis->m_hWnd = hwnd;
            }
            else
            {
                pThis = reinterpret_cast<WindowMainPlug *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            }

            // Llamar a callback personalizado si existe
            if (pThis && pThis->m_CustomWndProc)
            {
                LRESULT r = pThis->m_CustomWndProc(hwnd, msg, wp, lp);
                if (r != 0)
                    return r;
            }

            // Manejo de mensajes por defecto
            if (pThis)
            {
                switch (msg)
                {
                case WM_SIZE:
                    pThis->m_FramebufferResized = true;
                    return 0;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
                }
            }

            return DefWindowProcW(hwnd, msg, wp, lp);
        }

        void CreateWindowClass()
        {
            const wchar_t CLASS_NAME[] = L"MantraxVulkanWindow";

            WNDCLASSW wc{};
            wc.lpfnWndProc = WindowMainPlug::WndProc;
            wc.hInstance = m_hInstance;
            wc.lpszClassName = CLASS_NAME;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

            static bool registered = false;
            if (!registered)
            {
                if (!RegisterClassW(&wc))
                    throw std::runtime_error("Error registrando clase Win32");
                registered = true;
            }
        }

        void CreateWindowInternal()
        {
            const wchar_t CLASS_NAME[] = L"MantraxVulkanWindow";

            std::wstring title(m_Config.title, m_Config.title + strlen(m_Config.title));

            DWORD style = WS_OVERLAPPEDWINDOW;
            if (!m_Config.resizable)
            {
                style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            }

            m_hWnd = CreateWindowExW(
                0, CLASS_NAME, title.c_str(),
                style,
                CW_USEDEFAULT, CW_USEDEFAULT,
                m_Config.width, m_Config.height,
                nullptr, nullptr, m_hInstance, this);

            if (!m_hWnd)
                throw std::runtime_error("Error creando ventana");

            ShowWindow(m_hWnd, SW_SHOW);
            UpdateWindow(m_hWnd);
        }
    };

} // namespace Mantrax