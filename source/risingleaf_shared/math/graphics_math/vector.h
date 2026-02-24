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
#ifndef VULKAN_ENV_VECTOR_H
#define VULKAN_ENV_VECTOR_H

#include <cassert>
#include <initializer_list>
#include <string>
#include <typeinfo>

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
  class vector
  {
    Type Values[Dimension];
  public:
    template<typename Other>
    explicit constexpr vector(const vector<Other, Dimension> &from) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        Values[i] = static_cast<Type>(from[i]);
    }

    constexpr vector() noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        Values[i] = static_cast<Type>(0);
    }

    constexpr explicit vector(const Type &val) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        Values[i] = val;
    }

    template <typename... Args, typename = std::enable_if_t<
                sizeof...(Args) == Dimension
                && std::conjunction_v<std::is_convertible<Args, Type>...>>>
    explicit constexpr vector(Args &&...args) noexcept
    : Values{static_cast<Type>(std::forward<Args>(args))...}
    {
    }

    constexpr vector(const std::initializer_list<Type> init) noexcept
    {
      assert(init.size() == Dimension);
      asl::uint32 i = 0;
      for (auto e : init)
      {
        Values[i] = e;
        ++i;
      }
    }

    constexpr vector(const vector<Type, Dimension - 1> &base, const Type a) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension - 1; i++)
        Values[i] = base[i];
      Values[Dimension - 1] = a;
    }

    constexpr explicit vector(const vector<Type, Dimension + 1> &base) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        Values[i] = base[i];
    }

    constexpr       Type &operator[](const asl::uint32 i)       noexcept { assert(i < Dimension); return Values[i]; }
    constexpr const Type &operator[](const asl::uint32 i) const noexcept { assert(i < Dimension); return Values[i]; }

    constexpr bool operator==(const vector &other) const noexcept
    {
      for (asl::uint32 i = 0; i < Dimension; i++)
        if (this->Values[i] != other.Values[i])
          return false;
      return true;
    }

    constexpr vector operator+(const vector &other) const noexcept
    {
      vector result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        result.Values[i] = this->Values[i] + other.Values[i];
      return result;
    }
    constexpr void operator+=(const vector &other) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        this->Values[i] += other.Values[i];
    }

    constexpr vector operator-() const noexcept
    {
      vector result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        result.Values[i] = -this->Values[i];
      return result;
    }

    constexpr vector operator-(const vector &other) const noexcept
    {
      vector result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        result.Values[i] = this->Values[i] - other.Values[i];
      return result;
    }
    constexpr void operator-=(const vector &other) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        this->Values[i] -= other.Values[i];
    }

    constexpr vector operator*(const Type mult) const noexcept
    {
      vector result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        result.Values[i] = this->Values[i] * mult;
      return result;
    }
    constexpr void operator*=(const Type &mult) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        this->Values[i] *= mult;
    }

    constexpr vector operator/(const Type mult) const noexcept
    {
      vector result;
      for(asl::uint32 i = 0; i < Dimension; i++)
        result.Values[i] = this->Values[i] / mult;
      return result;
    }
    constexpr void operator/=(const Type &mult) noexcept
    {
      for(asl::uint32 i = 0; i < Dimension; i++)
        this->Values[i] /= mult;
    }

    constexpr Type dot(const vector &other) const noexcept
    {
      Type result = static_cast<Type>(0);
      for(asl::uint32 i = 0; i < Dimension; i++)
        result += this->Values[i] * other.Values[i];
      return result;
    }

    [[nodiscard]] std::string to_string() const
    {
      std::string out = "vec<" + std::to_string(Dimension) + "," + "?" + ">(";
      for(asl::uint32 i = 0; i < Dimension; i++)
      {
        out += std::to_string(Values[i]);
        if(i < Dimension - 1)
          out += ",";
      }
      out += ");";
      return out;
    }
  };
}

namespace std
{
  template<typename Type, asl::uint32 Dimension>
  std::string to_string(const gm::vector<Type, Dimension> &vec)
  {
    return vec.to_string();
  }
}

#endif // VULKAN_ENV_VECTOR_H
