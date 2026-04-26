#include "Timer.h"

Timer::Timer()
{
   m_startTime = std::chrono::high_resolution_clock::now();
}

double Timer::elapsedTime() const
{
   auto duration = std::chrono::high_resolution_clock::now() - m_startTime;
   return std::chrono::duration<double>( duration ).count();
}
