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
//_
#ifndef GRAPHICS_LAYER_IMPLEMENTATION
#define GRAPHICS_LAYER_IMPLEMENTATION

#include "graphics_layer.h"
#if defined(__APPLE__)
#include "metal/graphics_metal.h"
#elif defined(ASL_BUILD_WASM)
#include "wgpu/graphics_wgpu.h"
#else
#include "vulkan/graphics_vulkan.h"
#endif

#include "graphics/graphics_toplevel_defines.h"
#include "system/File.h"
#include "system/Log.h"

#include <cstring>

namespace
{
  uint32_t GetComponentsOfFormat(const GraphicsTypes::ImageFormat format)
  {
    switch(format)
    {
    case GraphicsTypes::ImageFormat::R: return 1;
    case GraphicsTypes::ImageFormat::RG: return 2;
    case GraphicsTypes::ImageFormat::RGB:
    case GraphicsTypes::ImageFormat::RGBA:
    case GraphicsTypes::ImageFormat::RGBA16F:
    case GraphicsTypes::ImageFormat::RGBA32F:
    case GraphicsTypes::ImageFormat::BGRA: return 4;
    case GraphicsTypes::ImageFormat::DEPTH: return 1;
    case GraphicsTypes::ImageFormat::INVALID: return 0;
    }
    throw std::runtime_error("Missing Format case in GetComponentsOfFormat");
  }
} // namespace

size_t graphics_layer::GetAlignmentOfType(const GraphicsTypes::ShaderType type)
{
  switch(type)
  {
  case GraphicsTypes::ShaderType::INT:
  case GraphicsTypes::ShaderType::FLOAT: return 4;
  case GraphicsTypes::ShaderType::INT2: return 8;
  case GraphicsTypes::ShaderType::INT3:
  case GraphicsTypes::ShaderType::INT4: return 16;
  case GraphicsTypes::ShaderType::FLOAT2: return 8;
  case GraphicsTypes::ShaderType::FLOAT3:
  case GraphicsTypes::ShaderType::FLOAT4: return 16;
#if defined(__APPLE__)
  case GraphicsTypes::ShaderType::MAT2: return 8;
#else
  case GraphicsTypes::ShaderType::MAT2:
#endif
  case GraphicsTypes::ShaderType::MAT3:
  case GraphicsTypes::ShaderType::MAT4: return 16;
  }
  throw std::runtime_error("ShaderType alignment not implemented");
}

size_t graphics_layer::GetSizeOfType(const GraphicsTypes::ShaderType type)
{
  switch(type)
  {
  case GraphicsTypes::ShaderType::INT:
  case GraphicsTypes::ShaderType::FLOAT: return 4;
  case GraphicsTypes::ShaderType::INT2: return 8;
  case GraphicsTypes::ShaderType::INT4: return 16;
  case GraphicsTypes::ShaderType::FLOAT2: return 8;
  case GraphicsTypes::ShaderType::FLOAT4:
  case GraphicsTypes::ShaderType::MAT2: return 16;
  case GraphicsTypes::ShaderType::MAT3: return 48;
  case GraphicsTypes::ShaderType::MAT4: return 64;
#if defined(__APPLE__) || defined(ASL_BUILD_WASM)
  case GraphicsTypes::ShaderType::INT3:
  case GraphicsTypes::ShaderType::FLOAT3: return 16;
#else
  case GraphicsTypes::ShaderType::INT3: return 12;
  case GraphicsTypes::ShaderType::FLOAT3: return 12;
#endif
  }
  throw std::runtime_error("ShaderType size not implemented");
}

std::unique_ptr<GraphicsTypes::GraphicsInstance> graphics_layer::Init(const int width, const int height)
{
#if defined(__APPLE__)
  return std::make_unique<graphics_metal::MetalGraphicsInstance>(width, height);
#elif defined(ASL_BUILD_WASM)
  return std::make_unique<graphics_wgpu::WGPUGraphicsInstance>(width, height);
#else
  return std::make_unique<graphics_vulkan::VulkanGraphicsInstance>(width, height);
#endif
}

namespace graphics_layer
{
  frame_buffer_handle::frame_buffer_handle(GraphicsTypes::GraphicsInstance *instance, const uint32_t width, const uint32_t height, const GraphicsTypes::RenderBufferType type, const uint32_t samples) :
    Instance(instance), Width(width), Height(height), Type(type), Samples(samples)
  {
    GraphicsTypes::FrameBufferInfo create_info;
    create_info.Width      = width;
    create_info.Height     = height;
    create_info.Samples    = Samples;
    create_info.HasColor   = Type != GraphicsTypes::RenderBufferType::DEPTH;
    create_info.HasDepth   = Type != GraphicsTypes::RenderBufferType::COLOR;
    create_info.Presenter  = false;
    create_info.TargetType = Type;
    create_info.Format     = Type == GraphicsTypes::RenderBufferType::DEPTH ? GraphicsTypes::ImageFormat::DEPTH : GraphicsTypes::ImageFormat::RGBA;
    Instance->CreateRenderBuffer(frame_buffer, create_info);
  }

  frame_buffer_handle::~frame_buffer_handle() = default;

  void frame_buffer_handle::Bind() const
  {
    Instance->BindRenderBuffer(frame_buffer.get());
    Instance->SetColorState(Type != GraphicsTypes::RenderBufferType::DEPTH);
  }

  void frame_buffer_handle::Finish() const { Instance->EndRenderBuffer(frame_buffer.get()); }

  void frame_buffer_handle::Resize(const uint32_t width, const uint32_t height)
  {
    frame_buffer.reset();
    Width  = width;
    Height = height;

    GraphicsTypes::FrameBufferInfo create_info;
    create_info.Width      = width;
    create_info.Height     = height;
    create_info.Samples    = Samples;
    create_info.HasColor   = Type != GraphicsTypes::RenderBufferType::DEPTH;
    create_info.HasDepth   = Type != GraphicsTypes::RenderBufferType::COLOR;
    create_info.Presenter  = false;
    create_info.TargetType = Type;
    create_info.Format     = Type == GraphicsTypes::RenderBufferType::DEPTH ? GraphicsTypes::ImageFormat::DEPTH : GraphicsTypes::ImageFormat::RGBA;
    Instance->CreateRenderBuffer(frame_buffer, create_info);
  }

  const GraphicsTypes::TextureInstance *frame_buffer_handle::GetTexture() const { return Instance->GetRenderBufferTexture(frame_buffer.get()); }

  ObjectHandle::ObjectHandle(const GraphicsTypes::GraphicsInstance *instance, const size_t size, const size_t type_size, const void *in_data, const std::vector<uint32_t> &indices) :
    Instance(instance), VertexBufferSize(size * type_size), Size(!indices.empty() ? indices.size() : size)
  {
    {
#if !defined(__APPLE__) && !defined(ASL_BUILD_WASM)
      std::unique_ptr<GraphicsTypes::BufferInstance> staging_buffer;
      Instance->CreateBuffer(staging_buffer, GraphicsTypes::BufferType::STAGING, VertexBufferSize);

      Instance->MapBuffer(staging_buffer.get(), in_data);

      Instance->CreateBuffer(VertexBuffer, GraphicsTypes::BufferType::VERTEX, VertexBufferSize);
      Instance->CopyBuffer(VertexBuffer.get(), staging_buffer.get());
#else
      Instance->CreateBuffer(VertexBuffer, GraphicsTypes::BufferType::VERTEX, VertexBufferSize, in_data);
#endif
    }

    // Now the index buffer:
    if(!indices.empty())
    {
      const size_t index_buffer_size = indices.size() * sizeof(indices[0]);
#if !defined(__APPLE__) && !defined(ASL_BUILD_WASM)
      std::unique_ptr<GraphicsTypes::BufferInstance> staging_buffer;
      Instance->CreateBuffer(staging_buffer, GraphicsTypes::BufferType::STAGING, index_buffer_size);

      Instance->MapBuffer(staging_buffer.get(), indices.data());

      Instance->CreateBuffer(IndexBuffer, GraphicsTypes::BufferType::INDEX, index_buffer_size);
      Instance->CopyBuffer(IndexBuffer.get(), staging_buffer.get());
#else
      Instance->CreateBuffer(IndexBuffer, GraphicsTypes::BufferType::INDEX, index_buffer_size, indices.data());
#endif
    }
  }

  ObjectHandle::~ObjectHandle() = default;

  void ObjectHandle::Draw(const GraphicsTypes::PrimitiveType prim_type, int start, int end) const
  {
    Instance->BindVertexBuffer(VertexBuffer.get());
    Instance->DrawIndexed(start, end > 0 ? end : Size, IndexBuffer.get(), prim_type);
  }

  TextureHandle::TextureHandle(const GraphicsTypes::GraphicsInstance *instance,
                               const void                            *data,
                               const int                              width,
                               const int                              height,
                               const int                              depth,
                               const GraphicsTypes::TextureType       type,
                               const GraphicsTypes::ImageFormat       format,
                               const GraphicsTypes::TextureTarget     target,
                               const GraphicsTypes::TextureAddressMode address_mode,
                               const GraphicsTypes::TextureFilter      filter) :
    Instance(instance)
  {
    assert((depth == 1 || type != GraphicsTypes::TextureType::TYPE_2D) && "Select a texture type with 3d support (array or 3d)!");
    GraphicsTypes::TextureInfo info;
    info.Width = width;

    info.MipLevels = 1;
    int mip_width = width, mip_height = height;
    while(true)
    {
      if(mip_width > 1 && mip_height > 1) info.MipLevels += 1;
      else break;
      mip_width  = mip_width / 2;
      mip_height = mip_height / 2;
    }

    info.Height     = height;
    info.Components = GetComponentsOfFormat(format);
    if(type == GraphicsTypes::TextureType::TYPE_3D)
    {
      info.Layers = 1;
      info.Depth  = depth;
    }
    else if(type == GraphicsTypes::TextureType::TYPE_2D_ARRAY)
    {
      info.Layers = depth;
      info.Depth  = 1;
    }
    else
    {
      info.Layers = 1;
      info.Depth  = 1;
    }
    info.Type   = type;
    info.Format = format;
    info.Target = target;
    info.AddressMode = address_mode;
    info.Filter = filter;
    Instance->CreateTexture(Texture, info, data);
  }


  TextureHandle::TextureHandle(const GraphicsTypes::GraphicsInstance  *instance,
                               const int                               width,
                               const int                               height,
                               const int                               depth,
                               const GraphicsTypes::TextureType        type,
                               const GraphicsTypes::ImageFormat        format,
                               const GraphicsTypes::TextureTarget      target,
                               const GraphicsTypes::TextureAddressMode address_mode,
                               const GraphicsTypes::TextureFilter      filter) :
    Instance(instance)
  {
    assert((depth == 1 || type != GraphicsTypes::TextureType::TYPE_2D) && "Select a texture type with 3d support (array or 3d)!");
    GraphicsTypes::TextureInfo info;
    info.Width = width;

    info.MipLevels = 1;
    int mip_width = width, mip_height = height;
    while(true)
    {
      if(mip_width > 1 && mip_height > 1) info.MipLevels += 1;
      else break;
      mip_width  = mip_width / 2;
      mip_height = mip_height / 2;
    }

    info.Height     = height;
    info.Components = GetComponentsOfFormat(format);
    if(type == GraphicsTypes::TextureType::TYPE_3D)
    {
      info.Layers = 1;
      info.Depth  = depth;
    }
    else if(type == GraphicsTypes::TextureType::TYPE_2D_ARRAY)
    {
      info.Layers = depth;
      info.Depth  = 1;
    }
    else
    {
      info.Layers = 1;
      info.Depth  = 1;
    }
    info.Type   = type;
    info.Format = format;
    info.Target = target;
    info.AddressMode = address_mode;
    info.Filter = filter;
    Instance->CreateTexture(Texture, info, {});
  }

  TextureHandle::~TextureHandle() = default;

  void TextureHandle::CreateMipMaps() const { Instance->CreateMipMaps(Texture.get()); }

  void TextureList::AddTexture(const GraphicsTypes::TextureInstance *tex, const unsigned int id, const bool vertex)
  {
    TexEntry entry{};
    entry.tex    = tex;
    entry.id     = static_cast<int>(id);
    entry.vertex = vertex;
    for(auto &e : Textures)
      if(e.id == entry.id)
      {
        e = entry;
        return;
      }
    Textures.emplace_back(entry);
  }

  void TextureList::Bind(const GraphicsTypes::GraphicsInstance *instance) const
  {
    std::vector<const GraphicsTypes::TextureInstance *> raw_data;
    raw_data.reserve(Textures.size());
    for(const auto &tex : Textures)
      raw_data.emplace_back(tex.tex);
    instance->BindTextures(raw_data.data(), static_cast<int>(raw_data.size()), 2);
  }
} // namespace graphics_layer

#endif
