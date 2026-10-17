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
#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "graphics/ShaderInfo.h"
#include "graphics/graphics_toplevel_defines.h"
#include "graphics_vulkan.h"
#include "system/File.h"
#include "system/Log.h"

#include "VulkanBootstrap.h"
#include "VulkanBufferInstance.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDeviceInstance.h"
#include "VulkanFrameBufferInstance.h"
#include "VulkanHelpers.h"
#include "VulkanShaderInstance.h"
#include "VulkanSwapChainInstance.h"
#include "VulkanTexture.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace graphics_vulkan
{
  VulkanObjects::VulkanCommandBuffer *ConvertCB(GraphicsTypes::CommandBufferInstance *cb_instance,
                                                const int                             line,
                                                const char                           *file = __FILE__)
  {
    if(!cb_instance)
    {
      Log::Error << "Trying to execute graphics command on invalid command buffer, in " << file << " : " << line << Log::End;
      return nullptr;
    }
    return reinterpret_cast<VulkanObjects::VulkanCommandBuffer *>(cb_instance);
  }
} // namespace graphics_vulkan

graphics_vulkan::VulkanGraphicsInstance::VulkanGraphicsInstance(SDL_Window *window, int width, int height)
{
  ShaderInfo::Init();

  Device         = std::make_unique<VulkanObjects::VulkanDeviceInstance>(window);
  CommandPool    = std::make_unique<VulkanObjects::VulkanCommandPool>(Device.get());
  DescriptorPool = std::make_unique<VulkanObjects::VulkanDescriptorPool>(Device.get());
  SwapChain      = std::make_unique<VulkanObjects::VulkanSwapChainInstance>(Device.get(), CommandPool.get(), width, height);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateShader(std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
                                                           const ShaderInfo                               &shader_info,
                                                           const std::vector<File::ShaderString>          &shader_code,
                                                           const std::string_view                          name) const
{
  shader_instance = std::make_unique<VulkanObjects::VulkanShaderInstance>(Device.get(), shader_info, shader_code, name);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                                                           const GraphicsTypes::BufferType                 type,
                                                           const size_t                                    buffer_size,
                                                           const std::string_view                          name) const
{
  buffer_instance = std::make_unique<VulkanObjects::VulkanBufferInstance>(Device.get(), type, buffer_size, name);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                                                           const GraphicsTypes::BufferType                 type,
                                                           const size_t                                    buffer_size,
                                                           const void                                     *data,
                                                           const std::string_view                          name) const
{
  buffer_instance = std::make_unique<VulkanObjects::VulkanBufferInstance>(Device.get(), type, buffer_size, name);

  void      *to_data;
  const auto vulkan_buffer = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance.get());
  vulkan_buffer->Map(&to_data);
  memcpy(to_data, data, buffer_size);
  vulkan_buffer->UnMap();
}

void graphics_vulkan::VulkanGraphicsInstance::MapBuffer(GraphicsTypes::BufferInstance *buffer_instance, const void *map_memory) const
{
  if(!buffer_instance)
    throw std::runtime_error("trying to map uninitialized buffer");
  const auto vulkan_buffer = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance);
  void      *to_data;
  vulkan_buffer->Map(&to_data);
  memcpy(to_data, map_memory, vulkan_buffer->GetSize());
  vulkan_buffer->UnMap();
}

// Copies lhs into rhs.
void graphics_vulkan::VulkanGraphicsInstance::CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs,
                                                         GraphicsTypes::BufferInstance *buffer_instance_lhs) const
{
  VulkanObjects::VulkanSingleCommandBuffer cmd(Device.get(), CommandPool.get());

  cmd.Begin();

  if(!buffer_instance_rhs)
  {
    Log::Error << "Invalid buffer given to copy into" << Log::End;
    return;
  }
  const auto *vulkan_buffer_instance_rhs = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance_rhs);
  if(!buffer_instance_lhs)
  {
    Log::Error << "Invalid buffer given to copy from" << Log::End;
    return;
  }
  const auto *vulkan_buffer_instance_lhs = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance_lhs);

  VkBufferCopy copy_region{};
  copy_region.size = vulkan_buffer_instance_lhs->GetSize();
  vkCmdCopyBuffer(cmd.Get(), vulkan_buffer_instance_lhs->Get(), vulkan_buffer_instance_rhs->Get(), 1, &copy_region);

  cmd.End();
}

std::vector<unsigned char> graphics_vulkan::VulkanGraphicsInstance::RetrieveBuffer(GraphicsTypes::BufferInstance *buffer_instance) const
{
  if(!buffer_instance)
  {
    Log::Error << "Invalid buffer given to retrieve from" << Log::End;
    return {};
  }
  const auto vulkan_buffer = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance);

  const size_t               size = vulkan_buffer->GetSize();
  std::vector<unsigned char> result(size);

  void *from_data = nullptr;
  vulkan_buffer->Map(&from_data);
  memcpy(result.data(), from_data, size);
  vulkan_buffer->UnMap();

  return result;
}

void graphics_vulkan::VulkanGraphicsInstance::CreateTexture(std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
                                                            const GraphicsTypes::TextureInfo                &texture_info,
                                                            const std::vector<const void *>                 &in_data,
                                                            const std::string_view                           name) const
{
  VulkanObjects::VulkanSingleCommandBuffer cmd(Device.get(), CommandPool.get());

  cmd.Begin();

  texture_instance = std::make_unique<VulkanObjects::VulkanTextureInstance>(Device.get(), name, cmd.Get(), in_data, texture_info);

  reinterpret_cast<VulkanObjects::VulkanTextureInstance *>(texture_instance.get())->GetImage()->CreateMipMaps(cmd.Get());

  cmd.End();
}

void graphics_vulkan::VulkanGraphicsInstance::SampleTexture(void                                 *destination,
                                                            const GraphicsTypes::TextureInstance *source,
                                                            const gm::ivec2                      &origin,
                                                            const gm::ivec2                      &size) const
{
  if(!source)
    return;

  const auto vulkan_texture_instance = reinterpret_cast<const VulkanObjects::VulkanTextureInstance *>(source);

  const size_t bytes_per_pixel = vulkan_texture_instance->GetInfo().Components;
  const size_t bytes_per_row   = bytes_per_pixel * size[0];
  const size_t buffer_size     = bytes_per_row * size[1];

  // Reuse a persistent staging buffer instead of allocating a new one every call.
  if(!SampleStagingBuffer || SampleStagingSize < buffer_size)
  {
    SampleStagingBuffer = std::make_unique<VulkanObjects::VulkanBufferInstance>(
        Device.get(),
        GraphicsTypes::BufferType::STAGING,
        buffer_size,
        "staging texture sample");
    SampleStagingSize = buffer_size;
  }
  const VulkanObjects::VulkanBufferInstance &staging_buffer = *SampleStagingBuffer;

  const auto last_layout = vulkan_texture_instance->GetImage()->GetLayout();

  VulkanObjects::VulkanSingleCommandBuffer single_command_buffer(Device.get(), CommandPool.get());
  single_command_buffer.Begin();
  vulkan_texture_instance->SetLayout(single_command_buffer.Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  VkBufferImageCopy region{};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;

  region.imageOffset = {origin[0], origin[1], 0};
  region.imageExtent = {static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1};

  vkCmdCopyImageToBuffer(single_command_buffer.Get(),
                         vulkan_texture_instance->GetImage()->Get(),
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         staging_buffer.Get(),
                         1,
                         &region);
  vulkan_texture_instance->SetLayout(single_command_buffer.Get(), last_layout);
  single_command_buffer.End();

  void *mapped_data;
  staging_buffer.Map(&mapped_data);
  memcpy(destination, mapped_data, buffer_size);
  staging_buffer.UnMap();
}

void graphics_vulkan::VulkanGraphicsInstance::UploadToTexture(const GraphicsTypes::TextureInstance *destination,
                                                              void                                 *source,
                                                              const size_t                          source_size,
                                                              const gm::ivec2                      &origin,
                                                              const gm::ivec2                      &size) const
{
  if(!source)
    return;

  const auto   vulkan_texture_instance = reinterpret_cast<const VulkanObjects::VulkanTextureInstance *>(destination);

  VulkanObjects::VulkanBufferInstance staging_buffer(Device.get(),
                                                     GraphicsTypes::BufferType::STAGING,
                                                     source_size,
                                                     "staging texture upload");
  MapBuffer(&staging_buffer, source);

  const auto last_layout = vulkan_texture_instance->GetImage()->GetLayout();

  VulkanObjects::VulkanSingleCommandBuffer single_command_buffer(Device.get(), CommandPool.get());
  single_command_buffer.Begin();
  vulkan_texture_instance->SetLayout(single_command_buffer.Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy region{};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;

  region.imageOffset = {origin[0], origin[1], 0};
  region.imageExtent = {static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), 1};

  vkCmdCopyBufferToImage(single_command_buffer.Get(),
                         staging_buffer.Get(),
                         vulkan_texture_instance->GetImage()->Get(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);
  vulkan_texture_instance->SetLayout(single_command_buffer.Get(), last_layout);

  single_command_buffer.End();
}

void graphics_vulkan::VulkanGraphicsInstance::CreateRenderBuffer(
    std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
    const GraphicsTypes::FrameBufferInfo                 &create_info,
    const std::string_view                                name) const
{
  GraphicsTypes::StateInfo state;
  state.Color   = create_info.ColorFormat != GraphicsTypes::ImageFormat::INVALID;
  state.Depth   = create_info.DepthFormat != GraphicsTypes::ImageFormat::INVALID;
  state.Samples = static_cast<int>(create_info.Samples);

  render_buffer_instance =
      std::make_unique<VulkanObjects::VulkanFrameBufferInstance>(Device.get(),
                                                                 create_info,
                                                                 state,
                                                                 VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT,
                                                                 name);
}

const GraphicsTypes::TextureInstance *graphics_vulkan::VulkanGraphicsInstance::GetRenderBufferTexture(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance)
  {
    Log::Error << "invalid render buffer to fetch texture from" << Log::End;
    return nullptr;
  }
  const auto *vulkan_render_buffer = reinterpret_cast<const VulkanObjects::VulkanFrameBufferInstance *>(render_buffer_instance);
  return vulkan_render_buffer->GetTexture();
}

void graphics_vulkan::VulkanGraphicsInstance::CreateCommandBuffer(std::unique_ptr<GraphicsTypes::CommandBufferInstance> &cb)
{
  cb = std::make_unique<VulkanObjects::VulkanCommandBuffer>(Device.get(), CommandPool.get());
}

void graphics_vulkan::VulkanGraphicsInstance::DispatchCompute(const GraphicsTypes::ShaderInstance         *shader,
                                                              const GraphicsTypes::TextureInstance *const *texture_instance,
                                                              const int                                    count,
                                                              const int                                    num_x,
                                                              const int                                    num_y,
                                                              const int                                    num_z) const
{
  const auto vulkan_shader = reinterpret_cast<const VulkanObjects::VulkanShaderInstance *>(shader);
  if(!vulkan_shader || !vulkan_shader->GetComputeShaderStage().module)
    return;

  std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
  layout_bindings.reserve(count);
  for(int x = 0; x < count; x++)
  {
    layout_bindings.emplace_back(VulkanBootstrap::GetDescriptorSetLayoutBinding(x,
                                                                                VulkanTranslate::ShaderStage::COMPUTE,
                                                                                VulkanTranslate::DescriptorType::STORAGE_TEXTURE));
  }

  VkDescriptorSetLayout compute_descriptor_set_layout;
  const auto            layout_info = VulkanBootstrap::GetDescriptorSetLayoutCreate(layout_bindings);

  if(vkCreateDescriptorSetLayout(Device->GetDevice(), &layout_info, nullptr, &compute_descriptor_set_layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create compute descriptor set layout!");

  const std::vector descriptor_set_layouts = {compute_descriptor_set_layout};

  VkPipelineLayout                 compute_layout;
  const VkPipelineLayoutCreateInfo compute_layout_info = VulkanBootstrap::GetPipelineLayoutCreate(descriptor_set_layouts, {});

  VulkanHelpers::VK_CHECK_RESULT(vkCreatePipelineLayout(Device->GetDevice(), &compute_layout_info, nullptr, &compute_layout),
                                 __LINE__,
                                 __FILE__);

  VkPipeline                  compute_pipeline;
  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.layout = compute_layout;
  pipelineInfo.stage  = vulkan_shader->GetComputeShaderStage();
  VulkanHelpers::VK_CHECK_RESULT(
      vkCreateComputePipelines(Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &compute_pipeline),
      __LINE__,
      __FILE__);

  VulkanObjects::VulkanSingleComputeCommandBuffer cmd(Device.get(), CommandPool.get());
  cmd.Begin();

  auto *const *vulkan_texture_instances = reinterpret_cast<const VulkanObjects::VulkanTextureInstance *const *>(texture_instance);
  for(int i = 0; i < count; i++)
    vulkan_texture_instances[i]->SetLayout(cmd.Get(), VK_IMAGE_LAYOUT_GENERAL);

  VkDescriptorSet descriptor_set = DescriptorPool->AllocateDescriptorSet(compute_descriptor_set_layout);

  for(int i = 0; i < count; i++)
  {
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // vulkan_texture_instances[i]->Layout;
    image_info.imageView   = vulkan_texture_instances[i]->GetView()->Get();
    image_info.sampler     = vulkan_texture_instances[i]->GetSampler()->Get();

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet           = descriptor_set;
    descriptor_write.dstBinding       = i;
    descriptor_write.dstArrayElement  = 0;
    descriptor_write.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptor_write.descriptorCount  = 1;
    descriptor_write.pBufferInfo      = nullptr;
    descriptor_write.pImageInfo       = &image_info;
    descriptor_write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(Device->GetDevice(), 1, &descriptor_write, 0, nullptr);
  }

  vkCmdBindPipeline(cmd.Get(), VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);
  vkCmdBindDescriptorSets(cmd.Get(), VK_PIPELINE_BIND_POINT_COMPUTE, compute_layout, 0, 1, &descriptor_set, 0, nullptr);
  vkCmdDispatch(cmd.Get(), num_x, num_y, num_z);

  for(int i = 0; i < count; i++)
    vulkan_texture_instances[i]->SetLayout(cmd.Get(), VK_IMAGE_LAYOUT_GENERAL);

  cmd.End();

  vkDestroyPipelineLayout(Device->GetDevice(), compute_layout, nullptr);
  vkDestroyPipeline(Device->GetDevice(), compute_pipeline, nullptr);
  vkDestroyDescriptorSetLayout(Device->GetDevice(), compute_descriptor_set_layout, nullptr);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const
{
  if(!texture_instance)
    return;
  auto vulkan_texture_instance = reinterpret_cast<const VulkanObjects::VulkanTextureInstance *>(texture_instance);

  VulkanObjects::VulkanSingleCommandBuffer cmd(Device.get(), CommandPool.get());
  cmd.Begin();

  vulkan_texture_instance->CreateMipMaps(cmd.Get());

  cmd.End();
}

bool graphics_vulkan::VulkanGraphicsInstance::StartDraw(const int width, const int height)
{
  Device->BeginFrame();

  if(!SwapChain->BeginFrame(CommandPool.get(), width, height))
    return false;

  return true;
}


int graphics_vulkan::VulkanGraphicsInstance::AcquireFrameIndex() const
{
  return Device->GetCurrentFrame();
}


void graphics_vulkan::VulkanGraphicsInstance::StartMainRenderPass(GraphicsTypes::CommandBufferInstance *cb)
{
  auto *vk_cb = ConvertCB(cb, __LINE__);
  if(!vk_cb)
    return;

  vk_cb->StartMainRenderPass(SwapChain.get());
}

void graphics_vulkan::VulkanGraphicsInstance::EndDraw(GraphicsTypes::CommandBufferInstance *cb, const int width, const int height)
{
  auto *vk_cb = ConvertCB(cb, __LINE__);
  if(!vk_cb)
    return;

  vk_cb->SubmitForPresent(SwapChain->GetRenderFinished());

  SwapChain->EndFrame(CommandPool.get(), width, height);
}

void graphics_vulkan::VulkanGraphicsInstance::Wait()
{
  vkDeviceWaitIdle(Device->GetDevice());
}

graphics_vulkan::VulkanGraphicsInstance::~VulkanGraphicsInstance()
{
  vkDeviceWaitIdle(Device->GetDevice());

  DescriptorPool.reset();
  SwapChain.reset();
  CommandPool.reset();
  Device.reset();
}
