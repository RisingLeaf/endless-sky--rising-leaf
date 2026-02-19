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

#ifndef MATRIX_H
#define MATRIX_H

#include <cassert>
#include <initializer_list>
#include <string>

#include "risingleaf_shared/base/ASLTypes.h"
#include "risingleaf_shared/base/concepts.h"

namespace gm
{
  template<typename Type, asl::uint32 Dimension>
  requires concepts::mathematics::addable<Type>
        && concepts::mathematics::subractable<Type>
        && concepts::mathematics::multipliable<Type>
        && concepts::mathematics::divisable<Type>
        && concepts::mathematics::zero_asignable<Type>
  class matrix
  {
    Type Values[Dimension][Dimension];

    using compatible_vector = vector<Type, Dimension>;

  public:
    template<typename Other>
    explicit constexpr matrix(const matrix<Other, Dimension> &from) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          Values[i][j] = static_cast<Type>(from[i, j]);
    }
    constexpr matrix() noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          Values[i][j] = static_cast<Type>(0);
    }
    constexpr explicit matrix(const Type &val) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          Values[i][j] = i==j ? val : static_cast<Type>(0);
    }
    constexpr matrix(std::initializer_list<Type> init) noexcept
    {
      assert(init.size() == Dimension * Dimension);
      asl::uint32 i = 0;
      for(auto e : init)
      {
        Values[i % Dimension][i / Dimension] = e;
        ++i;
      }
    }
    constexpr explicit matrix(const matrix<Type, Dimension + 1> &base) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          this->Values[i][j] = base[i, j];
    }
    constexpr explicit matrix(const matrix<Type, Dimension - 1> &base) noexcept
    {
      *this = matrix(static_cast<Type>(1));
      for(asl::uint32 i = 0; i < Dimension - 1; i++)
        for(asl::uint32 j = 0; j < Dimension - 1; j++)
          this->Values[i][j] = base[i, j];
    }

    constexpr       Type &operator[](const asl::uint32 i, const asl::uint32 j)       noexcept { assert(i < Dimension && j < Dimension); return Values[i][j]; }
    constexpr const Type &operator[](const asl::uint32 i, const asl::uint32 j) const noexcept { assert(i < Dimension && j < Dimension); return Values[i][j]; }

    constexpr bool operator==(const matrix &other) const noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          if (this->Values[i][j] != other.Values[i][j])
            return false;
      return true;
    }

    constexpr matrix operator+(const matrix &other) const noexcept
    {
      matrix result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          result.Values[i][j] = this->Values[i][j] + other.Values[i][j];
      return result;
    }
    constexpr void operator+=(const matrix &other) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          this->Values[i][j] += other.Values[i][j];
    }

    constexpr matrix operator-(const matrix &other) const noexcept
    {
      matrix result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          result.Values[i][j] = this->Values[i][j] - other.Values[i][j];
      return result;
    }

    constexpr void operator-=(const matrix &other) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          this->Values[i][j] -= other.Values[i][j];
    }

    constexpr matrix operator*(const Type &mult) const noexcept
    {
      matrix result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          result.Values[i][j] = this->Values[i][j] * mult;
      return result;
    }
    constexpr void operator*=(const Type &mult) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          this->Values[i][j] *= mult;
    }

    constexpr matrix operator*(const matrix &mult) const noexcept
    {
      matrix result(static_cast<Type>(0));
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
        {
          for(asl::uint32 k = 0; k < Dimension; k++)
            result.Values[i][j] += this->Values[i][k] * mult.Values[k][j];
        }
      return result;
    }
    constexpr void operator*=(const matrix &mult) noexcept
    {
      matrix result(static_cast<Type>(0));
      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
        {
          for(asl::uint32 k = 0; k < Dimension; k++)
            result.Values[i][j] += this->Values[i][k] * mult.Values[k][j];
        }
      *this = result;
    }

    constexpr compatible_vector operator*(const compatible_vector &mult) const noexcept
    {
      compatible_vector result(static_cast<Type>(0));

      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          result[j] += this->Values[i][j] * mult[i];
      return result;
    }

    constexpr matrix transpose() const noexcept
    {
      matrix result;

      for(asl::uint32 i = 0; i < Dimension; i++)
        for(asl::uint32 j = 0; j < Dimension; j++)
          result.Values[j][i] = this->Values[i][j];

      return result;
    }

    [[nodiscard]] std::string to_string() const
    {
      std::string out = "mat<" + std::to_string(Dimension) + "," + "?" + ">(\n";
      for(asl::uint32 i = 0; i < Dimension; i++)
      {
        out += "\t";
        for(asl::uint32 j = 0; j < Dimension; j++)
          out += std::to_string(Values[j][i]) + ", ";
        out += "\n";
      }
      out += ");";
      return out;
    }
  };
}

namespace std
{
  template<typename Type, asl::uint32 Dimension>
  std::string to_string(const gm::matrix<Type, Dimension> &mat)
  {
    return mat.to_string();
  }
}

#endif //MATRIX_H
