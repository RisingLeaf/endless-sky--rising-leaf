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

#ifndef ASTROLATIVE_FIELD_H
#define ASTROLATIVE_FIELD_H

#include <execution>
#include <functional>


#include "base/ASLList.h"
#include "gm.h"
#include "vector.h"

namespace gm
{
  template <typename Type>
    requires concepts::mathematics::zero_asignable<Type> && concepts::mathematics::subractable<Type> &&
             concepts::mathematics::multipliable<Type> && concepts::mathematics::divisable<Type> && concepts::mathematics::addable<Type> &&
             concepts::mathematics::subractable<Type>
  class field
  {
    std::vector<Type> data;
    asl::uint32       width, height;

    template <typename Other>
      requires concepts::mathematics::zero_asignable<Other> && concepts::mathematics::subractable<Other> &&
               concepts::mathematics::multipliable<Other> && concepts::mathematics::divisable<Other> &&
               concepts::mathematics::addable<Other> && concepts::mathematics::subractable<Other>
    friend class field;

  public:
    field(const std::vector<Type> &init, const asl::uint32 _width, const asl::uint32 _height) : data(init), width(_width), height(_height)
    {
      assert(width >= 2 && height >= 2 && "A field must be at least of dimension (2, 2)");
      assert(init.size() == width * height);
    }
    field(const asl::uint32 _width, const asl::uint32 _height) : data(_width * _height, Type(0)), width(_width), height(_height)
    {
      assert(width >= 2 && height >= 2 && "A field must be at least of dimension (2, 2)");
    }
    explicit field(const asl::uint32 dim) : data(dim * dim, 0), width(dim), height(dim)
    {
      assert(width >= 2 && height >= 2 && "A field must be at least of dimension (2, 2)");
    }

    Type &operator[](const asl::uint32 x, const asl::uint32 y)
    {
      const asl::uint32 index = x * height + y;
      assert(index < data.size() && "Index invalid");
      return data[index];
    }
    const Type &operator[](const asl::uint32 x, const asl::uint32 y) const
    {
      const asl::uint32 index = x * height + y;
      assert(index < data.size() && "Index invalid");
      return data[index];
    }

    Type &operator[](const vector<int, 2> &vi)
    {
      assert(vi[0] >= 0 && vi[1] >= 0 && "Indices must be greater 0");
      const asl::uint32 index = vi[0] * height + vi[1];
      assert(index < data.size() && "Index invalid");
      return data[index];
    }
    const Type &operator[](const vector<int, 2> &vi) const
    {
      assert(vi[0] >= 0 && vi[1] >= 0 && "Indices must be greater 0");
      const asl::uint32 index = vi[0] * height + vi[1];
      assert(index < data.size() && "Index invalid");
      return data[index];
    }

    void MakeZero()
    {
      for(auto &elem : data)
        elem = Type(0);
    }

    void Clamp(const Type min, const Type max)
      requires concepts::mathematics::comparable<Type>
    {
      for(auto &elem : data)
        elem = asl::clamp(elem, min, max);
    }

    [[nodiscard]] asl::uint32 Width() const { return width; }
    [[nodiscard]] asl::uint32 Height() const { return height; }

    [[nodiscard]] Type Min() const
      requires concepts::mathematics::comparable<Type>
    {
      Type minimum = std::numeric_limits<Type>::max();
      for(const auto &elem : data)
        if(elem < minimum)
          minimum = elem;
      return minimum;
    }
    [[nodiscard]] Type Max() const
      requires concepts::mathematics::comparable<Type>
    {
      Type maximum = std::numeric_limits<Type>::min();
      for(const auto &elem : data)
        if(elem > maximum)
          maximum = elem;
      return maximum;
    }
    [[nodiscard]] Type Sum() const
    {
      Type sum = Type(0);
      for(const auto &elem : data)
        sum += elem;
      return sum;
    }


    constexpr field operator+(const field &other) const noexcept
    {
      field result(width, height);
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          result[x, y] = (*this)[x, y] + other[x, y];
      return result;
    }
    constexpr void operator+=(const field &other) noexcept
    {
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          (*this)[x, y] += other[x, y];
    }

    constexpr field operator-() const noexcept
    {
      field result(width, height);
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          result[x, y] = -(*this)[x, y];
      return result;
    }
    constexpr field operator-(const field &other) const noexcept
    {
      field result(width, height);
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          result[x, y] = (*this)[x, y] + other[x, y];
      return result;
    }
    constexpr void operator-=(const field &other) noexcept
    {
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          (*this)[x, y] -= other[x, y];
    }

    constexpr field operator*(const Type &other) const noexcept
    {
      field result(width, height);
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          result[x, y] = (*this)[x, y] * other;
      return result;
    }
    constexpr void operator*=(const Type &other) noexcept
    {
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          (*this)[x, y] *= other;
    }
    constexpr field operator/(const Type &other) const noexcept
    {
      field result(width, height);
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          result[x, y] = (*this)[x, y] / other;
      return result;
    }
    constexpr void operator/=(const Type &other) noexcept
    {
      for(asl::uint32 x = 0; x < width; ++x)
        for(asl::uint32 y = 0; y < height; ++y)
          (*this)[x, y] /= other;
    }

    template <class F>
    [[nodiscard]] field<std::invoke_result_t<F, const Type &>> Apply(F &&rule) const
    {
      using R_Type = std::invoke_result_t<F, const Type &>;
      field<R_Type> result(width, height);

      const auto                count        = data.size();
      const auto                thread_count = std::min<std::size_t>(std::thread::hardware_concurrency(), count);
      std::vector<std::jthread> threads;
      threads.reserve(thread_count);

      const auto chunk_size = (count + thread_count - 1) / thread_count;
      for(std::size_t t = 0; t < thread_count; ++t)
      {
        const auto begin = t * chunk_size;
        const auto end   = std::min(begin + chunk_size, count);

        if(begin >= end)
          break;
        threads.emplace_back(
            [this, &result, &rule, begin, end]
            {
              for(auto i = begin; i < end; ++i)
                result.data[i] = rule(data[i]);
            });
      }
      return result;
    }
    template <class F>
    [[nodiscard]] field<std::invoke_result_t<F, const Type &, asl::uint32, asl::uint32>> ApplyPositional(F &&rule) const
    {
      using R_Type = std::invoke_result_t<F, const Type &, asl::uint32, asl::uint32>;
      field<R_Type> result(width, height);

      const auto                count        = data.size();
      const auto                thread_count = std::min<std::size_t>(std::thread::hardware_concurrency(), count);
      std::vector<std::jthread> threads;
      threads.reserve(thread_count);

      const auto chunk_size = (count + thread_count - 1) / thread_count;
      for(std::size_t t = 0; t < thread_count; ++t)
      {
        const auto begin = t * chunk_size;
        const auto end   = std::min(begin + chunk_size, count);

        if(begin >= end)
          break;

        threads.emplace_back(
            [this, &result, &rule, begin, end]
            {
              for(auto i = begin; i < end; ++i)
              {
                const auto x   = static_cast<asl::uint32>(i / height);
                const auto y   = static_cast<asl::uint32>(i % height);
                result.data[i] = rule(data[i], x, y);
              }
            });
      }

      return result;
    }

    // Returns a field of equal size but with the gradient at each point,
    // by default the gradient is calculated with (x, y), (x + 1,y) and (x, y + 1),
    // except at points where this is impossible (edges) where x-1 and y-1 are used respectively,
    // this results in constant gradient at these points.
    [[nodiscard]] field<vector<Type, 2>> GetGradientField(const asl::uint32 interval) const
    {
      field<vector<Type, 2>> result(width, height);

      if(width == 0 || height == 0)
        return result;

      for(asl::uint32 x = 0; x < width; ++x)
      {
        for(asl::uint32 y = 0; y < height; ++y)
        {
          const asl::uint32 xm = x > interval ? x - interval : 0;
          const asl::uint32 xp = x + interval < width ? x + interval : width - 1;

          const asl::uint32 ym = y > interval ? y - interval : 0;
          const asl::uint32 yp = y + interval < height ? y + interval : height - 1;

          const Type dx = ((*this)[xp, y] - (*this)[xm, y]) / Type(2. * (xp - xm));
          const Type dy = ((*this)[x, yp] - (*this)[x, ym]) / Type(2. * (yp - ym));

          result[x, y] = vector<Type, 2>(dx, dy);
        }
      }

      return result;
    }

    // Zero padded convolution filter
    template <asl::uint32 dim>
    [[nodiscard]] field Convolution(const matrix<float, dim> &filter) const
    {
      static_assert(dim > 0 && dim % 2 == 1, "Convolution filter dimension must be a positive odd number");
      field                 result(width, height);
      constexpr asl::uint32 radius = (dim - 1) / 2;
      for(asl::uint32 x = 0; x < width; ++x)
      {
        for(asl::uint32 y = 0; y < height; ++y)
        {
          Type sum{ 0 };
          for(asl::uint32 fx = 0; fx < dim; ++fx)
          {
            for(asl::uint32 fy = 0; fy < dim; ++fy)
            {
              const asl::int32 sx = static_cast<asl::int32>(x) + static_cast<asl::int32>(fx) - static_cast<asl::int32>(radius);
              const asl::int32 sy = static_cast<asl::int32>(y) + static_cast<asl::int32>(fy) - static_cast<asl::int32>(radius);

              if(sx < 0 || sy < 0 || sx >= static_cast<asl::int32>(width) || sy >= static_cast<asl::int32>(height))
                continue;

              sum += (*this)[static_cast<asl::uint32>(sx), static_cast<asl::uint32>(sy)] * filter[fx, fy];
            }
          }

          result[x, y] = sum;
        }
      }

      return result;
    }

    [[nodiscard]] field UpscaleBilinear(const asl::uint8 factor) const
    {
      assert(factor > 0 && "Upscaling factor must be greater than zero");

      if(factor == 1)
        return *this;

      const asl::uint32 new_width  = width * factor;
      const asl::uint32 new_height = height * factor;

      field result(new_width, new_height);

      struct AxisSample
      {
        asl::uint32 i0;
        asl::uint32 i1;
        float       t;
      };

      std::vector<AxisSample> x_samples(new_width);
      std::vector<AxisSample> y_samples(new_height);

      const float scale = 1.0f / static_cast<float>(factor);

      for(asl::uint32 x = 0; x < new_width; ++x)
      {
        const float src = static_cast<float>(x) * scale;
        const auto  i0  = static_cast<asl::uint32>(src);

        x_samples[x] = { i0, std::min(i0 + 1, width - 1), src - i0 };
      }

      for(asl::uint32 y = 0; y < new_height; ++y)
      {
        const float src = static_cast<float>(y) * scale;
        const auto  i0  = static_cast<asl::uint32>(src);

        y_samples[y] = { i0, std::min(i0 + 1, height - 1), src - i0 };
      }

      for(asl::uint32 x = 0; x < new_width; ++x)
      {
        const auto &xs = x_samples[x];

        for(asl::uint32 y = 0; y < new_height; ++y)
        {
          const auto &ys = y_samples[y];

          const Type p00 = (*this)[xs.i0, ys.i0];
          const Type p10 = (*this)[xs.i1, ys.i0];
          const Type p01 = (*this)[xs.i0, ys.i1];
          const Type p11 = (*this)[xs.i1, ys.i1];

          const Type tx = static_cast<Type>(xs.t);
          const Type ty = static_cast<Type>(ys.t);

          const Type top    = p00 + (p10 - p00) * tx;
          const Type bottom = p01 + (p11 - p01) * tx;

          result[x, y] = top + (bottom - top) * ty;
        }
      }

      return result;
    }
  };
  // clang-format off
  constexpr matrix<float, 3> convolution_gaussian3 = {
    1./16., 2./16., 1./16.,
    2./16., 4./16., 2./16.,
    1./16., 2./16., 1./16.,
  };
  constexpr matrix<float, 5> convolution_gaussian5 = {
    01./256., 04./256., 06./256., 04./256., 01./256.,
    04./256., 16./256., 24./256., 16./256., 04./256.,
    06./256., 24./256., 36./256., 24./256., 06./256.,
    04./256., 16./256., 24./256., 16./256., 04./256.,
    01./256., 04./256., 06./256., 04./256., 01./256.,
  };
  // clang-format on
} // namespace gm

#endif // ASTROLATIVE_FIELD_H
