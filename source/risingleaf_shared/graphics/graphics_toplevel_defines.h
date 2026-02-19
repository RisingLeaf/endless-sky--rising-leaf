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
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <memory>
#include <vector>

#include "ShaderInfo.h"

class Window;
namespace File
{
  struct ShaderString;
}
namespace GraphicsDefaults
{
  constexpr bool WIREFRAME = false;
}

namespace GraphicsTypes
{
  enum class ShaderStage
  {
    METAL_COMBINED,
    VULKAN_VERTEX,
    VULKAN_FRAGMENT,
    VULKAN_COMPUTE,
    GLSL_VERTEX,
    GLSL_FRAGMENT,
    GLSL_COMPUTE,
  };

  enum class CullState
  {
    OFF,
    FRONT,
    BACK
  };

  enum class BufferType
  {
    UNIFORM,
    UNIFORM_DYNAMIC,
    TEXTURE,
    VERTEX,
    VERTEX_DYNAMIC,
    INDEX,
    STAGING
  };

  enum class UBOBindPoint
  {
    Common,
    Specific
  };

  enum class ImageFormat
  {
    R,
    RG,
    RGB,
    RGBA,
    RGBA16F,
    RGBA32F,
    BGRA,
    DEPTH,
    INVALID
  };

  enum class TextureType
  {
    TYPE_2D,
    TYPE_2D_ARRAY,
    TYPE_3D,
    TYPE_CUBE,
    TYPE_CUBE_ARRAY,
  };

  enum class TextureTarget
  {
    READ,
    WRITE,
    READ_WRITE,
    DRAW,
  };

  enum class TextureAddressMode
  {
    REPEAT,
    CLAMP_TO_EDGE,
  };

  enum class TextureFilter
  {
    LINEAR,
    NEAREST,
  };

  enum class RenderBufferType
  {
    COLOR,
    DEPTH,
    BOTH,
  };

  enum class DataType
  {
    FLOAT,
    INT
  };

  enum class PrimitiveType : uint8_t
  {
    TRIANGLES,
    TRIANGLE_STRIP,
    LINES,
    POINTS,
  };

  enum class ShaderType
  {
    INT,
    FLOAT,

    INT2,
    INT3,
    INT4,

    FLOAT2,
    FLOAT3,
    FLOAT4,

    MAT2,
    MAT3,
    MAT4,
  };

  enum class BlendFactor
  {
    FACTOR_ONE,
    FACTOR_ZERO,
    FACTOR_SRC_ALPHA,
    FACTOR_ONE_MINUS_SRC_ALPHA,
  };

  struct BlendState
  {
    bool        BlendingEnabled = false;
    BlendFactor SRCColor        = BlendFactor::FACTOR_ONE;
    BlendFactor DSTColor        = BlendFactor::FACTOR_ONE;
    BlendFactor SRCAlpha        = BlendFactor::FACTOR_ONE;
    BlendFactor DSTAlpha        = BlendFactor::FACTOR_ONE;

    bool operator==(const BlendState &b) const
    {
      return this->BlendingEnabled == b.BlendingEnabled && this->SRCColor == b.SRCColor && this->DSTColor == b.DSTColor && this->SRCAlpha == b.SRCAlpha && this->DSTAlpha == b.DSTAlpha;
    }
  };

  enum class CullMode
  {
    CULL_NONE,
    CULL_BACK,
    CULL_FRONT
  };

  enum class DepthCompareMode
  {
    NONE,
    COMPARE_GREATER,
    COMPARE_GREATER_EQUALS,
    COMPARE_LESS,
    COMPARE_LESS_EQUALS,
  };

  struct Viewport
  {
    uint64_t OffsetX;
    uint64_t OffsetY;
    uint64_t ExtentX;
    uint64_t ExtentY;

    bool operator==(const Viewport &b) const = default;
  };

  struct VertexLayout
  {
    uint32_t                                   vertex_size = 0;
    std::vector<std::pair<DataType, uint32_t>> values;

    bool operator==(const VertexLayout &b) const
    {
      constexpr auto ValuesEquals = [](const std::vector<std::pair<DataType, uint32_t>> &a, const std::vector<std::pair<DataType, uint32_t>> &b)
      {
        for(size_t i = 0; i < a.size(); i++)
        {
          if(a[i].first != b[i].first || a[i].second != b[i].second) return false;
        }
        return true;
      };
      return this->vertex_size == b.vertex_size && this->values.size() == b.values.size() && ValuesEquals(this->values, b.values);
    }
  };

  struct Color
  {
    float r = 0.;
    float g = 0.;
    float b = 0.;
    float a = 0.;
    explicit Color(const float _r = 0.f, const float _g = 0.f, const float _b = 0.f, const float _a = 0.f)
      : r(_r), g(_g), b(_b), a(_a) {}
    bool operator==(const Color &b) const = default;
  };

  struct RenderState
  {
    PrimitiveType    DrawPrimitiveType = PrimitiveType::TRIANGLES;
    BlendState       Blending          = BlendState{false, BlendFactor::FACTOR_SRC_ALPHA, BlendFactor::FACTOR_SRC_ALPHA, BlendFactor::FACTOR_SRC_ALPHA, BlendFactor::FACTOR_SRC_ALPHA};
    CullMode         Culling           = CullMode::CULL_NONE;
    bool             DepthTest         = false;
    bool             DepthWrite        = false;
    DepthCompareMode DepthCompare      = DepthCompareMode::NONE;
    bool             WireFrame         = false;
    uint8_t          ColorMask         = 1 + 2 + 4 + 8;
    Viewport         DrawViewport      = Viewport{0, 0, 100, 100};
    Color            ClearColor        = Color(0.f, 0.f, 0.f, 1.f);
    float            ClearDepth        = 0.f;
    VertexLayout     PipelineVertexLayout;

    // Only compare what is relevant for the Pipeline
    bool operator==(const RenderState &b) const
    {
      return this->DrawPrimitiveType == b.DrawPrimitiveType && this->Blending == b.Blending && this->Culling == b.Culling && this->DepthTest == b.DepthTest && this->DepthWrite == b.DepthWrite &&
             this->DepthCompare == b.DepthCompare && this->WireFrame == b.WireFrame && this->ColorMask == b.ColorMask && this->PipelineVertexLayout == b.PipelineVertexLayout;
    }
  };

  struct TextureInfo
  {
    uint32_t           Width       = 1;
    uint32_t           Height      = 1;
    uint32_t           Layers      = 1;
    uint32_t           Depth       = 1;
    uint32_t           Components  = 4;
    uint32_t           MipLevels   = 1;
    uint32_t           Samples     = 1;
    ImageFormat        Format      = ImageFormat::RGBA;
    TextureType        Type        = TextureType::TYPE_2D;
    TextureTarget      Target      = TextureTarget::READ;
    TextureAddressMode AddressMode = TextureAddressMode::REPEAT;
    TextureFilter      Filter      = TextureFilter::LINEAR;
  };

  struct FrameBufferInfo
  {
    uint32_t         Width      = 1;
    uint32_t         Height     = 1;
    uint32_t         Samples    = 1;
    bool             HasColor   = false;
    bool             HasDepth   = false;
    bool             Presenter  = false;
    RenderBufferType TargetType = RenderBufferType::COLOR;
    ImageFormat      Format     = ImageFormat::RGBA;
    bool             operator==(const FrameBufferInfo &other) const
    {
      return this->Width == other.Width && this->Height == other.Height && this->Samples == other.Samples && this->HasColor == other.HasColor && this->HasDepth == other.HasDepth &&
             this->Presenter == other.Presenter && this->TargetType == other.TargetType && this->Format == other.Format;
    }
  };

  struct StateInfo
  {
    int  Samples    = 1;
    bool Color      = true;
    bool Depth      = true;
    bool DepthTest  = true;
    bool DepthWrite = true;
  };

  class CommandBufferInstance
  {
  public:
    CommandBufferInstance()          = default;
    virtual ~CommandBufferInstance() = default;
  };

  class RenderPassInstance
  {
    StateInfo State;

  public:
    explicit RenderPassInstance(const StateInfo &state) : State(state) {}
    virtual ~RenderPassInstance() = default;

    [[nodiscard]] const StateInfo &GetState() const { return State; }
    void                           SetSamples(const int samples) { State.Samples = samples; }
    void                           SetColor(const bool color) { State.Color = color; }
    void                           SetDepth(const bool depth) { State.Depth = depth; }
    void                           SetDepthTest(const bool depth_test) { State.DepthTest = depth_test; }
    void                           SetDepthWrite(const bool depth_write) { State.DepthWrite = depth_write; }
  };

  class ShaderInstance
  {
  public:
    ShaderInstance()          = default;
    virtual ~ShaderInstance() = default;
  };

  class BufferInstance
  {
  public:
    BufferInstance()          = default;
    virtual ~BufferInstance() = default;
  };

  class TextureInstance
  {
  protected:
    TextureInfo Info;

  public:
    TextureInstance() = default;
    explicit TextureInstance(const TextureInfo &info) : Info(info) {}
    virtual ~TextureInstance() = default;

    [[nodiscard]] const TextureInfo &GetInfo() const { return Info; }
  };

  class VertexStorageInstance
  {
  public:
    VertexStorageInstance()          = default;
    virtual ~VertexStorageInstance() = default;
  };

  class RenderBufferInstance
  {
    FrameBufferInfo Info;

  public:
    explicit RenderBufferInstance(const FrameBufferInfo &info) : Info(info) {}
    virtual ~RenderBufferInstance() = default;

    [[nodiscard]] const FrameBufferInfo &GetInfo() const { return Info; }
  };


  class GraphicsInstance
  {
  public:
    GraphicsInstance()          = default;
    virtual ~GraphicsInstance() = default;

    virtual void CreateShader(std::unique_ptr<ShaderInstance> &shader_instance, const ShaderInfo &shader_info, const std::vector<File::ShaderString> &shader_code) const = 0;

    virtual void CreateBuffer(std::unique_ptr<BufferInstance> &buffer_instance, BufferType type, size_t buffer_size) const  = 0;
    virtual void CreateBuffer(std::unique_ptr<BufferInstance> &buffer_instance, BufferType type, size_t buffer_size, const void *data) const = 0;
    virtual void MapBuffer(BufferInstance *buffer_instance, const void *map_memory) const = 0;
    // Copies lhs into rhs.
    virtual void CopyBuffer(BufferInstance *buffer_instance_rhs, BufferInstance *buffer_instance_lhs) const = 0;

    virtual void CreateTexture(std::unique_ptr<TextureInstance> &texture_instance, const TextureInfo &texture_info, const std::vector<const void *> &in_data) const = 0;

    virtual void CreateRenderBuffer(std::unique_ptr<RenderBufferInstance> &render_buffer_instance, const FrameBufferInfo &create_info) const = 0;
    virtual const TextureInstance *GetRenderBufferTexture(RenderBufferInstance *render_buffer_instance) const                                                                                   = 0;

    virtual void DispatchCompute(const ShaderInstance *shader, const TextureInstance *const *texture_instance, int count, int num_x, int num_y, int num_z) const = 0;
    virtual void CreateMipMaps(const TextureInstance *texture_instance) const                                                                                    = 0;

    virtual void Resize(int width, int height) = 0;

    virtual bool StartDraw(int width, int height) = 0;

    virtual void SetState(const RenderState &state) const = 0;

    virtual int AcquireFrameIndex() const = 0;

    virtual void SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const = 0;
    virtual void SetColorState(bool state) const                                          = 0;

    virtual void BindShader(const ShaderInstance *shader_instance) const                                  = 0;
    virtual void BindBufferDynamic(std::vector<unsigned char> &data, UBOBindPoint bind_point) const = 0;

    virtual void BindTextures(const TextureInstance *const*texture_instance, int count, int set) const = 0;
    virtual void BindVertexBuffer(BufferInstance *buffer_instance) const                                                     = 0;
    virtual void DrawIndexed(size_t count, const BufferInstance *buffer_instance, PrimitiveType prim_type) const             = 0;
    virtual void DrawDynamic(size_t count, size_t type_size, const void *data,    PrimitiveType prim_type) const             = 0;

    virtual void BindRenderBuffer(RenderBufferInstance *render_buffer_instance) const = 0;
    virtual void EndRenderBuffer(RenderBufferInstance *render_buffer_instance) = 0;

    virtual void StartMainRenderPass() = 0;
    virtual void EndRenderPass()       = 0;

    virtual void EndDraw(int width, int height) = 0;

    virtual void Wait() = 0;
  };
} // namespace GraphicsTypes


#endif // GRAPHICS_H
