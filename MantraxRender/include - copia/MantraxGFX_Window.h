#pragma once

#include <windows.h>
#include <functional>
#include "MantraxGFX_Input.h"

namespace MantraxGFX
{
    class Window
    {
    public:
        using ResizeCallback = std::function<void(UINT, UINT)>;

        void Create(HINSTANCE hInstance, const wchar_t *title, UINT width, UINT height)
        {
            this->width = width;
            this->height = height;

            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = WindowProcStatic;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.lpszClassName = L"MantraxGFX";
            RegisterClassExW(&wc);

            RECT rect = {0, 0, (LONG)width, (LONG)height};
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

            hwnd = CreateWindowExW(
                0, L"MantraxGFX", title,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                rect.right - rect.left, rect.bottom - rect.top,
                nullptr, nullptr, hInstance, this);
        }

        void Show(int nCmdShow)
        {
            ShowWindow(hwnd, nCmdShow);
        }

        bool ProcessMessages()
        {
            MSG msg = {};
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    return false;

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            return true;
        }

        void SetResizeCallback(ResizeCallback callback)
        {
            resizeCallback = callback;
        }

        HWND GetHandle() const { return hwnd; }
        UINT GetWidth() const { return width; }
        UINT GetHeight() const { return height; }
        bool IsResizing() const { return isResizing; }

    private:
        static LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            Window *window = nullptr;

            if (msg == WM_CREATE)
            {
                CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
                window = reinterpret_cast<Window *>(pCreate->lpCreateParams);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
            }
            else
            {
                window = reinterpret_cast<Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            }

            if (window)
                return window->WindowProc(hwnd, msg, wParam, lParam);

            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto &input = Input::Get();

            switch (msg)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            case WM_SIZE:
            {
                UINT newWidth = LOWORD(lParam);
                UINT newHeight = HIWORD(lParam);

                if (newWidth != width || newHeight != height)
                {
                    width = newWidth;
                    height = newHeight;

                    if (resizeCallback && !isResizing && width > 0 && height > 0)
                    {
                        resizeCallback(width, height);
                    }
                }
                return 0;
            }

            case WM_ENTERSIZEMOVE:
                isResizing = true;
                return 0;

            case WM_EXITSIZEMOVE:
                isResizing = false;
                if (resizeCallback && width > 0 && height > 0)
                {
                    resizeCallback(width, height);
                }
                return 0;

            // Keyboard
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                input.SetKeyState((int)wParam, true);
                return 0;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                input.SetKeyState((int)wParam, false);
                return 0;

            // Mouse buttons
            case WM_LBUTTONDOWN:
                input.SetMouseButton((int)MouseButton::Left, true);
                return 0;

            case WM_LBUTTONUP:
                input.SetMouseButton((int)MouseButton::Left, false);
                return 0;

            case WM_RBUTTONDOWN:
                input.SetMouseButton((int)MouseButton::Right, true);
                return 0;

            case WM_RBUTTONUP:
                input.SetMouseButton((int)MouseButton::Right, false);
                return 0;

            case WM_MBUTTONDOWN:
                input.SetMouseButton((int)MouseButton::Middle, true);
                return 0;

            case WM_MBUTTONUP:
                input.SetMouseButton((int)MouseButton::Middle, false);
                return 0;

            // Mouse movement
            case WM_MOUSEMOVE:
            {
                float x = (float)LOWORD(lParam);
                float y = (float)HIWORD(lParam);
                input.SetMousePosition(x, y);
                return 0;
            }

            // Mouse wheel
            case WM_MOUSEWHEEL:
            {
                float delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
                input.AddMouseWheelDelta(delta);
                return 0;
            }
            }

            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        HWND hwnd = nullptr;
        UINT width = 0;
        UINT height = 0;
        bool isResizing = false;
        ResizeCallback resizeCallback;
    };
}