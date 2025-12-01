#pragma once

#include <chrono>

namespace Mantrax
{
    class Timer
    {
    public:
        Timer()
            : m_DeltaTime(0.0f),
              m_TotalTime(0.0f),
              m_MaxDeltaTime(0.1f),
              m_FrameCount(0),
              m_FPSFrameCount(0),
              m_FPS(0)
        {
            Start();
        }

        // Inicializa/reinicia el timer
        void Start()
        {
            m_StartTime = Clock::now();
            m_LastFrameTime = m_StartTime;
            m_FPSCounterTime = m_StartTime;
            m_DeltaTime = 0.0f;
            m_TotalTime = 0.0f;
            m_FrameCount = 0;
            m_FPSFrameCount = 0;
            m_FPS = 0;
        }

        // Actualiza el timer y calcula delta time
        void Update()
        {
            auto currentTime = Clock::now();

            // Calcular delta time
            std::chrono::duration<float> deltaTimeDuration = currentTime - m_LastFrameTime;
            m_DeltaTime = deltaTimeDuration.count();

            // Limitar delta time para evitar saltos grandes
            if (m_DeltaTime > m_MaxDeltaTime)
                m_DeltaTime = m_MaxDeltaTime;

            m_LastFrameTime = currentTime;

            // Actualizar tiempo total
            std::chrono::duration<float> totalTimeDuration = currentTime - m_StartTime;
            m_TotalTime = totalTimeDuration.count();

            // Actualizar frame count
            m_FrameCount++;
            m_FPSFrameCount++;

            // Calcular FPS (cada segundo)
            std::chrono::duration<float> fpsElapsed = currentTime - m_FPSCounterTime;
            if (fpsElapsed.count() >= 1.0f)
            {
                m_FPS = m_FPSFrameCount;
                m_FPSFrameCount = 0;
                m_FPSCounterTime = currentTime;
            }
        }

        // Getters
        float GetDeltaTime() const { return m_DeltaTime; }
        float GetTotalTime() const { return m_TotalTime; }
        int GetFPS() const { return m_FPS; }
        int GetFrameCount() const { return m_FrameCount; }

        // Configuración
        void SetMaxDeltaTime(float maxDelta) { m_MaxDeltaTime = maxDelta; }
        float GetMaxDeltaTime() const { return m_MaxDeltaTime; }

    private:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        TimePoint m_StartTime;
        TimePoint m_LastFrameTime;
        TimePoint m_FPSCounterTime;

        float m_DeltaTime;
        float m_TotalTime;
        float m_MaxDeltaTime;

        int m_FrameCount;
        int m_FPSFrameCount;
        int m_FPS;
    };

}