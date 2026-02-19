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
#ifndef VULKANBUFFERINSTANCE_H
#define VULKANBUFFERINSTANCE_H

#include <vulkan/vulkan_core.h>

#include "graphics/graphics_toplevel_defines.h"
#include "external/vk_mem_alloc.h"


namespace VulkanObjects
{
  class VulkanDeviceInstance;

  class VulkanBufferInstance final : public GraphicsTypes::BufferInstance
  {
    size_t         Size       = 0;
    VkBuffer       Buffer     = nullptr;
    VmaAllocation  Allocation = nullptr;

    const VulkanDeviceInstance *Device;
  public:
    explicit VulkanBufferInstance(
      const VulkanDeviceInstance     *device,
      GraphicsTypes::BufferType       type,
      size_t                          size);

    ~VulkanBufferInstance() override;

    VulkanBufferInstance(const VulkanBufferInstance &other) = delete;
    VulkanBufferInstance(VulkanBufferInstance &&other) noexcept = delete;
    VulkanBufferInstance &operator=(const VulkanBufferInstance &other) = delete;
    VulkanBufferInstance &operator=(VulkanBufferInstance &&other) noexcept = delete;

    void Map(void **map_memory) const;

    void UnMap() const;

    [[nodiscard]] VkBuffer Get()     const { return Buffer; }
    [[nodiscard]] size_t   GetSize() const { return Size; }
  };
}


#endif //VULKANBUFFERINSTANCE_H
