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
#ifndef VULKANFRAMEBUFFERINSTANCE_H
#define VULKANFRAMEBUFFERINSTANCE_H

#include <memory>
#include <vulkan/vulkan_core.h>

#include "VulkanRenderPassInstance.h"
#include "graphics/graphics_toplevel_defines.h"


namespace VulkanObjects
{
  class VulkanCommandPool;
  class VulkanTextureInstance;
  class VulkanImageInstance;
  class VulkanViewInstance;
  class VulkanDevice;

  class VulkanFrameBufferInstance final : public GraphicsTypes::RenderBufferInstance
  {
    VulkanRenderPassInstance                          RenderPass;
    std::vector<std::unique_ptr<VulkanImageInstance>> Images;
    std::vector<std::unique_ptr<VulkanViewInstance>>  Views;
    std::vector<VkFramebuffer>                        FrameBuffer{nullptr};

    std::unique_ptr<VulkanTextureInstance>            Texture;

    bool InUse = false;

    const VulkanDeviceInstance      *Device;
  public:
    VulkanFrameBufferInstance(
      const VulkanDeviceInstance           *device,
      const GraphicsTypes::FrameBufferInfo &info,
      const GraphicsTypes::StateInfo       &state,
      size_t                                count);

    VulkanFrameBufferInstance(
      const VulkanDeviceInstance           *device,
      VkCommandBuffer                       cmd,
      const GraphicsTypes::FrameBufferInfo &info,
      const GraphicsTypes::StateInfo       &state,
      VkImage                               image);

    void SetInUse(const bool state) { InUse = state; }
    [[nodiscard]] bool IsInUse() const { return InUse; }

    ~VulkanFrameBufferInstance() override;
    VulkanFrameBufferInstance(const VulkanFrameBufferInstance &other) = delete;
    VulkanFrameBufferInstance(VulkanFrameBufferInstance &&other) noexcept = delete;
    VulkanFrameBufferInstance &operator=(const VulkanFrameBufferInstance &other) = delete;
    VulkanFrameBufferInstance &operator=(VulkanFrameBufferInstance &&other) noexcept = delete;

    [[nodiscard]] VkFramebuffer Get(const size_t i = 0) const { return FrameBuffer[i]; }
    [[nodiscard]] const VulkanRenderPassInstance *GetRenderPass() const { return &RenderPass; }
    [[nodiscard]] const VulkanTextureInstance    *GetTexture()    const { return Texture.get(); }
  };
}



#endif //VULKANFRAMEBUFFERINSTANCE_H
