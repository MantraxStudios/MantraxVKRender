#pragma once
#include <Windows.h>
#include <functional>

namespace Mantrax
{
    enum class KeyCode
    {
        W,
        A,
        S,
        D,
        Space,
        Shift,
        Ctrl,
        Escape,
        LeftMouse,
        RightMouse,
        MiddleMouse
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

        int KeyCodeToIndex(KeyCode key) const;
        KeyCode VKToKeyCode(WPARAM vk) const;
    };
}