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
#ifndef GRAPHICS_VULKAN_H
#define GRAPHICS_VULKAN_H

#include <memory>
#include <vector>

#include "VulkanCommandBuffer.h"
#include "VulkanDeviceInstance.h"
#include "VulkanPipelineState.h"
#include "graphics/ShaderInfo.h"
#include "graphics/graphics_toplevel_defines.h"


namespace VulkanObjects
{
  class VulkanCommandPool;
  class VulkanDescriptorPool;
  class VulkanSwapChainInstance;
  class VulkanBufferInstance;
  class VulkanShaderInstance;
  class VulkanTextureInstance;
} // namespace VulkanObjects
namespace File
{
  struct ShaderString;
}
class ShaderInfo;
class Window;


namespace graphics_vulkan
{
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
    const VulkanObjects::VulkanTextureInstance *const*TextureInstances;
    VkDescriptorSet                                   DescriptorSet;
    int                                               Set;
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
    std::vector<unsigned char> Data;
  };

  class VulkanGraphicsInstance final : public GraphicsTypes::GraphicsInstance
  {
    std::unique_ptr<VulkanObjects::VulkanDeviceInstance>    Device;
    std::unique_ptr<VulkanObjects::VulkanCommandPool>       CommandPool;
    std::unique_ptr<VulkanObjects::VulkanDescriptorPool>    DescriptorPool;
    std::unique_ptr<VulkanObjects::VulkanSwapChainInstance> SwapChain;

    // From here on this stuff should really be packed into some sort of command buffer info struct

    mutable std::array<VkCommandBuffer, VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT>                                      CommandBuffers{};
    mutable std::array<std::unique_ptr<VulkanObjects::VulkanBufferInstance>, VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> DynamicUniformBuffersCommon;
    mutable size_t                                                                                                                      DynamicUniformBuffersCommonOffset = 0;
    mutable std::array<std::unique_ptr<VulkanObjects::VulkanBufferInstance>, VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> DynamicUniformBuffersSpecific;
    mutable size_t                                                                                                                      DynamicUniformBuffersSpecificOffset = 0;
    mutable std::array<std::unique_ptr<VulkanObjects::VulkanBufferInstance>, VulkanObjects::VulkanDeviceInstance::MAX_FRAMES_IN_FLIGHT> DynamicVertexBuffers;
    mutable size_t                                                                                                                      DynamicVertexBuffersOffset = 0;

    mutable std::vector<const VulkanObjects::VulkanShaderInstance *> BoundShaders;
    mutable std::vector<const VulkanObjects::VulkanBufferInstance *> BoundBuffers;
    mutable std::vector<TextureBinding>                              BoundTextures;
    mutable std::vector<DrawCall>                                    DrawCalls;
    mutable std::vector<DynamicDrawCall>                             DynamicDrawCalls;
    mutable std::vector<std::vector<unsigned char>>                  CommonUniformBufferBindings;
    mutable std::vector<std::vector<unsigned char>>                  CustomUniformBufferBindings;

    mutable std::vector<std::pair<CommandType, size_t>> CommandsRecorded;

    mutable ShaderInfo::CommonUniformBufferData CommonData{};
    mutable bool                                CommonDataChanged = false;
    mutable VulkanObjects::VulkanPipelineState  State;

    friend void BindBuffers(const VulkanGraphicsInstance *instance, const VulkanObjects::VulkanShaderInstance *current_shader_instance, const GraphicsTypes::BufferInstance *const *buffer_instance, int count, GraphicsTypes::UBOBindPoint bind_point, int set, size_t offset, size_t size);
    friend void SubmitDrawCommands(const VulkanGraphicsInstance *graphics_instance);

  public:
    explicit VulkanGraphicsInstance(int width, int height);

    void CreateShader(std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance, const ShaderInfo &shader_info, const std::vector<File::ShaderString> &shader_code, std::string_view name) const override;

    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance, GraphicsTypes::BufferType type, size_t buffer_size, std::string_view name) const override;
    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance, GraphicsTypes::BufferType type, size_t buffer_size, const void *data, std::string_view name) const override;
    void MapBuffer(GraphicsTypes::BufferInstance *buffer_instance, const void *map_memory) const override;
    // Copies lhs into rhs.
    void CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs, GraphicsTypes::BufferInstance *buffer_instance_lhs) const override;

    void CreateTexture(std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance, const GraphicsTypes::TextureInfo &texture_info, const void *in_data, std::string_view name) const override;

    void CreateRenderBuffer(std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance, const GraphicsTypes::FrameBufferInfo &create_info, std::string_view name) const override;

    const GraphicsTypes::TextureInstance *GetRenderBufferTexture(GraphicsTypes::RenderBufferInstance *render_buffer_instance) const override;

    void DispatchCompute(const GraphicsTypes::ShaderInstance *shader, const GraphicsTypes::TextureInstance *const *texture_instance, int count, int num_x, int num_y, int num_z) const override;
    void CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const override;

    void Resize(int width, int height) override {};

    bool StartDraw(int width, int height) override;

    void SetState(const GraphicsTypes::RenderState &state) const override;

    [[nodiscard]] int AcquireFrameIndex() const override;

    void SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const override;
    void SetColorState(bool state) const override;

    void BindShader(const GraphicsTypes::ShaderInstance *shader_instance) const override;
    void BindBufferDynamic(const std::vector<unsigned char> &data, GraphicsTypes::UBOBindPoint bind_point) const override;

    void BindTextures(const GraphicsTypes::TextureInstance *const*texture_instance, int count, int set) const override;
    void BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) const override;
    void DrawIndexed(size_t start, size_t count, const GraphicsTypes::BufferInstance *buffer_instance, GraphicsTypes::PrimitiveType prim_type) const override;
    void DrawDynamic(size_t count, size_t type_size, const void *data, GraphicsTypes::PrimitiveType prim_type) const override;

    void BindRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) const override;
    void EndRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) override;

    void StartMainRenderPass() override;
    void EndRenderPass() override;

    void EndDraw(int width, int height) override;

    void Wait() override;
    ~VulkanGraphicsInstance() override;
  };
}; // namespace graphics_vulkan


#endif // GRAPHICS_VULKAN_H
