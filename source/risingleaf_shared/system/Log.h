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
#ifndef LOG_H
#define LOG_H

#include <cstdint>
#include <string>

#include "../base/concepts.h"


namespace Log {
  struct EndValue {};
  constexpr EndValue End;

  class Stream
  {
    std::string_view Name;
    const uint8_t    Priority;

    mutable std::string current_buffer;
  public:
    Stream() = delete;
    Stream(const std::string_view name, const uint8_t priority)
    : Name(name), Priority(priority) {}

    const Stream &operator<<(const std::string_view in) const
    {
      current_buffer += in;
      return *this;
    }

    const Stream &operator<<(const char in) const
    {
      current_buffer += in;
      return *this;
    }

    template<class T>
    requires concepts::string::to_string_able<T>
    const Stream &operator<<(const T &in) const
    {
      current_buffer += std::to_string(in);
      return *this;
    }

    void operator<<(const EndValue &end) const;
  };

  extern thread_local Stream Info;
  extern thread_local Stream Warn;
  extern thread_local Stream Error;

  void ExitError(const std::string &message);

  void SetLogLevel(uint8_t level);
}



#endif //LOG_H
