#pragma once

#include <chrono>

class Timer
{
public:
   Timer();

   double elapsedTime() const;

public:
   std::chrono::high_resolution_clock::time_point m_startTime;
};