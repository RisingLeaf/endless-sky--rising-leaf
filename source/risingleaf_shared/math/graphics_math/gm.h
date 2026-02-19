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
#ifndef VULKAN_ENV_GM_H
#define VULKAN_ENV_GM_H

#include "vector.h"
#include "matrix.h"

#include "vec_helpers.h"
#include "mat_helpers.h"

namespace gm
{
  typedef vector<int, 2> ivec2;
  typedef vector<int, 3> ivec3;
  typedef vector<int, 4> ivec4;

  typedef vector<float, 2> vec2;
  typedef vector<float, 3> vec3;
  typedef vector<float, 4> vec4;

  typedef vector<double, 2> dvec2;
  typedef vector<double, 3> dvec3;
  typedef vector<double, 4> dvec4;

  typedef matrix<float, 2> mat2;
  typedef matrix<float, 3> mat3;
  typedef matrix<float, 4> mat4;

  typedef matrix<double, 2> dmat2;
  typedef matrix<double, 3> dmat3;
  typedef matrix<double, 4> dmat4;
}

#endif // VULKAN_ENV_GM_H
