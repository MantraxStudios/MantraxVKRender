#pragma once
#include <chrono>
#include "../../MantraxECS/include/EngineLoaderDLL.h"

namespace Mantrax
{
    class MANTRAX_API Timer
    {
    public:
        Timer();

        // Inicializa/reinicia el timer
        void Start();

        // Actualiza el timer y calcula delta time
        void Update();

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