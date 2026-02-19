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
#ifndef ASLVECTOR_H
#define ASLVECTOR_H

#include <cassert>
#include <initializer_list>
#include <memory>


#include "ASLTypes.h"
#include "ASLListAllocator.h"


namespace asl
{
  template<typename Type, class allocator = list_allocator_base<Type>>
    requires
      concepts::inheritance::is_base_of<list_allocator_base<Type>, allocator>
  class list {
  public:
    using value_type           = Type;
    using const_reference_type = const value_type &;

    class iterator
    {
    public:
      using iterator_category = std::random_access_iterator_tag;
      using value_type        = Type;
      using difference_type   = ptrdiff_t;
      using pointer           = Type*;
      using reference         = Type&;

    private:
      const list  *base;
      asl::uint32  index;

    public:
      iterator() : base(nullptr), index(0) {}
      iterator(const asl::uint32 index, list &base) : base(&base), index(index) {}

      [[nodiscard]] iterator &operator=(const iterator &other) = default;

      iterator operator+(const asl::uint32 increment) { return iterator(index + increment, *base); }

      Type *operator->() const { assert( index < base->alloc.used ); return &base->alloc.memory[index]; }
      operator Type *()  const { assert( index < base->alloc.used ); return &base->alloc.memory[index]; }

      bool operator==( const iterator &it ) const { return base == it.base && index == it.index; }
      bool operator!=( const iterator &it ) const { return base != it.base || index != it.index; }

      iterator &operator++() { ++index; return *this; }

      [[nodiscard]] bool        is_valid()  const { return index < base->alloc.used; }
      [[nodiscard]] asl::uint32 get_index() const { return index; }
    };

    class const_iterator
    {
    public:
      using iterator_category = std::random_access_iterator_tag;
      using value_type        = Type;
      using difference_type   = ptrdiff_t;
      using pointer           = Type*;
      using reference         = Type&;

    private:
      const list  *const base;
      asl::uint32        index;

    public:
      const_iterator() : base(nullptr), index(0) {}
      const_iterator(const asl::uint32 index, const list &base) : base(&base), index(index) {}

      [[nodiscard]] const_iterator &operator=(const const_iterator &other) = default;

      const_iterator operator+(const asl::uint32 increment) { return const_iterator(index + increment, *base); }

      Type *operator->() const { assert( index < base->alloc.used ); return &base->alloc.memory[index]; }
      operator Type *()  const { assert( index < base->alloc.used ); return &base->alloc.memory[index]; }

      bool operator==( const iterator &it ) const { return base == it.base && index == it.index; }
      bool operator!=( const iterator &it ) const { return base != it.base || index != it.index; }

      iterator &operator++() { ++index; return *this; }

      [[nodiscard]] bool        is_valid()  const { return index < base->alloc.used; }
      [[nodiscard]] asl::uint32 get_index() const { return index; }
    };

  private:
    static constexpr asl::uint32 TYPE_SIZE = sizeof(Type);

    allocator alloc;

    static constexpr asl::uint32 get_next_size(const asl::uint32 size)
    {
      constexpr asl::uint32 MINIMUM_SIZE = TYPE_SIZE > 4 ? TYPE_SIZE : 4;
      return  MINIMUM_SIZE + (size * 3) / 2;
    }
  public:
    list() = default;

    explicit list(const asl::uint32 size)
    {
      alloc.resize(size);
    }

    list(const asl::uint32 size, const Type &fill_value)
    {
      alloc.resize(size);
      alloc.used = size;
      for(asl::uint32 i = 0; i < alloc.used; i++)
        alloc.memory[i] = fill_value;
    }

    list(const asl::uint32 size, std::initializer_list<Type> list)
    {
      alloc.resize(size);
      alloc.used = size;
      for(asl::uint32 i = 0; i < alloc.used; i++)
        std::construct_at(alloc.memory[i], list);
    }

    void clear()                 { alloc.used = 0;                }
    void swap(list<Type> &other) { this->alloc.swap(other.alloc); }
    void reserve(const asl::uint32 size)
    {
      if(size > alloc.get_space())
        alloc.resize(size);
    }

    void resize(const asl::uint32 size)
    {
      if(size > alloc.get_space())
        alloc.resize(size);
    }

    void emplace_back(const Type &t)
    {
      if(alloc.used == alloc.get_space())
        alloc.increase();
      alloc.memory[alloc.used] = t;
      ++alloc.used;
    }

    std::enable_if_t<std::is_default_constructible_v<Type>>
    emplace_back()
    {
      if(alloc.used == alloc.get_space())
        alloc.increase();
      alloc.memory[alloc.used] = Type();
      ++alloc.used;
    }

    void emplace_back(std::initializer_list<Type> list)
    {
      if(alloc.used == alloc.get_space())
        alloc.increase();
      std::construct_at(alloc.memory[alloc.used], list);
      ++alloc.used;
    }

    [[nodiscard]] asl::uint32 size()  const { return alloc.used; }
    [[nodiscard]] bool        empty() const { return alloc.used == 0; }
    [[nodiscard]]       Type *data()        { return alloc.memory; }
    [[nodiscard]] const Type *data()  const { return alloc.memory; }

          Type &operator[](const asl::uint32 index)       { return alloc.memory[index]; }
    const Type &operator[](const asl::uint32 index) const { return alloc.memory[index]; }
          Type &back()       { return alloc.memory[alloc.used - 1]; }
    const Type &back() const { return alloc.memory[alloc.used - 1]; }


    auto begin()        { return iterator(0, *this);          }
    auto end()          { return iterator(alloc.used, *this); }
    auto cbegin() const { return const_iterator(0, *this);          }
    auto cend()   const { return const_iterator(alloc.used, *this); }
  };
}



#endif //ASLVECTOR_H
