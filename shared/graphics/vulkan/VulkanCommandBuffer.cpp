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
#include "VulkanCommandBuffer.h"

#include <cassert>

#include "VulkanBootstrap.h"
#include "VulkanBufferInstance.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDeviceInstance.h"
#include "VulkanFrameBufferInstance.h"
#include "VulkanHelpers.h"
#include "VulkanShaderInstance.h"
#include "VulkanSwapChainInstance.h"
#include "VulkanTexture.h"
#include "system/Log.h"

#include <cstring>


VulkanObjects::VulkanSingleCommandBuffer::VulkanSingleCommandBuffer(
    const VulkanDeviceInstance *device,
    const VulkanCommandPool    *command_pool) :
  Device(device), CommandPool(command_pool)
{
  const auto alloc_info = VulkanBootstrap::GetCommandBufferAllocate(CommandPool->Get(), 1);
  VulkanHelpers::VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->GetDevice(), &alloc_info, &cmd), __LINE__, __FILE__);
}

void VulkanObjects::VulkanSingleCommandBuffer::Begin()
{
  assert(state == 0 && "invalid second usage of begin on single command buffer");
  const auto begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::ONE_TIME);
  VulkanHelpers::VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_info), __LINE__, __FILE__);

  state = 1;
}

void VulkanObjects::VulkanSingleCommandBuffer::End()
{
  assert(state == 1 && "trying to unstarted or already finished command buffer");
  vkEndCommandBuffer(cmd);
  const std::vector buffers     = { cmd };
  const auto        submit_info = VulkanBootstrap::GetSubmit(buffers, {}, {}, {});
  vkQueueSubmit(Device->GetGraphicsQueue(), 1, &submit_info, nullptr);
  vkQueueWaitIdle(Device->GetGraphicsQueue());
  state = 2;
}

VulkanObjects::VulkanSingleCommandBuffer::~VulkanSingleCommandBuffer()
{
  assert(state != 1 && "deleting command buffer that is still in use");
#ifndef NDEBUG
  if(state == 0)
    Log::Warn << "deleting command buffer that was never used!" << Log::End;
#endif
  if(cmd)
    vkFreeCommandBuffers(Device->GetDevice(), CommandPool->Get(), 1, &cmd);
}

VulkanObjects::VulkanSingleComputeCommandBuffer::VulkanSingleComputeCommandBuffer(
    const VulkanDeviceInstance *device,
    const VulkanCommandPool    *command_pool) :
  Device(device), CommandPool(command_pool)
{
  const auto alloc_info = VulkanBootstrap::GetCommandBufferAllocate(CommandPool->Get(), 1);
  VulkanHelpers::VK_CHECK_RESULT(vkAllocateCommandBuffers(Device->GetDevice(), &alloc_info, &cmd), __LINE__, __FILE__);
}

void VulkanObjects::VulkanSingleComputeCommandBuffer::Begin()
{
  assert(state == 0 && "invalid second usage of begin on single command buffer");
  const auto begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::ONE_TIME);
  VulkanHelpers::VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin_info), __LINE__, __FILE__);
  state = 1;
}

void VulkanObjects::VulkanSingleComputeCommandBuffer::End()
{
  assert(state == 1 && "trying to unstarted or already finished command buffer");
  vkEndCommandBuffer(cmd);
  const std::vector buffers     = { cmd };
  const auto        submit_info = VulkanBootstrap::GetSubmit(buffers, {}, {}, {});
  vkQueueSubmit(Device->GetComputeQueue(), 1, &submit_info, nullptr);
  vkQueueWaitIdle(Device->GetComputeQueue());
  state = 2;
}

VulkanObjects::VulkanSingleComputeCommandBuffer::~VulkanSingleComputeCommandBuffer()
{
  assert(state % 2 == 0 && "deleting command buffer that is still in use");
#ifndef NDEBUG
  if(state == 0)
    Log::Warn << "deleting command buffer that was never used!" << Log::End;
#endif
  if(cmd)
    vkFreeCommandBuffers(Device->GetDevice(), CommandPool->Get(), 1, &cmd);
}

asl::int64 VulkanObjects::VulkanCommandBuffer::DynamicBuffer::AddData(const asl::uint8 index, const void *data, const size_t size)
{
  Offset = (Offset + AlignmentMask) & ~(AlignmentMask);

  if(Offset + size > DYNAMIC_BUFFER_SIZE)
  {
    Log::Error << "Dynamic buffer bindings exceeded allocated size, skipping binding" << Log::End;
    return -1;
  }
  std::memcpy(static_cast<unsigned char *>(Mapped[index]) + Offset, data, size);

  const size_t offset = Offset;

  Offset += size;
  Offset  = (Offset + AlignmentMask) & ~(AlignmentMask);

  return static_cast<asl::int64>(offset);
}

VulkanObjects::VulkanCommandBuffer::VulkanCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool) :
  Device(device), CommandPool(command_pool), DescriptorPool(Device)
{
  const auto buffer_allocate_info =
      VulkanBootstrap::GetCommandBufferAllocate(CommandPool->Get(), VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT);
  VulkanHelpers::VK_CHECK_RESULT(
      vkAllocateCommandBuffers(Device->GetDevice(), &buffer_allocate_info, CommandBuffers.data()),
      __LINE__,
      __FILE__);

  for(size_t i = 0; i < VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT; i++)
  {
    DynamicBufferCommon.Buffers[i] = std::make_unique<VulkanBufferInstance>(
        Device,
        GraphicsTypes::BufferType::UNIFORM_DYNAMIC,
        DYNAMIC_BUFFER_SIZE,
        "dynamic_ubo_common");
    DynamicBufferCommon.Buffers[i]->Map(&DynamicBufferCommon.Mapped[i]);
    DynamicBufferCommon.AlignmentMask = Device->GetProperties().limits.minUniformBufferOffsetAlignment - 1;

    DynamicBufferCustom.Buffers[i] = std::make_unique<VulkanBufferInstance>(
        Device,
        GraphicsTypes::BufferType::UNIFORM_DYNAMIC,
        DYNAMIC_BUFFER_SIZE,
        "dynamic_ubo_custom");
    DynamicBufferCustom.Buffers[i]->Map(&DynamicBufferCustom.Mapped[i]);
    DynamicBufferCustom.AlignmentMask = Device->GetProperties().limits.minUniformBufferOffsetAlignment - 1;

    DynamicBufferVertex.Buffers[i] = std::make_unique<VulkanBufferInstance>(
        Device,
        GraphicsTypes::BufferType::VERTEX_DYNAMIC,
        DYNAMIC_BUFFER_SIZE,
        "dynamic_buf_vertex");
    DynamicBufferVertex.Buffers[i]->Map(&DynamicBufferVertex.Mapped[i]);
    DynamicBufferVertex.AlignmentMask = 16 - 1;
  }

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for(asl::uint32 i = 0; i < VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT; i++)
  {
    VulkanHelpers::VK_CHECK_RESULT(vkCreateFence(Device->GetDevice(), &fence_info, nullptr, &InFlightFences[i]), __LINE__, __FILE__);
    Device->NameObject(VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(InFlightFences[i]), "command_buffer_owned_fence");
  }
}

VulkanObjects::VulkanCommandBuffer::~VulkanCommandBuffer()
{
  for(const auto &fence : InFlightFences)
    vkDestroyFence(Device->GetDevice(), fence, nullptr);

  vkDeviceWaitIdle(Device->GetDevice());

  vkFreeCommandBuffers(Device->GetDevice(), CommandPool->Get(), VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT, CommandBuffers.data());
}

void VulkanObjects::VulkanCommandBuffer::Begin()
{
  ASLM_DEBUG_ONLY_STATEMENT(if(Recording) {
    Log::Error << "Can not begin already recording commandbuffer, ignoring." << Log::End;
    return;
  } Recording = true;);

  if(SubmitState[Device->GetCurrentFrame()] == SubmitStateInfo::OWNED)
  {
    Wait();
    vkResetFences(Device->GetDevice(), 1, &InFlightFences[Device->GetCurrentFrame()]);
  }
  SubmitState[Device->GetCurrentFrame()] = SubmitStateInfo::NONE;

  DescriptorPool.BeginFrame(Device->GetCurrentFrame());

  DynamicBufferCommon.Offset = 0;
  DynamicBufferCustom.Offset = 0;
  DynamicBufferVertex.Offset = 0;

  CurrentPipeline = nullptr;

  vkResetCommandBuffer(CommandBuffers[Device->GetCurrentFrame()], 0);

  const auto buffer_begin_info = VulkanBootstrap::GetCommandBufferBegin(VulkanTranslate::CommandBufferType::REUSE);
  VulkanHelpers::VK_CHECK_RESULT(vkBeginCommandBuffer(CommandBuffers[Device->GetCurrentFrame()], &buffer_begin_info), __LINE__, __FILE__);
}

#define ASLM_DEBUG_CHECK_RECORDING(name)                                                                                                   \
  ASLM_DEBUG_ONLY_STATEMENT(if(!Recording) {                                                                                               \
    Log::Error << "Can act on inactive command buffer, ignoring call to " << name << Log::End;                                             \
    return;                                                                                                                                \
  })

void VulkanObjects::VulkanCommandBuffer::SetState(const GraphicsTypes::RenderState &state)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.RenderState = state;
}

void VulkanObjects::VulkanCommandBuffer::SetCommonUBO(std::vector<unsigned char> &ubo_data)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  const asl::int64 offset = DynamicBufferCommon.AddData(Device->GetCurrentFrame(), ubo_data.data(), ubo_data.size());
  if(offset < 0)
  {
    Log::Error << "Dynamic common buffer bindings exceeded allocated size, skipping binding" << Log::End;
    return;
  }

  if(State.Shader)
  {
    const auto                          *buffer = DynamicBufferCommon.Buffers[Device->GetCurrentFrame()].get();
    const GraphicsTypes::BufferInstance *tmp    = buffer;
    BindBuffers(State.Shader, &tmp, 1, BufferBindPoint::UBO_GLOB, 0, offset, ubo_data.size());
  }
}

void VulkanObjects::VulkanCommandBuffer::BindBufferDynamic(
    const std::vector<unsigned char> &data,
    const GraphicsTypes::UBOBindPoint bind_point)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  switch(bind_point)
  {
    case GraphicsTypes::UBOBindPoint::Common:
    {
      const asl::int64 offset = DynamicBufferCommon.AddData(Device->GetCurrentFrame(), data.data(), data.size());
      if(offset < 0)
      {
        Log::Error << "Dynamic common buffer bindings exceeded allocated size, skipping binding" << Log::End;
        return;
      }

      if(State.Shader)
      {
        const auto                          *buffer = DynamicBufferCommon.Buffers[Device->GetCurrentFrame()].get();
        const GraphicsTypes::BufferInstance *tmp    = buffer;
        BindBuffers(State.Shader, &tmp, 1, BufferBindPoint::UBO_GLOB, 0, offset, data.size());
      }
      break;
    }
    case GraphicsTypes::UBOBindPoint::Specific:
    {
      const asl::int64 offset = DynamicBufferCustom.AddData(Device->GetCurrentFrame(), data.data(), data.size());
      if(offset < 0)
      {
        Log::Error << "Dynamic specific buffer bindings exceeded allocated size, skipping binding" << Log::End;
        return;
      }

      if(State.Shader)
      {
        const auto                          *buffer = DynamicBufferCustom.Buffers[Device->GetCurrentFrame()].get();
        const GraphicsTypes::BufferInstance *tmp    = buffer;
        BindBuffers(State.Shader, &tmp, 1, BufferBindPoint::UBO_SPEC, 1, offset, data.size());
      }
      break;
    }
  }
}

void VulkanObjects::VulkanCommandBuffer::BindBufferSSO(const GraphicsTypes::BufferInstance *buffer_instance, const int binding)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  if(!buffer_instance)
    return;
  const auto vk_buffer = reinterpret_cast<const VulkanBufferInstance *>(buffer_instance);
  BindBuffers(State.Shader, &buffer_instance, 1, BufferBindPoint::SSO, 3, binding, vk_buffer->GetSize());
  // TODO
}

void VulkanObjects::VulkanCommandBuffer::SetColorState(const bool color)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.Color = color;
}

void VulkanObjects::VulkanCommandBuffer::BindShader(const GraphicsTypes::ShaderInstance *shader_instance)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.Shader = reinterpret_cast<const VulkanShaderInstance *>(shader_instance);
}

void VulkanObjects::VulkanCommandBuffer::BindTextures(
    const GraphicsTypes::TextureInstance *const *texture_instance,
    const int                                    count,
    const int                                    set)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
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
  auto *const *vulkan_texture_instances = reinterpret_cast<const VulkanTextureInstance *const *>(texture_instance);

  VkDescriptorSet descriptor_set = DescriptorPool.AllocateDescriptorSet(State.Shader->GetDescriptorSetLayoutTexturesSpecial());

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

  vkCmdBindDescriptorSets(
      CommandBuffers[Device->GetCurrentFrame()],
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      State.Shader->GetPipelineLayout(),
      set,
      1,
      &descriptor_set,
      0,
      nullptr);
}

void VulkanObjects::VulkanCommandBuffer::BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  if(!buffer_instance)
  {
    Log::Error << "Trying to bind invalid buffer." << Log::End;
    return;
  }
  const auto *vulkan_buffer_instance = reinterpret_cast<VulkanBufferInstance *>(buffer_instance);

  const VkBuffer         vertexBuffers[] = { vulkan_buffer_instance->Get() };
  constexpr VkDeviceSize offsets[]       = { 0 };

  vkCmdBindVertexBuffers(CommandBuffers[Device->GetCurrentFrame()], 0, 1, vertexBuffers, offsets);
}

void VulkanObjects::VulkanCommandBuffer::BindInstanceBuffer(GraphicsTypes::BufferInstance *buffer_instance)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  if(!buffer_instance)
  {
    Log::Error << "Trying to bind invalid buffer." << Log::End;
    return;
  }
  const auto *vulkan_buffer_instance = reinterpret_cast<VulkanBufferInstance *>(buffer_instance);

  const VkBuffer         vertexBuffers[] = { vulkan_buffer_instance->Get() };
  constexpr VkDeviceSize offsets[]       = { 0 };

  vkCmdBindVertexBuffers(CommandBuffers[Device->GetCurrentFrame()], 1, 1, vertexBuffers, offsets);
}

void VulkanObjects::VulkanCommandBuffer::DrawIndexed(
    const size_t                         start,
    const size_t                         count,
    const GraphicsTypes::BufferInstance *buffer_instance,
    const GraphicsTypes::PrimitiveType   prim_type)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.RenderState.DrawPrimitiveType = prim_type;

  VkPipeline pipeline = State.Shader->GetPipelineForState(State);
  if(!pipeline)
  {
    Log::Error << "Error at pipeline creation, skipping draw call" << Log::End;
    return;
  }
  if(CurrentPipeline != pipeline)
  {
    vkCmdBindPipeline(CommandBuffers[Device->GetCurrentFrame()], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    CurrentPipeline = pipeline;
  }

  if(buffer_instance)
  {
    const auto *vulkan_buffer_instance = reinterpret_cast<const VulkanBufferInstance *>(buffer_instance);

    vkCmdBindIndexBuffer(CommandBuffers[Device->GetCurrentFrame()], vulkan_buffer_instance->Get(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(CommandBuffers[Device->GetCurrentFrame()], count, 1, start, 0, 0);
  }
  else
  {
    vkCmdDraw(CommandBuffers[Device->GetCurrentFrame()], count, 1, start, 0);
  }
}

void VulkanObjects::VulkanCommandBuffer::DrawIndexedInstanced(
    const size_t                         instance_count,
    const size_t                         start,
    const size_t                         count,
    const GraphicsTypes::BufferInstance *buffer_instance,
    const GraphicsTypes::PrimitiveType   prim_type)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.RenderState.DrawPrimitiveType = prim_type;

  VkPipeline pipeline = State.Shader->GetPipelineForState(State);
  if(!pipeline)
  {
    Log::Error << "Error at pipeline creation, skipping draw call" << Log::End;
    return;
  }
  if(CurrentPipeline != pipeline)
  {
    vkCmdBindPipeline(CommandBuffers[Device->GetCurrentFrame()], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    CurrentPipeline = pipeline;
  }

  if(buffer_instance)
  {
    const auto *vulkan_buffer_instance = reinterpret_cast<const VulkanBufferInstance *>(buffer_instance);

    vkCmdBindIndexBuffer(CommandBuffers[Device->GetCurrentFrame()], vulkan_buffer_instance->Get(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(CommandBuffers[Device->GetCurrentFrame()], count, instance_count, start, 0, 0);
  }
  else
  {
    vkCmdDraw(CommandBuffers[Device->GetCurrentFrame()], count, instance_count, start, 0);
  }
}

void VulkanObjects::VulkanCommandBuffer::DrawDynamic(
    const size_t                       count,
    const size_t                       type_size,
    const void                        *data,
    const GraphicsTypes::PrimitiveType prim_type)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  State.RenderState.DrawPrimitiveType = prim_type;

  const asl::int64 offset = DynamicBufferVertex.AddData(Device->GetCurrentFrame(), data, count * type_size);
  if(offset < 0)
  {
    Log::Error << "Dynamic vertex buffer bindings exceeded allocated size, skipping binding" << Log::End;
    return;
  }

  const VkBuffer     vertexBuffers[] = { DynamicBufferVertex.Buffers[Device->GetCurrentFrame()]->Get() };
  const VkDeviceSize offsets[]       = { static_cast<VkDeviceSize>(offset) };

  vkCmdBindVertexBuffers(CommandBuffers[Device->GetCurrentFrame()], 0, 1, vertexBuffers, offsets);

  VkPipeline pipeline = State.Shader->GetPipelineForState(State);
  if(!pipeline)
  {
    Log::Error << "Error at pipeline creation, skipping draw call" << Log::End;
    return;
  }
  if(CurrentPipeline != pipeline)
  {
    vkCmdBindPipeline(CommandBuffers[Device->GetCurrentFrame()], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    CurrentPipeline = pipeline;
  }
  vkCmdDraw(CommandBuffers[Device->GetCurrentFrame()], count, 1, 0, 0);
}

void VulkanObjects::VulkanCommandBuffer::BindRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  if(!render_buffer_instance)
  {
    Log::Error << "failed to bind invalid render buffer!" << Log::End;
    return;
  }
  auto *vulkan_render_buffer_instance = reinterpret_cast<VulkanFrameBufferInstance *>(render_buffer_instance);

  if(!vulkan_render_buffer_instance->IsInUse())
    vulkan_render_buffer_instance->SetInUse(true);

  State.RenderPass = vulkan_render_buffer_instance->GetRenderPass();
  State.Samples    = static_cast<int>(vulkan_render_buffer_instance->GetInfo().Samples);

  std::vector<VkClearValue> clear_values;
  if(State.Color)
  {
    clear_values.emplace_back(
        VkClearValue{ .color = { { State.RenderState.ClearColor.r,
                                   State.RenderState.ClearColor.g,
                                   State.RenderState.ClearColor.b,
                                   State.RenderState.ClearColor.a } } });
    if(State.Samples > 1)
    {
      clear_values.emplace_back(
          VkClearValue{ .color = { { State.RenderState.ClearColor.r,
                                     State.RenderState.ClearColor.g,
                                     State.RenderState.ClearColor.b,
                                     State.RenderState.ClearColor.a } } });
    }
  }
  clear_values.emplace_back(VkClearValue{ .depthStencil = { State.RenderState.ClearDepth, 0 } });

  vulkan_render_buffer_instance->GetTexture()->GetImage()->SetLayout(
      CommandBuffers[Device->GetCurrentFrame()],
      State.Color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  const VkExtent2D extent_2d = { vulkan_render_buffer_instance->GetTexture()->GetInfo().Width,
                                 vulkan_render_buffer_instance->GetTexture()->GetInfo().Height };

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = vulkan_render_buffer_instance->GetRenderPass()->Get();
  render_pass_info.framebuffer       = vulkan_render_buffer_instance->Get(Device->GetCurrentFrame());
  render_pass_info.renderArea.offset = { 0, 0 };
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
  scissor.offset = { 0, 0 };
  scissor.extent = extent_2d;
  vkCmdSetScissor(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &scissor);
}

void VulkanObjects::VulkanCommandBuffer::EndRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  if(!render_buffer_instance)
  {
    Log::Error << "failed to unbind invalid render buffer!" << Log::End;
    return;
  }
  const auto vulkan_render_buffer_instance = reinterpret_cast<VulkanFrameBufferInstance *>(render_buffer_instance);


  EndRenderPass();

  if(vulkan_render_buffer_instance->IsInUse())
  {
    vulkan_render_buffer_instance->SetInUse(false);

    const bool color = vulkan_render_buffer_instance->GetTexture()->GetImage()->GetFormat() != GraphicsTypes::ImageFormat::DEPTH;
    const VkImageAspectFlags aspect = color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    const auto     old_layout       = color ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    constexpr auto new_layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkImageMemoryBarrier2 image_barrier{};
    image_barrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    image_barrier.srcStageMask     = color ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    image_barrier.srcAccessMask    = color ? VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    image_barrier.dstStageMask     = color ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
                                           : VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    image_barrier.dstAccessMask    = color ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
                                           : VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    image_barrier.oldLayout        = old_layout;
    image_barrier.newLayout        = new_layout;
    image_barrier.image            = vulkan_render_buffer_instance->GetTexture()->GetImage()->Get();
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

void VulkanObjects::VulkanCommandBuffer::StartMainRenderPass(const VulkanSwapChainInstance *swap_chain)
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  std::vector<VkClearValue> clear_values(3);
  clear_values[0].color = {
    { State.RenderState.ClearColor.r, State.RenderState.ClearColor.g, State.RenderState.ClearColor.b, State.RenderState.ClearColor.a }
  };
  clear_values[1].color = {
    { State.RenderState.ClearColor.r, State.RenderState.ClearColor.g, State.RenderState.ClearColor.b, State.RenderState.ClearColor.a }
  };
  clear_values[2].depthStencil = { State.RenderState.ClearDepth, 0 };

  const auto *current_frame_buffer = swap_chain->GetCurrentFrameBuffer();

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass  = current_frame_buffer->GetRenderPass()->Get();
  render_pass_info.framebuffer = current_frame_buffer->Get();
  ;
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = VkExtent2D(current_frame_buffer->GetInfo().Width, current_frame_buffer->GetInfo().Height);
  render_pass_info.clearValueCount   = static_cast<uint32_t>(clear_values.size());
  render_pass_info.pClearValues      = clear_values.data();

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
  scissor.offset = { 0, 0 };
  scissor.extent = VkExtent2D(current_frame_buffer->GetInfo().Width, current_frame_buffer->GetInfo().Height);
  vkCmdSetScissor(CommandBuffers[Device->GetCurrentFrame()], 0, 1, &scissor);

  State.RenderPass = swap_chain->GetCurrentFrameBuffer()->GetRenderPass();
}

void VulkanObjects::VulkanCommandBuffer::EndRenderPass()
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  vkCmdEndRenderPass(CommandBuffers[Device->GetCurrentFrame()]);
}

void VulkanObjects::VulkanCommandBuffer::End()
{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  VulkanHelpers::VK_CHECK_RESULT(vkEndCommandBuffer(CommandBuffers[Device->GetCurrentFrame()]), __LINE__, __FILE__);

  ASLM_DEBUG_ONLY_STATEMENT(Recording = false;)

  SubmitState[Device->GetCurrentFrame()] = SubmitStateInfo::PENDING;
}

void VulkanObjects::VulkanCommandBuffer::Submit()
{
  VkSubmitInfo submit_info{};
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount   = 0;
  submit_info.pWaitSemaphores      = nullptr;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = &CommandBuffers[Device->GetCurrentFrame()];
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores    = nullptr;
  vkResetFences(Device->GetDevice(), 1, &InFlightFences[Device->GetCurrentFrame()]);
  VulkanHelpers::VK_CHECK_RESULT(
      vkQueueSubmit(Device->GetGraphicsQueue(), 1, &submit_info, InFlightFences[Device->GetCurrentFrame()]),
      __LINE__,
      __FILE__);
  SubmitState[Device->GetCurrentFrame()] = SubmitStateInfo::OWNED;
}

void VulkanObjects::VulkanCommandBuffer::SubmitForPresent(VkSemaphore render_finished)
{
  const VkSemaphore              wait_semaphores[]   = { Device->GetImageAvailable() };
  constexpr VkPipelineStageFlags wait_stages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  const VkSemaphore              signal_semaphores[] = { render_finished };
  VkSubmitInfo                   submit_info{};
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount   = 1;
  submit_info.pWaitSemaphores      = wait_semaphores;
  submit_info.pWaitDstStageMask    = wait_stages;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = &CommandBuffers[Device->GetCurrentFrame()];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_semaphores;
  VulkanHelpers::VK_CHECK_RESULT(vkQueueSubmit(Device->GetGraphicsQueue(), 1, &submit_info, Device->GetFence()), __LINE__, __FILE__);
  SubmitState[Device->GetCurrentFrame()] = SubmitStateInfo::EXTERNAL;
}

void VulkanObjects::VulkanCommandBuffer::Wait()
{
  if(SubmitState[Device->GetCurrentFrame()] == SubmitStateInfo::OWNED)
  {
    VulkanHelpers::VK_CHECK_RESULT(
        vkWaitForFences(Device->GetDevice(), 1, &InFlightFences[Device->GetCurrentFrame()], VK_TRUE, UINT64_MAX),
        __LINE__,
        __FILE__);
  }
  ASLM_DEBUG_ONLY_STATEMENT(
      else Log::Warn << "Trying to wait on externally submitted command buffer (likely used to present), waiting is impossible"
                     << Log::End;)
}

void VulkanObjects::VulkanCommandBuffer::BindBuffers(
    const VulkanShaderInstance                 *current_shader_instance,
    const GraphicsTypes::BufferInstance *const *buffer_instance,
    const int                                   count,
    const BufferBindPoint                       bind_point,
    const int                                   set,
    const size_t                                offset,
    const size_t                                size) const

{
  ASLM_DEBUG_CHECK_RECORDING(__func__)
  auto *const *vulkan_buffer_instances = reinterpret_cast<const VulkanBufferInstance *const *>(buffer_instance);

  VkDescriptorSet descriptor_set = nullptr;
  switch(bind_point)
  {
    case BufferBindPoint::UBO_GLOB:
      descriptor_set = DescriptorPool.AllocateDescriptorSet(current_shader_instance->GetDescriptorSetLayoutUBOCommon());
      break;
    case BufferBindPoint::UBO_SPEC:
      descriptor_set = DescriptorPool.AllocateDescriptorSet(current_shader_instance->GetDescriptorSetLayoutUBOSpecial());
      break;
    case BufferBindPoint::SSO:
      descriptor_set = DescriptorPool.AllocateDescriptorSet(current_shader_instance->GetDescriptorSetLayoutSSO());
      break;
  }

  for(int i = 0; i < count; i++)
  {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = vulkan_buffer_instances[i]->Get();
    buffer_info.offset = offset;
    buffer_info.range  = size ? size : vulkan_buffer_instances[i]->GetSize();

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet          = descriptor_set;
    descriptor_write.dstBinding      = i;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType =
        bind_point == BufferBindPoint::SSO ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount  = 1;
    descriptor_write.pBufferInfo      = &buffer_info;
    descriptor_write.pImageInfo       = nullptr;
    descriptor_write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(Device->GetDevice(), 1, &descriptor_write, 0, nullptr);
  }

  vkCmdBindDescriptorSets(
      CommandBuffers[Device->GetCurrentFrame()],
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      current_shader_instance->GetPipelineLayout(),
      set,
      1,
      &descriptor_set,
      0,
      nullptr);
}

#undef ASLM_DEBUG_CHECK_RECORDING
