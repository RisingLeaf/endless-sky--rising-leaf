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
#include "graphics_metal.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "graphics/ShaderInfo.h"
#include "system/File.h"
#include "system/Log.h"

#include <memory>
#include <mutex>

#include "GameWindow.h"


namespace graphics_metal
{
  constexpr MTL::PixelFormat   PIXEL_FORMAT    = MTL::PixelFormatBGRA8Unorm;
  constexpr NS::StringEncoding STRING_ENCODING = NS::StringEncoding::UTF8StringEncoding;
  constexpr int                SAMPLE_COUNT    = 4;

  constexpr size_t DYNAMIC_UNIFORM_BUFFER_SIZE = 1024 * 1024 * 4; // 4 MB

  constexpr MTL::PixelFormat GetMetalPixelFormat(const GraphicsTypes::ImageFormat format)
  {
    switch(format)
    {
    case GraphicsTypes::ImageFormat::R:       return MTL::PixelFormatR8Unorm;
    case GraphicsTypes::ImageFormat::RG:      return MTL::PixelFormatRG8Unorm;
    case GraphicsTypes::ImageFormat::RGB:
    case GraphicsTypes::ImageFormat::RGBA:    return MTL::PixelFormatRGBA8Unorm;
    case GraphicsTypes::ImageFormat::RGBA16F: return MTL::PixelFormatRGBA16Float;
    case GraphicsTypes::ImageFormat::RGBA32F: return MTL::PixelFormatRGBA32Float;
    case GraphicsTypes::ImageFormat::BGRA:    return MTL::PixelFormatBGRA8Unorm;
    case GraphicsTypes::ImageFormat::DEPTH:   return MTL::PixelFormatDepth32Float;
    case GraphicsTypes::ImageFormat::INVALID: return MTL::PixelFormatInvalid;
    }
    assert(false && "Missing case for switch statement in GetMetalPixelFormat!");
    return MTL::PixelFormatInvalid;
  }

  MTL::TextureType GetMetalTextureType(const GraphicsTypes::TextureType type)
  {
    switch(type)
    {
    case GraphicsTypes::TextureType::TYPE_2D:         return MTL::TextureType2D;
    case GraphicsTypes::TextureType::TYPE_2D_ARRAY:   return MTL::TextureType2DArray;
    case GraphicsTypes::TextureType::TYPE_3D:         return MTL::TextureType3D;
    case GraphicsTypes::TextureType::TYPE_CUBE:       return MTL::TextureTypeCube;
    case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY: return MTL::TextureTypeCubeArray;
    }
    assert(false && "Missing case for switch statement in GetMetalTextureType!");
    return MTL::TextureType2D;
  }

  MTL::BlendFactor GetMetalBlendFactor(const GraphicsTypes::BlendFactor factor)
  {
    switch(factor)
    {
    case GraphicsTypes::BlendFactor::FACTOR_ONE:                 return MTL::BlendFactorOne;
    case GraphicsTypes::BlendFactor::FACTOR_ZERO:                return MTL::BlendFactorZero;
    case GraphicsTypes::BlendFactor::FACTOR_SRC_ALPHA:           return MTL::BlendFactorSourceAlpha;
    case GraphicsTypes::BlendFactor::FACTOR_ONE_MINUS_SRC_ALPHA: return MTL::BlendFactorOneMinusSourceAlpha;
    }
    assert(false && "Missing case for switch statement in GetMetalBlendFactor!");
    return MTL::BlendFactorZero;
  }

  struct MetalShaderInstance final : GraphicsTypes::ShaderInstance
  {
    const ShaderInfo &shader_info;
    MTL::Function    *vert_fn = nullptr;
    MTL::Function    *frag_fn = nullptr;
    MTL::Function    *comp_fn = nullptr;

    const std::string Name;

    explicit MetalShaderInstance(const ShaderInfo &s, const std::string_view name) : shader_info(s), Name(name) {}

    ~MetalShaderInstance() override
    {
      if(vert_fn) vert_fn->release();
      if(frag_fn) frag_fn->release();
      if(comp_fn) comp_fn->release();
    }
  };

  struct MetalPipelineState
  {
    bool                           color = true;
    GraphicsTypes::FrameBufferInfo Info;
    GraphicsTypes::RenderState     State;
    const MetalShaderInstance     *shader = nullptr;

    bool operator==(const MetalPipelineState &other) const
    {
      return this->color == other.color && this->shader == other.shader && this->Info == other.Info &&
             this->State == other.State;
    }
  };

  struct MetalPipelineWithState
  {
    MTL::RenderPipelineState *Pipeline = nullptr;
    MetalPipelineState        State;
  };

  struct MetalDepthWithState
  {
    MTL::DepthStencilState *Depth = nullptr;

    bool                            DepthTest    = false;
    bool                            DepthWrite   = false;
    GraphicsTypes::DepthCompareMode DepthCompare = GraphicsTypes::DepthCompareMode::NONE;
  };

  struct MetalBufferInstance final : GraphicsTypes::BufferInstance
  {
    MTL::Buffer *Buffer     = nullptr;
    size_t       BufferSize = 0;

    ~MetalBufferInstance() override
    {
      if(Buffer) Buffer->release();
    }
  };

  struct MetalTextureInstance final : GraphicsTypes::TextureInstance
  {
    MTL::Texture      *texture  = nullptr;
    MTL::SamplerState *sampler  = nullptr;
    MTL::Buffer       *argument = nullptr;

    MetalTextureInstance() = default;

    explicit MetalTextureInstance(const GraphicsTypes::TextureInfo &info) : TextureInstance(info) {}

    ~MetalTextureInstance() override
    {
      if(argument) argument->release();
      if(texture) texture->release();
      if(sampler) sampler->release();
    }

    void CreateArgumentBuffer(MTL::Device *Device)
    {
      if(!texture) return;
      MTL::SamplerDescriptor *sampler_descriptor = MTL::SamplerDescriptor::alloc()->init();
      switch(Info.Filter)
      {
      case GraphicsTypes::TextureFilter::NEAREST:
        sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterNearest);
        sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterNearest);
        break;
      case GraphicsTypes::TextureFilter::LINEAR:
        sampler_descriptor->setMagFilter(MTL::SamplerMinMagFilterLinear);
        sampler_descriptor->setMinFilter(MTL::SamplerMinMagFilterLinear);
        break;
      }
      switch(Info.AddressMode)
      {
      case GraphicsTypes::TextureAddressMode::REPEAT:
        sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);
        sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeRepeat);
        sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeRepeat);
        break;
      case GraphicsTypes::TextureAddressMode::CLAMP_TO_EDGE:
        sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
        sampler_descriptor->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
        sampler_descriptor->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
        break;
      }
      sampler_descriptor->setSupportArgumentBuffers(true);
      sampler = Device->newSamplerState(sampler_descriptor);
      sampler_descriptor->release();

      MTL::ArgumentDescriptor *tex_arg = MTL::ArgumentDescriptor::alloc()->init();
      tex_arg->setIndex(0);
      tex_arg->setDataType(MTL::DataTypeTexture);
      tex_arg->setTextureType(GetMetalTextureType(Info.Type));
      tex_arg->setAccess(MTL::ArgumentAccessReadOnly);

      MTL::ArgumentDescriptor *sam_arg = MTL::ArgumentDescriptor::alloc()->init();
      sam_arg->setIndex(1);
      sam_arg->setDataType(MTL::DataTypeSampler);

      NS::Array *array = NS::Array::alloc()->init((const NS::Object *[]){tex_arg, sam_arg}, 2);

      MTL::ArgumentEncoder *argument_encoder = Device->newArgumentEncoder(array);
      argument = Device->newBuffer(argument_encoder->encodedLength(), MTL::ResourceStorageModeManaged);
      argument_encoder->setArgumentBuffer(argument, 0);
      argument_encoder->setTexture(texture, 0);
      argument_encoder->setSamplerState(sampler, 1);

      argument_encoder->release();
      array->release();
      tex_arg->release();
      sam_arg->release();
    }

    GraphicsTypes::TextureInfo &GetInfo() { return Info; }
  };

  struct MetalRenderBufferInstance final : GraphicsTypes::RenderBufferInstance
  {
    MTL::RenderPassDescriptor *RenderPassDescriptorFrameBuffer = nullptr;
    MetalTextureInstance       FrameBuffer;
    MetalTextureInstance       OptionalDepth;
    MetalTextureInstance       OptionalColor;

    explicit MetalRenderBufferInstance(const GraphicsTypes::FrameBufferInfo &info) : RenderBufferInstance(info) {}

    ~MetalRenderBufferInstance() override
    {
      if(RenderPassDescriptorFrameBuffer) RenderPassDescriptorFrameBuffer->release();
    }
  };

  MTL::CommandBuffer        *current_buffer;
  MTL::RenderCommandEncoder *current_encoder;
  MetalPipelineState         current_state;


  ShaderInfo::CommonUniformBufferData common_data;
  bool                                common_data_changed = false;

  void CreateRenderPassDescriptor(MetalGraphicsInstance *instance)
  {
    instance->RenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

    MTL::RenderPassColorAttachmentDescriptor *colorAttachment =
        instance->RenderPassDescriptor->colorAttachments()->object(0);
    MTL::RenderPassDepthAttachmentDescriptor *depthAttachment = instance->RenderPassDescriptor->depthAttachment();

    colorAttachment->setTexture(instance->MSAARenderTargetTexture);
    if(instance->CurrentDrawable) colorAttachment->setResolveTexture(instance->CurrentDrawable->texture());
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(0., 0., 0., 0.0));
    colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);

    depthAttachment->setTexture(instance->DepthTexture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setStoreAction(MTL::StoreActionDontCare);
    depthAttachment->setClearDepth(0.0);
  }

  void CreateDepthAndMSAAResources(MetalGraphicsInstance *instance, const int width, const int height)
  {
    MTL::TextureDescriptor *msaaTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    msaaTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    msaaTextureDescriptor->setPixelFormat(PIXEL_FORMAT);
    msaaTextureDescriptor->setWidth(width);
    msaaTextureDescriptor->setHeight(height);
    msaaTextureDescriptor->setSampleCount(SAMPLE_COUNT);
    msaaTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);

    instance->MSAARenderTargetTexture = instance->Device->newTexture(msaaTextureDescriptor);

    MTL::TextureDescriptor *depthTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    depthTextureDescriptor->setTextureType(MTL::TextureType2DMultisample);
    depthTextureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDescriptor->setWidth(width);
    depthTextureDescriptor->setHeight(height);
    depthTextureDescriptor->setUsage(MTL::TextureUsageRenderTarget);
    depthTextureDescriptor->setSampleCount(SAMPLE_COUNT);

    instance->DepthTexture = instance->Device->newTexture(depthTextureDescriptor);

    msaaTextureDescriptor->release();
    depthTextureDescriptor->release();
  }

  void UpdateRenderPassDescriptor(const MetalGraphicsInstance *instance)
  {
    instance->RenderPassDescriptor->colorAttachments()->object(0)->setTexture(instance->MSAARenderTargetTexture);
    if(instance->CurrentDrawable)
      instance->RenderPassDescriptor->colorAttachments()->object(0)->setResolveTexture(
          instance->CurrentDrawable->texture());
    instance->RenderPassDescriptor->depthAttachment()->setTexture(instance->DepthTexture);
  }

  void Resize(MetalGraphicsInstance *instance, const int width, const int height)
  {
    if(instance->MSAARenderTargetTexture)
    {
      instance->MSAARenderTargetTexture->release();
      instance->MSAARenderTargetTexture = nullptr;
    }
    if(instance->DepthTexture)
    {
      instance->DepthTexture->release();
      instance->DepthTexture = nullptr;
    }

    CreateDepthAndMSAAResources(instance, width, height);
    UpdateRenderPassDescriptor(instance);
  }


  const MTL::DepthStencilState *GetDepthStencilForState(const MetalGraphicsInstance          *instance,
                                                        const bool                            depth_test,
                                                        const bool                            depth_write,
                                                        const GraphicsTypes::DepthCompareMode depth_compare)
  {
    std::lock_guard guard(instance->DepthStatesListMutex);

    for(const auto &depth : instance->DepthStateList)
    {
      if(depth.DepthTest == depth_test && depth.DepthCompare == depth_compare && depth.DepthWrite == depth_write)
        return depth.Depth;
    }

    MTL::DepthStencilDescriptor *depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();

    if(depth_test)
    {
      switch(depth_compare)
      {
      case GraphicsTypes::DepthCompareMode::NONE:
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionAlways);
        break;
      case GraphicsTypes::DepthCompareMode::COMPARE_GREATER:
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionGreater);
        break;
      case GraphicsTypes::DepthCompareMode::COMPARE_GREATER_EQUALS:
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionGreaterEqual);
        break;
      case GraphicsTypes::DepthCompareMode::COMPARE_LESS:
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
        break;
      case GraphicsTypes::DepthCompareMode::COMPARE_LESS_EQUALS:
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
        break;
      }
    }
    else
    {
      depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionAlways);
    }

    depthStencilDescriptor->setDepthWriteEnabled(depth_write);

    auto &it        = instance->DepthStateList.emplace_back();
    it.DepthTest    = depth_test;
    it.DepthCompare = depth_compare;
    it.DepthWrite   = depth_write;

    it.Depth = instance->Device->newDepthStencilState(depthStencilDescriptor);

    depthStencilDescriptor->release();

    return it.Depth;
  }


  const MTL::RenderPipelineState *GetPipelineForState(const MetalGraphicsInstance *instance,
                                                      const MetalPipelineState    &state)
  {
    std::lock_guard guard(instance->PipelinesListMutex);

    for(const auto &pipeline : instance->Pipelines)
    {
      if(pipeline.State == state) return pipeline.Pipeline;
    }

    MTL::RenderPipelineDescriptor *render_pipeline_descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    render_pipeline_descriptor->setLabel(NS::String::string(state.shader->Name.c_str(), NS::ASCIIStringEncoding));

    render_pipeline_descriptor->setVertexFunction(state.shader->vert_fn);
    if(state.shader->frag_fn && state.color) render_pipeline_descriptor->setFragmentFunction(state.shader->frag_fn);

    if(state.Info.HasDepth) render_pipeline_descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    else render_pipeline_descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatInvalid);

    if(!state.color)
    {
      render_pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatInvalid);
      render_pipeline_descriptor->setSampleCount(1);
    }
    else
    {
      render_pipeline_descriptor->setSampleCount(state.Info.Samples);

      auto color_attachment = render_pipeline_descriptor->colorAttachments()->object(0);
      color_attachment->setBlendingEnabled(true);
      color_attachment->setRgbBlendOperation(MTL::BlendOperationAdd);
      color_attachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
      color_attachment->setSourceRGBBlendFactor(GetMetalBlendFactor(state.State.Blending.SRCColor));
      color_attachment->setSourceAlphaBlendFactor(GetMetalBlendFactor(state.State.Blending.SRCAlpha));
      color_attachment->setDestinationRGBBlendFactor(GetMetalBlendFactor(state.State.Blending.DSTColor));
      color_attachment->setDestinationAlphaBlendFactor(GetMetalBlendFactor(state.State.Blending.DSTAlpha));

      color_attachment->setPixelFormat(GetMetalPixelFormat(state.Info.Format));
    }

    const MTL::VertexDescriptor *vertex_descriptor = render_pipeline_descriptor->vertexDescriptor();

    for(const auto &[Type, Offset, Location] : state.shader->shader_info.GetVertexAttribs())
    {
      MTL::VertexFormat format;
      switch(Type)
      {
      case GraphicsTypes::ShaderType::INT:    format = MTL::VertexFormatInt; break;
      case GraphicsTypes::ShaderType::FLOAT:  format = MTL::VertexFormatFloat; break;
      case GraphicsTypes::ShaderType::INT2:   format = MTL::VertexFormatInt2; break;
      case GraphicsTypes::ShaderType::INT3:   format = MTL::VertexFormatInt3; break;
      case GraphicsTypes::ShaderType::INT4:   format = MTL::VertexFormatInt4; break;
      case GraphicsTypes::ShaderType::FLOAT2: format = MTL::VertexFormatFloat2; break;
      case GraphicsTypes::ShaderType::FLOAT3: format = MTL::VertexFormatFloat3; break;
      case GraphicsTypes::ShaderType::FLOAT4: format = MTL::VertexFormatFloat4; break;
      case GraphicsTypes::ShaderType::MAT2:
      case GraphicsTypes::ShaderType::MAT3:
      case GraphicsTypes::ShaderType::MAT4:
      default:                                assert(false && "invalid type for input attribute, reconsider");
      }
      vertex_descriptor->attributes()->object(Location)->setFormat(format);
      vertex_descriptor->attributes()->object(Location)->setBufferIndex(0);
      vertex_descriptor->attributes()->object(Location)->setOffset(Offset);
    }

    vertex_descriptor->layouts()->object(0)->setStride(state.shader->shader_info.GetVertexSize());

    render_pipeline_descriptor->setVertexDescriptor(vertex_descriptor);

    auto &it = instance->Pipelines.emplace_back();
    it.State = state;

    NS::Error *error;
    it.Pipeline = instance->Device->newRenderPipelineState(render_pipeline_descriptor, &error);
    if(error)
    {
      Log::Error << error->localizedDescription()->cString(NS::StringEncoding::UTF8StringEncoding) << Log::End;
      return nullptr;
    }

    render_pipeline_descriptor->release();

    return it.Pipeline;
  }

  MetalGraphicsInstance *
  ConvertGraphicsInstance(GraphicsTypes::GraphicsInstance *metal_instance, const int line, const char *file = __FILE__)
  {
    if(!metal_instance)
    {
      Log::Error << "Trying to execute graphics command on invalid instance, in " << file << " : " << line << Log::End;
      return nullptr;
    }
    return reinterpret_cast<MetalGraphicsInstance *>(metal_instance);
  }

  const MetalGraphicsInstance *ConvertGraphicsInstance(const GraphicsTypes::GraphicsInstance *metal_instance,
                                                       const int                              line,
                                                       const char                            *file = __FILE__)
  {
    if(!metal_instance)
    {
      Log::Error << "Trying to execute graphics command on invalid instance, in " << file << " : " << line << Log::End;
      return nullptr;
    }
    return reinterpret_cast<const MetalGraphicsInstance *>(metal_instance);
  }
} // namespace graphics_metal

graphics_metal::MetalGraphicsInstance::MetalGraphicsInstance(int width, int height)
{
  ShaderInfo::Init();

  Device = MTL::CreateSystemDefaultDevice();

  View  = SDL_Metal_CreateView(GameWindow::GetWindow());
  Layer = static_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(View));
  Layer->setDevice(Device);

  CurrentDrawable = nullptr;

  MetalCommandQueue = Device->newCommandQueue(1000);

  CreateRenderPassDescriptor(this);

  graphics_metal::Resize(this, width, height);

  DynamicVertexBuffer = Device->newBuffer(DYNAMIC_UNIFORM_BUFFER_SIZE, MTL::ResourceStorageModeShared);
}

void graphics_metal::MetalGraphicsInstance::CreateShader(
    std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
    const ShaderInfo                               &shader_info,
    const std::vector<File::ShaderString>          &shader_code,
    const std::string_view                          name) const
{
  if(shader_instance)
  {
    Log::Error << "Error at shader creation, trying to assign already used shader instance" << Log::End;
    return;
  }

  auto *metal_shader_instance = new MetalShaderInstance(shader_info, name);

  const File::ShaderString *metal_shader_code = nullptr;

  for(const auto &code : shader_code)
    if(code.stage == GraphicsTypes::ShaderStage::METAL_COMBINED)
    {
      metal_shader_code = &code;
      break;
    }

  if(!metal_shader_code)
  {
    Log::Error << "Error at shader creation, no valid shader code given." << Log::End;
    return;
  }

  MTL::CompileOptions *options = MTL::CompileOptions::alloc()->init();
  options->setEnableLogging(true);
  options->setOptimizationLevel(MTL::LibraryOptimizationLevelDefault);

  NS::Error    *error = nullptr;
  MTL::Library *lib =
      Device->newLibrary(NS::String::string(reinterpret_cast<const char *>(metal_shader_code->code.data()),
                                            NS::StringEncoding::ASCIIStringEncoding),
                         options,
                         &error);

  options->release();

  if(!lib)
  {
    Log::Error << error->localizedDescription()->utf8String() << Log::End;
    return;
  }

  metal_shader_instance->vert_fn = lib->newFunction(NS::String::string("vertexShader", STRING_ENCODING));
  metal_shader_instance->frag_fn = lib->newFunction(NS::String::string("fragmentShader", STRING_ENCODING));
  metal_shader_instance->comp_fn = lib->newFunction(NS::String::string("kernel_main", STRING_ENCODING));

  lib->release();

  shader_instance.reset(metal_shader_instance);
}

void graphics_metal::MetalGraphicsInstance::CreateBuffer(
    std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
    const GraphicsTypes::BufferType,
    const size_t buffer_size) const
{
  if(buffer_instance)
  {
    Log::Error << "Error at buffer creation, trying to assign already used buffer instance" << Log::End;
    return;
  }

  auto *metal_buffer_instance = new MetalBufferInstance;

  metal_buffer_instance->Buffer     = Device->newBuffer(buffer_size, MTL::ResourceStorageModeManaged);
  metal_buffer_instance->BufferSize = buffer_size;

  buffer_instance.reset(metal_buffer_instance);
}

void graphics_metal::MetalGraphicsInstance::CreateBuffer(
    std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
    const GraphicsTypes::BufferType,
    const size_t buffer_size,
    const void  *data) const
{
  if(buffer_instance)
  {
    Log::Error << "Error at buffer creation, trying to assign already used buffer instance" << Log::End;
    return;
  }

  if(buffer_size == 0) return;

  auto *metal_buffer_instance = new MetalBufferInstance;

  metal_buffer_instance->Buffer     = Device->newBuffer(data, buffer_size, MTL::ResourceStorageModeManaged);
  metal_buffer_instance->BufferSize = buffer_size;

  buffer_instance.reset(metal_buffer_instance);
}

void graphics_metal::MetalGraphicsInstance::MapBuffer(GraphicsTypes::BufferInstance *buffer_instance,
                                                      const void                    *map_memory) const
{
  if(!buffer_instance)
  {
    Log::Error << "Trying to map to invalid buffer." << Log::End;
    return;
  }
  const auto *metal_buffer_instance = reinterpret_cast<MetalBufferInstance *>(buffer_instance);

  memcpy(metal_buffer_instance->Buffer->contents(), map_memory, metal_buffer_instance->BufferSize);
  metal_buffer_instance->Buffer->didModifyRange(NS::Range::Make(0, metal_buffer_instance->Buffer->length()));
}

void graphics_metal::MetalGraphicsInstance::CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs,
                                                       GraphicsTypes::BufferInstance *buffer_instance_lhs) const
{
  if(!buffer_instance_rhs)
  {
    Log::Error << "Invalid buffer given to copy into" << Log::End;
    return;
  }
  auto *metal_buffer_instance_rhs = reinterpret_cast<MetalBufferInstance *>(buffer_instance_rhs);
  if(!buffer_instance_lhs)
  {
    Log::Error << "Invalid buffer given to copy from" << Log::End;
    return;
  }
  auto *metal_buffer_instance_lhs = reinterpret_cast<MetalBufferInstance *>(buffer_instance_lhs);
  auto  single_command_buffer     = MetalCommandQueue->commandBuffer();
  auto  encoder                   = single_command_buffer->blitCommandEncoder();
  encoder->copyFromBuffer(metal_buffer_instance_lhs->Buffer,
                          0,
                          metal_buffer_instance_rhs->Buffer,
                          0,
                          metal_buffer_instance_lhs->BufferSize);
  encoder->endEncoding();
  single_command_buffer->commit();
  single_command_buffer->waitUntilCompleted();
}

void graphics_metal::MetalGraphicsInstance::CreateTexture(
    std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
    const GraphicsTypes::TextureInfo                &texture_info,
    const void *const                                in_data) const
{
  if(texture_instance)
  {
    Log::Error << "Error at texture creation, trying to assign already used texture instance." << Log::End;
    return;
  }

  auto *metal_texture_instance = new MetalTextureInstance;

  metal_texture_instance->GetInfo() = texture_info;
  metal_texture_instance->GetInfo().MipLevels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(texture_info.Width, texture_info.Height)))) + 1;

  MTL::TextureDescriptor *textureDescriptor = MTL::TextureDescriptor::alloc()->init();
  textureDescriptor->setPixelFormat(GetMetalPixelFormat(metal_texture_instance->GetInfo().Format));
  textureDescriptor->setWidth(metal_texture_instance->GetInfo().Width);
  textureDescriptor->setHeight(metal_texture_instance->GetInfo().Height);
  textureDescriptor->setArrayLength(metal_texture_instance->GetInfo().Layers);
  textureDescriptor->setDepth(metal_texture_instance->GetInfo().Depth);
  textureDescriptor->setMipmapLevelCount(metal_texture_instance->GetInfo().MipLevels);
  textureDescriptor->setTextureType(GetMetalTextureType(metal_texture_instance->GetInfo().Type));
  MTL::TextureUsage usage = MTL::TextureUsageUnknown;
  if(texture_info.Target != GraphicsTypes::TextureTarget::WRITE)
  {
    usage |= MTL::TextureUsageShaderRead;
  }
  if(texture_info.Target != GraphicsTypes::TextureTarget::READ)
  {
    usage |= MTL::TextureUsageShaderWrite;
  }
  textureDescriptor->setUsage(usage);
  textureDescriptor->setStorageMode(MTL::StorageModeShared);

  metal_texture_instance->texture = Device->newTexture(textureDescriptor);

  textureDescriptor->release();

  const MTL::Region region =
      MTL::Region(0, 0, 0, metal_texture_instance->GetInfo().Width, metal_texture_instance->GetInfo().Height, 1);
  NS::UInteger bytesPerRow = metal_texture_instance->GetInfo().Components * metal_texture_instance->GetInfo().Width;
  if(metal_texture_instance->GetInfo().Format == GraphicsTypes::ImageFormat::RGBA16F) bytesPerRow *= 2;
  else if(metal_texture_instance->GetInfo().Format == GraphicsTypes::ImageFormat::RGBA32F) bytesPerRow *= 4;

  for(int i = 0; i < texture_info.Layers; i++)
    metal_texture_instance->texture->replaceRegion(region,
                                                   0,
                                                   i,
                                                   static_cast<const unsigned char *>(in_data) +
                                                       i * bytesPerRow * metal_texture_instance->GetInfo().Height,
                                                   bytesPerRow,
                                                   bytesPerRow * metal_texture_instance->GetInfo().Height);

  if(texture_info.MipLevels > 1)
  {
    const auto blit_cmd = MetalCommandQueue->commandBuffer();
    const auto blit_enc = blit_cmd->blitCommandEncoder();

    blit_enc->generateMipmaps(metal_texture_instance->texture);

    blit_enc->endEncoding();
    blit_cmd->commit();
    blit_cmd->waitUntilCompleted();
  }

  metal_texture_instance->CreateArgumentBuffer(Device);

  texture_instance.reset(metal_texture_instance);
}

void graphics_metal::MetalGraphicsInstance::CreateRenderBuffer(
    std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
    const GraphicsTypes::FrameBufferInfo                 &info) const
{
  if(render_buffer_instance)
  {
    Log::Error << "Error at render buffer creation, trying to assign already used render buffer instance." << Log::End;
    return;
  }

  render_buffer_instance             = std::make_unique<MetalRenderBufferInstance>(info);
  auto *metal_render_buffer_instance = reinterpret_cast<MetalRenderBufferInstance *>(render_buffer_instance.get());

  constexpr int main_attachment_samples = 1; // type != GraphicsTypes::RenderBufferType::DEPTH ? 1 : samples;

  MTL::TextureDescriptor *framebuffer_descriptor = MTL::TextureDescriptor::alloc()->init();
  framebuffer_descriptor->setTextureType(main_attachment_samples == 1 ? MTL::TextureType2D
                                                                      : MTL::TextureType2DMultisample);
  framebuffer_descriptor->setPixelFormat(GetMetalPixelFormat(info.Format));
  framebuffer_descriptor->setWidth(info.Width);
  framebuffer_descriptor->setHeight(info.Height);
  framebuffer_descriptor->setSampleCount(main_attachment_samples);
  framebuffer_descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
  framebuffer_descriptor->setStorageMode(MTL::StorageModePrivate);

  metal_render_buffer_instance->FrameBuffer.texture = Device->newTexture(framebuffer_descriptor);
  metal_render_buffer_instance->FrameBuffer.CreateArgumentBuffer(Device);
  framebuffer_descriptor->release();

  if(info.TargetType != GraphicsTypes::RenderBufferType::DEPTH && info.Samples > 1)
  {
    MTL::TextureDescriptor *color_descriptor = MTL::TextureDescriptor::alloc()->init();
    color_descriptor->setTextureType(MTL::TextureType2DMultisample);
    color_descriptor->setPixelFormat(GetMetalPixelFormat(info.Format));
    color_descriptor->setWidth(info.Width);
    color_descriptor->setHeight(info.Height);
    color_descriptor->setSampleCount(info.Samples);
    color_descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    color_descriptor->setStorageMode(MTL::StorageModePrivate);

    metal_render_buffer_instance->OptionalColor.texture = Device->newTexture(color_descriptor);
    metal_render_buffer_instance->OptionalColor.CreateArgumentBuffer(Device);
    color_descriptor->release();
  }

  if(info.TargetType == GraphicsTypes::RenderBufferType::BOTH)
  {
    MTL::TextureDescriptor *depth_descriptor = MTL::TextureDescriptor::alloc()->init();
    depth_descriptor->setTextureType(info.Samples > 1 ? MTL::TextureType2DMultisample : MTL::TextureType2D);
    depth_descriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
    depth_descriptor->setWidth(info.Width);
    depth_descriptor->setHeight(info.Height);
    depth_descriptor->setSampleCount(info.Samples);
    depth_descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    depth_descriptor->setStorageMode(MTL::StorageModePrivate);

    metal_render_buffer_instance->OptionalDepth.texture = Device->newTexture(depth_descriptor);
    metal_render_buffer_instance->OptionalDepth.CreateArgumentBuffer(Device);
    depth_descriptor->release();
  }

  metal_render_buffer_instance->RenderPassDescriptorFrameBuffer = MTL::RenderPassDescriptor::alloc()->init();

  if(info.TargetType == GraphicsTypes::RenderBufferType::COLOR ||
     info.TargetType == GraphicsTypes::RenderBufferType::BOTH)
  {
    MTL::RenderPassColorAttachmentDescriptor *colorAttachment =
        metal_render_buffer_instance->RenderPassDescriptorFrameBuffer->colorAttachments()->object(0);

    if(info.Samples > 1)
    {
      colorAttachment->setTexture(metal_render_buffer_instance->OptionalColor.texture);
      colorAttachment->setResolveTexture(metal_render_buffer_instance->FrameBuffer.texture);
    }
    else colorAttachment->setTexture(metal_render_buffer_instance->FrameBuffer.texture);
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(0., .0, .0, 0.0));
    colorAttachment->setStoreAction(info.Samples > 1 ? MTL::StoreActionMultisampleResolve : MTL::StoreActionStore);
  }
  if(info.TargetType == GraphicsTypes::RenderBufferType::DEPTH ||
     info.TargetType == GraphicsTypes::RenderBufferType::BOTH)
  {
    MTL::RenderPassDepthAttachmentDescriptor *depthAttachment =
        metal_render_buffer_instance->RenderPassDescriptorFrameBuffer->depthAttachment();

    depthAttachment->setTexture(info.TargetType == GraphicsTypes::RenderBufferType::BOTH
                                    ? metal_render_buffer_instance->OptionalDepth.texture
                                    : metal_render_buffer_instance->FrameBuffer.texture);
    depthAttachment->setLoadAction(MTL::LoadActionClear);
    depthAttachment->setClearDepth(0.);
    depthAttachment->setStoreAction(MTL::StoreActionStore);
  }
}

GraphicsTypes::TextureInstance *graphics_metal::MetalGraphicsInstance::GetRenderBufferTexture(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance) return nullptr;
  auto *metal_render_buffer_instance = reinterpret_cast<MetalRenderBufferInstance *>(render_buffer_instance);
  return &metal_render_buffer_instance->FrameBuffer;
}

void graphics_metal::MetalGraphicsInstance::DispatchCompute(
    const GraphicsTypes::ShaderInstance         *shader,
    const GraphicsTypes::TextureInstance *const *texture_instance,
    const int                                    count,
    const int                                    num_x,
    const int                                    num_y,
    const int                                    num_z) const
{
  const auto metal_shader = reinterpret_cast<const MetalShaderInstance *>(shader);
  if(!metal_shader || !metal_shader->comp_fn) return;

  NS::Error *error;
  const auto pipeline = Device->newComputePipelineState(metal_shader->comp_fn, &error);

  const auto buffer  = MetalCommandQueue->commandBuffer();
  const auto encoder = buffer->computeCommandEncoder();

  encoder->setComputePipelineState(pipeline);

  if(!texture_instance)
  {
    Log::Error << "Trying to bind invalid texture." << Log::End;
    return;
  }
  auto *const *metal_texture_instances = reinterpret_cast<const MetalTextureInstance *const *>(texture_instance);

  for(int i = 0; i < count; i++)
  {
    encoder->setTexture(metal_texture_instances[i]->texture, i);
  }

  // NS::UInteger threadGroupSize = pipeline->maxTotalThreadsPerThreadgroup();
  const MTL::Size thread_groups(8, 8, 8); // TODO: make this smart
  const MTL::Size thread_groups_per(num_x, num_y, num_z);

  encoder->dispatchThreadgroups(thread_groups_per, thread_groups);
  encoder->endEncoding();
  buffer->commit();
  buffer->waitUntilCompleted();
  pipeline->release();
}

void graphics_metal::MetalGraphicsInstance::CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const
{
  if(!texture_instance) return;
  const auto metal_texture_instance = reinterpret_cast<const MetalTextureInstance *>(texture_instance);

  if(texture_instance->GetInfo().MipLevels <= 1) return;

  const auto blit_cmd = MetalCommandQueue->commandBuffer();
  const auto blit_enc = blit_cmd->blitCommandEncoder();

  blit_enc->generateMipmaps(metal_texture_instance->texture);

  blit_enc->endEncoding();
  blit_cmd->commit();
  blit_cmd->waitUntilCompleted();
}

void graphics_metal::MetalGraphicsInstance::Resize(const int width, const int height)
{
  graphics_metal::Resize(this, width, height);
}

bool graphics_metal::MetalGraphicsInstance::StartDraw(int, int)
{
  DynamicVertexBufferOffset = 0;

  // This moves to StartMain
  if(auto *next_drawable = Layer->nextDrawable())
  {
    CurrentDrawable = next_drawable;
  }
  else
  {
    CurrentDrawable = nullptr;
    return false;
  }

  Resize(static_cast<int>(CurrentDrawable->texture()->width()), static_cast<int>(CurrentDrawable->texture()->height()));
  UpdateRenderPassDescriptor(this);

  // This returns a CommandBufferInstance
  current_buffer = MetalCommandQueue->commandBuffer();

  return true;
}

void graphics_metal::MetalGraphicsInstance::SetState(const GraphicsTypes::RenderState &state) const
{
  current_state.State = state;
}

int graphics_metal::MetalGraphicsInstance::AcquireFrameIndex() const { return 0; }

void graphics_metal::MetalGraphicsInstance::BindShader(const GraphicsTypes::ShaderInstance *shader_instance) const
{
  if(!shader_instance)
  {
    Log::Error << "Trying to bind invalid shader." << Log::End;
    return;
  }
  if(!current_encoder)
  {
    Log::Error << "Trying to bind shader outside draw call." << Log::End;
    return;
  }

  auto *metal_shader_instance = reinterpret_cast<const MetalShaderInstance *>(shader_instance);

  current_state.shader = metal_shader_instance;
}

void graphics_metal::MetalGraphicsInstance::SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const
{
  common_data         = data;
  common_data_changed = true;
}

void graphics_metal::MetalGraphicsInstance::SetColorState(const bool state) const { current_state.color = state; }

void graphics_metal::MetalGraphicsInstance::BindBufferDynamic(std::vector<unsigned char>       &data,
                                                              const GraphicsTypes::UBOBindPoint bind_point) const
{
  switch(bind_point)
  {
  case GraphicsTypes::UBOBindPoint::Common:
    current_encoder->setVertexBytes(data.data(), data.size(), 2);
    current_encoder->setFragmentBytes(data.data(), data.size(), 2);
    break;
  case GraphicsTypes::UBOBindPoint::Specific:
    current_encoder->setVertexBytes(data.data(), data.size(), 3);
    current_encoder->setFragmentBytes(data.data(), data.size(), 3);
    break;
  }
}

void graphics_metal::MetalGraphicsInstance::BindTextures(const GraphicsTypes::TextureInstance *const *texture_instance,
                                                         const int                                    count,
                                                         int) const
{
  if(!texture_instance)
  {
    Log::Error << "Trying to bind invalid texture." << Log::End;
    return;
  }
  const auto *const *metal_texture_instances = reinterpret_cast<const MetalTextureInstance *const *>(texture_instance);

  for(int i = 0; i < count; i++)
  {
    constexpr size_t offset = 4;
    current_encoder->useResource(metal_texture_instances[i]->texture, MTL::ResourceUsageSample);
    current_encoder->setVertexBuffer(metal_texture_instances[i]->argument, 0, i + offset);
    current_encoder->setFragmentBuffer(metal_texture_instances[i]->argument, 0, i + offset);
  }
}

void graphics_metal::MetalGraphicsInstance::BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) const
{
  if(!buffer_instance)
  {
    Log::Error << "Trying to bind invalid buffer." << Log::End;
    return;
  }
  auto *metal_buffer_instance = reinterpret_cast<MetalBufferInstance *>(buffer_instance);

  current_encoder->setVertexBuffer(metal_buffer_instance->Buffer, 0, 0);
}

void graphics_metal::MetalGraphicsInstance::DrawIndexed(const size_t                         count,
                                                        const GraphicsTypes::BufferInstance *buffer_instance,
                                                        const GraphicsTypes::PrimitiveType   prim_type) const
{
  const auto pipeline = GetPipelineForState(this, current_state);

  if(!pipeline)
  {
    Log::Error << "Failed to retrieve pipeline, skipping draw." << Log::End;
    return;
  }

  current_encoder->setRenderPipelineState(pipeline);
  current_encoder->setDepthStencilState(GetDepthStencilForState(this,
                                                                current_state.State.DepthTest,
                                                                current_state.State.DepthWrite,
                                                                current_state.State.DepthCompare));

  current_encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  switch(current_state.State.Culling)
  {
  case GraphicsTypes::CullMode::CULL_BACK:  current_encoder->setCullMode(MTL::CullModeBack); break;
  case GraphicsTypes::CullMode::CULL_FRONT: current_encoder->setCullMode(MTL::CullModeFront); break;
  case GraphicsTypes::CullMode::CULL_NONE:  current_encoder->setCullMode(MTL::CullModeNone); break;
  }

  if(current_state.State.WireFrame) current_encoder->setTriangleFillMode(MTL::TriangleFillModeLines);
  else current_encoder->setTriangleFillMode(MTL::TriangleFillModeFill);

  if(common_data_changed)
  {
    std::vector<unsigned char> ubo_data(ShaderInfo::GetCommonUniformSize());
    ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data.data(), common_data);
    current_encoder->setVertexBytes(ubo_data.data(), ShaderInfo::GetCommonUniformSize(), 2);
    current_encoder->setFragmentBytes(ubo_data.data(), ShaderInfo::GetCommonUniformSize(), 2);
    common_data_changed = false;
  }

  MTL::PrimitiveType mtl_prim_type = MTL::PrimitiveTypeTriangle;
  switch(prim_type)
  {
  case GraphicsTypes::PrimitiveType::TRIANGLES:      mtl_prim_type = MTL::PrimitiveTypeTriangle; break;
  case GraphicsTypes::PrimitiveType::TRIANGLE_STRIP: mtl_prim_type = MTL::PrimitiveTypeTriangleStrip; break;
  case GraphicsTypes::PrimitiveType::LINES:          mtl_prim_type = MTL::PrimitiveTypeLineStrip; break;
  case GraphicsTypes::PrimitiveType::POINTS:         mtl_prim_type = MTL::PrimitiveTypePoint; break;
  }

  if(buffer_instance)
  {
    const auto metal_buffer_instance = reinterpret_cast<const MetalBufferInstance *>(buffer_instance);
    current_encoder
        ->drawIndexedPrimitives(mtl_prim_type, count, MTL::IndexTypeUInt32, metal_buffer_instance->Buffer, 0, 1);
  }
  else
  {
    current_encoder->drawPrimitives(mtl_prim_type, static_cast<NS::UInteger>(0), count);
  }
}

void graphics_metal::MetalGraphicsInstance::DrawDynamic(const size_t                       count,
                                                        const size_t                       type_size,
                                                        const void                        *data,
                                                        const GraphicsTypes::PrimitiveType prim_type) const
{
  const auto pipeline = GetPipelineForState(this, current_state);

  if(!pipeline)
  {
    Log::Error << "Failed to retrieve pipeline, skipping draw." << Log::End;
    return;
  }

  current_encoder->setRenderPipelineState(pipeline);
  current_encoder->setDepthStencilState(GetDepthStencilForState(this,
                                                                current_state.State.DepthTest,
                                                                current_state.State.DepthWrite,
                                                                current_state.State.DepthCompare));

  current_encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  switch(current_state.State.Culling)
  {
  case GraphicsTypes::CullMode::CULL_BACK:  current_encoder->setCullMode(MTL::CullModeBack); break;
  case GraphicsTypes::CullMode::CULL_FRONT: current_encoder->setCullMode(MTL::CullModeFront); break;
  case GraphicsTypes::CullMode::CULL_NONE:  current_encoder->setCullMode(MTL::CullModeNone); break;
  }

  if(current_state.State.WireFrame) current_encoder->setTriangleFillMode(MTL::TriangleFillModeLines);
  else current_encoder->setTriangleFillMode(MTL::TriangleFillModeFill);

  if(common_data_changed)
  {
    std::vector<unsigned char> ubo_data(ShaderInfo::GetCommonUniformSize());
    ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data.data(), common_data);
    current_encoder->setVertexBytes(ubo_data.data(), ShaderInfo::GetCommonUniformSize(), 2);
    current_encoder->setFragmentBytes(ubo_data.data(), ShaderInfo::GetCommonUniformSize(), 2);
    common_data_changed = false;
  }

  MTL::PrimitiveType mtl_prim_type = MTL::PrimitiveTypeTriangle;
  switch(prim_type)
  {
  case GraphicsTypes::PrimitiveType::TRIANGLES:      mtl_prim_type = MTL::PrimitiveTypeTriangle; break;
  case GraphicsTypes::PrimitiveType::TRIANGLE_STRIP: mtl_prim_type = MTL::PrimitiveTypeTriangleStrip; break;
  case GraphicsTypes::PrimitiveType::LINES:          mtl_prim_type = MTL::PrimitiveTypeLineStrip; break;
  case GraphicsTypes::PrimitiveType::POINTS:         mtl_prim_type = MTL::PrimitiveTypePoint; break;
  }

  if(count * type_size < MaxBindBytes) current_encoder->setVertexBytes(data, count * type_size, 0);
  else
  {
    memcpy(static_cast<unsigned char *>(DynamicVertexBuffer->contents()) + DynamicVertexBufferOffset,
           data,
           count * type_size);
    DynamicVertexBuffer->didModifyRange(NS::Range(DynamicVertexBufferOffset, count * type_size));
    current_encoder->setVertexBuffer(DynamicVertexBuffer, DynamicVertexBufferOffset, 0);
    DynamicVertexBufferOffset += count * type_size;
  }

  current_encoder->drawPrimitives(mtl_prim_type, static_cast<NS::UInteger>(0), count);
}

void graphics_metal::MetalGraphicsInstance::BindRenderBuffer(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance)
  {
    Log::Error << "failed to bind invalid render buffer!" << Log::End;
    return;
  }
  const auto metal_render_buffer_instance = reinterpret_cast<MetalRenderBufferInstance *>(render_buffer_instance);

  current_encoder = current_buffer->renderCommandEncoder(metal_render_buffer_instance->RenderPassDescriptorFrameBuffer);

  current_encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  current_encoder->setCullMode(MTL::CullModeBack);

  current_state.Info = metal_render_buffer_instance->GetInfo();
}

void graphics_metal::MetalGraphicsInstance::EndRenderBuffer(GraphicsTypes::RenderBufferInstance *)
{
  current_encoder->endEncoding();
}

void graphics_metal::MetalGraphicsInstance::StartMainRenderPass()
{
  current_encoder = current_buffer->renderCommandEncoder(RenderPassDescriptor);

  current_encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  current_encoder->setCullMode(MTL::CullModeBack);

  current_encoder->setDepthStencilState(GetDepthStencilForState(this,
                                                                current_state.State.DepthTest,
                                                                current_state.State.DepthWrite,
                                                                current_state.State.DepthCompare));

  current_state.Info.Samples  = 4;
  current_state.Info.Format   = GraphicsTypes::ImageFormat::BGRA;
  current_state.Info.HasDepth = true;
  current_state.Info.HasColor = true;
}

void graphics_metal::MetalGraphicsInstance::EndRenderPass()
{
  current_encoder->endEncoding();
  current_encoder = nullptr;
}

void graphics_metal::MetalGraphicsInstance::EndDraw(int, int)
{
  if(CurrentDrawable)
  {
    current_buffer->presentDrawable(CurrentDrawable);
    // swift_msg_draw_finished();
  }
  CurrentDrawable = nullptr;
  current_buffer->commit();
  current_buffer = nullptr;
}

void graphics_metal::MetalGraphicsInstance::Wait() {}

graphics_metal::MetalGraphicsInstance::~MetalGraphicsInstance()
{
  DynamicVertexBuffer->release();

  DepthTexture->release();
  MSAARenderTargetTexture->release();
  RenderPassDescriptor->release();

  for(const auto &[Depth, Test, Write, Compare] : DepthStateList)
    if(Depth) Depth->release();
  for(const auto &[Pipeline, State] : Pipelines)
    if(Pipeline) Pipeline->release();

  SDL_Metal_DestroyView(View);

  MetalCommandQueue->release();
}

void graphics_metal::MessageNewDrawable()
{
  throw std::runtime_error("graphics_metal::MessageNewDrawable, This function is not available in ES");
}
