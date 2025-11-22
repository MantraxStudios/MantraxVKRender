#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Mantrax
{
    class Camera
    {
    public:
        Camera(float distance = 3.0f, float fov = 45.0f, float nearPlane = 0.1f, float farPlane = 100.0f)
            : m_Distance(distance),
              m_FOV(fov),
              m_NearPlane(nearPlane),
              m_FarPlane(farPlane),
              m_Yaw(0.0f),
              m_Pitch(0.0f),
              m_Target(0.0f, 0.0f, 0.0f),
              m_AspectRatio(16.0f / 9.0f),
              m_MinDistance(1.0f),
              m_MaxDistance(20.0f),
              m_MinPitch(-89.0f),
              m_MaxPitch(89.0f)
        {
            UpdateVectors();
        }

        // Obtener matrices
        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(m_Position, m_Target, m_Up);
        }

        glm::mat4 GetProjectionMatrix() const
        {
            glm::mat4 proj = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearPlane, m_FarPlane);
            proj[1][1] *= -1; // Corrección para Vulkan
            return proj;
        }

        // Control orbital
        void Orbit(float deltaYaw, float deltaPitch)
        {
            m_Yaw += deltaYaw;
            m_Pitch += deltaPitch;

            // Limitar pitch para evitar gimbal lock
            if (m_Pitch > m_MaxPitch)
                m_Pitch = m_MaxPitch;
            if (m_Pitch < m_MinPitch)
                m_Pitch = m_MinPitch;

            UpdateVectors();
        }

        void Zoom(float delta)
        {
            m_Distance -= delta;

            if (m_Distance < m_MinDistance)
                m_Distance = m_MinDistance;
            if (m_Distance > m_MaxDistance)
                m_Distance = m_MaxDistance;

            UpdateVectors();
        }

        void Pan(float deltaX, float deltaY)
        {
            glm::vec3 right = glm::normalize(glm::cross(m_Forward, m_Up));
            glm::vec3 up = glm::normalize(glm::cross(right, m_Forward));

            m_Target += right * deltaX * m_Distance * 0.01f;
            m_Target += up * deltaY * m_Distance * 0.01f;

            UpdateVectors();
        }

        // Setters
        void SetAspectRatio(float aspect)
        {
            m_AspectRatio = aspect;
        }

        void SetTarget(const glm::vec3 &target)
        {
            m_Target = target;
            UpdateVectors();
        }

        void SetDistance(float distance)
        {
            m_Distance = glm::clamp(distance, m_MinDistance, m_MaxDistance);
            UpdateVectors();
        }

        void SetFOV(float fov)
        {
            m_FOV = fov;
        }

        void SetDistanceLimits(float minDist, float maxDist)
        {
            m_MinDistance = minDist;
            m_MaxDistance = maxDist;
        }

        void SetPitchLimits(float minPitch, float maxPitch)
        {
            m_MinPitch = minPitch;
            m_MaxPitch = maxPitch;
        }

        // Getters
        glm::vec3 GetPosition() const { return m_Position; }
        glm::vec3 GetTarget() const { return m_Target; }
        glm::vec3 GetForward() const { return m_Forward; }
        float GetDistance() const { return m_Distance; }
        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }

    private:
        void UpdateVectors()
        {
            // Calcular posición de la cámara en coordenadas esféricas
            float yawRad = glm::radians(m_Yaw);
            float pitchRad = glm::radians(m_Pitch);

            m_Position.x = m_Target.x + m_Distance * cos(pitchRad) * sin(yawRad);
            m_Position.y = m_Target.y + m_Distance * sin(pitchRad);
            m_Position.z = m_Target.z + m_Distance * cos(pitchRad) * cos(yawRad);

            m_Forward = glm::normalize(m_Target - m_Position);
            m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Propiedades de la cámara
        glm::vec3 m_Position;
        glm::vec3 m_Target;
        glm::vec3 m_Forward;
        glm::vec3 m_Up;

        float m_Distance;
        float m_Yaw;
        float m_Pitch;

        // Proyección
        float m_FOV;
        float m_AspectRatio;
        float m_NearPlane;
        float m_FarPlane;

        // Límites
        float m_MinDistance;
        float m_MaxDistance;
        float m_MinPitch;
        float m_MaxPitch;
    };
} // namespace Mantrax