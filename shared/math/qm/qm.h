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

#ifndef ASTROLATIVE_QM_H
#define ASTROLATIVE_QM_H
#include <cmath>

namespace qm
{
  struct complex
  {
    // a + bi
    double a;
    double b;

    [[nodiscard]] complex conjugate() const
    {
      return {a, -b};
    }
    [[nodiscard]] double absolute() const
    {
      return std::sqrt(a*a + b*b);
    }
    [[nodiscard]] complex operator+(const complex other) const
    {
      return {a + other.a, b + other.b};
    }
    [[nodiscard]] complex operator-(const complex other) const
    {
      return {a - other.a, b - other.b};
    }
    [[nodiscard]] complex operator*(const complex other) const
    {
      return {a * other.a - b * other.b, a*other.b + b*other.a};
    }
    [[nodiscard]] complex operator/(const complex other) const
    {
      const double value = other.absolute();
      const complex reciprocal = {other.a/absolute(), -other.b/absolute()};
      return *this * reciprocal;
    }
    [[nodiscard]] complex exp() const
    {
      const double x_exp = std::exp(a);
      return {x_exp * std::cos(b), x_exp * std::sin(b)};
    }
  };

  namespace concepts
  {
    template <class T>
    concept scalar_product = requires(T a, T b, T c)
    {
      complex result = a * b;
    };
  }
}

#endif // ASTROLATIVE_QM_H
