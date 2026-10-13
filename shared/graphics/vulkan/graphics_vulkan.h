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
  class VulkanGraphicsInstance final : public GraphicsTypes::GraphicsInstance
  {
    std::unique_ptr<VulkanObjects::VulkanDeviceInstance>    Device;
    std::unique_ptr<VulkanObjects::VulkanCommandPool>       CommandPool;
    std::unique_ptr<VulkanObjects::VulkanDescriptorPool>    DescriptorPool;
    std::unique_ptr<VulkanObjects::VulkanSwapChainInstance> SwapChain;

    mutable std::unique_ptr<VulkanObjects::VulkanBufferInstance> SampleStagingBuffer;
    mutable size_t                                               SampleStagingSize = 0;

    friend void BindBuffers(const VulkanGraphicsInstance               *instance,
                            const VulkanObjects::VulkanShaderInstance  *current_shader_instance,
                            const GraphicsTypes::BufferInstance *const *buffer_instance,
                            int                                         count,
                            GraphicsTypes::UBOBindPoint                 bind_point,
                            int                                         set,
                            size_t                                      offset,
                            size_t                                      size);
    friend void SubmitDrawCommands(const VulkanGraphicsInstance *graphics_instance);

  public:
    explicit VulkanGraphicsInstance(SDL_Window *window, int width, int height);

    void CreateShader(std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
                      const ShaderInfo                               &shader_info,
                      const std::vector<File::ShaderString>          &shader_code,
                      std::string_view                                name) const override;

    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                      GraphicsTypes::BufferType                       type,
                      size_t                                          buffer_size,
                      std::string_view                                name) const override;
    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                      GraphicsTypes::BufferType                       type,
                      size_t                                          buffer_size,
                      const void                                     *data,
                      std::string_view                                name) const override;
    void MapBuffer(GraphicsTypes::BufferInstance *buffer_instance, const void *map_memory) const override;
    // Copies lhs into rhs.
    void CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs, GraphicsTypes::BufferInstance *buffer_instance_lhs) const override;
    std::vector<unsigned char> RetrieveBuffer(GraphicsTypes::BufferInstance *buffer_instance) const override;

    void CreateTexture(std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
                       const GraphicsTypes::TextureInfo                &texture_info,
                       const std::vector<const void *>                 &in_data,
                       std::string_view                                 name) const override;
    void SampleTexture(void                                 *destination,
                       const GraphicsTypes::TextureInstance *source,
                       const gm::ivec2                      &origin,
                       const gm::ivec2                      &size) const override;
    void UploadToTexture(const GraphicsTypes::TextureInstance *destination,
                         void                                 *source,
                         size_t                                source_size,
                         const gm::ivec2                      &origin,
                         const gm::ivec2                      &size) const override;

    void CreateRenderBuffer(std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
                            const GraphicsTypes::FrameBufferInfo                 &create_info,
                            std::string_view                                      name) const override;

    const GraphicsTypes::TextureInstance *GetRenderBufferTexture(
        GraphicsTypes::RenderBufferInstance *render_buffer_instance) const override;

    void CreateCommandBuffer(std::unique_ptr<GraphicsTypes::CommandBufferInstance> &cb) override;

    void DispatchCompute(const GraphicsTypes::ShaderInstance         *shader,
                         const GraphicsTypes::TextureInstance *const *texture_instance,
                         int                                          count,
                         int                                          num_x,
                         int                                          num_y,
                         int                                          num_z) const override;
    void CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const override;

    void Resize(int, int) override {}

    bool              StartDraw(int width, int height) override;
    [[nodiscard]] int AcquireFrameIndex() const override;
    void              StartMainRenderPass(GraphicsTypes::CommandBufferInstance *cb) override;

    void EndDraw(GraphicsTypes::CommandBufferInstance *cb, int width, int height) override;

    void Wait() override;
    ~VulkanGraphicsInstance() override;
  };
}; // namespace graphics_vulkan


#endif // GRAPHICS_VULKAN_H
