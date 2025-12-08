#include "../include/MantraxGFX_Timer.h"

namespace Mantrax
{
    Timer::Timer()
        : m_DeltaTime(0.0f),
          m_TotalTime(0.0f),
          m_MaxDeltaTime(0.1f),
          m_FrameCount(0),
          m_FPSFrameCount(0),
          m_FPS(0)
    {
        Start();
    }

    void Timer::Start()
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

    void Timer::Update()
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

}