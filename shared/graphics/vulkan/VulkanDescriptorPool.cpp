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
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see
//  <https://www.gnu.org/licenses/>.
//
#include "VulkanDescriptorPool.h"

#include <stdexcept>

#include "VulkanHelpers.h"


VulkanObjects::VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDeviceInstance *device) : Device(device)
{
  // Create Descriptor Pools:
  VkDescriptorPoolSize pool_size_ubo{};
  pool_size_ubo.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size_ubo.descriptorCount = 16'384; // just a huge number, pray I never need to change it
  VkDescriptorPoolSize pool_size_sampler{};
  pool_size_sampler.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size_sampler.descriptorCount = 8192; // just a huge number, pray I never need to change it
  VkDescriptorPoolSize pool_size_storage_image{};
  pool_size_storage_image.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_size_storage_image.descriptorCount = 8192; // just a huge number, pray I never need to change it
  VkDescriptorPoolSize pool_size_storage_buffer{};
  pool_size_storage_buffer.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_size_storage_buffer.descriptorCount = 8192; // just a huge number, pray I never need to change it

  const std::array pool_sizes = {pool_size_ubo, pool_size_sampler, pool_size_storage_image, pool_size_storage_buffer};

  VkDescriptorPoolCreateInfo descriptor_pool_info{};
  descriptor_pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_info.poolSizeCount = pool_sizes.size();
  descriptor_pool_info.pPoolSizes    = pool_sizes.data();
  descriptor_pool_info.flags         = 0;//VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  descriptor_pool_info.maxSets       = 16'384 + 8192 + 8192 + 8192;

  for(auto &pool : DescriptorPools)
    VulkanHelpers::VK_CHECK_RESULT(vkCreateDescriptorPool(Device->GetDevice(), &descriptor_pool_info, nullptr, &pool), __LINE__, __FILE__);
}

VulkanObjects::VulkanDescriptorPool::~VulkanDescriptorPool()
{
  for(const auto &pool : DescriptorPools)
  {
    VulkanHelpers::VK_CHECK_RESULT(vkResetDescriptorPool(Device->GetDevice(), pool, 0), __LINE__, __FILE__);
    vkDestroyDescriptorPool(Device->GetDevice(), pool, nullptr);
  }
}

VkDescriptorSet VulkanObjects::VulkanDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout layout) const
{
  VkDescriptorSet             set;
  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool     = DescriptorPools[FrameIndex];
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts        = &layout;

  VulkanHelpers::VK_CHECK_RESULT(vkAllocateDescriptorSets(Device->GetDevice(), &alloc_info, &set), __LINE__, __FILE__);

  return set;
}

void VulkanObjects::VulkanDescriptorPool::BeginFrame(const asl::uint8 frame_index)
{
  FrameIndex = frame_index;
  VulkanHelpers::VK_CHECK_RESULT(vkResetDescriptorPool(Device->GetDevice(), DescriptorPools[FrameIndex], 0), __LINE__, __FILE__);
}
