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
#include "VulkanCommandPool.h"

#include <stdexcept>

#include "VulkanBootstrap.h"
#include "VulkanDeviceInstance.h"
#include "VulkanHelpers.h"


VulkanObjects::VulkanCommandPool::VulkanCommandPool(const VulkanObjects::VulkanDeviceInstance *device)
: Device(device)
{
  const VulkanHelpers::QueueFamilyIndices queue_family_indices = VulkanHelpers::FindQueueFamilies(Device->GetPhysicalDevice(), Device->GetSurface());
  const auto command_pool_create_info = VulkanBootstrap::GetCommandPoolCreate(queue_family_indices.GraphicsFamily.value());
  VulkanHelpers::VK_CHECK_RESULT(vkCreateCommandPool(Device->GetDevice(), &command_pool_create_info, nullptr, &CommandPool), __LINE__, __FILE__);
}

VulkanObjects::VulkanCommandPool::~VulkanCommandPool()
{
  if(CommandPool) vkDestroyCommandPool(Device->GetDevice(), CommandPool, nullptr);
}