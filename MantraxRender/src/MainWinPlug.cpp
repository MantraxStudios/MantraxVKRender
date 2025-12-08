#include "../include/MainWinPlug.h"
#include <stdexcept>

namespace Mantrax
{
    WindowMainPlug::WindowMainPlug(HINSTANCE hInstance, const WindowConfig &config)
        : m_hInstance(hInstance),
          m_Config(config),
          m_hWnd(nullptr),
          m_FramebufferResized(false)
    {
        CreateWindowClass();
        CreateWindowInternal();
    }

    WindowMainPlug::~WindowMainPlug()
    {
        if (m_hWnd)
        {
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }
    }

    void WindowMainPlug::GetWindowSize(uint32_t &width, uint32_t &height) const
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }

    void WindowMainPlug::SetCustomWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> &proc)
    {
        m_CustomWndProc = proc;
    }

    bool WindowMainPlug::ProcessMessages()
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

    LRESULT CALLBACK WindowMainPlug::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
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

    void WindowMainPlug::CreateWindowClass()
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

    void WindowMainPlug::CreateWindowInternal()
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

    WindowMainPlug::WindowMainPlug(WindowMainPlug &&) noexcept = default;
    WindowMainPlug &WindowMainPlug::operator=(WindowMainPlug &&) noexcept = default;

} // namespace Mantrax