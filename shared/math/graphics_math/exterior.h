//
// This file is part of Astrolative.
//
// Copyright (c) 2026 by Torben Hans
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

#ifndef ASTROLATIVE_EXTERIOR_H
#define ASTROLATIVE_EXTERIOR_H
#include "matrix.h"
#include "vector.h"

namespace gm
{
  template <typename T, asl::uint32 N>
  constexpr matrix<T, N> outer(const vector<T, N> &a, const vector<T, N> &b)
  {
    matrix<T, N> result{};
    for(asl::uint32 i = 0; i < N; i++)
      for(asl::uint32 j = 0; j < N; j++)
        result[i, j] = a[i] * b[j];
    return result;
  }

  template <typename T, asl::uint32 N>
  constexpr matrix<T, N> exterior(const vector<T, N> &a, const vector<T, N> &b)
  {
    matrix<T, N> result{};
    for(asl::uint32 i = 0; i < N; ++i)
    {
      result[i, i] = T{};
      for(asl::uint32 j = i + 1; j < N; ++j)
      {
        T v          = a[i] * b[j] - b[i] * a[j];
        result[i, j] = v;
        result[j, i] = -v;
      }
    }
    return result;
  }

  consteval asl::uint32 bivector_dim(const asl::uint32 N)
  {
    return N * (N - 1) / 2;
  }

  template <typename T, asl::uint32 N>
  constexpr vector<T, bivector_dim(N)> red_exterior(const vector<T, N> &a, const vector<T, N> &b)
  {
    vector<T, bivector_dim(N)> result{};
    asl::uint32                k = 0;
    for(asl::uint32 i = 0; i < N; ++i)
    {
      for(asl::uint32 j = i + 1; j < N; ++j)
      {
        result[k] = a[i] * b[j] - b[i] * a[j];
        ++k;
      }
    }
    return result;
  }

  template <typename T, asl::uint32 N>
  struct bivector
  {
    T                          Scalar;
    vector<T, bivector_dim(N)> Vectorial;
  };

  template <typename T, asl::uint32 N>
  bivector<T, N> geometric(const vector<T, N> &a, const vector<T, N> &b)
  {
    bivector<T, N> result;
    result.Scalar    = a.dot(b);
    result.Vectorial = red_exterior(a, b);
    return result;
  }
} // namespace gm

#endif // ASTROLATIVE_EXTERIOR_H
