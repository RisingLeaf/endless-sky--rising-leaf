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
#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <type_traits>


namespace concepts::inheritance
{
  template <class Base, class Child>
  concept is_base_of = std::is_base_of_v<Base, Child>;
}

namespace concepts::string
{
  template <class T>
  concept to_string_able = requires(T a)
  {
    std::to_string(a);
  };
}

namespace concepts::mathematics
{
  template<class T>
  concept comparable = requires(T a, T b)
  {
    a < b;
  };

  template<class T>
  concept addable = requires(T a, T b)
  {
    a +  b;
    a += b;
  };

  template<class T>
  concept subractable = requires(T a, T b)
  {
    a -  b;
    a -= b;
  };

  template<class T>
  concept multipliable = requires(T a, T b)
  {
    a *  b;
    a *= b;
  };

  template<class T>
  concept divisable = requires(T a, T b)
  {
    a /  b;
    a /= b;
  };

  template<class T>
  concept zero_asignable = requires(T a)
  {
    a = static_cast<T>(0);
  };
}


#endif //CONCEPTS_H
