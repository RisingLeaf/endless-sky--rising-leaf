//
// This file is part of Astrolative.
//
// Copyright (c) 2025 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see <https://www.gnu.org/licenses/>.
//
#include <sys/time.h>
#include <unistd.h>

#include "Timer.h"

static double time_program_start = 0;

int Timer::Init()
{
  time_program_start = 0;
  time_program_start = Timer::TimerSecD();
  return 1;
}
void Timer::Sleep(const int milli_seconds)
{
  usleep(1000 * milli_seconds);
}
void Timer::SleepUntil(const double time)
{
  double time_now = TimerSecD();

  while (time_now < time)
  {
      usleep(100);
      time_now = TimerSecD();
  }
}
double Timer::TimerSecD()
{
  timeval tt{};
  gettimeofday(&tt, nullptr);
  return static_cast<double>(tt.tv_sec) + 0.000001 * static_cast<double>(tt.tv_usec) - time_program_start;
}