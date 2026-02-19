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
#include "VulkanBootstrap.h"


//////////////
/// COMMAND BUFFERS
//////////////
VkCommandPoolCreateInfo VulkanBootstrap::GetCommandPoolCreate(const uint32_t queue_family_index)
{
  VkCommandPoolCreateInfo command_pool_create_info{};
  command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_create_info.queueFamilyIndex = queue_family_index;
  return command_pool_create_info;
}

VkSemaphoreCreateInfo VulkanBootstrap::GetSemaphoreCreate()
{
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  return semaphore_info;
}

VkFenceCreateInfo VulkanBootstrap::GetFenceCreate()
{
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  return fence_info;
}

VkCommandBufferAllocateInfo VulkanBootstrap::GetCommandBufferAllocate(VkCommandPool command_pool, const uint32_t count)
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool        = command_pool;
  alloc_info.commandBufferCount = count;
  return alloc_info;
}

VkCommandBufferBeginInfo VulkanBootstrap::GetCommandBufferBegin(const VulkanTranslate::CommandBufferType type)
{
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  switch(type)
  {
  case VulkanTranslate::CommandBufferType::ONE_TIME: begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; break;
  case VulkanTranslate::CommandBufferType::REUSE:    begin_info.flags = 0; break;
  }
  begin_info.pInheritanceInfo = nullptr;
  return begin_info;
}

VkSubmitInfo VulkanBootstrap::GetSubmit(const std::vector<VkCommandBuffer>      &buffer,
                                        const std::vector<VkSemaphore>          &wait_semaphores,
                                        const std::vector<VkPipelineStageFlags> &wait_stages,
                                        const std::vector<VkSemaphore>          &signal_semaphores)
{
  VkSubmitInfo submit_info{};
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pCommandBuffers      = buffer.data();
  submit_info.commandBufferCount   = buffer.size();
  submit_info.pWaitSemaphores      = wait_semaphores.data();
  submit_info.waitSemaphoreCount   = wait_semaphores.size();
  submit_info.pWaitDstStageMask    = wait_stages.data();
  submit_info.pSignalSemaphores    = signal_semaphores.data();
  submit_info.signalSemaphoreCount = signal_semaphores.size();
  return submit_info;
}

//////////////
/// SHADERS
//////////////
VkShaderModuleCreateInfo VulkanBootstrap::GetShaderModuleCreate(const std::vector<std::byte> &code)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode    = reinterpret_cast<const uint32_t *>(code.data());
  return create_info;
}

VkPipelineShaderStageCreateInfo VulkanBootstrap::GetShaderStageCreate(VkShaderModule shader, const VulkanTranslate::ShaderStage stage)
{
  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  switch(stage)
  {
  case VulkanTranslate::ShaderStage::VERTEX:   shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT; break;
  case VulkanTranslate::ShaderStage::FRAGMENT: shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
  case VulkanTranslate::ShaderStage::COMPUTE:  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
  case VulkanTranslate::ShaderStage::ALL:      throw std::runtime_error("Stage ShaderStage::ALL not supported in stage creation");
  }
  shader_stage_info.module = shader;
  shader_stage_info.pName  = "main";
  return shader_stage_info;
}

VkPipelineLayoutCreateInfo VulkanBootstrap::GetPipelineLayoutCreate(const std::vector<VkDescriptorSetLayout> &descriptor_set_layouts, const std::vector<VkPushConstantRange> &push_constant_ranges)
{
  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount         = descriptor_set_layouts.size();
  pipeline_layout_info.pSetLayouts            = descriptor_set_layouts.data();
  pipeline_layout_info.pushConstantRangeCount = push_constant_ranges.size();
  pipeline_layout_info.pPushConstantRanges    = push_constant_ranges.data();
  return pipeline_layout_info;
}

//////////////
/// DESCRIPTOR SETS
//////////////
VkDescriptorSetLayoutBinding VulkanBootstrap::GetDescriptorSetLayoutBinding(const uint32_t binding, const VulkanTranslate::ShaderStage stage, const VulkanTranslate::DescriptorType type)
{
  VkDescriptorSetLayoutBinding binding_info{};
  binding_info.binding         = binding;
  binding_info.descriptorCount = 1;
  switch(type)
  {
  case VulkanTranslate::DescriptorType::UNIFORM_BUFFER:  binding_info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
  case VulkanTranslate::DescriptorType::TEXTURE:         binding_info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
  case VulkanTranslate::DescriptorType::STORAGE_TEXTURE: binding_info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
  }
  switch(stage)
  {
  case VulkanTranslate::ShaderStage::VERTEX:   binding_info.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; break;
  case VulkanTranslate::ShaderStage::FRAGMENT: binding_info.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; break;
  case VulkanTranslate::ShaderStage::COMPUTE:  binding_info.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; break;
  case VulkanTranslate::ShaderStage::ALL:      binding_info.stageFlags = VK_SHADER_STAGE_ALL; break;
  }
  binding_info.pImmutableSamplers = nullptr;
  return binding_info;
}

VkDescriptorSetLayoutCreateInfo VulkanBootstrap::GetDescriptorSetLayoutCreate(const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
  descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptor_set_layout_create_info.bindingCount = bindings.size();
  descriptor_set_layout_create_info.pBindings    = bindings.data();
  return descriptor_set_layout_create_info;
}

//////////////
/// ATTACHMENTS
//////////////
VkAttachmentDescription
VulkanBootstrap::GetAttachmentDescription(const VulkanTranslate::AttachmentType type, const GraphicsTypes::ImageFormat image_format, const uint32_t sample_count, const bool presenter)
{
  VkAttachmentDescription attach_desc{};
  attach_desc.format  = VulkanTranslate::GetVkFormat(image_format);
  attach_desc.samples = VulkanTranslate::GetVkSampleCountFromInt(sample_count);

  attach_desc.initialLayout = presenter ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_UNDEFINED;

  switch(type)
  {
  case VulkanTranslate::AttachmentType::COLOR:
    attach_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    break;
  case VulkanTranslate::AttachmentType::COLOR_RESOLVE:
    attach_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    break;
  case VulkanTranslate::AttachmentType::DEPTH:
    attach_desc.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    break;
  }

  if(presenter) attach_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  else if(image_format == GraphicsTypes::ImageFormat::DEPTH) attach_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  else attach_desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attach_desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attach_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  return attach_desc;
}


VkAttachmentReference VulkanBootstrap::GetAttachmentReference(const uint32_t index, const VulkanTranslate::AttachmentType type)
{
  VkAttachmentReference ref{};
  ref.attachment = index;
  switch(type)
  {
  case VulkanTranslate::AttachmentType::COLOR:
  case VulkanTranslate::AttachmentType::COLOR_RESOLVE: ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
  case VulkanTranslate::AttachmentType::DEPTH:         ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
  }
  return ref;
}

VkSubpassDescription VulkanBootstrap::GetSubPassDescription(const std::vector<VkAttachmentReference> &color_attachments,
                                                            const VkAttachmentReference              &depth_attachment,
                                                            const VkAttachmentReference              &color_resolve_attachments)
{
  VkSubpassDescription sub_pass{};
  sub_pass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  sub_pass.colorAttachmentCount = color_attachments.size();
  sub_pass.pColorAttachments    = color_attachments.data();
  if(depth_attachment.layout != VK_IMAGE_LAYOUT_UNDEFINED) sub_pass.pDepthStencilAttachment = &depth_attachment;
  if(color_resolve_attachments.layout != VK_IMAGE_LAYOUT_UNDEFINED) sub_pass.pResolveAttachments = &color_resolve_attachments;
  return sub_pass;
}

VkSubpassDependency VulkanBootstrap::GetSubPassDependencyResolve()
{
  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  return dependency;
}

VkSubpassDependency VulkanBootstrap::GetSubPassDependencyResolveExit()
{
  VkSubpassDependency dependency{};
  dependency.srcSubpass    = 0;
  dependency.dstSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependency.dstAccessMask = 0;
  return dependency;
}

//////////////
/// RENDER PASS
//////////////
VkRenderPassCreateInfo
VulkanBootstrap::GetRenderPassCreate(const std::vector<VkAttachmentDescription> &attachments, const std::vector<VkSubpassDescription> &sub_passes, const std::vector<VkSubpassDependency> &dependencies)
{
  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = attachments.size();
  render_pass_info.pAttachments    = attachments.data();
  render_pass_info.subpassCount    = sub_passes.size();
  render_pass_info.pSubpasses      = sub_passes.data();
  render_pass_info.dependencyCount = dependencies.size();
  render_pass_info.pDependencies   = dependencies.data();
  return render_pass_info;
}

//////////////
/// MEMORY
//////////////
VkMemoryAllocateInfo VulkanBootstrap::GetMemoryAllocateInfo(const VkDeviceSize alloc_size, const uint32_t type_index)
{
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize  = alloc_size;
  alloc_info.memoryTypeIndex = type_index;
  return alloc_info;
}

//////////////
/// BUFFER
//////////////
VkBufferCreateInfo VulkanBootstrap::GetBufferCreate(const GraphicsTypes::BufferType type, const VkDeviceSize size)
{
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  switch(type)
  {
  case GraphicsTypes::BufferType::UNIFORM:
  case GraphicsTypes::BufferType::UNIFORM_DYNAMIC: buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
  case GraphicsTypes::BufferType::TEXTURE:         buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; break;
  case GraphicsTypes::BufferType::VERTEX:          buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
  case GraphicsTypes::BufferType::VERTEX_DYNAMIC:  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
  case GraphicsTypes::BufferType::INDEX:           buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
  case GraphicsTypes::BufferType::STAGING:         buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; break;
  }
  buffer_info.size        = size;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return buffer_info;
}

//////////////
/// IMAGE
//////////////
VkImageCreateInfo VulkanBootstrap::GetImageCreate(const GraphicsTypes::TextureType   type,
                                                  const GraphicsTypes::ImageFormat   format,
                                                  const GraphicsTypes::TextureTarget target,
                                                  const uint32_t                     samples,
                                                  const uint32_t                     width,
                                                  const uint32_t                     height,
                                                  const uint32_t                     depth,
                                                  const uint32_t                     layers,
                                                  const uint32_t                     mip_levels)
{
  VkImageCreateInfo image_info{};
  image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType     = VulkanTranslate::GetVkImageType(type);
  image_info.extent.width  = width;
  image_info.extent.height = height;
  image_info.extent.depth  = depth;
  image_info.arrayLayers   = layers;
  image_info.mipLevels     = mip_levels;
  image_info.format        = VulkanTranslate::GetVkFormat(format);
  image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  switch(target)
  {
  case GraphicsTypes::TextureTarget::READ:
  case GraphicsTypes::TextureTarget::WRITE:      image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; break;
  case GraphicsTypes::TextureTarget::READ_WRITE: image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT; break;
  case GraphicsTypes::TextureTarget::DRAW:
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | (format == GraphicsTypes::ImageFormat::DEPTH ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    break;
  }
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples     = VulkanTranslate::GetVkSampleCountFromInt(samples);
  image_info.flags       = 0;
  return image_info;
}

VkImageViewCreateInfo VulkanBootstrap::GetimageViewCreate(
    VkImage image, const VkImageAspectFlags aspect_flags, const GraphicsTypes::TextureType type, const GraphicsTypes::ImageFormat format, const uint32_t layers, const uint32_t mip_levels)
{
  VkImageViewCreateInfo view_info{};
  view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image                           = image;
  view_info.viewType                        = VulkanTranslate::GetVkViewType(type);
  view_info.format                          = VulkanTranslate::GetVkFormat(format);
  view_info.subresourceRange.aspectMask     = aspect_flags;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount     = layers;
  view_info.subresourceRange.baseMipLevel   = 0;
  view_info.subresourceRange.levelCount     = mip_levels;
  return view_info;
}

VkSamplerCreateInfo VulkanBootstrap::GetSamplerCreate(const float max_anisotropy, const float max_lod, const GraphicsTypes::TextureAddressMode address_mode, const GraphicsTypes::TextureFilter filter)
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter               = filter == GraphicsTypes::TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
  sampler_info.minFilter               = filter == GraphicsTypes::TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
  sampler_info.addressModeU            = address_mode == GraphicsTypes::TextureAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV            = address_mode == GraphicsTypes::TextureAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW            = address_mode == GraphicsTypes::TextureAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.anisotropyEnable        = VK_TRUE;
  sampler_info.maxAnisotropy           = max_anisotropy;
  sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  // Might be important for shadow mapping
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp     = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias    = 0.0f;
  sampler_info.minLod        = 0.0f;
  sampler_info.maxLod        = max_lod;
  return sampler_info;
}

VkImageMemoryBarrier VulkanBootstrap::GetImageMemoryBarrierWithoutAccess(
    VkImage image, const VkImageAspectFlags aspect, const VkImageLayout src_layout, const VkImageLayout dst_layout, const uint32_t layers, const uint32_t mip_levels)
{
  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = src_layout;
  barrier.newLayout                       = dst_layout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = aspect;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = mip_levels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = layers;
  return barrier;
}

VkBufferImageCopy VulkanBootstrap::GetSimpleBufferImageCopyRegion(
    const VkImageAspectFlags aspect, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t start_layer, const uint32_t end_layer, const uint32_t mip_level)
{
  VkBufferImageCopy region{};
  region.bufferOffset                    = 0;
  region.bufferRowLength                 = 0;
  region.bufferImageHeight               = 0;
  region.imageSubresource.aspectMask     = aspect;
  region.imageSubresource.mipLevel       = mip_level;
  region.imageSubresource.baseArrayLayer = start_layer;
  region.imageSubresource.layerCount     = end_layer + 1;
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {width, height, depth};
  return region;
}

//////////////
/// FRAME BUFFER
//////////////
VkFramebufferCreateInfo VulkanBootstrap::GetFrameBufferCreate(VkRenderPass render_pass, const std::vector<VkImageView> &attachments, const uint32_t width, const uint32_t height)
{
  VkFramebufferCreateInfo frame_buffer_info{};
  frame_buffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frame_buffer_info.renderPass      = render_pass;
  frame_buffer_info.attachmentCount = attachments.size();
  frame_buffer_info.pAttachments    = attachments.data();
  frame_buffer_info.width           = width;
  frame_buffer_info.height          = height;
  frame_buffer_info.layers          = 1;
  return frame_buffer_info;
}

//////////////
/// SWAP CHAIN
//////////////
VkSwapchainCreateInfoKHR VulkanBootstrap::GetSwapChainCreate(VkSurfaceKHR                        surface_khr,
                                                             const uint32_t                      image_count,
                                                             const VkSurfaceFormatKHR            surface_format_khr,
                                                             const VkExtent2D                    extent_2d,
                                                             const uint32_t                     *queueFamilyIndices,
                                                             const VkSurfaceTransformFlagBitsKHR transform,
                                                             const VkPresentModeKHR              present_mode_khr)
{
  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface          = surface_khr;
  create_info.minImageCount    = image_count;
  create_info.imageFormat      = surface_format_khr.format;
  create_info.imageColorSpace  = surface_format_khr.colorSpace;
  create_info.imageExtent      = extent_2d;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if(queueFamilyIndices)
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;
  }
  create_info.preTransform   = transform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode    = present_mode_khr;
  create_info.clipped        = VK_TRUE;        // Might be error-prone, but not really.
  create_info.oldSwapchain   = VK_NULL_HANDLE; // Important in the future
  return create_info;
}
