#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Mantrax
{
    // Enum para direcciones de movimiento
    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    enum class ProjectionMode
    {
        Perspective,
        Orthographic
    };

    class FPSCamera
    {
    public:
        FPSCamera(const glm::vec3 &position = glm::vec3(0.0f, 35.0f, 0.0f),
                  float fov = 60.0f,
                  float nearPlane = 0.1f,
                  float farPlane = 500.0f)
            : m_Position(position),
              m_FOV(fov),
              m_NearPlane(nearPlane),
              m_FarPlane(farPlane),
              m_Yaw(-90.0f),
              m_Pitch(0.0f),
              m_AspectRatio(16.0f / 9.0f),
              m_MovementSpeed(15.0f),
              m_MouseSensitivity(0.1f),
              m_SprintMultiplier(2.5f),
              m_WorldUp(0.0f, 1.0f, 0.0f),
              m_ProjectionMode(ProjectionMode::Perspective),
              m_OrthoSize(10.0f)
        {
            UpdateVectors();
        }

        // Obtener matrices
        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
        }

        glm::mat4 GetProjectionMatrix() const
        {
            glm::mat4 proj;

            if (m_ProjectionMode == ProjectionMode::Perspective)
            {
                proj = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
                proj[1][1] *= -1;
            }
            else // Orthographic
            {
                float halfWidth = m_OrthoSize * m_AspectRatio;
                float halfHeight = m_OrthoSize;

                // ✅ SOLUCIÓN: Para ortho, usa un rango más amplio y centrado
                float orthoNear = -100.0f; // Permite objetos DETRÁS de la cámara
                float orthoFar = 100.0f;   // Y delante

                proj = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, orthoNear, orthoFar);
                proj[1][1] *= -1;
            }

            return proj;
        }

        // Movimiento FPS
        void ProcessKeyboard(CameraMovement direction, float deltaTime, bool sprint = false)
        {
            float velocity = m_MovementSpeed * deltaTime;
            if (sprint)
                velocity *= m_SprintMultiplier;

            switch (direction)
            {
            case FORWARD:
                m_Position += m_Forward * velocity;
                break;
            case BACKWARD:
                m_Position -= m_Forward * velocity;
                break;
            case LEFT:
                m_Position -= m_Right * velocity;
                break;
            case RIGHT:
                m_Position += m_Right * velocity;
                break;
            case UP:
                m_Position += m_WorldUp * velocity;
                break;
            case DOWN:
                m_Position -= m_WorldUp * velocity;
                break;
            }
        }

        // Rotación con mouse
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
        {
            xoffset *= m_MouseSensitivity;
            yoffset *= m_MouseSensitivity;

            m_Yaw += xoffset;
            m_Pitch += yoffset;

            // Limitar pitch para evitar gimbal lock
            if (constrainPitch)
            {
                if (m_Pitch > 89.0f)
                    m_Pitch = 89.0f;
                if (m_Pitch < -89.0f)
                    m_Pitch = -89.0f;
            }

            UpdateVectors();
        }

        // Zoom con scroll
        void ProcessMouseScroll(float yoffset)
        {
            if (m_ProjectionMode == ProjectionMode::Perspective)
            {
                m_FOV -= yoffset;
                if (m_FOV < 1.0f)
                    m_FOV = 1.0f;
                if (m_FOV > 120.0f)
                    m_FOV = 120.0f;
            }
            else // Orthographic
            {
                m_OrthoSize -= yoffset * 0.1f;
                if (m_OrthoSize < 0.5f)
                    m_OrthoSize = 0.5f;
                if (m_OrthoSize > 100.0f)
                    m_OrthoSize = 100.0f;
            }
        }

        // Actualizar cuando cambie el tamaño de ventana
        void OnWindowResize(int width, int height)
        {
            if (height == 0)
                height = 1; // Prevenir división por cero
            m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        }

        // Cambiar modo de proyección
        void SetProjectionMode(ProjectionMode mode)
        {
            m_ProjectionMode = mode;
        }

        void ToggleProjectionMode()
        {
            m_ProjectionMode = (m_ProjectionMode == ProjectionMode::Perspective)
                                   ? ProjectionMode::Orthographic
                                   : ProjectionMode::Perspective;
        }

        // Setters
        void SetPosition(const glm::vec3 &position)
        {
            m_Position = position;
        }

        void SetAspectRatio(float aspect)
        {
            m_AspectRatio = aspect;
        }

        void SetMovementSpeed(float speed)
        {
            m_MovementSpeed = speed;
        }

        void SetMouseSensitivity(float sensitivity)
        {
            m_MouseSensitivity = sensitivity;
        }

        void SetFOV(float fov)
        {
            m_FOV = fov;
        }

        void SetOrthoSize(float size)
        {
            m_OrthoSize = glm::clamp(size, 0.5f, 100.0f);
        }

        void SetYaw(float yaw)
        {
            m_Yaw = yaw;
            UpdateVectors();
        }

        void SetPitch(float pitch)
        {
            m_Pitch = pitch;
            UpdateVectors();
        }

        // Getters
        glm::vec3 GetPosition() const { return m_Position; }
        glm::vec3 GetForward() const { return m_Forward; }
        glm::vec3 GetRight() const { return m_Right; }
        glm::vec3 GetUp() const { return m_Up; }
        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }
        float GetFOV() const { return m_FOV; }
        float GetMovementSpeed() const { return m_MovementSpeed; }
        float GetAspectRatio() const { return m_AspectRatio; }
        float GetOrthoSize() const { return m_OrthoSize; }
        ProjectionMode GetProjectionMode() const { return m_ProjectionMode; }
        bool IsPerspective() const { return m_ProjectionMode == ProjectionMode::Perspective; }
        bool IsOrthographic() const { return m_ProjectionMode == ProjectionMode::Orthographic; }

    private:
        void UpdateVectors()
        {
            // Calcular nuevo vector Forward
            glm::vec3 front;
            front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            front.y = sin(glm::radians(m_Pitch));
            front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            m_Forward = glm::normalize(front);

            // Recalcular Right y Up
            m_Right = glm::normalize(glm::cross(m_Forward, m_WorldUp));
            m_Up = glm::normalize(glm::cross(m_Right, m_Forward));
        }

        // Atributos de cámara
        glm::vec3 m_Position;
        glm::vec3 m_Forward;
        glm::vec3 m_Right;
        glm::vec3 m_Up;
        glm::vec3 m_WorldUp;

        // Ángulos de Euler
        float m_Yaw;
        float m_Pitch;

        // Opciones de cámara
        float m_MovementSpeed;
        float m_MouseSensitivity;
        float m_SprintMultiplier;

        // Proyección
        float m_FOV;
        float m_AspectRatio;
        float m_NearPlane;
        float m_FarPlane;

        // Modo de proyección
        ProjectionMode m_ProjectionMode;
        float m_OrthoSize; // Tamaño del viewport ortográfico
    };

}