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
#ifndef VULKANSWAPCHAININSTANCE_H
#define VULKANSWAPCHAININSTANCE_H

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "VulkanDeviceInstance.h"


namespace VulkanObjects
{
  class VulkanCommandPool;
  class VulkanFrameBufferInstance;
  class VulkanSwapChainInstance final
  {
    VkSwapchainKHR                                          SwapChain    = nullptr;
    std::vector<std::unique_ptr<VulkanFrameBufferInstance>> FrameBuffers;
    std::vector<VkSemaphore>                                RenderFinishedSemaphores{};
    uint32_t                                                ImageIndex = 0;

    const VulkanDeviceInstance *Device;


    void Create(const VulkanCommandPool *command_pool, uint32_t width, uint32_t height);

  public:
    VulkanSwapChainInstance(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool, uint32_t width, uint32_t height);

    ~VulkanSwapChainInstance();

    VulkanSwapChainInstance(const VulkanSwapChainInstance &other) = delete;
    VulkanSwapChainInstance(VulkanSwapChainInstance &&other) noexcept = delete;
    VulkanSwapChainInstance &operator=(const VulkanSwapChainInstance &other) = delete;
    VulkanSwapChainInstance &operator=(VulkanSwapChainInstance &&other) noexcept = delete;

    void Recreate(const VulkanCommandPool *command_pool, uint32_t width, uint32_t height);

    bool BeginFrame(const VulkanCommandPool *command_pool, uint32_t width, uint32_t height);

    void EndFrame(const VulkanCommandPool *command_pool, uint32_t width, uint32_t height);

    [[nodiscard]] const VulkanFrameBufferInstance *GetCurrentFrameBuffer() const { return FrameBuffers[Device->GetCurrentFrame()].get(); }
    [[nodiscard]] VkSemaphore                      GetRenderFinished()     const { return RenderFinishedSemaphores[ImageIndex]; }
  };
}



#endif //VULKANSWAPCHAININSTANCE_H
