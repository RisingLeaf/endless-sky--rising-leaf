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
#include "VulkanHelpers.h"

#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <limits>
#include <set>
#include <stdexcept>

#include "system/Log.h"

namespace
{
  constexpr std::string_view ErrorString( const VkResult errorCode )
  {
    switch( errorCode )
    {
#define STR( r ) \
case VK_##r:   \
return #r
      STR( NOT_READY );
      STR( TIMEOUT );
      STR( EVENT_SET );
      STR( EVENT_RESET );
      STR( INCOMPLETE );
      STR( ERROR_OUT_OF_HOST_MEMORY );
      STR( ERROR_OUT_OF_DEVICE_MEMORY );
      STR( ERROR_INITIALIZATION_FAILED );
      STR( ERROR_DEVICE_LOST );
      STR( ERROR_MEMORY_MAP_FAILED );
      STR( ERROR_LAYER_NOT_PRESENT );
      STR( ERROR_EXTENSION_NOT_PRESENT );
      STR( ERROR_FEATURE_NOT_PRESENT );
      STR( ERROR_INCOMPATIBLE_DRIVER );
      STR( ERROR_TOO_MANY_OBJECTS );
      STR( ERROR_FORMAT_NOT_SUPPORTED );
      STR( ERROR_SURFACE_LOST_KHR );
      STR( ERROR_NATIVE_WINDOW_IN_USE_KHR );
      STR( SUBOPTIMAL_KHR );
      STR( ERROR_OUT_OF_DATE_KHR );
      STR( ERROR_INCOMPATIBLE_DISPLAY_KHR );
      STR( ERROR_VALIDATION_FAILED_EXT );
      STR( ERROR_INVALID_SHADER_NV );
#undef STR
    default:
      return "UNKNOWN_ERROR";
    }
  }
}

void VulkanHelpers::VK_CHECK_RESULT(const VkResult result, const int line, const char *file)
{
  if(result != VK_SUCCESS)
    throw std::runtime_error(std::format("Fatal vulkan error at line {} of {} with error: {}", line, file, ErrorString(result)));
}

std::vector<const char *> VulkanHelpers::GetRequiredExtensions()
{
  Uint32 extension_count;
  const auto extensions_raw = SDL_Vulkan_GetInstanceExtensions(&extension_count);

  std::vector extensions(extensions_raw, extensions_raw + extension_count);

  return extensions;
}

bool VulkanHelpers::CheckValidationLayerSupport()
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char* layer_name : EXTENSION_LAYERS)
  {
    bool layer_found = false;
    for (const auto& layer_properties : available_layers)
      if (strcmp(layer_name, layer_properties.layerName) == 0)
      {
        layer_found = true;
        break;
      }
    if (!layer_found)
      return false;
  }
  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanHelpers::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
    void                                       *)
{
  std::string header = "vulkan validation(";
  switch(message_type)
  {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:                header += "general";     break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:             header += "validation";  break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:            header += "performance"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: header += "address";     break;
    default: break;
  }
  header += ") ";
  switch(message_severity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: header += "VERBOSE"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    header += "INFO";    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: header += "WARNING"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   header += "ERROR";   break;
    default: break;
  }
  header += ":\n  ";

  Log::Warn<<header<<p_callback_data->pMessage<<Log::End;

  return VK_FALSE;
}

VkResult VulkanHelpers::CreateDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    const VkAllocationCallbacks              *p_allocator,
    VkDebugUtilsMessengerEXT                 *p_debug_messenger)
{
  const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr)
    return func(instance, p_create_info, p_allocator, p_debug_messenger);

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanHelpers::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator)
{
  const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if(func != nullptr)
    func(instance, debug_messenger, p_allocator);
}

VulkanHelpers::QueueFamilyIndices VulkanHelpers::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  QueueFamilyIndices indices;
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  int i = 0;
  for(const auto &queueFamily : queue_families)
  {
    if((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
      indices.GraphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if(presentSupport)
      indices.PresentFamily = i;

    if(indices.IsComplete())
      break;

    i++;
  }
  return indices;
}

VulkanHelpers::SwapChainSupportDetails VulkanHelpers::AcquireSwapChainSupportDetails(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.Capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
  if (format_count != 0)
  {
    details.Formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.Formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
  if (present_mode_count != 0)
  {
    details.PresentModes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.PresentModes.data());
  }

  return details;
}

bool VulkanHelpers::IsDeviceSuitable(VkPhysicalDevice vk_physical_device, VkSurfaceKHR surface)
{
  VkPhysicalDeviceProperties device_properties;
  VkPhysicalDeviceFeatures device_features;
  vkGetPhysicalDeviceProperties(vk_physical_device, &device_properties);
  vkGetPhysicalDeviceFeatures(vk_physical_device, &device_features);

  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &extension_count, available_extensions.data());

  std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
  for (const auto &extension : available_extensions)
    required_extensions.erase(extension.extensionName);
  bool extensions_supported = required_extensions.empty();

  QueueFamilyIndices queue_family_indices = FindQueueFamilies(vk_physical_device, surface);

  bool swapChainAdequate = false;
  if (extensions_supported)
  {
    SwapChainSupportDetails swapChainSupport = AcquireSwapChainSupportDetails(vk_physical_device, surface);
    swapChainAdequate                        = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
  }

  return queue_family_indices.IsComplete() && extensions_supported && swapChainAdequate && device_features.samplerAnisotropy;
}

VkSampleCountFlagBits VulkanHelpers::GetMaxUsableSampleCount(VkPhysicalDevice vk_physical_device)
{
  VkPhysicalDeviceProperties physical_device_properties;
  vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device_properties);

  const VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
  if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
  if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
  if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
  if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
  if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;

  return VK_SAMPLE_COUNT_1_BIT;
}

VkSurfaceFormatKHR VulkanHelpers::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
  for (const auto& availableFormat : availableFormats)
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return availableFormat;

  return availableFormats[0];
}

VkPresentModeKHR VulkanHelpers::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
  //return VK_PRESENT_MODE_IMMEDIATE_KHR;
  for (const auto &available_present_mode : available_present_modes)
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return available_present_mode;

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanHelpers::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const uint32_t width, const uint32_t height)
{
  if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities.currentExtent;

  VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  actual_extent.width =
      std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actual_extent.height =
      std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actual_extent;
}

uint32_t VulkanHelpers::FindMemoryType(VkPhysicalDevice physical_device, const uint32_t type_filter, const VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    if (type_filter & (1 << i) && mem_properties.memoryTypes[i].propertyFlags & properties)
      return i;

  throw std::runtime_error("failed to find suitable memory type!");
}