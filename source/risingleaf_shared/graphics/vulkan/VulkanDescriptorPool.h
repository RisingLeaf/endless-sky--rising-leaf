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
#ifndef VULKANDESCRIPTORPOOL_H
#define VULKANDESCRIPTORPOOL_H

#include <array>

#include <vulkan/vulkan_core.h>

#include "VulkanDeviceInstance.h"


namespace VulkanObjects
{
  class VulkanDescriptorPool final
  {
    std::array<VkDescriptorPool, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> DescriptorPools{nullptr};

    const VulkanDeviceInstance *Device;

  public:
    explicit VulkanDescriptorPool(const VulkanDeviceInstance *device);

    ~VulkanDescriptorPool();

    VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout) const;

    void BeginFrame() const;

    VulkanDescriptorPool(const VulkanDescriptorPool &other) = delete;
    VulkanDescriptorPool(VulkanDescriptorPool &&other) noexcept = delete;
    VulkanDescriptorPool &operator=(const VulkanDescriptorPool &other) = delete;
    VulkanDescriptorPool &operator=(VulkanDescriptorPool &&other) noexcept = delete;
  };
}



#endif //VULKANDESCRIPTORPOOL_H
