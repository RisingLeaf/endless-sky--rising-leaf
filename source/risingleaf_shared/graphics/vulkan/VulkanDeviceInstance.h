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
#ifndef VULKANDEVICEINSTANCE_H
#define VULKANDEVICEINSTANCE_H

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>

#include "external/vk_mem_alloc.h"

class Window;


namespace VulkanObjects
{
  class VulkanDeviceInstance
  {
  public:
    constexpr static size_t MAX_FRAMES_IN_FLIGHT = 3;

  private:
    VkInstance               Instance       = nullptr;
    VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
    VkSurfaceKHR             Surface        = nullptr;
    VkPhysicalDevice         PhysicalDevice = nullptr;
    VkDevice                 Device         = nullptr;
    VmaAllocator             Allocator      = nullptr;

    VkQueue GraphicsQueue = nullptr;
    VkQueue ComputeQueue  = nullptr;
    VkQueue PresentQueue  = nullptr;

    VkPhysicalDeviceProperties PhysicalDeviceProperties{};

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> ImageAvailableSemaphores{nullptr};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT>     InFlightFences{nullptr};

#ifdef NDEBUG
    static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

    mutable uint8_t CurrentFrame = 0;

    mutable std::array<std::vector<std::pair<VkBuffer, VmaAllocation>>, MAX_FRAMES_IN_FLIGHT> BufferDeleteQueue;
    mutable std::array<std::vector<std::pair<VkImage, VmaAllocation>>, MAX_FRAMES_IN_FLIGHT>  ImageDeleteQueue;
    mutable std::array<std::vector<VkImageView>, MAX_FRAMES_IN_FLIGHT>                        ImageViewDeleteQueue;
    mutable std::array<std::vector<VkSampler>, MAX_FRAMES_IN_FLIGHT>                          SamplerDeleteQueue;
    mutable std::array<std::vector<VkFramebuffer>, MAX_FRAMES_IN_FLIGHT>                      FrameBufferDeleteQueue;
    mutable std::array<std::vector<VkRenderPass>, MAX_FRAMES_IN_FLIGHT>                       RenderPassDeleteQueue;

  public:
    explicit VulkanDeviceInstance();

    VulkanDeviceInstance(const VulkanDeviceInstance &other)                = delete;
    VulkanDeviceInstance(VulkanDeviceInstance &&other) noexcept            = delete;
    VulkanDeviceInstance &operator=(const VulkanDeviceInstance &other)     = delete;
    VulkanDeviceInstance &operator=(VulkanDeviceInstance &&other) noexcept = delete;

    void QueueBufferForDeletion(VkBuffer buffer, VmaAllocation allocation) const;
    void QueueImageForDeletion(VkImage image, VmaAllocation allocation) const;
    void QueueImageViewForDeletion(VkImageView view) const;
    void QueueSamplerForDeletion(VkSampler sampler) const;
    void QueueFrameBufferForDeletion(VkFramebuffer framebuffer) const;
    void QueueRenderPassForDeletion(VkRenderPass render_pass) const;

    void BeginFrame() const;

    ~VulkanDeviceInstance();

    [[nodiscard]] VkSurfaceKHR               GetSurface() const { return Surface; }
    [[nodiscard]] VkPhysicalDevice           GetPhysicalDevice() const { return PhysicalDevice; }
    [[nodiscard]] VkPhysicalDeviceProperties GetProperties() const { return PhysicalDeviceProperties; }
    [[nodiscard]] VkDevice                   GetDevice() const { return Device; }
    [[nodiscard]] VmaAllocator               GetAllocator() const { return Allocator; }
    [[nodiscard]] VkQueue                    GetGraphicsQueue() const { return GraphicsQueue; }
    [[nodiscard]] VkQueue                    GetComputeQueue() const { return ComputeQueue; }
    [[nodiscard]] VkQueue                    GetPresentQueue() const { return PresentQueue; }
    [[nodiscard]] uint8_t                    GetCurrentFrame() const { return CurrentFrame; }
    [[nodiscard]] VkSemaphore GetImageAvailable() const { return ImageAvailableSemaphores[CurrentFrame]; }
    [[nodiscard]] VkFence     GetFence() const { return InFlightFences[CurrentFrame]; }
#ifndef NDEBUG
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
#endif

  };
} // namespace VulkanObjects


#endif // VULKANDEVICEINSTANCE_H
