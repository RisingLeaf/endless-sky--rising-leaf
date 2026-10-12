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
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see
//  <https://www.gnu.org/licenses/>.
//

#include "perlin.h"

#include "math/graphics_math/gm.h"

#include <numbers>

namespace
{
  gm::vec2 random_gradient(const gm::ivec2 &i)
  {
    constexpr asl::uint32 w = 8 * sizeof(asl::uint32);
    constexpr asl::uint32 s = w / 2;
    asl::uint32           a = i[0], b = i[1];
    a *= 3'284'157'443;

    b ^= a << s | a >> (w - s);
    b *= 1'911'520'717;

    a ^= b << s | b >> (w - s);
    a *= 2'048'419'325;
    const float random = static_cast<float>(a) * static_cast<float>(std::numbers::pi / ~(~0u >> 1));

    gm::vec2 v;
    v[0] = std::sin(random);
    v[1] = std::cos(random);

    return v;
  }

  float dot_grid_gradient(const gm::ivec2 &i, const gm::vec2 &p)
  {
    const gm::vec2 gradient = random_gradient(i);
    const gm::vec2 delta    = p - gm::vec2(i);

    return gm::dot(gradient, delta);
  }

  float cubic_interpolate(const float a, const float b, const float w)
  {
    return (b - a) * (3.f - w * 2.f) * w * w + a;
  }
} // namespace

float noise::perlin(const gm::vec2 &point)
{
  const gm::ivec2 p0 = gm::ivec2(point);

  const gm::vec2 s = point - gm::vec2(p0);

  const float n00 = dot_grid_gradient(p0 + gm::ivec2{0, 0}, point);
  const float n01 = dot_grid_gradient(p0 + gm::ivec2{0, 1}, point);
  const float n10 = dot_grid_gradient(p0 + gm::ivec2{1, 0}, point);
  const float n11 = dot_grid_gradient(p0 + gm::ivec2{1, 1}, point);
  const float ix0 = cubic_interpolate(n00, n10, s[0]);
  const float ix1 = cubic_interpolate(n01, n11, s[0]);
  return cubic_interpolate(ix0, ix1, s[1]);
}
