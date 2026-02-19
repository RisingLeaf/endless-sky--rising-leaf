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
#ifndef VULKANCOMMANDPOOL_H
#define VULKANCOMMANDPOOL_H

#include <vulkan/vulkan_core.h>


namespace VulkanObjects
{
  class VulkanDeviceInstance;

  class VulkanCommandPool final
  {
    VkCommandPool CommandPool = nullptr;

    const VulkanDeviceInstance *Device;
  public:
    explicit VulkanCommandPool(const VulkanDeviceInstance *device);

    ~VulkanCommandPool();

    VulkanCommandPool(const VulkanCommandPool &other) = delete;
    VulkanCommandPool(VulkanCommandPool &&other) noexcept = delete;
    VulkanCommandPool &operator=(const VulkanCommandPool &other) = delete;
    VulkanCommandPool &operator=(VulkanCommandPool &&other) noexcept = delete;

    [[nodiscard]] VkCommandPool Get() const { return CommandPool; }
  };
}



#endif //VULKANCOMMANDPOOL_H
