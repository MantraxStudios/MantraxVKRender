#include "../include/InputSystem.h"
#include <iostream>

namespace Mantrax
{
    InputSystem::InputSystem()
        : m_MouseSensitivity(1.0f), m_Hwnd(nullptr)
    {
        Reset();
    }

    InputSystem::~InputSystem()
    {
    }

    void InputSystem::ProcessMessage(UINT msg, WPARAM wp, LPARAM lp, HWND hwnd)
    {
        m_Hwnd = hwnd;

        switch (msg)
        {
        case WM_KEYDOWN:
        {
            KeyCode key = VKToKeyCode(wp);
            if (key != KeyCode(-1))
            {
                int idx = KeyCodeToIndex(key);
                m_Keys[idx].current = true;
            }
            break;
        }

        case WM_KEYUP:
        {
            KeyCode key = VKToKeyCode(wp);
            if (key != KeyCode(-1))
            {
                int idx = KeyCodeToIndex(key);
                m_Keys[idx].current = false;
            }
            break;
        }

        case WM_LBUTTONDOWN:
            m_Keys[KeyCodeToIndex(KeyCode::LeftMouse)].current = true;
            break;

        case WM_LBUTTONUP:
            m_Keys[KeyCodeToIndex(KeyCode::LeftMouse)].current = false;
            break;

        case WM_RBUTTONDOWN:
        {
            m_Keys[KeyCodeToIndex(KeyCode::RightMouse)].current = true;
            m_MouseState.firstMouse = true;
            m_MouseState.delta = {0, 0}; // Reset delta

            SetCapture(hwnd);
            break;
        }

        case WM_RBUTTONUP:
        {
            m_Keys[KeyCodeToIndex(KeyCode::RightMouse)].current = false;
            m_MouseState.firstMouse = true;
            m_MouseState.delta = {0, 0}; // Reset delta

            ReleaseCapture();
            break;
        }

        case WM_MBUTTONDOWN:
            m_Keys[KeyCodeToIndex(KeyCode::MiddleMouse)].current = true;
            break;

        case WM_MBUTTONUP:
            m_Keys[KeyCodeToIndex(KeyCode::MiddleMouse)].current = false;
            break;

        case WM_MOUSEMOVE:
        {
            POINT currentPos;
            GetCursorPos(&currentPos);
            ScreenToClient(hwnd, &currentPos);

            if (m_MouseState.firstMouse)
            {
                m_MouseState.lastPosition = currentPos;
                m_MouseState.position = currentPos;
                m_MouseState.delta = {0, 0};
                m_MouseState.firstMouse = false;
                break;
            }

            m_MouseState.position = currentPos;
            m_MouseState.delta.x = currentPos.x - m_MouseState.lastPosition.x;
            m_MouseState.delta.y = m_MouseState.lastPosition.y - currentPos.y;
            m_MouseState.lastPosition = currentPos;
            break;
        }

        case WM_MOUSEWHEEL:
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wp) / 120.0f;
            m_MouseState.wheelDelta = delta;
            break;
        }
        }
    }

    void InputSystem::Update()
    {
        // Actualizar estado previo de teclas
        for (int i = 0; i < 32; ++i)
        {
            m_Keys[i].previous = m_Keys[i].current;
        }

        // IMPORTANTE: Resetear delta aquí
        // El delta solo debe ser válido por un frame
        m_MouseState.delta = {0, 0};

        // Reset wheel delta
        m_MouseState.wheelDelta = 0.0f;
    }

    bool InputSystem::IsKeyPressed(KeyCode key) const
    {
        int idx = KeyCodeToIndex(key);
        return m_Keys[idx].current && !m_Keys[idx].previous;
    }

    bool InputSystem::IsKeyHeld(KeyCode key) const
    {
        int idx = KeyCodeToIndex(key);
        return m_Keys[idx].current && m_Keys[idx].previous;
    }

    bool InputSystem::IsKeyReleased(KeyCode key) const
    {
        int idx = KeyCodeToIndex(key);
        return !m_Keys[idx].current && m_Keys[idx].previous;
    }

    bool InputSystem::IsKeyDown(KeyCode key) const
    {
        int idx = KeyCodeToIndex(key);
        return m_Keys[idx].current;
    }

    bool InputSystem::IsMouseButtonPressed(KeyCode button) const
    {
        return IsKeyPressed(button);
    }

    bool InputSystem::IsMouseButtonHeld(KeyCode button) const
    {
        return IsKeyHeld(button);
    }

    bool InputSystem::IsMouseButtonReleased(KeyCode button) const
    {
        return IsKeyReleased(button);
    }

    bool InputSystem::IsMouseButtonDown(KeyCode button) const
    {
        return IsKeyDown(button);
    }

    void InputSystem::Reset()
    {
        for (int i = 0; i < 32; ++i)
        {
            m_Keys[i].current = false;
            m_Keys[i].previous = false;
        }

        m_MouseState.position = {0, 0};
        m_MouseState.lastPosition = {0, 0};
        m_MouseState.delta = {0, 0};
        m_MouseState.wheelDelta = 0.0f;
        m_MouseState.firstMouse = true;
    }

    void InputSystem::PrintMouseState() const
    {
        std::cout << "Mouse State:" << std::endl;
        std::cout << "  Position: (" << m_MouseState.position.x << ", " << m_MouseState.position.y << ")" << std::endl;
        std::cout << "  Delta: (" << m_MouseState.delta.x << ", " << m_MouseState.delta.y << ")" << std::endl;
        std::cout << "  Wheel: " << m_MouseState.wheelDelta << std::endl;
        std::cout << "  Right Button: " << (IsMouseButtonDown(KeyCode::RightMouse) ? "Down" : "Up") << std::endl;
    }

    int InputSystem::KeyCodeToIndex(KeyCode key) const
    {
        return static_cast<int>(key);
    }

    KeyCode InputSystem::VKToKeyCode(WPARAM vk) const
    {
        switch (vk)
        {
        case 'W':
            return KeyCode::W;
        case 'A':
            return KeyCode::A;
        case 'S':
            return KeyCode::S;
        case 'D':
            return KeyCode::D;
        case VK_SPACE:
            return KeyCode::Space;
        case VK_SHIFT:
            return KeyCode::Shift;
        case VK_CONTROL:
            return KeyCode::Ctrl;
        case VK_ESCAPE:
            return KeyCode::Escape;
        default:
            return KeyCode(-1);
        }
    }
}