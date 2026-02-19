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
#include "VulkanCommandBuffer.h"

#include <cassert>

#include "VulkanBootstrap.h"
#include "VulkanCommandPool.h"
#include "VulkanDeviceInstance.h"
#include "VulkanHelpers.h"
#include "system/Log.h"



VulkanObjects::VulkanSingleCommandBuffer::VulkanSingleCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool)
: Device(device), CommandPool(command_pool)
{
  const auto alloc_info  = VulkanBootstrap::GetCommandBufferAllocate(CommandPool->Get(), 1);
  VulkanHelpers::VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->GetDevice(), &alloc_info, &cmd), __LINE__, __FILE__);
}

void VulkanObjects::VulkanSingleCommandBuffer::Begin()
{
  assert(state==0 && "invalid second usage of begin on single command buffer");
  const auto begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::ONE_TIME);
  VulkanHelpers::VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_info), __LINE__, __FILE__);

  state = 1;
}

void VulkanObjects::VulkanSingleCommandBuffer::End()
{
  assert(state==1 && "trying to unstarted or already finished command buffer");
  vkEndCommandBuffer(cmd);
  const std::vector buffers = {cmd};
  const auto submit_info = VulkanBootstrap::GetSubmit(buffers, {}, {}, {});
  vkQueueSubmit(Device->GetGraphicsQueue(), 1, &submit_info, nullptr);
  vkQueueWaitIdle(Device->GetGraphicsQueue());
  state = 2;
}

VulkanObjects::VulkanSingleCommandBuffer::~VulkanSingleCommandBuffer()
{
  assert(state % 2 == 0 && "deleting command buffer that is still in use");
#ifndef NDEBUG
  if(state == 0) Log::Warn<<"deleting command buffer that was never used!"<<Log::End;
#endif
  if(cmd) vkFreeCommandBuffers(Device->GetDevice(), CommandPool->Get(), 1, &cmd);
}

VulkanObjects::VulkanSingleComputeCommandBuffer::VulkanSingleComputeCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool)
: Device(device), CommandPool(command_pool)
{
  const auto alloc_info  = VulkanBootstrap::GetCommandBufferAllocate(CommandPool->Get(), 1);
  VulkanHelpers::VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->GetDevice(), &alloc_info, &cmd), __LINE__, __FILE__);
}

void VulkanObjects::VulkanSingleComputeCommandBuffer::Begin()
{
  assert(state==0 && "invalid second usage of begin on single command buffer");
  const auto begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::ONE_TIME);
  VulkanHelpers::VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_info), __LINE__, __FILE__);
  state = 1;
}

void VulkanObjects::VulkanSingleComputeCommandBuffer::End()
{
  assert(state==1 && "trying to unstarted or already finished command buffer");
  vkEndCommandBuffer(cmd);
  const std::vector buffers = {cmd};
  const auto submit_info = VulkanBootstrap::GetSubmit(buffers, {}, {}, {});
  vkQueueSubmit(Device->GetComputeQueue(), 1, &submit_info, nullptr);
  vkQueueWaitIdle(Device->GetComputeQueue());
  state = 2;
}

VulkanObjects::VulkanSingleComputeCommandBuffer::~VulkanSingleComputeCommandBuffer()
{
  assert(state % 2 == 0 && "deleting command buffer that is still in use");
#ifndef NDEBUG
  if(state == 0) Log::Warn<<"deleting command buffer that was never used!"<<Log::End;
#endif
  if(cmd) vkFreeCommandBuffers(Device->GetDevice(), CommandPool->Get(), 1, &cmd);
}