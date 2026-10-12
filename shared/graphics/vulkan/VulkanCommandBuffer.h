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
#ifndef VULKANCOMMANDBUFFER_H
#define VULKANCOMMANDBUFFER_H

#include "VulkanBufferInstance.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDeviceInstance.h"
#include "VulkanPipelineState.h"
#include "VulkanSwapChainInstance.h"
#include "graphics/graphics_toplevel_defines.h"
#include "graphics_vulkan.h"

#include <vulkan/vulkan_core.h>


namespace VulkanObjects
{
  class VulkanCommandPool;
  class VulkanDeviceInstance;

  class VulkanSingleCommandBuffer
  {
    VkCommandBuffer cmd = nullptr;

    const VulkanDeviceInstance *Device;
    const VulkanCommandPool    *CommandPool;

    uint8_t state = 0;

  public:
    VulkanSingleCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool);

    void Begin();

    void End();

    ~VulkanSingleCommandBuffer();

    VulkanSingleCommandBuffer(const VulkanSingleCommandBuffer &other)                = delete;
    VulkanSingleCommandBuffer(VulkanSingleCommandBuffer &&other) noexcept            = delete;
    VulkanSingleCommandBuffer &operator=(const VulkanSingleCommandBuffer &other)     = delete;
    VulkanSingleCommandBuffer &operator=(VulkanSingleCommandBuffer &&other) noexcept = delete;

    [[nodiscard]] VkCommandBuffer Get() const { return cmd; }
  };

  class VulkanSingleComputeCommandBuffer
  {
    VkCommandBuffer cmd = nullptr;

    const VulkanDeviceInstance *Device;
    const VulkanCommandPool    *CommandPool;

    uint8_t state = 0;

  public:
    VulkanSingleComputeCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool);

    void Begin();

    void End();

    ~VulkanSingleComputeCommandBuffer();

    VulkanSingleComputeCommandBuffer(const VulkanSingleComputeCommandBuffer &other)                = delete;
    VulkanSingleComputeCommandBuffer(VulkanSingleComputeCommandBuffer &&other) noexcept            = delete;
    VulkanSingleComputeCommandBuffer &operator=(const VulkanSingleComputeCommandBuffer &other)     = delete;
    VulkanSingleComputeCommandBuffer &operator=(VulkanSingleComputeCommandBuffer &&other) noexcept = delete;

    [[nodiscard]] VkCommandBuffer Get() const { return cmd; }
  };


  enum class CommandType
  {
    SHADER_BIND,
    INDEX_BIND,
    VERTEX_BIND,
    TEXTURE_BIND,
    COMMON_UNIFORM_UPDATE,
    CUSTOM_UNIFORM_UPDATE,
    DRAW,
    DRAW_INDEXED,
    DRAW_DYNAMIC
  };

  struct TextureBinding
  {
    const class VulkanTextureInstance *const *TextureInstances;
    VkDescriptorSet                           DescriptorSet;
    int                                       Set;
  };

  struct DrawCall
  {
    VkPipeline Pipeline{};
    size_t     Count = 0;
    size_t     Start = 0;
  };

  struct DynamicDrawCall
  {
    VkPipeline Pipeline;
    size_t     Count;
  };

  class VulkanCommandBuffer final : public GraphicsTypes::CommandBufferInstance
  {
  public:
    static constexpr size_t DYNAMIC_BUFFER_SIZE = 128 * 1024 * 1024;

    enum class SubmitStateInfo
    {
      PENDING,
      OWNED,
      EXTERNAL,
      NONE
    };

    enum class BufferBindPoint
    {
      UBO_GLOB,
      UBO_SPEC,
      SSO
    };

  private:
    std::array<VkFence, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> InFlightFences{ nullptr };

    std::array<VkCommandBuffer, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> CommandBuffers{};

    struct DynamicBuffer
    {
      std::array<std::unique_ptr<VulkanBufferInstance>, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> Buffers;

      size_t                                                         Offset        = 0;
      size_t                                                         AlignmentMask = 0;
      std::array<void *, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> Mapped;

      asl::int64 AddData(asl::uint8 index, const void *data, size_t size);
    };

    DynamicBuffer DynamicBufferCommon;
    DynamicBuffer DynamicBufferCustom;
    DynamicBuffer DynamicBufferVertex;

    VulkanPipelineState                 State;

    VkPipeline CurrentPipeline = nullptr;

    const VulkanDeviceInstance *Device;
    const VulkanCommandPool    *CommandPool;
    VulkanDescriptorPool        DescriptorPool;

    std::array<SubmitStateInfo, VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> SubmitState;
    ASLM_DEBUG_ONLY_STATEMENT(bool Recording = false;)

    void BindBuffers(
        const VulkanShaderInstance                 *current_shader_instance,
        const GraphicsTypes::BufferInstance *const *buffer_instance,
        int                                         count,
        BufferBindPoint                             bind_point,
        int                                         set,
        size_t                                      offset,
        size_t                                      size) const;

  public:
    VulkanCommandBuffer(const VulkanDeviceInstance *device, const VulkanCommandPool *command_pool);
    ~VulkanCommandBuffer() override;

    void Begin() override;

    void SetState(const GraphicsTypes::RenderState &state) override;
    void SetCommonUBO(std::vector<unsigned char> &ubo_data) override;
    void BindBufferDynamic(const std::vector<unsigned char> &data, GraphicsTypes::UBOBindPoint bind_point) override;
    void BindBufferSSO(const GraphicsTypes::BufferInstance *buffer_instance, int binding) override;
    void SetColorState(bool color) override;
    void BindShader(const GraphicsTypes::ShaderInstance *shader_instance) override;
    void BindTextures(const GraphicsTypes::TextureInstance *const *texture_instance, int count, int set) override;
    void BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) override;
    void BindInstanceBuffer(GraphicsTypes::BufferInstance *buffer_instance) override;

    void DrawIndexed(
        size_t                               start,
        size_t                               count,
        const GraphicsTypes::BufferInstance *buffer_instance,
        GraphicsTypes::PrimitiveType         prim_type) override;
    void DrawIndexedInstanced(
        size_t                               instance_count,
        size_t                               start,
        size_t                               count,
        const GraphicsTypes::BufferInstance *buffer_instance,
        GraphicsTypes::PrimitiveType         prim_type) override;
    void DrawDynamic(size_t count, size_t type_size, const void *data, GraphicsTypes::PrimitiveType prim_type) override;

    void BindRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) override;
    void EndRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) override;

    void StartMainRenderPass(const VulkanSwapChainInstance *swap_chain);
    void EndRenderPass() override;

    void End() override;

    void Submit() override;

    void SubmitForPresent(VkSemaphore render_finished);

    void Wait() override;
  };
} // namespace VulkanObjects


#endif // VULKANCOMMANDBUFFER_H
