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
#include "VulkanTexture.h"

#include <cassert>
#include <cstring>

#include "VulkanBootstrap.h"
#include "VulkanBufferInstance.h"
#include "VulkanDeviceInstance.h"
#include "VulkanHelpers.h"


VulkanObjects::VulkanImageInstance::VulkanImageInstance(const VulkanDeviceInstance        *device,
                                                        const GraphicsTypes::TextureType   type,
                                                        const GraphicsTypes::ImageFormat   format,
                                                        const GraphicsTypes::TextureTarget target,
                                                        const uint32_t                     samples,
                                                        const uint32_t                     width,
                                                        const uint32_t                     height,
                                                        const uint32_t                     depth,
                                                        const uint32_t                     layers,
                                                        const uint32_t                     mip_levels,
                                                        VkImage                            image) :
  Device(device), Format(format), Width(width), Height(height), Depth(depth), Layers(layers), MipLevels(mip_levels), Owning(!image)
{
  // In case you just want to store the vulkan object
  if(image)
  {
    Image = image;
    return;
  }

  const auto create_info = VulkanBootstrap::GetImageCreate(type, format, target, samples, width, height, depth, layers, mip_levels);

  VmaAllocationCreateInfo allocation_create_info{};
  allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;

  VulkanHelpers::VK_CHECK_RESULT(vmaCreateImage(Device->GetAllocator(), &create_info, &allocation_create_info, &Image, &Memory, nullptr), __LINE__, __FILE__);
}

VulkanObjects::VulkanImageInstance::~VulkanImageInstance()
{
  if(Image && Owning && Memory) Device->QueueImageForDeletion(Image, Memory);
}

void VulkanObjects::VulkanImageInstance::SetLayout(VkCommandBuffer cmd, const VkImageLayout dest) const
{
  if(Layout == dest) return;

  auto barrier =
      VulkanBootstrap::GetImageMemoryBarrierWithoutAccess(Image, Format == GraphicsTypes::ImageFormat::DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, Layout, dest, Layers, MipLevels);

  const auto old_layout_info = VulkanTranslate::GetVkLayoutInfo(Layout);
  const auto new_layout_info = VulkanTranslate::GetVkLayoutInfo(dest);

  barrier.srcAccessMask = old_layout_info.second;
  barrier.dstAccessMask = new_layout_info.second;

  vkCmdPipelineBarrier(cmd, old_layout_info.first, new_layout_info.first, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  Layout = dest;
}

void VulkanObjects::VulkanImageInstance::Upload(VkCommandBuffer cmd, const std::vector<const void *> &data, const uint32_t start_layer, const uint32_t end_layer, const uint32_t mip_level) const
{
  assert(end_layer >= start_layer && data.size() == end_layer - start_layer + 1 && end_layer < Layers && mip_level < MipLevels && "Invalid input");

  const auto old_layout = Layout;

  SetLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

  const size_t layer_size = Width * Height * VulkanTranslate::GetComponentsOfFormat(Format) * VulkanTranslate::GetByteCountOfFormat(Format);
  const size_t image_size = layer_size * (end_layer - start_layer + 1);

  const VulkanBufferInstance buffer(Device, GraphicsTypes::BufferType::TEXTURE, image_size);

  void *to_data;

  buffer.Map(&to_data);
  for(size_t i = 0; i < data.size(); i++)
    memcpy(static_cast<unsigned char *>(to_data) + i * layer_size, data[i], layer_size);
  buffer.UnMap();

  const VkBufferImageCopy region = VulkanBootstrap::GetSimpleBufferImageCopyRegion(Format == GraphicsTypes::ImageFormat::DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, Width, Height,
                                                                                   Depth, start_layer, end_layer, mip_level);

  vkCmdCopyBufferToImage(cmd, buffer.Get(), Image, VK_IMAGE_LAYOUT_GENERAL, 1, &region);

  if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED) SetLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);
  else SetLayout(cmd, old_layout);
}


void VulkanObjects::VulkanImageInstance::CreateMipMaps(VkCommandBuffer cmd) const
{
  SetLayout(cmd, VK_IMAGE_LAYOUT_GENERAL);

  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(Device->GetPhysicalDevice(), VulkanTranslate::GetVkFormat(Format), &format_properties);

  // TODO: need proper handling here
  if(!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) throw std::runtime_error("texture image format does not support linear blitting!");

  auto mip_width  = static_cast<int32_t>(Width);
  auto mip_height = static_cast<int32_t>(Height);
  auto mip_depth  = static_cast<int32_t>(Depth);

  for(uint32_t i = 1; i < MipLevels; i++)
  {
    VkImageBlit blit{};
    blit.srcOffsets[0]                 = {0, 0, 0};
    blit.srcOffsets[1]                 = VkOffset3D(mip_width, mip_height, mip_depth);
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = Layers;
    blit.dstOffsets[0]                 = {0, 0, 0};
    blit.dstOffsets[1]                 = VkOffset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, mip_depth > 1 ? mip_depth / 2 : 1);
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = Layers;

    vkCmdBlitImage(cmd, Image, VK_IMAGE_LAYOUT_GENERAL, Image, VK_IMAGE_LAYOUT_GENERAL, 1, &blit, VK_FILTER_LINEAR);

    if(mip_width > 1) mip_width /= 2;
    if(mip_height > 1) mip_height /= 2;
    if(mip_depth > 1) mip_depth /= 2;
  }
}


VulkanObjects::VulkanViewInstance::VulkanViewInstance(
    const VulkanDeviceInstance *device, VkImage image, const GraphicsTypes::ImageFormat format, const GraphicsTypes::TextureType type, const uint32_t layers, const uint32_t mip_levels) :
  Device(device)
{
  const auto create_info =
      VulkanBootstrap::GetimageViewCreate(image, format == GraphicsTypes::ImageFormat::DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, type, format, layers, mip_levels);

  vkCreateImageView(Device->GetDevice(), &create_info, nullptr, &View);
}

VulkanObjects::VulkanViewInstance::~VulkanViewInstance()
{
  Device->QueueImageViewForDeletion(View);
}


VulkanObjects::VulkanSamplerInstance::VulkanSamplerInstance(const VulkanDeviceInstance             *device,
                                                            const uint32_t                          mip_levels,
                                                            const GraphicsTypes::TextureAddressMode address_mode,
                                                            const GraphicsTypes::TextureFilter      filter) :
  Device(device)
{
  const auto create_info = VulkanBootstrap::GetSamplerCreate(Device->GetProperties().limits.maxSamplerAnisotropy, static_cast<float>(mip_levels), address_mode, filter);

  vkCreateSampler(Device->GetDevice(), &create_info, nullptr, &Sampler);
}

VulkanObjects::VulkanSamplerInstance::~VulkanSamplerInstance()
{
  Device->QueueSamplerForDeletion(Sampler);
}


VulkanObjects::VulkanTextureInstance::VulkanTextureInstance(const VulkanDeviceInstance *device, VkCommandBuffer cmd, const std::vector<const void *> &data, const GraphicsTypes::TextureInfo &info)
{
  Info = info;
  assert(data.size() <= Info.Layers && "input data not matching layer count");
  Image = std::make_unique<VulkanImageInstance>(device, Info.Type, Info.Format, Info.Target, Info.Samples, Info.Width, Info.Height, Info.Depth, Info.Layers, Info.MipLevels);

  if(!data.empty()) Image->Upload(cmd, data, 0, data.size() - 1, 0);

  Image->SetLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  View    = std::make_unique<VulkanViewInstance>(device, Image->Get(), Info.Format, Info.Type, Info.Layers, Info.MipLevels);
  Sampler = std::make_unique<VulkanSamplerInstance>(device, Info.MipLevels, Info.AddressMode, Info.Filter);
}

VulkanObjects::VulkanTextureInstance::VulkanTextureInstance(const VulkanDeviceInstance       *device,
                                                            const VulkanImageInstance        *image,
                                                            const VulkanViewInstance         *view,
                                                            const GraphicsTypes::TextureInfo &info)
{
  Info = info;
  ImageLink    = image;
  ViewLink     = view;
  Sampler      = std::make_unique<VulkanSamplerInstance>(device, Info.MipLevels, Info.AddressMode, Info.Filter);
}

void VulkanObjects::VulkanTextureInstance::SetLayout(VkCommandBuffer cmd, const VkImageLayout layout) const
{
  if(Image) Image->SetLayout(cmd, layout);
  if(ImageLink.has_value() && ImageLink.value()) ImageLink.value()->SetLayout(cmd, layout);
}

void VulkanObjects::VulkanTextureInstance::CreateMipMaps(VkCommandBuffer cmd) const
{
  if(Image) Image->CreateMipMaps(cmd);
  if(ImageLink.has_value() && ImageLink.value()) ImageLink.value()->CreateMipMaps(cmd);
}
