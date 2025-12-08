#pragma once
#include <Windows.h>
#include <unordered_map>
#include "IService.h"
#include "EngineLoaderDLL.h"

namespace Mantrax
{
    enum class KeyCode
    {
        // Letras
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        // Números fila superior
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,

        // Teclas del teclado numérico (Numpad)
        Numpad0,
        Numpad1,
        Numpad2,
        Numpad3,
        Numpad4,
        Numpad5,
        Numpad6,
        Numpad7,
        Numpad8,
        Numpad9,
        NumpadMultiply,
        NumpadAdd,
        NumpadSeparator,
        NumpadSubtract,
        NumpadDecimal,
        NumpadDivide,

        // Teclas de función
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,

        // Navegación
        Left,
        Right,
        Up,
        Down,
        PageUp,
        PageDown,
        Home,
        End,
        Insert,
        Delete,

        // Modificadores
        Shift,
        Ctrl,
        Alt,
        LShift,
        RShift,
        LCtrl,
        RCtrl,
        LAlt,
        RAlt,
        LWin,
        RWin,

        // Caracteres especiales
        Escape,
        Space,
        Tab,
        Enter,
        Backspace,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,

        // Símbolos
        Minus,
        Equals,
        BracketLeft,
        BracketRight,
        Backslash,
        Semicolon,
        Apostrophe,
        Comma,
        Period,
        Slash,
        GraveAccent,

        // Multimedia
        VolumeMute,
        VolumeDown,
        VolumeUp,
        MediaNext,
        MediaPrevious,
        MediaPlayPause,
        MediaStop,

        Unknown
    };

    enum class MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4
    };

    struct MANTRAX_API MouseState
    {
        POINT position;
        POINT lastPosition;
        POINT delta;
        float wheelDelta;
        bool firstMouse;
    };

    class MANTRAX_API InputSystem : public IService
    {
    public:
        InputSystem();
        ~InputSystem();

        // Implementación de IService
        std::string getName() override { return "InputSystem"; }

        // Procesar mensajes de Windows
        void ProcessMessage(UINT msg, WPARAM wp, LPARAM lp, HWND hwnd);

        // Actualizar estado (llamar cada frame)
        void Update();

        // Queries de estado de teclas del TECLADO
        bool IsKeyPressed(KeyCode key) const;
        bool IsKeyHeld(KeyCode key) const;
        bool IsKeyReleased(KeyCode key) const;
        bool IsKeyDown(KeyCode key) const;

        // Queries de MOUSE (ahora usan MouseButton)
        bool IsMouseButtonPressed(MouseButton button) const;
        bool IsMouseButtonHeld(MouseButton button) const;
        bool IsMouseButtonReleased(MouseButton button) const;
        bool IsMouseButtonDown(MouseButton button) const;

        // Obtener estado del mouse
        const MouseState &GetMouseState() const { return m_MouseState; }
        POINT GetMousePosition() const { return m_MouseState.position; }
        POINT GetMouseDelta() const { return m_MouseState.delta; }
        float GetMouseWheelDelta() const { return m_MouseState.wheelDelta; }

        // Reset
        void Reset();

        // Configuración
        void SetMouseSensitivity(float sensitivity) { m_MouseSensitivity = sensitivity; }
        float GetMouseSensitivity() const { return m_MouseSensitivity; }

        // Debug
        void PrintMouseState() const;

    private:
        struct KeyState
        {
            bool current = false;
            bool previous = false;
        };

        struct MouseButtonState
        {
            bool current = false;
            bool previous = false;
        };

        // SEPARACIÓN: Teclado usa mapa, Mouse usa array
        std::unordered_map<KeyCode, KeyState> m_KeyboardKeys;
        MouseButtonState m_MouseButtons[5]; // Left, Right, Middle, Button4, Button5

        MouseState m_MouseState;
        float m_MouseSensitivity;
        HWND m_Hwnd;

        KeyCode VKToKeyCode(WPARAM vk) const;
    };
}