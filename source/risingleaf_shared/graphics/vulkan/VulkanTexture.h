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
#ifndef VULKANTEXTURE_H
#define VULKANTEXTURE_H

#include <memory>
#include <optional>

#include <vulkan/vulkan_core.h>

#include "external/vk_mem_alloc.h"
#include "graphics/graphics_toplevel_defines.h"


namespace VulkanObjects
{
  class VulkanDeviceInstance;
  class VulkanImageInstance final
  {
    const VulkanDeviceInstance *Device;

    VkImage       Image  = nullptr;
    VmaAllocation Memory = nullptr;

    mutable VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;

    const GraphicsTypes::ImageFormat Format;
    const uint32_t                   Width;
    const uint32_t                   Height;
    const uint32_t                   Depth;
    uint32_t                         Layers;
    const uint32_t                   MipLevels;
    const bool                       Owning;

  public:
    VulkanImageInstance(const VulkanDeviceInstance  *device,
                        GraphicsTypes::TextureType   type,
                        GraphicsTypes::ImageFormat   format,
                        GraphicsTypes::TextureTarget target,
                        uint32_t                     samples,
                        uint32_t                     width,
                        uint32_t                     height,
                        uint32_t                     depth      = 1,
                        uint32_t                     layers     = 1,
                        uint32_t                     mip_levels = 1,
                        VkImage                      image      = nullptr);

    ~VulkanImageInstance();

    VulkanImageInstance(const VulkanImageInstance &other)                = delete;
    VulkanImageInstance(VulkanImageInstance &&other) noexcept            = delete;
    VulkanImageInstance &operator=(const VulkanImageInstance &other)     = delete;
    VulkanImageInstance &operator=(VulkanImageInstance &&other) noexcept = delete;

    void SetLayout(VkCommandBuffer cmd, VkImageLayout dest) const;
    void ExternalSetLayout(const VkImageLayout dest) const { Layout = dest; }

    void Upload(VkCommandBuffer                  cmd,
                const void                      *data,
                uint32_t                         start_layer = 0,
                uint32_t                         end_layer   = 0,
                uint32_t                         mip_level   = 0) const;

    void CreateMipMaps(VkCommandBuffer cmd) const;

    [[nodiscard]] VkImage                    Get() const { return Image; }
    [[nodiscard]] VkImageLayout              GetLayout() const { return Layout; }
    [[nodiscard]] GraphicsTypes::ImageFormat GetFormat() const { return Format; }
  };

  class VulkanViewInstance final
  {
    VkImageView View = nullptr;

    const VulkanDeviceInstance *Device;

  public:
    VulkanViewInstance(const VulkanDeviceInstance *device,
                       VkImage                     image,
                       GraphicsTypes::ImageFormat  format,
                       GraphicsTypes::TextureType  type,
                       uint32_t                    layers,
                       uint32_t                    mip_levels);

    ~VulkanViewInstance();

    VulkanViewInstance(const VulkanViewInstance &other)                = delete;
    VulkanViewInstance(VulkanViewInstance &&other) noexcept            = delete;
    VulkanViewInstance &operator=(const VulkanViewInstance &other)     = delete;
    VulkanViewInstance &operator=(VulkanViewInstance &&other) noexcept = delete;

    [[nodiscard]] VkImageView Get() const { return View; }
  };

  class VulkanSamplerInstance final
  {
    VkSampler Sampler = nullptr;

    const VulkanDeviceInstance *Device;

  public:
    VulkanSamplerInstance(const VulkanDeviceInstance       *device,
                          uint32_t                          mip_levels,
                          GraphicsTypes::TextureAddressMode address_mode,
                          GraphicsTypes::TextureFilter      filter);

    ~VulkanSamplerInstance();

    VulkanSamplerInstance(const VulkanSamplerInstance &other)                = delete;
    VulkanSamplerInstance(VulkanSamplerInstance &&other) noexcept            = delete;
    VulkanSamplerInstance &operator=(const VulkanSamplerInstance &other)     = delete;
    VulkanSamplerInstance &operator=(VulkanSamplerInstance &&other) noexcept = delete;

    [[nodiscard]] VkSampler Get() const { return Sampler; }
  };

  class VulkanTextureInstance final : public GraphicsTypes::TextureInstance
  {
    std::unique_ptr<VulkanImageInstance>   Image;
    std::unique_ptr<VulkanViewInstance>    View;
    std::unique_ptr<VulkanSamplerInstance> Sampler;

    std::optional<const VulkanImageInstance *> ImageLink;
    std::optional<const VulkanViewInstance *>  ViewLink;

  public:
    VulkanTextureInstance(const VulkanDeviceInstance       *device,
                          VkCommandBuffer                   cmd,
                          const void                       *data,
                          const GraphicsTypes::TextureInfo &info);

    VulkanTextureInstance(const VulkanDeviceInstance       *device,
                          const VulkanImageInstance        *image,
                          const VulkanViewInstance         *view,
                          const GraphicsTypes::TextureInfo &info);

    void SetLayout(VkCommandBuffer cmd, VkImageLayout layout) const;

    void CreateMipMaps(VkCommandBuffer cmd) const;
    VulkanTextureInstance(const VulkanTextureInstance &other)                = delete;
    VulkanTextureInstance(VulkanTextureInstance &&other) noexcept            = delete;
    VulkanTextureInstance &operator=(const VulkanTextureInstance &other)     = delete;
    VulkanTextureInstance &operator=(VulkanTextureInstance &&other) noexcept = delete;

    [[nodiscard]] const VulkanImageInstance *GetImage() const
    {
      return ImageLink.has_value() ? ImageLink.value() : Image.get();
    }
    [[nodiscard]] const VulkanViewInstance *GetView() const
    {
      return ViewLink.has_value() ? ViewLink.value() : View.get();
    }
    [[nodiscard]] const VulkanSamplerInstance *GetSampler() const { return Sampler.get(); }
  };
} // namespace VulkanObjects


#endif // VULKANTEXTURE_H
