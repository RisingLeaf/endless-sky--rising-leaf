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
#ifndef VULKANBOOTSTRAP_H
#define VULKANBOOTSTRAP_H

#include <vector>

#include "VulkanTranslate.h"

namespace VulkanBootstrap
{
  //////////////
  /// COMMAND BUFFERS
  //////////////
  VkCommandPoolCreateInfo     GetCommandPoolCreate(uint32_t queue_family_index);
  VkSemaphoreCreateInfo       GetSemaphoreCreate();
  VkFenceCreateInfo           GetFenceCreate();
  VkCommandBufferAllocateInfo GetCommandBufferAllocate(VkCommandPool command_pool, uint32_t count);
  VkCommandBufferBeginInfo    GetCommandBufferBegin(VulkanTranslate::CommandBufferType type);

  VkSubmitInfo GetSubmit(const std::vector<VkCommandBuffer>      &buffer,
                         const std::vector<VkSemaphore>          &wait_semaphores   = {},
                         const std::vector<VkPipelineStageFlags> &wait_stages       = {},
                         const std::vector<VkSemaphore>          &signal_semaphores = {});

  //////////////
  /// SHADERS
  //////////////
  VkShaderModuleCreateInfo        GetShaderModuleCreate(const std::vector<std::byte> &code);
  VkPipelineShaderStageCreateInfo GetShaderStageCreate(VkShaderModule shader, VulkanTranslate::ShaderStage stage);
  VkPipelineLayoutCreateInfo      GetPipelineLayoutCreate(const std::vector<VkDescriptorSetLayout> &descriptor_set_layouts, const std::vector<VkPushConstantRange> &push_constant_ranges);

  //////////////
  /// DESCRIPTOR SETS
  //////////////
  VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(uint32_t binding, VulkanTranslate::ShaderStage stage, VulkanTranslate::DescriptorType type);

  VkDescriptorSetLayoutCreateInfo GetDescriptorSetLayoutCreate(const std::vector<VkDescriptorSetLayoutBinding> &bindings);

  //////////////
  /// ATTACHMENTS
  //////////////
  VkAttachmentDescription GetAttachmentDescription(VulkanTranslate::AttachmentType type, GraphicsTypes::ImageFormat image_format, uint32_t sample_count, bool presenter = false);

  VkAttachmentReference GetAttachmentReference(uint32_t index, VulkanTranslate::AttachmentType type);

  VkSubpassDescription
  GetSubPassDescription(const std::vector<VkAttachmentReference> &color_attachments, const VkAttachmentReference &depth_attachment, const VkAttachmentReference &color_resolve_attachments);

  VkSubpassDependency GetSubPassDependencyResolve();

  VkSubpassDependency GetSubPassDependencyResolveExit();

  //////////////
  /// RENDER PASS
  //////////////
  VkRenderPassCreateInfo
  GetRenderPassCreate(const std::vector<VkAttachmentDescription> &attachments, const std::vector<VkSubpassDescription> &sub_passes, const std::vector<VkSubpassDependency> &dependencies);

  //////////////
  /// MEMORY
  //////////////
  VkMemoryAllocateInfo GetMemoryAllocateInfo(VkDeviceSize alloc_size, uint32_t type_index);

  //////////////
  /// BUFFER
  //////////////
  VkBufferCreateInfo GetBufferCreate(GraphicsTypes::BufferType type, VkDeviceSize size);

  //////////////
  /// IMAGE
  //////////////
  VkImageCreateInfo GetImageCreate(GraphicsTypes::TextureType   type,
                                   GraphicsTypes::ImageFormat   format,
                                   GraphicsTypes::TextureTarget target,
                                   uint32_t                     samples    = 1,
                                   uint32_t                     width      = 1,
                                   uint32_t                     height     = 1,
                                   uint32_t                     depth      = 1,
                                   uint32_t                     layers     = 1,
                                   uint32_t                     mip_levels = 1);

  VkImageViewCreateInfo
  GetimageViewCreate(VkImage image, VkImageAspectFlags aspect_flags, GraphicsTypes::TextureType type, GraphicsTypes::ImageFormat format, uint32_t layers = 1, uint32_t mip_levels = 1);

  VkSamplerCreateInfo GetSamplerCreate(float max_anisotropy, float max_lod, GraphicsTypes::TextureAddressMode address_mode, GraphicsTypes::TextureFilter filter);

  VkImageMemoryBarrier GetImageMemoryBarrierWithoutAccess(VkImage image, VkImageAspectFlags aspect, VkImageLayout src_layout, VkImageLayout dst_layout, uint32_t layers = 1, uint32_t mip_levels = 1);

  VkBufferImageCopy GetSimpleBufferImageCopyRegion(VkImageAspectFlags aspect, uint32_t width, uint32_t height, uint32_t depth, uint32_t start_layer, uint32_t end_layer, uint32_t mip_level);

  //////////////
  /// FRAME BUFFER
  //////////////
  VkFramebufferCreateInfo GetFrameBufferCreate(VkRenderPass render_pass, const std::vector<VkImageView> &attachments, uint32_t width, uint32_t height);

  //////////////
  /// SWAP CHAIN
  //////////////
  VkSwapchainCreateInfoKHR GetSwapChainCreate(VkSurfaceKHR                  surface_khr,
                                              uint32_t                      image_count,
                                              VkSurfaceFormatKHR            surface_format_khr,
                                              VkExtent2D                    extent_2d,
                                              const uint32_t               *queueFamilyIndices,
                                              VkSurfaceTransformFlagBitsKHR transform,
                                              VkPresentModeKHR              present_mode_khr);
} // namespace VulkanBootstrap

#endif // VULKANBOOTSTRAP_H
