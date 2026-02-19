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
#ifndef VULKAN_ENV_ASLUTILITY_H
#define VULKAN_ENV_ASLUTILITY_H
#include "concepts.h"

#include <vector>

namespace asl
{
  template<typename T>
    requires
      concepts::mathematics::comparable<T>
  constexpr T min(T a, T b)
  {
    return a < b ? a : b;
  }

  template<typename T>
    requires
      concepts::mathematics::comparable<T>
  constexpr T max(T a, T b)
  {
    return a < b ? b : a;
  }

  template<typename T>
    requires
       concepts::mathematics::comparable<T>
    && concepts::mathematics::zero_asignable<T>
    && concepts::mathematics::subractable<T>
  constexpr T abs(T v)
  {
    return v < static_cast<T>(0) ? -v : v;
  }

  template<typename T>
    requires
      concepts::mathematics::comparable<T>
  constexpr T clamp(T v, T min, T max)
  {
    return v < min ? min : max < v ? max : v;
  }
  template<typename T, typename B>
    requires
         concepts::mathematics::subractable<T>
      && concepts::mathematics::addable<T>
  constexpr T mix(T a, T b, B i)
  {
    return a * i + b * (1. - i);
  }

  template<class T>
  T BiLinear(const std::vector<T> &values, const size_t tex_width, const size_t tex_height, const double phi, const double theta)
  {
    const auto   res_x = static_cast<float>(tex_width - 1);
    const auto   res_y = static_cast<float>(tex_height - 1);
    const size_t lo_x  = static_cast<int>(res_x * phi);
    const size_t lo_y  = static_cast<int>(res_y * theta);
    const size_t hi_x  = lo_x == tex_width  - 1 ? 0              : lo_x + 1;
    const size_t hi_y  = lo_y == tex_height - 1 ? tex_height - 2 : lo_y + 1;
    const float  x_fac = res_x * phi   - static_cast<float>(lo_x);
    const float  y_fac = res_y * theta - static_cast<float>(lo_y);
    const T p_00  = values[lo_x + lo_y  * tex_width];
    const T p_10  = values[hi_x + lo_y  * tex_width];
    const T p_01  = values[lo_x + hi_y  * tex_width];
    const T p_11  = values[hi_x + hi_y  * tex_width];
    constexpr auto Step = [](const float v) -> float { return v; };
    //constexpr auto Step = [](const float v) -> float { return 3.f * v * v - 2.f * v * v * v; };
    const T p_0   = p_00 + Step(x_fac) * (p_10 - p_00);
    const T p_1   = p_01 + Step(x_fac) * (p_11 - p_01);
    return          p_0  + Step(y_fac) * (p_1  - p_0);
  }

  template<typename Type> requires concepts::mathematics::multipliable<Type>
  constexpr Type COMP_TIME_POW(const Type base, const uint64 exponent)
  {
    Type result = base;
    for(uint64 i = 0; i < exponent - 1; i++)
    {
      result *= base;
    }
    return result;
  }
}

#endif // VULKAN_ENV_ASLUTILITY_H
