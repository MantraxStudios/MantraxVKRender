#include "../include/InputSystem.h"
#include <iostream>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

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
        // ==================== TECLADO ====================
        case WM_KEYDOWN:
        {
            KeyCode key = VKToKeyCode(wp);
            if (key != KeyCode::Unknown)
            {
                m_KeyboardKeys[key].current = true;
            }
            break;
        }

        case WM_KEYUP:
        {
            KeyCode key = VKToKeyCode(wp);
            if (key != KeyCode::Unknown)
            {
                m_KeyboardKeys[key].current = false;
            }
            break;
        }

        // ==================== MOUSE BUTTONS ====================
        case WM_LBUTTONDOWN:
            m_MouseButtons[static_cast<int>(MouseButton::Left)].current = true;
            break;

        case WM_LBUTTONUP:
            m_MouseButtons[static_cast<int>(MouseButton::Left)].current = false;
            break;

        case WM_RBUTTONDOWN:
        {
            m_MouseButtons[static_cast<int>(MouseButton::Right)].current = true;
            m_MouseState.firstMouse = true;
            m_MouseState.delta = {0, 0};
            SetCapture(hwnd);
            break;
        }

        case WM_RBUTTONUP:
        {
            m_MouseButtons[static_cast<int>(MouseButton::Right)].current = false;
            m_MouseState.firstMouse = true;
            m_MouseState.delta = {0, 0};
            ReleaseCapture();
            break;
        }

        case WM_MBUTTONDOWN:
            m_MouseButtons[static_cast<int>(MouseButton::Middle)].current = true;
            break;

        case WM_MBUTTONUP:
            m_MouseButtons[static_cast<int>(MouseButton::Middle)].current = false;
            break;

        case WM_XBUTTONDOWN:
        {
            int xbutton = GET_XBUTTON_WPARAM(wp);
            if (xbutton == XBUTTON1)
                m_MouseButtons[static_cast<int>(MouseButton::Button4)].current = true;
            else if (xbutton == XBUTTON2)
                m_MouseButtons[static_cast<int>(MouseButton::Button5)].current = true;
            break;
        }

        case WM_XBUTTONUP:
        {
            int xbutton = GET_XBUTTON_WPARAM(wp);
            if (xbutton == XBUTTON1)
                m_MouseButtons[static_cast<int>(MouseButton::Button4)].current = false;
            else if (xbutton == XBUTTON2)
                m_MouseButtons[static_cast<int>(MouseButton::Button5)].current = false;
            break;
        }

        // ==================== MOUSE MOVEMENT ====================
        case WM_MOUSEMOVE:
        {
            int xPos = GET_X_LPARAM(lp);
            int yPos = GET_Y_LPARAM(lp);
            POINT currentPos = {xPos, yPos};

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
            m_MouseState.wheelDelta += delta;
            break;
        }
        }
    }

    void InputSystem::Update()
    {
        // Actualizar estado previo de teclas del TECLADO
        for (auto &pair : m_KeyboardKeys)
        {
            pair.second.previous = pair.second.current;
        }

        // Actualizar estado previo de botones del MOUSE
        for (int i = 0; i < 5; ++i)
        {
            m_MouseButtons[i].previous = m_MouseButtons[i].current;
        }

        // Resetear deltas al final del frame
        m_MouseState.delta = {0, 0};
        m_MouseState.wheelDelta = 0.0f;
    }

    // ==================== TECLADO ====================
    bool InputSystem::IsKeyPressed(KeyCode key) const
    {
        auto it = m_KeyboardKeys.find(key);
        if (it == m_KeyboardKeys.end())
            return false;
        return it->second.current && !it->second.previous;
    }

    bool InputSystem::IsKeyHeld(KeyCode key) const
    {
        auto it = m_KeyboardKeys.find(key);
        if (it == m_KeyboardKeys.end())
            return false;
        return it->second.current && it->second.previous;
    }

    bool InputSystem::IsKeyReleased(KeyCode key) const
    {
        auto it = m_KeyboardKeys.find(key);
        if (it == m_KeyboardKeys.end())
            return false;
        return !it->second.current && it->second.previous;
    }

    bool InputSystem::IsKeyDown(KeyCode key) const
    {
        auto it = m_KeyboardKeys.find(key);
        if (it == m_KeyboardKeys.end())
            return false;
        return it->second.current;
    }

    // ==================== MOUSE ====================
    bool InputSystem::IsMouseButtonPressed(MouseButton button) const
    {
        int idx = static_cast<int>(button);
        return m_MouseButtons[idx].current && !m_MouseButtons[idx].previous;
    }

    bool InputSystem::IsMouseButtonHeld(MouseButton button) const
    {
        int idx = static_cast<int>(button);
        return m_MouseButtons[idx].current && m_MouseButtons[idx].previous;
    }

    bool InputSystem::IsMouseButtonReleased(MouseButton button) const
    {
        int idx = static_cast<int>(button);
        return !m_MouseButtons[idx].current && m_MouseButtons[idx].previous;
    }

    bool InputSystem::IsMouseButtonDown(MouseButton button) const
    {
        int idx = static_cast<int>(button);
        return m_MouseButtons[idx].current;
    }

    void InputSystem::Reset()
    {
        m_KeyboardKeys.clear();

        for (int i = 0; i < 5; ++i)
        {
            m_MouseButtons[i].current = false;
            m_MouseButtons[i].previous = false;
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
        std::cout << "  Position: (" << m_MouseState.position.x << ", "
                  << m_MouseState.position.y << ")" << std::endl;
        std::cout << "  Delta: (" << m_MouseState.delta.x << ", "
                  << m_MouseState.delta.y << ")" << std::endl;
        std::cout << "  Wheel: " << m_MouseState.wheelDelta << std::endl;
        std::cout << "  Right Button: "
                  << (IsMouseButtonDown(MouseButton::Right) ? "Down" : "Up") << std::endl;
    }

    KeyCode InputSystem::VKToKeyCode(WPARAM vk) const
    {
        switch (vk)
        {
        // Letras A–Z
        case 'A':
            return KeyCode::A;
        case 'B':
            return KeyCode::B;
        case 'C':
            return KeyCode::C;
        case 'D':
            return KeyCode::D;
        case 'E':
            return KeyCode::E;
        case 'F':
            return KeyCode::F;
        case 'G':
            return KeyCode::G;
        case 'H':
            return KeyCode::H;
        case 'I':
            return KeyCode::I;
        case 'J':
            return KeyCode::J;
        case 'K':
            return KeyCode::K;
        case 'L':
            return KeyCode::L;
        case 'M':
            return KeyCode::M;
        case 'N':
            return KeyCode::N;
        case 'O':
            return KeyCode::O;
        case 'P':
            return KeyCode::P;
        case 'Q':
            return KeyCode::Q;
        case 'R':
            return KeyCode::R;
        case 'S':
            return KeyCode::S;
        case 'T':
            return KeyCode::T;
        case 'U':
            return KeyCode::U;
        case 'V':
            return KeyCode::V;
        case 'W':
            return KeyCode::W;
        case 'X':
            return KeyCode::X;
        case 'Y':
            return KeyCode::Y;
        case 'Z':
            return KeyCode::Z;

        // Números fila superior
        case '0':
            return KeyCode::Num0;
        case '1':
            return KeyCode::Num1;
        case '2':
            return KeyCode::Num2;
        case '3':
            return KeyCode::Num3;
        case '4':
            return KeyCode::Num4;
        case '5':
            return KeyCode::Num5;
        case '6':
            return KeyCode::Num6;
        case '7':
            return KeyCode::Num7;
        case '8':
            return KeyCode::Num8;
        case '9':
            return KeyCode::Num9;

        // Numpad
        case VK_NUMPAD0:
            return KeyCode::Numpad0;
        case VK_NUMPAD1:
            return KeyCode::Numpad1;
        case VK_NUMPAD2:
            return KeyCode::Numpad2;
        case VK_NUMPAD3:
            return KeyCode::Numpad3;
        case VK_NUMPAD4:
            return KeyCode::Numpad4;
        case VK_NUMPAD5:
            return KeyCode::Numpad5;
        case VK_NUMPAD6:
            return KeyCode::Numpad6;
        case VK_NUMPAD7:
            return KeyCode::Numpad7;
        case VK_NUMPAD8:
            return KeyCode::Numpad8;
        case VK_NUMPAD9:
            return KeyCode::Numpad9;
        case VK_MULTIPLY:
            return KeyCode::NumpadMultiply;
        case VK_ADD:
            return KeyCode::NumpadAdd;
        case VK_SEPARATOR:
            return KeyCode::NumpadSeparator;
        case VK_SUBTRACT:
            return KeyCode::NumpadSubtract;
        case VK_DECIMAL:
            return KeyCode::NumpadDecimal;
        case VK_DIVIDE:
            return KeyCode::NumpadDivide;

        // Función F1–F24
        case VK_F1:
            return KeyCode::F1;
        case VK_F2:
            return KeyCode::F2;
        case VK_F3:
            return KeyCode::F3;
        case VK_F4:
            return KeyCode::F4;
        case VK_F5:
            return KeyCode::F5;
        case VK_F6:
            return KeyCode::F6;
        case VK_F7:
            return KeyCode::F7;
        case VK_F8:
            return KeyCode::F8;
        case VK_F9:
            return KeyCode::F9;
        case VK_F10:
            return KeyCode::F10;
        case VK_F11:
            return KeyCode::F11;
        case VK_F12:
            return KeyCode::F12;
        case VK_F13:
            return KeyCode::F13;
        case VK_F14:
            return KeyCode::F14;
        case VK_F15:
            return KeyCode::F15;
        case VK_F16:
            return KeyCode::F16;
        case VK_F17:
            return KeyCode::F17;
        case VK_F18:
            return KeyCode::F18;
        case VK_F19:
            return KeyCode::F19;
        case VK_F20:
            return KeyCode::F20;
        case VK_F21:
            return KeyCode::F21;
        case VK_F22:
            return KeyCode::F22;
        case VK_F23:
            return KeyCode::F23;
        case VK_F24:
            return KeyCode::F24;

        // Navegación
        case VK_LEFT:
            return KeyCode::Left;
        case VK_RIGHT:
            return KeyCode::Right;
        case VK_UP:
            return KeyCode::Up;
        case VK_DOWN:
            return KeyCode::Down;
        case VK_HOME:
            return KeyCode::Home;
        case VK_END:
            return KeyCode::End;
        case VK_PRIOR:
            return KeyCode::PageUp;
        case VK_NEXT:
            return KeyCode::PageDown;
        case VK_INSERT:
            return KeyCode::Insert;
        case VK_DELETE:
            return KeyCode::Delete;

        // Modificadores
        case VK_SHIFT:
            return KeyCode::Shift;
        case VK_CONTROL:
            return KeyCode::Ctrl;
        case VK_MENU:
            return KeyCode::Alt;
        case VK_LSHIFT:
            return KeyCode::LShift;
        case VK_RSHIFT:
            return KeyCode::RShift;
        case VK_LCONTROL:
            return KeyCode::LCtrl;
        case VK_RCONTROL:
            return KeyCode::RCtrl;
        case VK_LMENU:
            return KeyCode::LAlt;
        case VK_RMENU:
            return KeyCode::RAlt;
        case VK_LWIN:
            return KeyCode::LWin;
        case VK_RWIN:
            return KeyCode::RWin;

        // Teclas especiales
        case VK_ESCAPE:
            return KeyCode::Escape;
        case VK_SPACE:
            return KeyCode::Space;
        case VK_TAB:
            return KeyCode::Tab;
        case VK_RETURN:
            return KeyCode::Enter;
        case VK_BACK:
            return KeyCode::Backspace;
        case VK_CAPITAL:
            return KeyCode::CapsLock;
        case VK_SCROLL:
            return KeyCode::ScrollLock;
        case VK_NUMLOCK:
            return KeyCode::NumLock;
        case VK_SNAPSHOT:
            return KeyCode::PrintScreen;
        case VK_PAUSE:
            return KeyCode::Pause;

        // Símbolos
        case VK_OEM_MINUS:
            return KeyCode::Minus;
        case VK_OEM_PLUS:
            return KeyCode::Equals;
        case VK_OEM_4:
            return KeyCode::BracketLeft;
        case VK_OEM_6:
            return KeyCode::BracketRight;
        case VK_OEM_5:
            return KeyCode::Backslash;
        case VK_OEM_1:
            return KeyCode::Semicolon;
        case VK_OEM_7:
            return KeyCode::Apostrophe;
        case VK_OEM_COMMA:
            return KeyCode::Comma;
        case VK_OEM_PERIOD:
            return KeyCode::Period;
        case VK_OEM_2:
            return KeyCode::Slash;
        case VK_OEM_3:
            return KeyCode::GraveAccent;

        // Multimedia
        case VK_VOLUME_MUTE:
            return KeyCode::VolumeMute;
        case VK_VOLUME_DOWN:
            return KeyCode::VolumeDown;
        case VK_VOLUME_UP:
            return KeyCode::VolumeUp;
        case VK_MEDIA_NEXT_TRACK:
            return KeyCode::MediaNext;
        case VK_MEDIA_PREV_TRACK:
            return KeyCode::MediaPrevious;
        case VK_MEDIA_PLAY_PAUSE:
            return KeyCode::MediaPlayPause;
        case VK_MEDIA_STOP:
            return KeyCode::MediaStop;

        default:
            return KeyCode::Unknown;
        }
    }
}