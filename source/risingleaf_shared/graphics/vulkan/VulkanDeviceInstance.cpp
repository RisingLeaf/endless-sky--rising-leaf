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
#include "VulkanDeviceInstance.h"

#include <set>
#include <stdexcept>

#include "VulkanHelpers.h"
#include "system/Log.h"

#include <SDL3/SDL_vulkan.h>

#include "../../../GameWindow.h"

VulkanObjects::VulkanDeviceInstance::VulkanDeviceInstance()
{
  VkApplicationInfo app_info{};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = "Vulkan Env";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName        = "VK Engine";
  app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion         = VK_API_VERSION_1_3;

  VkInstanceCreateInfo create_info{};
  create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  // TODO: maybe not have glfw included here
  std::vector<const char *> required_extensions = VulkanHelpers::GetRequiredExtensions();
  if(ENABLE_VALIDATION_LAYERS) required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  create_info.enabledExtensionCount   = static_cast<uint32_t>(required_extensions.size());
  create_info.ppEnabledExtensionNames = required_extensions.data();

  if constexpr(ENABLE_VALIDATION_LAYERS)
  {
    if(!VulkanHelpers::CheckValidationLayerSupport())
      throw std::runtime_error("validation layers requested, but not available!");
    create_info.enabledLayerCount   = static_cast<uint32_t>(VulkanHelpers::EXTENSION_LAYERS.size());
    create_info.ppEnabledLayerNames = VulkanHelpers::EXTENSION_LAYERS.data();
  }
  else create_info.enabledLayerCount = 0;
  VulkanHelpers::VK_CHECK_RESULT(vkCreateInstance(&create_info, nullptr, &Instance), __LINE__, __FILE__);

  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
  std::vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

  if constexpr(ENABLE_VALIDATION_LAYERS)
  {
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    //  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    //| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = VulkanHelpers::DebugCallback;
    debug_create_info.pUserData       = nullptr; // Optional

    VulkanHelpers::VK_CHECK_RESULT(VulkanHelpers::CreateDebugUtilsMessengerEXT(Instance, &debug_create_info, nullptr, &DebugMessenger), __LINE__, __FILE__);
  }

  if(!SDL_Vulkan_CreateSurface(GameWindow::GetWindow(), Instance, nullptr, &Surface))
    throw std::runtime_error("Error creating sdl vulkan surface");

  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(Instance, &device_count, nullptr);
  if(device_count == 0) throw std::runtime_error("failed to find GPUs with Vulkan support");

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(Instance, &device_count, devices.data());
  for(const auto &device : devices)
    if(VulkanHelpers::IsDeviceSuitable(device, Surface))
    {
      PhysicalDevice = device;
      break;
    }
  if(PhysicalDevice == VK_NULL_HANDLE) throw std::runtime_error("failed to find a suitable GPU!");

  vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

  ////
  // Create a logical device
  ////
  VulkanHelpers::QueueFamilyIndices    indices = VulkanHelpers::FindQueueFamilies(PhysicalDevice, Surface);
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set                             unique_queue_families = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};
  float                                queue_priority        = 1.0f;
  for(uint32_t queue_family : unique_queue_families)
  {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount       = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceSynchronization2Features sync2Features{};
  sync2Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  sync2Features.synchronization2 = VK_TRUE;

  VkPhysicalDeviceFeatures2 deviceFeatures2{};
  deviceFeatures2.sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures2.pNext                      = &sync2Features;
  deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
  deviceFeatures2.features.fillModeNonSolid  = VK_TRUE;

  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos       = queue_create_infos.data();
  device_create_info.queueCreateInfoCount    = queue_create_infos.size();
  device_create_info.pEnabledFeatures        = nullptr;
  device_create_info.enabledLayerCount       = 0;
  device_create_info.enabledExtensionCount   = VulkanHelpers::DEVICE_EXTENSIONS.size();
  device_create_info.ppEnabledExtensionNames = VulkanHelpers::DEVICE_EXTENSIONS.data();
  device_create_info.pNext                   = &deviceFeatures2;

  VulkanHelpers::VK_CHECK_RESULT(vkCreateDevice(PhysicalDevice, &device_create_info, nullptr, &Device), __LINE__, __FILE__);

  vkGetDeviceQueue(Device, indices.GraphicsFamily.value(), 0, &GraphicsQueue);
  vkGetDeviceQueue(Device, indices.GraphicsFamily.value(), 0, &ComputeQueue);
  vkGetDeviceQueue(Device, indices.PresentFamily.value(), 0, &PresentQueue);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    VulkanHelpers::VK_CHECK_RESULT(vkCreateSemaphore(Device, &semaphore_info, nullptr, &ImageAvailableSemaphores[i]), __LINE__, __FILE__);
    VulkanHelpers::VK_CHECK_RESULT(vkCreateFence(Device, &fence_info, nullptr, &InFlightFences[i]), __LINE__, __FILE__);
  }

  VmaAllocatorCreateInfo allocator_create_info{};
  allocator_create_info.instance       = Instance;
  allocator_create_info.physicalDevice = PhysicalDevice;
  allocator_create_info.device         = Device;
  VulkanHelpers::VK_CHECK_RESULT(vmaCreateAllocator(&allocator_create_info, &Allocator), __LINE__, __FILE__);

#ifndef NDEBUG
  vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(Device, "vkSetDebugUtilsObjectNameEXT"));
  if(!vkSetDebugUtilsObjectNameEXT) Log::Warn<<"Could not find vkSetDebugUtilsObjectNameEXT"<<Log::End;
#endif

}

void VulkanObjects::VulkanDeviceInstance::QueueBufferForDeletion(VkBuffer buffer, VmaAllocation allocation) const { BufferDeleteQueue[CurrentFrame].emplace_back(buffer, allocation); }
void VulkanObjects::VulkanDeviceInstance::QueueImageForDeletion(VkImage image, VmaAllocation allocation) const { ImageDeleteQueue[CurrentFrame].emplace_back(image, allocation); }
void VulkanObjects::VulkanDeviceInstance::QueueImageViewForDeletion(VkImageView view) const { ImageViewDeleteQueue[CurrentFrame].emplace_back(view); }
void VulkanObjects::VulkanDeviceInstance::QueueSamplerForDeletion(VkSampler sampler) const { SamplerDeleteQueue[CurrentFrame].emplace_back(sampler); }
void VulkanObjects::VulkanDeviceInstance::QueueFrameBufferForDeletion(VkFramebuffer framebuffer) const { FrameBufferDeleteQueue[CurrentFrame].emplace_back(framebuffer); }
void VulkanObjects::VulkanDeviceInstance::QueueRenderPassForDeletion(VkRenderPass render_pass) const { RenderPassDeleteQueue[CurrentFrame].emplace_back(render_pass); }

void VulkanObjects::VulkanDeviceInstance::BeginFrame() const
{
  CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  vkWaitForFences(Device, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(Device, 1, &InFlightFences[CurrentFrame]);

  for(const auto &[buffer, allocation] : BufferDeleteQueue[CurrentFrame])
    if(buffer && allocation) vmaDestroyBuffer(Allocator, buffer, allocation);
  BufferDeleteQueue[CurrentFrame].clear();

  for(const auto &render_pass : RenderPassDeleteQueue[CurrentFrame])
    if(render_pass) vkDestroyRenderPass(Device, render_pass, nullptr);
  RenderPassDeleteQueue[CurrentFrame].clear();

  for(const auto &framebuffer : FrameBufferDeleteQueue[CurrentFrame])
    if(framebuffer) vkDestroyFramebuffer(Device, framebuffer, nullptr);
  FrameBufferDeleteQueue[CurrentFrame].clear();

  for(const auto &sampler : SamplerDeleteQueue[CurrentFrame])
    if(sampler) vkDestroySampler(Device, sampler, nullptr);
  SamplerDeleteQueue[CurrentFrame].clear();

  for(const auto &view : ImageViewDeleteQueue[CurrentFrame])
    if(view) vkDestroyImageView(Device, view, nullptr);
  ImageViewDeleteQueue[CurrentFrame].clear();

  for(const auto &[image, allocation] : ImageDeleteQueue[CurrentFrame])
    if(image && allocation) vmaDestroyImage(Allocator, image, allocation);
  ImageDeleteQueue[CurrentFrame].clear();
}


VulkanObjects::VulkanDeviceInstance::~VulkanDeviceInstance()
{
  for(uint8_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++)
  {
    if(InFlightFences[frame]) vkDestroyFence(Device, InFlightFences[frame], nullptr);
    if(ImageAvailableSemaphores[frame]) vkDestroySemaphore(Device, ImageAvailableSemaphores[frame], nullptr);

    for(const auto &[buffer, allocation] : BufferDeleteQueue[frame])
      if(buffer && allocation) vmaDestroyBuffer(Allocator, buffer, allocation);

    for(const auto &render_pass : RenderPassDeleteQueue[frame])
      if(render_pass) vkDestroyRenderPass(Device, render_pass, nullptr);

    for(const auto &framebuffer : FrameBufferDeleteQueue[frame])
      if(framebuffer) vkDestroyFramebuffer(Device, framebuffer, nullptr);

    for(const auto &sampler : SamplerDeleteQueue[frame])
      if(sampler) vkDestroySampler(Device, sampler, nullptr);

    for(const auto &view : ImageViewDeleteQueue[frame])
      if(view) vkDestroyImageView(Device, view, nullptr);

    for(const auto &[image, allocation] : ImageDeleteQueue[frame])
      if(image && allocation) vmaDestroyImage(Allocator, image, allocation);
  }

  if(Allocator) vmaDestroyAllocator(Allocator);

  if(Device) vkDestroyDevice(Device, nullptr);
  if(Surface) vkDestroySurfaceKHR(Instance, Surface, nullptr);
  if(DebugMessenger) VulkanHelpers::DestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
  if(Instance) vkDestroyInstance(Instance, nullptr);
}
