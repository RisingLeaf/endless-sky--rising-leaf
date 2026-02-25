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
#ifndef VULKANHELPERS_H
#define VULKANHELPERS_H

#include <vulkan/vulkan_core.h>

#include <array>
#include <optional>
#include <vector>

namespace VulkanHelpers
{
  void VK_CHECK_RESULT(VkResult result, int line, const char *file);

  std::vector<const char *> GetRequiredExtensions();

  constexpr std::array<const char *, 2> EXTENSION_LAYERS = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_synchronization2"
  };
  bool CheckValidationLayerSupport();

  VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
      VkDebugUtilsMessageTypeFlagsEXT             message_type,
      const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
      void                                       *p_user_data);

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance                                instance,
      const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
      const VkAllocationCallbacks              *p_allocator,
      VkDebugUtilsMessengerEXT                 *p_debug_messenger);

  void DestroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debug_messenger,
    const VkAllocationCallbacks *p_allocator);

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    [[nodiscard]] bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
  };
  QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

  struct SwapChainSupportDetails
  {
    VkSurfaceCapabilitiesKHR        Capabilities{};
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR>   PresentModes;
  };
  SwapChainSupportDetails AcquireSwapChainSupportDetails(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

  constexpr

  std::array<const char*, 2> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
  };
  bool IsDeviceSuitable(VkPhysicalDevice vk_physical_device, VkSurfaceKHR surface);

  VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice vk_physical_device);

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes);

  VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);

  uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);
}

#endif //VULKANHELPERS_H
