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
#ifndef VULKAN_ENV_VEC3_HELPERS_H
#define VULKAN_ENV_VEC3_HELPERS_H

#include <valarray>


#include "vector.h"

namespace gm
{
  template<typename Type>
  constexpr vector<Type, 3> cross(const vector<Type, 3> &a, const vector<Type, 3> &b)
  {
    return vector<Type, 3>{
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0]
    };
  }

  template<typename Type, asl::uint32 Dimension>
  constexpr Type dot(const vector<Type, Dimension> &a, const vector<Type, Dimension> &b)
  {
    return a.dot(b);
  }

  template<typename Type, asl::uint32 Dimension>
  constexpr vector<Type, Dimension> normalize(const vector<Type, Dimension> &a)
  {
    return a * (static_cast<Type>(1) / std::sqrt(a.dot(a)));
  }

  template<typename Type, asl::uint32 Dimension>
  constexpr Type length(const vector<Type, Dimension> &a)
  {
    return std::sqrt(a.dot(a));
  }

  template<typename Type, asl::uint32 Dimension>
  constexpr vector<Type, Dimension> operator*(const Type a, const vector<Type, Dimension> &b)
  {
    return b * a;
  }

  constexpr void hash_combine(size_t &seed, size_t hash)
  {
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
  }
}

#include <functional>
namespace std
{
  template<typename Type, asl::uint32 Dimension>
  struct hash<gm::vector<Type, Dimension>>
  {
    size_t operator()(gm::vector<Type, Dimension> const& v) const noexcept
    {
      std::hash<Type> hasher;
      size_t seed = 0;

      for (asl::uint32 i = 0; i < Dimension; ++i)
      {
        seed ^= hasher(v[i]) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
      }

      return seed;
    }
  };
}

#endif // VULKAN_ENV_VEC3_HELPERS_H
