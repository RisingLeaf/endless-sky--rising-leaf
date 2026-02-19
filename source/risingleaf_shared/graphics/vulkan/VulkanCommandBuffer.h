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
#ifndef VULKANCOMMANDBUFFER_H
#define VULKANCOMMANDBUFFER_H

#include "VulkanCommandBuffer.h"

#include <vulkan/vulkan_core.h>


namespace VulkanObjects {
  class VulkanCommandPool;
  class VulkanDeviceInstance;

  class VulkanSingleCommandBuffer
  {
    VkCommandBuffer cmd = nullptr;

    const VulkanDeviceInstance *Device;
    const VulkanCommandPool    *CommandPool;

    uint8_t state = 0;
  public:
    VulkanSingleCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool);

    void Begin();

    void End();

    ~VulkanSingleCommandBuffer();

    VulkanSingleCommandBuffer(const VulkanSingleCommandBuffer &other) = delete;
    VulkanSingleCommandBuffer(VulkanSingleCommandBuffer &&other) noexcept = delete;
    VulkanSingleCommandBuffer &operator=(const VulkanSingleCommandBuffer &other) = delete;
    VulkanSingleCommandBuffer &operator=(VulkanSingleCommandBuffer &&other) noexcept = delete;

    [[nodiscard]] VkCommandBuffer Get() const { return cmd; }
  };

  class VulkanSingleComputeCommandBuffer
  {
    VkCommandBuffer cmd = nullptr;

    const VulkanDeviceInstance *Device;
    const VulkanCommandPool    *CommandPool;

    uint8_t state = 0;
  public:
    VulkanSingleComputeCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool);

    void Begin();

    void End();

    ~VulkanSingleComputeCommandBuffer();

    VulkanSingleComputeCommandBuffer(const VulkanSingleComputeCommandBuffer &other) = delete;
    VulkanSingleComputeCommandBuffer(VulkanSingleComputeCommandBuffer &&other) noexcept = delete;
    VulkanSingleComputeCommandBuffer &operator=(const VulkanSingleComputeCommandBuffer &other) = delete;
    VulkanSingleComputeCommandBuffer &operator=(VulkanSingleComputeCommandBuffer &&other) noexcept = delete;

    [[nodiscard]] VkCommandBuffer Get() const { return cmd; }
  };
}


#endif //VULKANCOMMANDBUFFER_H
