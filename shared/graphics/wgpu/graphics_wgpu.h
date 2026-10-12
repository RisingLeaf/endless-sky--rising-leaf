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
#ifndef GRAPHICS_WGPU_H
#define GRAPHICS_WGPU_H

#include <emscripten/html5_webgl.h>
#include <memory>
#include <vector>

#include "shared/graphics/graphics_toplevel_defines.h"

namespace File
{
  struct ShaderString;
}

namespace graphics_wgpu
{
  class WGPUGraphicsInstance final : public GraphicsTypes::GraphicsInstance
  {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

    mutable std::unique_ptr<GraphicsTypes::BufferInstance> DynamicCommon;
    mutable size_t                                         CurrentOffsetCommon = 0;
    mutable std::unique_ptr<GraphicsTypes::BufferInstance> DynamicSpecific;
    mutable size_t                                         CurrentOffsetSpecific = 0;

    mutable std::vector<unsigned int> VAOs;

    mutable std::vector<std::unique_ptr<GraphicsTypes::BufferInstance>> DynamicBuffers;

  public:
    explicit WGPUGraphicsInstance(class Window &window);

    void CreateShader(std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
                      const ShaderInfo                               &shader_info,
                      const std::vector<File::ShaderString>          &shader_code) const override;

    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                      GraphicsTypes::BufferType                       type,
                      size_t                                          buffer_size) const override;
    void CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                      GraphicsTypes::BufferType                       type,
                      size_t                                          buffer_size,
                      const void                                     *data) const override;
    void MapBuffer(GraphicsTypes::BufferInstance *buffer_instance, const void *map_memory) const override;
    // Copies lhs into rhs.
    void CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs, GraphicsTypes::BufferInstance *buffer_instance_lhs) const override;

    void CreateTexture(std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
                       const GraphicsTypes::TextureInfo                &texture_info,
                       const std::vector<const void *>                 &in_data) const override;

    void                            CreateRenderBuffer(std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
                                                       const GraphicsTypes::FrameBufferInfo                 &create_info) const override;
    GraphicsTypes::TextureInstance *GetRenderBufferTexture(GraphicsTypes::RenderBufferInstance *render_buffer_instance) const override;

    void DispatchCompute(const GraphicsTypes::ShaderInstance         *shader,
                         const GraphicsTypes::TextureInstance *const *texture_instance,
                         int                                          count,
                         int                                          num_x,
                         int                                          num_y,
                         int                                          num_z) const override;
    void CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const override;

    void Resize(int, int) override {}

    bool StartDraw(Window &window) override;

    void SetState(const GraphicsTypes::RenderState &state) const override;

    int AcquireFrameIndex() const override;

    void SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const override;
    void SetColorState(bool state) const override;

    void BindShader(const GraphicsTypes::ShaderInstance *shader_instance) const override;
    void BindBufferDynamic(std::vector<unsigned char> &data, GraphicsTypes::UBOBindPoint bind_point) const override;

    void BindTextures(const GraphicsTypes::TextureInstance *const *texture_instance,
                      int                                          count,
                      GraphicsTypes::UBOBindPoint                  bind_point,
                      int                                          set) const override;
    void BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) const override;
    void DrawIndexed(size_t                               count,
                     const GraphicsTypes::BufferInstance *buffer_instance,
                     GraphicsTypes::PrimitiveType         prim_type) const override;
    void DrawDynamic(size_t count, size_t type_size, const void *data, GraphicsTypes::PrimitiveType prim_type) const override;

    void BindRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) const override;
    void EndRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) override;

    void StartMainRenderPass() override;
    void EndRenderPass() override;

    void EndDraw(Window &window) override;

    void Wait() override;
    ~WGPUGraphicsInstance() override;
  };
} // namespace graphics_wgpu


#endif // GRAPHICS_METAL_H
