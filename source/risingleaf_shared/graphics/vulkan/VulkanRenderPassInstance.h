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
#ifndef VULKANRENDERPASSINSTANCE_H
#define VULKANRENDERPASSINSTANCE_H

#include <vulkan/vulkan_core.h>

#include "VulkanDeviceInstance.h"
#include "graphics/graphics_toplevel_defines.h"


namespace VulkanObjects
{
  class VulkanRenderPassInstance final : public GraphicsTypes::RenderPassInstance
  {
    VkRenderPass RenderPass = nullptr;

    const VulkanDeviceInstance *Device;

  public:
    VulkanRenderPassInstance(
        const VulkanDeviceInstance     *device,
        GraphicsTypes::ImageFormat      image_format,
        const GraphicsTypes::StateInfo &state,
        std::string_view                name,
        bool                            is_swap_chain = false);

    ~VulkanRenderPassInstance() override;

    VulkanRenderPassInstance(const VulkanRenderPassInstance &other)                = delete;
    VulkanRenderPassInstance(VulkanRenderPassInstance &&other) noexcept            = delete;
    VulkanRenderPassInstance &operator=(const VulkanRenderPassInstance &other)     = delete;
    VulkanRenderPassInstance &operator=(VulkanRenderPassInstance &&other) noexcept = delete;

    [[nodiscard]] VkRenderPass Get() const { return RenderPass; }
  };
} // namespace VulkanObjects


#endif // VULKANRENDERPASSINSTANCE_H
