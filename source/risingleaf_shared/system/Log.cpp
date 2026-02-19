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
#include "Log.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <ostream>


namespace Log
{
  uint8_t LOG_LEVEL = 1;

  std::mutex io_mutex;

  thread_local Stream Info("INFO ", 1);
  thread_local Stream Warn("WARN ", 2);
  thread_local Stream Error("ERROR", 3);
}

void Log::Stream::operator<<(const EndValue &) const
{
  if(Priority >= LOG_LEVEL)
  {
    const auto time = std::time(nullptr);
    std::lock_guard guard(io_mutex);
    std::cout
      <<"["<<std::put_time(std::localtime(&time), "%T")<<" "
      <<Name<<"] "
      <<current_buffer<<std::endl;
  }

  current_buffer.clear();
}



void Log::ExitError(const std::string &message)
{
  std::cerr << message << std::endl;
  exit(1);
}



void Log::SetLogLevel(const uint8_t level)
{
  LOG_LEVEL = level;
}

