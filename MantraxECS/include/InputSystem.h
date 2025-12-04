#pragma once
#include <Windows.h>
#include <functional>

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
        NumpadSeparator, // Tecla Enter del numpad
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

        // Símbolos fila superior
        Minus,  // -
        Equals, // =
        BracketLeft,
        BracketRight,
        Backslash,
        Semicolon,
        Apostrophe,
        Comma,
        Period,
        Slash,
        GraveAccent, // `

        // Mouse
        LeftMouse,
        RightMouse,
        MiddleMouse,
        MouseButton4,
        MouseButton5,

        // Multimedia (opcional)
        VolumeMute,
        VolumeDown,
        VolumeUp,
        MediaNext,
        MediaPrevious,
        MediaPlayPause,
        MediaStop,

        // Otros
        Unknown
    };

    enum class InputState
    {
        Pressed,
        Released,
        Held
    };

    struct MouseState
    {
        POINT position;
        POINT lastPosition;
        POINT delta;
        float wheelDelta;
        bool firstMouse;
    };

    class InputSystem
    {
    public:
        InputSystem();
        ~InputSystem();

        // Procesar mensajes de Windows
        void ProcessMessage(UINT msg, WPARAM wp, LPARAM lp, HWND hwnd);

        // Actualizar estado (llamar cada frame)
        void Update();

        // Queries de estado de teclas
        bool IsKeyPressed(KeyCode key) const;
        bool IsKeyHeld(KeyCode key) const;
        bool IsKeyReleased(KeyCode key) const;
        bool IsKeyDown(KeyCode key) const;

        // Queries de mouse
        bool IsMouseButtonPressed(KeyCode button) const;
        bool IsMouseButtonHeld(KeyCode button) const;
        bool IsMouseButtonReleased(KeyCode button) const;
        bool IsMouseButtonDown(KeyCode button) const;

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

        KeyState m_Keys[32];
        MouseState m_MouseState;
        float m_MouseSensitivity;
        HWND m_Hwnd; // AGREGAR ESTA LÍNEA
        bool m_SkipMouseEvents;

        int KeyCodeToIndex(KeyCode key) const;
        KeyCode VKToKeyCode(WPARAM vk) const;
    };
}