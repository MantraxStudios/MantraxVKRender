#pragma once

#include <windows.h>
#include <unordered_map>

namespace MantraxGFX
{
    enum class KeyCode
    {
        W = 'W',
        A = 'A',
        S = 'S',
        D = 'D',
        Q = 'Q',
        E = 'E',
        R = 'R',
        F = 'F',
        Space = VK_SPACE,
        Shift = VK_SHIFT,
        Ctrl = VK_CONTROL,
        Alt = VK_MENU,
        Escape = VK_ESCAPE,
        Enter = VK_RETURN,
        Tab = VK_TAB,
        Up = VK_UP,
        Down = VK_DOWN,
        Left = VK_LEFT,
        Right = VK_RIGHT
    };

    enum class MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2
    };

    class Input
    {
    public:
        static Input &Get()
        {
            static Input instance;
            return instance;
        }

        void Update()
        {
            // Actualizar estados previos
            prevKeyStates = keyStates;
            prevMouseButtons = mouseButtons;

            mouseDeltaX = mouseX - prevMouseX;
            mouseDeltaY = mouseY - prevMouseY;
            prevMouseX = mouseX;
            prevMouseY = mouseY;

            mouseWheelDelta = 0.0f; // Reset wheel cada frame
        }

        // Keyboard
        bool IsKeyDown(KeyCode key) const
        {
            auto it = keyStates.find((int)key);
            return it != keyStates.end() && it->second;
        }

        bool IsKeyPressed(KeyCode key) const
        {
            auto curr = keyStates.find((int)key);
            auto prev = prevKeyStates.find((int)key);

            bool currDown = curr != keyStates.end() && curr->second;
            bool prevDown = prev != prevKeyStates.end() && prev->second;

            return currDown && !prevDown;
        }

        bool IsKeyReleased(KeyCode key) const
        {
            auto curr = keyStates.find((int)key);
            auto prev = prevKeyStates.find((int)key);

            bool currDown = curr != keyStates.end() && curr->second;
            bool prevDown = prev != prevKeyStates.end() && prev->second;

            return !currDown && prevDown;
        }

        // Mouse buttons
        bool IsMouseButtonDown(MouseButton button) const
        {
            auto it = mouseButtons.find((int)button);
            return it != mouseButtons.end() && it->second;
        }

        bool IsMouseButtonPressed(MouseButton button) const
        {
            auto curr = mouseButtons.find((int)button);
            auto prev = prevMouseButtons.find((int)button);

            bool currDown = curr != mouseButtons.end() && curr->second;
            bool prevDown = prev != prevMouseButtons.end() && prev->second;

            return currDown && !prevDown;
        }

        bool IsMouseButtonReleased(MouseButton button) const
        {
            auto curr = mouseButtons.find((int)button);
            auto prev = prevMouseButtons.find((int)button);

            bool currDown = curr != mouseButtons.end() && curr->second;
            bool prevDown = prev != prevMouseButtons.end() && prev->second;

            return !currDown && prevDown;
        }

        // Mouse position
        float GetMouseX() const { return mouseX; }
        float GetMouseY() const { return mouseY; }
        float GetMouseDeltaX() const { return mouseDeltaX; }
        float GetMouseDeltaY() const { return mouseDeltaY; }
        float GetMouseWheelDelta() const { return mouseWheelDelta; }

        // Internal - llamadas desde WindowProc
        void SetKeyState(int key, bool down)
        {
            keyStates[key] = down;
        }

        void SetMouseButton(int button, bool down)
        {
            mouseButtons[button] = down;
        }

        void SetMousePosition(float x, float y)
        {
            mouseX = x;
            mouseY = y;
        }

        void AddMouseWheelDelta(float delta)
        {
            mouseWheelDelta += delta;
        }

    private:
        Input() = default;

        std::unordered_map<int, bool> keyStates;
        std::unordered_map<int, bool> prevKeyStates;
        std::unordered_map<int, bool> mouseButtons;
        std::unordered_map<int, bool> prevMouseButtons;

        float mouseX = 0.0f;
        float mouseY = 0.0f;
        float prevMouseX = 0.0f;
        float prevMouseY = 0.0f;
        float mouseDeltaX = 0.0f;
        float mouseDeltaY = 0.0f;
        float mouseWheelDelta = 0.0f;
    };
}