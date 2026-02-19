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
#ifndef VULKAN_ENV_ASLVECTORALLOCATOR_H
#define VULKAN_ENV_ASLVECTORALLOCATOR_H

#include "ASLTypes.h"
#include "ASLUtility.h"
#include "system/Log.h"

namespace asl
{
  template<typename Type>
  class list_allocator_base
  {
  public:
    Type        *memory;
    asl::uint32  used  = 0;

  protected:
    asl::uint32  space = 0;

    virtual constexpr asl::uint32 get_next_recommended_size(const asl::uint32 size)
    {
      return size > 0 ? size * 2 : 4;
    }

  public:
    list_allocator_base()
    {
      space  = list_allocator_base::get_next_recommended_size(0);
      memory = static_cast<Type *>(calloc(space, sizeof(Type)));
    }

    virtual ~list_allocator_base()
    {
      free(memory);
    }

    list_allocator_base(const list_allocator_base &other) = delete;
    list_allocator_base(list_allocator_base &&other) noexcept = delete;
    list_allocator_base &operator=(const list_allocator_base &other) = delete;
    list_allocator_base &operator=(list_allocator_base &&other) noexcept = delete;

    void swap(list_allocator_base &other) noexcept
    {
      Type        *inter_memory = this->memory;
      asl::uint32  inter_used   = this->used;
      asl::uint32  inter_space  = this->space;

      this->memory = other.memory; other.memory = inter_memory;
      this->used   = other.used;   other.used   = inter_used;
      this->space  = other.space;  other.space  = inter_space;
    }

    void resize(const asl::uint32 size)
    {
      if(space == size)
        return;
      space  = size;
      used   = size;
      memory = static_cast<Type *>(realloc(static_cast<void *>(memory), space * sizeof(Type)));
    }

    void reset()
    {
      space  = get_next_recommended_size(0);
      used   = 0;
      memory = static_cast<Type *>(realloc(static_cast<void *>(memory), space * sizeof(Type)));
    }

    void increase()
    {
      space  = get_next_recommended_size(space);
      memory = static_cast<Type *>(realloc(static_cast<void *>(memory), space * sizeof(Type)));
    }

    [[nodiscard]] asl::uint32 get_space() const { return space; }
  };

  template<typename Type>
  class list_allocator_linear_growth final : public list_allocator_base<Type>
  {
    asl::uint32 get_next_recommended_size(const asl::uint32 size) override
    {
      return size + 1;
    }
  };
}

#endif // VULKAN_ENV_ASLVECTORALLOCATOR_H
