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
#include "VulkanBufferInstance.h"

#include "VulkanBootstrap.h"
#include "VulkanDeviceInstance.h"
#include "VulkanHelpers.h"

VulkanObjects::VulkanBufferInstance::VulkanBufferInstance(const VulkanDeviceInstance     *device,
                                                          const GraphicsTypes::BufferType type,
                                                          const size_t                    size,
                                                          const std::string_view          name) :
  BufferInstance(), Size(size), Device(device)
{
  VkMemoryPropertyFlags properties{};
  switch(type)
  {
  case GraphicsTypes::BufferType::UNIFORM:
  case GraphicsTypes::BufferType::UNIFORM_DYNAMIC:
  case GraphicsTypes::BufferType::TEXTURE:
  case GraphicsTypes::BufferType::STAGING:
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    break;
  case GraphicsTypes::BufferType::VERTEX: properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break;
  case GraphicsTypes::BufferType::VERTEX_DYNAMIC:
    properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    break;
  case GraphicsTypes::BufferType::INDEX: properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break;
  }

  const auto create_info = VulkanBootstrap::GetBufferCreate(type, size);

  VmaAllocationCreateInfo vma_allocation_create_info{};
  vma_allocation_create_info.usage         = VMA_MEMORY_USAGE_AUTO;
  vma_allocation_create_info.requiredFlags = properties;
  vma_allocation_create_info.flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VulkanHelpers::VK_CHECK_RESULT(
      vmaCreateBuffer(Device->GetAllocator(), &create_info, &vma_allocation_create_info, &Buffer, &Allocation, nullptr),
      __LINE__,
      __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(Buffer), name);
}

VulkanObjects::VulkanBufferInstance::~VulkanBufferInstance() { Device->QueueBufferForDeletion(Buffer, Allocation); }
void VulkanObjects::VulkanBufferInstance::Map(void **map_memory) const
{
  vmaMapMemory(Device->GetAllocator(), Allocation, map_memory);
}

void VulkanObjects::VulkanBufferInstance::UnMap() const { vmaUnmapMemory(Device->GetAllocator(), Allocation); }
