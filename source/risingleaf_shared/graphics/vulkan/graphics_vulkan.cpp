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
#include "VulkanPipelineState.h"
#include "VulkanShaderInstance.h"
#include "VulkanSwapChainInstance.h"
#include "VulkanTexture.h"

#define VMA_IMPLEMENTATION
#include "external/vk_mem_alloc.h"

namespace graphics_vulkan
{
  void BindBuffers(
      const VulkanGraphicsInstance               *graphics_instance,
      const VulkanObjects::VulkanShaderInstance  *current_shader_instance,
      const GraphicsTypes::BufferInstance *const *buffer_instance,
      const int                                   count,
      const GraphicsTypes::UBOBindPoint           bind_point,
      const int                                   set,
      const size_t                                offset,
      const size_t                                size)
  {
    if(!buffer_instance)
    {
      Log::Error << "Trying to bind invalid buffer." << Log::End;
      return;
    }
    auto *const *vulkan_buffer_instances =
        reinterpret_cast<const VulkanObjects::VulkanBufferInstance *const *>(buffer_instance);

    VkDescriptorSet descriptor_set = nullptr;
    switch(bind_point)
    {
    case GraphicsTypes::UBOBindPoint::Common:
      descriptor_set = graphics_instance->DescriptorPool->AllocateDescriptorSet(
          current_shader_instance->GetDescriptorSetLayoutUBOCommon());
      break;
    case GraphicsTypes::UBOBindPoint::Specific:
      descriptor_set = graphics_instance->DescriptorPool->AllocateDescriptorSet(
          current_shader_instance->GetDescriptorSetLayoutUBOSpecial());
      break;
    }

    for(int i = 0; i < count; i++)
    {
      VkDescriptorBufferInfo buffer_info{};
      buffer_info.buffer = vulkan_buffer_instances[i]->Get();
      buffer_info.offset = offset;
      buffer_info.range  = size ? size : vulkan_buffer_instances[i]->GetSize();

      VkWriteDescriptorSet descriptor_write{};
      descriptor_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_write.dstSet           = descriptor_set;
      descriptor_write.dstBinding       = i;
      descriptor_write.dstArrayElement  = 0;
      descriptor_write.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptor_write.descriptorCount  = 1;
      descriptor_write.pBufferInfo      = &buffer_info;
      descriptor_write.pImageInfo       = nullptr;
      descriptor_write.pTexelBufferView = nullptr;

      vkUpdateDescriptorSets(graphics_instance->Device->GetDevice(), 1, &descriptor_write, 0, nullptr);
    }

    vkCmdBindDescriptorSets(
        graphics_instance->CommandBuffers[graphics_instance->Device->GetCurrentFrame()],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        current_shader_instance->GetPipelineLayout(),
        set,
        1,
        &descriptor_set,
        0,
        nullptr);
  }

  void SubmitDrawCommands(const VulkanGraphicsInstance *graphics_instance)
  {
    // Populate dynamic buffers:
    std::vector<uint32_t> common_offsets;
    std::vector<uint32_t> custom_offsets;
    std::vector<uint32_t> vertex_offsets;
    {
      void *to_data;
      graphics_instance->DynamicUniformBuffersCommon[graphics_instance->Device->GetCurrentFrame()]->Map(&to_data);
      for(const auto &buffer : graphics_instance->CommonUniformBufferBindings)
      {
        memcpy(
            static_cast<unsigned char *>(to_data) + graphics_instance->DynamicUniformBuffersCommonOffset,
            buffer.data(),
            buffer.size());

        common_offsets.emplace_back(graphics_instance->DynamicUniformBuffersCommonOffset);

        graphics_instance->DynamicUniformBuffersCommonOffset += buffer.size();
        graphics_instance->DynamicUniformBuffersCommonOffset =
            (graphics_instance->DynamicUniformBuffersCommonOffset + 255) & ~(255);
      }
      graphics_instance->DynamicUniformBuffersCommon[graphics_instance->Device->GetCurrentFrame()]->UnMap();
    }
    {
      void *to_data;
      graphics_instance->DynamicUniformBuffersSpecific[graphics_instance->Device->GetCurrentFrame()]->Map(&to_data);
      for(const auto &buffer : graphics_instance->CustomUniformBufferBindings)
      {
        memcpy(
            static_cast<unsigned char *>(to_data) + graphics_instance->DynamicUniformBuffersSpecificOffset,
            buffer.data(),
            buffer.size());

        custom_offsets.emplace_back(graphics_instance->DynamicUniformBuffersSpecificOffset);

        graphics_instance->DynamicUniformBuffersSpecificOffset += buffer.size();
        graphics_instance->DynamicUniformBuffersSpecificOffset =
            (graphics_instance->DynamicUniformBuffersSpecificOffset + 255) & ~(255);
      }
      graphics_instance->DynamicUniformBuffersSpecific[graphics_instance->Device->GetCurrentFrame()]->UnMap();
    }
    {
      void *to_data;
      graphics_instance->DynamicVertexBuffers[graphics_instance->Device->GetCurrentFrame()]->Map(&to_data);
      for(const auto &buffer : graphics_instance->DynamicDrawCalls)
      {
        memcpy(
            static_cast<unsigned char *>(to_data) + graphics_instance->DynamicVertexBuffersOffset,
            buffer.Data.data(),
            buffer.Data.size());

        vertex_offsets.emplace_back(graphics_instance->DynamicVertexBuffersOffset);

        graphics_instance->DynamicVertexBuffersOffset += buffer.Data.size();
        graphics_instance->DynamicVertexBuffersOffset = (graphics_instance->DynamicVertexBuffersOffset + 255) & ~(255);
      }
      graphics_instance->DynamicVertexBuffers[graphics_instance->Device->GetCurrentFrame()]->UnMap();
    }

    const auto &current_command_buffer =
        graphics_instance->CommandBuffers[graphics_instance->Device->GetCurrentFrame()];
    const VulkanObjects::VulkanShaderInstance *current_shader_instance = nullptr;
    VkPipeline                                 current_pipeline        = nullptr;

    for(const auto &[command, index] : graphics_instance->CommandsRecorded)
    {
      switch(command)
      {
      case CommandType::SHADER_BIND:
        {
          current_shader_instance = graphics_instance->BoundShaders[index];
          break;
        }
      case CommandType::INDEX_BIND:
        {
          vkCmdBindIndexBuffer(
              current_command_buffer,
              graphics_instance->BoundBuffers[index]->Get(),
              0,
              VK_INDEX_TYPE_UINT32);
          break;
        }
      case CommandType::VERTEX_BIND:
        {
          const VkBuffer         vertexBuffers[] = {graphics_instance->BoundBuffers[index]->Get()};
          constexpr VkDeviceSize offsets[]       = {0};

          vkCmdBindVertexBuffers(current_command_buffer, 0, 1, vertexBuffers, offsets);
          break;
        }
      case CommandType::TEXTURE_BIND:
        {
          if(current_shader_instance)
          {
            vkCmdBindDescriptorSets(
                current_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                current_shader_instance->GetPipelineLayout(),
                graphics_instance->BoundTextures[index].Set,
                1,
                &graphics_instance->BoundTextures[index].DescriptorSet,
                0,
                nullptr);
          }
          break;
        }
      case CommandType::COMMON_UNIFORM_UPDATE:
        {
          if(current_shader_instance)
          {
            auto *buffer =
                graphics_instance->DynamicUniformBuffersCommon[graphics_instance->Device->GetCurrentFrame()].get();
            BindBuffers(
                graphics_instance,
                current_shader_instance,
                reinterpret_cast<GraphicsTypes::BufferInstance **>(&buffer),
                1,
                GraphicsTypes::UBOBindPoint::Common,
                0,
                common_offsets[index],
                graphics_instance->CommonUniformBufferBindings[index].size());
          }
          break;
        }
      case CommandType::CUSTOM_UNIFORM_UPDATE:
        {
          if(current_shader_instance)
          {
            auto *buffer =
                graphics_instance->DynamicUniformBuffersSpecific[graphics_instance->Device->GetCurrentFrame()].get();
            BindBuffers(
                graphics_instance,
                current_shader_instance,
                reinterpret_cast<GraphicsTypes::BufferInstance **>(&buffer),
                1,
                GraphicsTypes::UBOBindPoint::Specific,
                1,
                custom_offsets[index],
                graphics_instance->CustomUniformBufferBindings[index].size());
          }
          break;
        }
      case CommandType::DRAW:
        {
          if(current_pipeline != graphics_instance->DrawCalls[index].Pipeline)
          {
            vkCmdBindPipeline(
                current_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphics_instance->DrawCalls[index].Pipeline);
            current_pipeline = graphics_instance->DrawCalls[index].Pipeline;
          }
          vkCmdDraw(
              current_command_buffer,
              graphics_instance->DrawCalls[index].Count,
              1,
              graphics_instance->DrawCalls[index].Start,
              0);
          break;
        }
      case CommandType::DRAW_INDEXED:
        {
          if(current_pipeline != graphics_instance->DrawCalls[index].Pipeline)
          {
            vkCmdBindPipeline(
                current_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphics_instance->DrawCalls[index].Pipeline);
            current_pipeline = graphics_instance->DrawCalls[index].Pipeline;
          }
          vkCmdDrawIndexed(
              current_command_buffer,
              graphics_instance->DrawCalls[index].Count,
              1,
              graphics_instance->DrawCalls[index].Start,
              0,
              0);
          break;
        }
      case CommandType::DRAW_DYNAMIC:
        {
          const VkBuffer vertexBuffers[] = {
              graphics_instance->DynamicVertexBuffers[graphics_instance->Device->GetCurrentFrame()]->Get()};
          const VkDeviceSize offsets[] = {vertex_offsets[index]};

          vkCmdBindVertexBuffers(current_command_buffer, 0, 1, vertexBuffers, offsets);

          if(current_pipeline != graphics_instance->DynamicDrawCalls[index].Pipeline)
          {
            vkCmdBindPipeline(
                current_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphics_instance->DynamicDrawCalls[index].Pipeline);
            current_pipeline = graphics_instance->DynamicDrawCalls[index].Pipeline;
          }
          vkCmdDraw(current_command_buffer, graphics_instance->DynamicDrawCalls[index].Count, 1, 0, 0);
          break;
        }
      }
    }

    graphics_instance->BoundShaders.clear();
    graphics_instance->BoundBuffers.clear();
    graphics_instance->BoundTextures.clear();
    graphics_instance->DrawCalls.clear();
    graphics_instance->DynamicDrawCalls.clear();
    graphics_instance->CommonUniformBufferBindings.clear();
    graphics_instance->CustomUniformBufferBindings.clear();
    graphics_instance->CommandsRecorded.clear();
  }
} // namespace graphics_vulkan

graphics_vulkan::VulkanGraphicsInstance::VulkanGraphicsInstance(const int width, const int height)
{
  ShaderInfo::Init();

  Device         = std::make_unique<VulkanObjects::VulkanDeviceInstance>();
  CommandPool    = std::make_unique<VulkanObjects::VulkanCommandPool>(Device.get());
  DescriptorPool = std::make_unique<VulkanObjects::VulkanDescriptorPool>(Device.get());
  SwapChain = std::make_unique<VulkanObjects::VulkanSwapChainInstance>(Device.get(), CommandPool.get(), width, height);

  const auto buffer_allocate_info = VulkanBootstrap::GetCommandBufferAllocate(
      CommandPool->Get(),
      VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT);
  VulkanHelpers::VK_CHECK_RESULT(
      vkAllocateCommandBuffers(Device->GetDevice(), &buffer_allocate_info, CommandBuffers.data()),
      __LINE__,
      __FILE__);

  for(size_t i = 0; i < VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT; i++)
  {
    DynamicUniformBuffersCommon[i] = std::make_unique<VulkanObjects::VulkanBufferInstance>(
        Device.get(),
        GraphicsTypes::BufferType::UNIFORM_DYNAMIC,
        1024 * 1024 * 8,
        "dynamic_ubo_cm");
    DynamicUniformBuffersSpecific[i] = std::make_unique<VulkanObjects::VulkanBufferInstance>(
        Device.get(),
        GraphicsTypes::BufferType::UNIFORM_DYNAMIC,
        1024 * 1024 * 8,
        "dynamic_ubo_spec");
    DynamicVertexBuffers[i] = std::make_unique<VulkanObjects::VulkanBufferInstance>(
        Device.get(),
        GraphicsTypes::BufferType::VERTEX_DYNAMIC,
        1024 * 1024 * 8,
        "dynamic_vert");
  }
}

void graphics_vulkan::VulkanGraphicsInstance::CreateShader(
    std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
    const ShaderInfo                               &shader_info,
    const std::vector<File::ShaderString>          &shader_code,
    const std::string_view                          name) const
{
  shader_instance = std::make_unique<VulkanObjects::VulkanShaderInstance>(Device.get(), shader_info, shader_code, name);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateBuffer(
    std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
    const GraphicsTypes::BufferType                 type,
    const size_t                                    buffer_size,
    const std::string_view                          name) const
{
  buffer_instance = std::make_unique<VulkanObjects::VulkanBufferInstance>(Device.get(), type, buffer_size, name);
}

void graphics_vulkan::VulkanGraphicsInstance::CreateBuffer(
    std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
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

void graphics_vulkan::VulkanGraphicsInstance::MapBuffer(
    GraphicsTypes::BufferInstance *buffer_instance,
    const void                    *map_memory) const
{
  if(!buffer_instance) throw std::runtime_error("trying to map uninitialized buffer");
  const auto vulkan_buffer = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance);
  void      *to_data;
  vulkan_buffer->Map(&to_data);
  memcpy(to_data, map_memory, vulkan_buffer->GetSize());
  vulkan_buffer->UnMap();
}

// Copies lhs into rhs.
void graphics_vulkan::VulkanGraphicsInstance::CopyBuffer(
    GraphicsTypes::BufferInstance *buffer_instance_rhs,
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

void graphics_vulkan::VulkanGraphicsInstance::CreateTexture(
    std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
    const GraphicsTypes::TextureInfo                &texture_info,
    const void                                      *in_data,
    const std::string_view                           name) const
{
  VulkanObjects::VulkanSingleCommandBuffer cmd(Device.get(), CommandPool.get());

  cmd.Begin();

  texture_instance =
      std::make_unique<VulkanObjects::VulkanTextureInstance>(Device.get(), name, cmd.Get(), in_data, texture_info);

  reinterpret_cast<VulkanObjects::VulkanTextureInstance *>(texture_instance.get())
      ->GetImage()
      ->CreateMipMaps(cmd.Get());

  cmd.End();
}

void graphics_vulkan::VulkanGraphicsInstance::CreateRenderBuffer(
    std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
    const GraphicsTypes::FrameBufferInfo                 &create_info,
    const std::string_view                                name) const
{
  GraphicsTypes::StateInfo state;
  state.Color   = create_info.HasColor;
  state.Depth   = create_info.HasDepth;
  state.Samples = static_cast<int>(create_info.Samples);

  render_buffer_instance = std::make_unique<VulkanObjects::VulkanFrameBufferInstance>(
      Device.get(),
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
  const auto *vulkan_render_buffer =
      reinterpret_cast<const VulkanObjects::VulkanFrameBufferInstance *>(render_buffer_instance);
  return vulkan_render_buffer->GetTexture();
}

void graphics_vulkan::VulkanGraphicsInstance::DispatchCompute(
    const GraphicsTypes::ShaderInstance         *shader,
    const GraphicsTypes::TextureInstance *const *texture_instance,
    const int                                    count,
    const int                                    num_x,
    const int                                    num_y,
    const int                                    num_z) const
{
  const auto vulkan_shader = reinterpret_cast<const VulkanObjects::VulkanShaderInstance *>(shader);
  if(!vulkan_shader || !vulkan_shader->GetComputeShaderStage().module) return;

  std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
  for(int x = 0; x < count; x++)
  {
    layout_bindings.emplace_back(
        VulkanBootstrap::GetDescriptorSetLayoutBinding(
            x,
            VulkanTranslate::ShaderStage::COMPUTE,
            VulkanTranslate::DescriptorType::STORAGE_TEXTURE));
  }

  VkDescriptorSetLayout compute_descriptor_set_layout;
  const auto            layout_info = VulkanBootstrap::GetDescriptorSetLayoutCreate(layout_bindings);

  if(vkCreateDescriptorSetLayout(Device->GetDevice(), &layout_info, nullptr, &compute_descriptor_set_layout) !=
     VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute descriptor set layout!");
  }

  const std::vector descriptor_set_layouts = {compute_descriptor_set_layout};

  VkPipelineLayout                 compute_layout;
  const VkPipelineLayoutCreateInfo compute_layout_info =
      VulkanBootstrap::GetPipelineLayoutCreate(descriptor_set_layouts, {});

  VulkanHelpers::VK_CHECK_RESULT(
      vkCreatePipelineLayout(Device->GetDevice(), &compute_layout_info, nullptr, &compute_layout),
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

  auto *const *vulkan_texture_instances =
      reinterpret_cast<const VulkanObjects::VulkanTextureInstance *const *>(texture_instance);
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

void graphics_vulkan::VulkanGraphicsInstance::CreateMipMaps(
    const GraphicsTypes::TextureInstance *texture_instance) const
{
  if(!texture_instance) return;
  auto vulkan_texture_instance = reinterpret_cast<const VulkanObjects::VulkanTextureInstance *>(texture_instance);

  VulkanObjects::VulkanSingleCommandBuffer cmd(Device.get(), CommandPool.get());
  cmd.Begin();

  vulkan_texture_instance->CreateMipMaps(cmd.Get());

  cmd.End();
}

bool graphics_vulkan::VulkanGraphicsInstance::StartDraw(const int width, const int height)
{
  Device->BeginFrame();

  DynamicUniformBuffersCommonOffset   = 0;
  DynamicUniformBuffersSpecificOffset = 0;
  DynamicVertexBuffersOffset          = 0;

  if(!SwapChain->BeginFrame(CommandPool.get(), width, height)) return false;

  DescriptorPool->BeginFrame();

  vkResetCommandBuffer(CommandBuffers[Device->GetCurrentFrame()], 0);

  const auto buffer_begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::REUSE);
  VulkanHelpers::VK_CHECK_RESULT(
      vkBeginCommandBuffer(CommandBuffers[Device->GetCurrentFrame()], &buffer_begin_info),
      __LINE__,
      __FILE__);

  return true;
}

void graphics_vulkan::VulkanGraphicsInstance::SetState(const GraphicsTypes::RenderState &state) const
{
  State.RenderState = state;
}

int graphics_vulkan::VulkanGraphicsInstance::AcquireFrameIndex() const { return Device->GetCurrentFrame(); }

void graphics_vulkan::VulkanGraphicsInstance::SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const
{
  std::vector<unsigned char> ubo_data;
  ubo_data.resize(ShaderInfo::GetCommonUniformSize());
  ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data.data(), data);

  CommonUniformBufferBindings.emplace_back();
  CommonUniformBufferBindings.back().swap(ubo_data);
  CommandsRecorded.emplace_back(CommandType::COMMON_UNIFORM_UPDATE, CommonUniformBufferBindings.size() - 1);
}

void graphics_vulkan::VulkanGraphicsInstance::SetColorState(const bool state) const { State.Color = state; }

void graphics_vulkan::VulkanGraphicsInstance::BindShader(const GraphicsTypes::ShaderInstance *shader_instance) const
{
  BoundShaders.emplace_back(reinterpret_cast<const VulkanObjects::VulkanShaderInstance *>(shader_instance));
  CommandsRecorded.emplace_back(CommandType::SHADER_BIND, BoundShaders.size() - 1);
  State.Shader = reinterpret_cast<const VulkanObjects::VulkanShaderInstance *>(shader_instance);
}

void graphics_vulkan::VulkanGraphicsInstance::BindBufferDynamic(
    const std::vector<unsigned char> &data,
    const GraphicsTypes::UBOBindPoint bind_point) const
{
  switch(bind_point)
  {
  case GraphicsTypes::UBOBindPoint::Common:
    {
      CommonUniformBufferBindings.emplace_back(data);
      CommandsRecorded.emplace_back(CommandType::COMMON_UNIFORM_UPDATE, CommonUniformBufferBindings.size() - 1);
      break;
    }
  case GraphicsTypes::UBOBindPoint::Specific:
    {
      CustomUniformBufferBindings.emplace_back(data);
      CommandsRecorded.emplace_back(CommandType::CUSTOM_UNIFORM_UPDATE, CustomUniformBufferBindings.size() - 1);
      break;
    }
  }
}

void graphics_vulkan::VulkanGraphicsInstance::BindTextures(
    const GraphicsTypes::TextureInstance *const *texture_instance,
    const int                                    count,
    const int                                    set) const
{
  if(!State.Shader)
  {
    Log::Warn << "Trying to bind textures while no shader is bound, ignoring!" << Log::End;
    return;
  }
  if(!texture_instance)
  {
    Log::Error << "Trying to bind invalid texture." << Log::End;
    return;
  }
  auto *const *vulkan_texture_instances =
      reinterpret_cast<const VulkanObjects::VulkanTextureInstance *const *>(texture_instance);

  VkDescriptorSet descriptor_set =
      DescriptorPool->AllocateDescriptorSet(State.Shader->GetDescriptorSetLayoutTexturesSpecial());

  for(int i = 0; i < count; i++)
  {
    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = vulkan_texture_instances[i]->GetImage()->GetLayout();
    image_info.imageView   = vulkan_texture_instances[i]->GetView()->Get();
    image_info.sampler     = vulkan_texture_instances[i]->GetSampler()->Get();

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet           = descriptor_set;
    descriptor_write.dstBinding       = i;
    descriptor_write.dstArrayElement  = 0;
    descriptor_write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.descriptorCount  = 1;
    descriptor_write.pBufferInfo      = nullptr;
    descriptor_write.pImageInfo       = &image_info;
    descriptor_write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(Device->GetDevice(), 1, &descriptor_write, 0, nullptr);
  }

  BoundTextures.emplace_back(vulkan_texture_instances, descriptor_set, set);
  CommandsRecorded.emplace_back(CommandType::TEXTURE_BIND, BoundTextures.size() - 1);
}

void graphics_vulkan::VulkanGraphicsInstance::BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) const
{
  if(!buffer_instance)
  {
    Log::Error << "Trying to bind invalid buffer." << Log::End;
    return;
  }
  const auto *vulkan_buffer_instance = reinterpret_cast<VulkanObjects::VulkanBufferInstance *>(buffer_instance);

  BoundBuffers.emplace_back(vulkan_buffer_instance);
  CommandsRecorded.emplace_back(CommandType::VERTEX_BIND, BoundBuffers.size() - 1);
}

void graphics_vulkan::VulkanGraphicsInstance::DrawIndexed(
    const size_t                         start,
    const size_t                         count,
    const GraphicsTypes::BufferInstance *buffer_instance,
    const GraphicsTypes::PrimitiveType   prim_type) const
{
  State.RenderState.DrawPrimitiveType = prim_type;

  if(buffer_instance)
  {
    const auto *vulkan_buffer_instance = reinterpret_cast<const VulkanObjects::VulkanBufferInstance *>(buffer_instance);

    BoundBuffers.emplace_back(vulkan_buffer_instance);
    CommandsRecorded.emplace_back(CommandType::INDEX_BIND, BoundBuffers.size() - 1);

    DrawCalls.emplace_back(State.Shader->GetPipelineForState(State), count, start);
    CommandsRecorded.emplace_back(CommandType::DRAW_INDEXED, DrawCalls.size() - 1);
  }
  else {
    DrawCalls.emplace_back(State.Shader->GetPipelineForState(State), count, start);
    CommandsRecorded.emplace_back(CommandType::DRAW, DrawCalls.size() - 1);
  }
}

void graphics_vulkan::VulkanGraphicsInstance::DrawDynamic(
    const size_t                       count,
    const size_t                       type_size,
    const void                        *data,
    const GraphicsTypes::PrimitiveType prim_type) const
{
  State.RenderState.DrawPrimitiveType = prim_type;

  auto &entry = DynamicDrawCalls.emplace_back(State.Shader->GetPipelineForState(State), count);

  entry.Data.resize(count * type_size);
  std::memcpy(entry.Data.data(), data, entry.Data.size());

  CommandsRecorded.emplace_back(CommandType::DRAW_DYNAMIC, DynamicDrawCalls.size() - 1);
}

void graphics_vulkan::VulkanGraphicsInstance::BindRenderBuffer(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance)
  {
    Log::Error << "failed to bind invalid render buffer!" << Log::End;
    return;
  }
  auto vulkan_render_buffer_instance =
      reinterpret_cast<VulkanObjects::VulkanFrameBufferInstance *>(render_buffer_instance);

  if(!vulkan_render_buffer_instance->IsInUse()) vulkan_render_buffer_instance->SetInUse(true);

  State.RenderPass = vulkan_render_buffer_instance->GetRenderPass();
  State.Samples    = static_cast<int>(vulkan_render_buffer_instance->GetInfo().Samples);
}

void graphics_vulkan::VulkanGraphicsInstance::EndRenderBuffer(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance)
{
  if(!render_buffer_instance)
  {
    Log::Error << "failed to unbind invalid render buffer!" << Log::End;
    return;
  }
  const auto vulkan_render_buffer_instance =
      reinterpret_cast<VulkanObjects::VulkanFrameBufferInstance *>(render_buffer_instance);

  std::vector<VkClearValue> clear_values;
  if(State.Color)
  {
    clear_values.emplace_back(
        VkClearValue{
            .color = {
                {State.RenderState.ClearColor.r,
                 State.RenderState.ClearColor.g,
                 State.RenderState.ClearColor.b,
                 State.RenderState.ClearColor.a}}});
    if(State.Samples > 1)
    {
      clear_values.emplace_back(
          VkClearValue{
              .color = {
                  {State.RenderState.ClearColor.r,
                   State.RenderState.ClearColor.g,
                   State.RenderState.ClearColor.b,
                   State.RenderState.ClearColor.a}}});
    }
  }
  clear_values.emplace_back(VkClearValue{.depthStencil = {State.RenderState.ClearDepth, 0}});

  vulkan_render_buffer_instance->GetTexture()->GetImage()->SetLayout(
      CommandBuffers[Device->GetCurrentFrame()],
      State.Color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  const VkExtent2D extent_2d = {
      vulkan_render_buffer_instance->GetTexture()->GetInfo().Width,
      vulkan_render_buffer_instance->GetTexture()->GetInfo().Height};

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = vulkan_render_buffer_instance->GetRenderPass()->Get();
  render_pass_info.framebuffer       = vulkan_render_buffer_instance->Get(Device->GetCurrentFrame());
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = extent_2d;
  render_pass_info.clearValueCount   = static_cast<uint32_t>(clear_values.size());
  render_pass_info.pClearValues      = clear_values.data();

  vkCmdBeginRenderPass(CommandBuffers[Device->GetCurrentFrame()], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(extent_2d.width);
  viewport.height   = static_cast<float>(extent_2d.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &viewport);
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent_2d;
  vkCmdSetScissor(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &scissor);

  EndRenderPass();

  if(vulkan_render_buffer_instance->IsInUse())
  {
    vulkan_render_buffer_instance->SetInUse(false);

    const bool color =
        vulkan_render_buffer_instance->GetTexture()->GetImage()->GetFormat() != GraphicsTypes::ImageFormat::DEPTH;
    const VkImageAspectFlags aspect = color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    const auto               old_layout =
        color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    constexpr auto new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkImageMemoryBarrier2 image_barrier{};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    image_barrier.srcStageMask =
        color ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    image_barrier.srcAccessMask =
        color ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    image_barrier.dstStageMask =
        color ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
              : VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    image_barrier.dstAccessMask = color ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                                        : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    image_barrier.oldLayout     = old_layout;
    image_barrier.newLayout     = new_layout;
    image_barrier.image         = vulkan_render_buffer_instance->GetTexture()->GetImage()->Get();
    image_barrier.subresourceRange = {
        .aspectMask     = aspect,
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    VkDependencyInfo dep_info{};
    dep_info.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &image_barrier;

    vkCmdPipelineBarrier2(CommandBuffers[Device->GetCurrentFrame()], &dep_info);
    vulkan_render_buffer_instance->GetTexture()->GetImage()->ExternalSetLayout(new_layout);
  }
}

void graphics_vulkan::VulkanGraphicsInstance::StartMainRenderPass()
{
  std::vector<VkClearValue> clear_values(3);
  clear_values[0].color = {
      {State.RenderState.ClearColor.r,
       State.RenderState.ClearColor.g,
       State.RenderState.ClearColor.b,
       State.RenderState.ClearColor.a}};
  clear_values[1].color = {
      {State.RenderState.ClearColor.r,
       State.RenderState.ClearColor.g,
       State.RenderState.ClearColor.b,
       State.RenderState.ClearColor.a}};
  clear_values[2].depthStencil = {State.RenderState.ClearDepth, 0};

  const auto *current_frame_buffer = SwapChain->GetCurrentFrameBuffer();

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass  = current_frame_buffer->GetRenderPass()->Get();
  render_pass_info.framebuffer = current_frame_buffer->Get();
  ;
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent =
      VkExtent2D(current_frame_buffer->GetInfo().Width, current_frame_buffer->GetInfo().Height);
  render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
  render_pass_info.pClearValues    = clear_values.data();

  vkCmdBeginRenderPass(CommandBuffers[Device->GetCurrentFrame()], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(current_frame_buffer->GetInfo().Width);
  viewport.height   = static_cast<float>(current_frame_buffer->GetInfo().Height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &viewport);
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = VkExtent2D(current_frame_buffer->GetInfo().Width, current_frame_buffer->GetInfo().Height);
  vkCmdSetScissor(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &scissor);

  State.RenderPass = SwapChain->GetCurrentFrameBuffer()->GetRenderPass();
}

void graphics_vulkan::VulkanGraphicsInstance::EndRenderPass()
{
  SubmitDrawCommands(this);

  vkCmdEndRenderPass(CommandBuffers[Device->GetCurrentFrame()]);
}

void graphics_vulkan::VulkanGraphicsInstance::EndDraw(const int width, const int height)
{
  VulkanHelpers::VK_CHECK_RESULT(vkEndCommandBuffer(CommandBuffers[Device->GetCurrentFrame()]), __LINE__, __FILE__);

  const VkSemaphore              wait_semaphores[]   = {Device->GetImageAvailable()};
  constexpr VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const VkSemaphore              signal_semaphores[] = {SwapChain->GetRenderFinished()};
  VkSubmitInfo                   submit_info{};
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount   = 1;
  submit_info.pWaitSemaphores      = wait_semaphores;
  submit_info.pWaitDstStageMask    = wait_stages;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = &CommandBuffers[Device->GetCurrentFrame()];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_semaphores;
  VulkanHelpers::VK_CHECK_RESULT(
      vkQueueSubmit(Device->GetGraphicsQueue(), 1, &submit_info, Device->GetFence()),
      __LINE__,
      __FILE__);

  SwapChain->EndFrame(CommandPool.get(), width, height);
}

void graphics_vulkan::VulkanGraphicsInstance::Wait() { vkDeviceWaitIdle(Device->GetDevice()); }

graphics_vulkan::VulkanGraphicsInstance::~VulkanGraphicsInstance()
{
  for(auto &buffer : DynamicUniformBuffersCommon)
    buffer.reset();
  for(auto &buffer : DynamicUniformBuffersSpecific)
    buffer.reset();
  for(auto &buffer : DynamicVertexBuffers)
    buffer.reset();
  Wait();

  vkFreeCommandBuffers(
      Device->GetDevice(),
      CommandPool->Get(),
      VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT,
      CommandBuffers.data());

  DescriptorPool.reset();
  SwapChain.reset();
  CommandPool.reset();
  Device.reset();
}
