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
#include "VulkanSwapChainInstance.h"

#include "VulkanBootstrap.h"
#include "VulkanCommandBuffer.h"
#include "VulkanFrameBufferInstance.h"
#include "VulkanHelpers.h"
#include "risingleaf_shared/base/ASLTypes.h"


void VulkanObjects::VulkanSwapChainInstance::Create(const VulkanCommandPool *command_pool, const uint32_t width, const uint32_t height)
{
  VulkanSingleCommandBuffer cmd(Device, command_pool);

  cmd.Begin();

  const VulkanHelpers::SwapChainSupportDetails swap_chain_support =
      VulkanHelpers::AcquireSwapChainSupportDetails(Device->GetPhysicalDevice(), Device->GetSurface());

  const VkSurfaceFormatKHR surface_format = VulkanHelpers::ChooseSwapSurfaceFormat(swap_chain_support.Formats);
  const VkPresentModeKHR   present_mode   = VulkanHelpers::ChooseSwapPresentMode(swap_chain_support.PresentModes);
  const VkExtent2D         extent_2d      = VulkanHelpers::ChooseSwapExtent(swap_chain_support.Capabilities, width, height);

  uint32_t image_count = swap_chain_support.Capabilities.minImageCount + 1;
  if (swap_chain_support.Capabilities.maxImageCount > 0 && image_count > swap_chain_support.Capabilities.maxImageCount)
    image_count = swap_chain_support.Capabilities.maxImageCount;

  VulkanHelpers::QueueFamilyIndices indices = VulkanHelpers::FindQueueFamilies(Device->GetPhysicalDevice(), Device->GetSurface());
  uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

  const auto create_info = VulkanBootstrap::GetSwapChainCreate(
    Device->GetSurface(),
    image_count,
    surface_format,
    extent_2d,
    indices.GraphicsFamily != indices.PresentFamily ? queueFamilyIndices : nullptr,
    swap_chain_support.Capabilities.currentTransform,
    present_mode );

  VulkanHelpers::VK_CHECK_RESULT(vkCreateSwapchainKHR(Device->GetDevice(), &create_info, nullptr, &SwapChain), __LINE__, __FILE__);

  std::vector<VkImage> images;
  vkGetSwapchainImagesKHR(Device->GetDevice(), SwapChain, &image_count, nullptr);
  images.resize(image_count);
  vkGetSwapchainImagesKHR(Device->GetDevice(), SwapChain, &image_count, images.data());

  GraphicsTypes::FrameBufferInfo info{};
  info.Format    = GraphicsTypes::ImageFormat::BGRA;
  info.Width     = extent_2d.width;
  info.Height    = extent_2d.height;
  info.Presenter = true;
  info.Samples   = 4;
  info.HasColor  = true;
  info.HasDepth  = true;
  GraphicsTypes::StateInfo state{};
  state.Color      = true;
  state.Depth      = true;
  state.Samples    = 4;
  state.DepthTest  = true;
  state.DepthWrite = true;
  for(uint32_t i = 0; i < image_count; i++)
  {
    FrameBuffers.emplace_back(std::make_unique<VulkanFrameBufferInstance>(
      Device,
      cmd.Get(),
      info,
      state,
      images[i]
    ));
  }

  cmd.End();

  RenderFinishedSemaphores.resize(image_count);
  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for(asl::uint32 i = 0; i < image_count; i++)
  {
    vkCreateSemaphore(Device->GetDevice(), &semaphore_create_info, nullptr, &RenderFinishedSemaphores[i]);
  }
}

VulkanObjects::VulkanSwapChainInstance::VulkanSwapChainInstance(
  const VulkanDeviceInstance *device,
  const VulkanCommandPool *command_pool,
  const uint32_t width,
  const uint32_t height)
: Device(device)
{
  Create(command_pool, width, height);
}

VulkanObjects::VulkanSwapChainInstance::~VulkanSwapChainInstance()
{
  if(SwapChain) vkDestroySwapchainKHR(Device->GetDevice(), SwapChain, nullptr);
  for(const auto &semaphore : RenderFinishedSemaphores) vkDestroySemaphore(Device->GetDevice(), semaphore, nullptr);
  RenderFinishedSemaphores.clear();
}

void VulkanObjects::VulkanSwapChainInstance::Recreate(const VulkanCommandPool *command_pool, const uint32_t width, const uint32_t height)
{
  vkDeviceWaitIdle(Device->GetDevice());

  FrameBuffers.clear();
  for(const auto &semaphore : RenderFinishedSemaphores) vkDestroySemaphore(Device->GetDevice(), semaphore, nullptr);
  RenderFinishedSemaphores.clear();

  if(SwapChain) vkDestroySwapchainKHR(Device->GetDevice(), SwapChain, nullptr);

  Create(command_pool, width, height);
}

bool VulkanObjects::VulkanSwapChainInstance::BeginFrame(const VulkanCommandPool *command_pool, const uint32_t width, const uint32_t height)
{
  const auto result = vkAcquireNextImageKHR(Device->GetDevice(), SwapChain, UINT64_MAX, Device->GetImageAvailable(), VK_NULL_HANDLE, &ImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
    Recreate(command_pool, width, height);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    return false;
  return true;
}

void VulkanObjects::VulkanSwapChainInstance::EndFrame(const VulkanCommandPool *command_pool, const uint32_t width, const uint32_t height)
{
  const VkSemaphore wait_semaphores[] = {RenderFinishedSemaphores[ImageIndex]};
  VkPresentInfoKHR present_info{};
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = wait_semaphores;

  const VkSwapchainKHR swap_chain_khrs[] = {SwapChain};
  present_info.swapchainCount  = 1;
  present_info.pSwapchains     = swap_chain_khrs;
  present_info.pImageIndices   = &ImageIndex;

  if (const auto result = vkQueuePresentKHR(Device->GetPresentQueue(), &present_info); result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    Recreate(command_pool, width, height);
  else if (result != VK_SUCCESS)
    throw std::runtime_error("failed to present swap chain image!");
}