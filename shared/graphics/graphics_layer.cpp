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
} // namespace

size_t graphics_layer::GetAlignmentOfType(const GraphicsTypes::ShaderType type)
{
  switch(type)
  {
    case GraphicsTypes::ShaderType::INT:
    case GraphicsTypes::ShaderType::FLOAT:  return 4;
    case GraphicsTypes::ShaderType::INT2:   return 8;
    case GraphicsTypes::ShaderType::INT3:
    case GraphicsTypes::ShaderType::INT4:   return 16;
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
    case GraphicsTypes::ShaderType::FLOAT:  return 4;
    case GraphicsTypes::ShaderType::INT2:   return 8;
    case GraphicsTypes::ShaderType::INT4:   return 16;
    case GraphicsTypes::ShaderType::FLOAT2: return 8;
    case GraphicsTypes::ShaderType::FLOAT4: return 16;
#if defined(__APPLE__) || defined(ASL_BUILD_WASM)
    case GraphicsTypes::ShaderType::MAT2: return 16;
#else
    case GraphicsTypes::ShaderType::MAT2: return 32;
#endif
    case GraphicsTypes::ShaderType::MAT3: return 48;
    case GraphicsTypes::ShaderType::MAT4: return 64;
#if defined(__APPLE__) || defined(ASL_BUILD_WASM)
    case GraphicsTypes::ShaderType::INT3:
    case GraphicsTypes::ShaderType::FLOAT3: return 16;
#else
    case GraphicsTypes::ShaderType::INT3:
    case GraphicsTypes::ShaderType::FLOAT3: return 16;
#endif
  }
  throw std::runtime_error("ShaderType size not implemented");
}

std::unique_ptr<GraphicsTypes::GraphicsInstance> graphics_layer::Init(SDL_Window *window, const int width, const int height)
{
#if defined(__APPLE__)
  return std::make_unique<graphics_metal::MetalGraphicsInstance>(window);
#elif defined(ASL_BUILD_WASM)
  return std::make_unique<graphics_wgpu::WGPUGraphicsInstance>(window);
#else
  return std::make_unique<graphics_vulkan::VulkanGraphicsInstance>(window, width, height);
#endif
}

namespace graphics_layer
{
  FrameBufferHandle::FrameBufferHandle(
      GraphicsTypes::GraphicsInstance *instance,
      const uint32_t                   width,
      const uint32_t                   height,
      const GraphicsTypes::ImageFormat color_format,
      const GraphicsTypes::ImageFormat depth_format,
      const uint32_t                   samples,
      const std::string_view           name) :
    Instance(instance), Width(width), Height(height), ColorFormat(color_format), DepthFormat(depth_format), Samples(samples), Name(name)
  {
    GraphicsTypes::FrameBufferInfo create_info;
    create_info.Width       = Width;
    create_info.Height      = Height;
    create_info.Samples     = Samples;
    create_info.ColorFormat = ColorFormat;
    create_info.DepthFormat = DepthFormat;
    create_info.Presenter   = false;
    Instance->CreateRenderBuffer(FrameBuffer, create_info, Name);
  }

  FrameBufferHandle::~FrameBufferHandle() = default;

  void FrameBufferHandle::Bind(GraphicsTypes::CommandBufferInstance *cb) const
  {
    cb->SetColorState(ColorFormat != GraphicsTypes::ImageFormat::INVALID);
    cb->BindRenderBuffer(FrameBuffer.get());
  }

  void FrameBufferHandle::Finish(GraphicsTypes::CommandBufferInstance *cb) const
  {
    cb->EndRenderBuffer(FrameBuffer.get());
  }

  void FrameBufferHandle::Resize(const uint32_t width, const uint32_t height)
  {
    FrameBuffer.reset();
    Width  = width;
    Height = height;

    GraphicsTypes::FrameBufferInfo create_info;
    create_info.Width       = Width;
    create_info.Height      = Height;
    create_info.Samples     = Samples;
    create_info.ColorFormat = ColorFormat;
    create_info.DepthFormat = DepthFormat;
    create_info.Presenter   = false;

    Instance->CreateRenderBuffer(FrameBuffer, create_info, Name);
  }

  const GraphicsTypes::TextureInstance *FrameBufferHandle::GetTexture() const
  {
    return Instance->GetRenderBufferTexture(FrameBuffer.get());
  }

  ObjectHandle::ObjectHandle(
      const GraphicsTypes::GraphicsInstance *instance,
      const size_t                           size,
      const size_t                           type_size,
      const void                            *in_data,
      const std::vector<uint32_t>           &indices,
      const std::string_view                 name) :
    Instance(instance), VertexBufferSize(size * type_size), Size(!indices.empty() ? indices.size() : size)
  {
    {
#if !defined(__APPLE__) && !defined(ASL_BUILD_WASM)
      std::unique_ptr<GraphicsTypes::BufferInstance> staging_buffer;
      Instance->CreateBuffer(staging_buffer, GraphicsTypes::BufferType::STAGING, VertexBufferSize, std::string(name) + "_vert_staging");

      Instance->MapBuffer(staging_buffer.get(), in_data);

      Instance->CreateBuffer(VertexBuffer, GraphicsTypes::BufferType::VERTEX, VertexBufferSize, std::string(name) + "_vert");
      Instance->CopyBuffer(VertexBuffer.get(), staging_buffer.get());
#else
      Instance->CreateBuffer(VertexBuffer, GraphicsTypes::BufferType::VERTEX, VertexBufferSize, in_data, name);
#endif
    }

    // Now the index buffer:
    if(!indices.empty())
    {
      const size_t index_buffer_size = indices.size() * sizeof(indices[0]);
#if !defined(__APPLE__) && !defined(ASL_BUILD_WASM)
      std::unique_ptr<GraphicsTypes::BufferInstance> staging_buffer;
      Instance->CreateBuffer(staging_buffer, GraphicsTypes::BufferType::STAGING, index_buffer_size, std::string(name) + "_index_staging");

      Instance->MapBuffer(staging_buffer.get(), indices.data());

      Instance->CreateBuffer(IndexBuffer, GraphicsTypes::BufferType::INDEX, index_buffer_size, std::string(name) + "_index");
      Instance->CopyBuffer(IndexBuffer.get(), staging_buffer.get());
#else
      Instance->CreateBuffer(IndexBuffer, GraphicsTypes::BufferType::INDEX, index_buffer_size, indices.data(), name);
#endif
    }
  }

  ObjectHandle::~ObjectHandle() = default;

  void ObjectHandle::Draw(
      GraphicsTypes::CommandBufferInstance *cb,
      const GraphicsTypes::PrimitiveType    prim_type,
      const int                             start,
      const int                             end) const
  {
    cb->BindVertexBuffer(VertexBuffer.get());
    cb->DrawIndexed(start, end > 0 ? end : Size, IndexBuffer.get(), prim_type);
  }

  void ObjectHandle::DrawInstanced(
      GraphicsTypes::CommandBufferInstance *cb,
      const GraphicsTypes::PrimitiveType    prim_type,
      GraphicsTypes::BufferInstance        *instance_buffer,
      const unsigned                        instance_count,
      const int                             start,
      const int                             end) const
  {
    cb->BindVertexBuffer(VertexBuffer.get());
    cb->BindInstanceBuffer(instance_buffer);
    cb->DrawIndexedInstanced(instance_count, start, end > 0 ? end : Size, IndexBuffer.get(), prim_type);
  }

  TextureHandle::TextureHandle(
      const GraphicsTypes::GraphicsInstance  *instance,
      const std::string_view                  path,
      const bool                              bpd,
      const GraphicsTypes::TextureAddressMode address_mode,
      const GraphicsTypes::TextureFilter      filter) :
    Instance(instance)
  {

    GraphicsTypes::TextureInfo info;
    info.Type        = GraphicsTypes::TextureType::TYPE_2D;
    info.Layers      = 1;
    info.AddressMode = address_mode;
    info.Filter      = filter;

    if(!bpd)
    {
      int        width, height, channels;
      const auto pixels = File::ReadImage(path, width, height, channels);

      int min_dim = std::min(width, height);
      // Calculate mip levels
      int mip_levels = 1;
      while(true)
      {
        if(min_dim < 2)
          break;
        min_dim /= 2;
        mip_levels++;
      }
      info.Format    = GraphicsTypes::ImageFormat::RGBA;
      info.Width     = width;
      info.Height    = height;
      info.MipLevels = mip_levels;

      Instance->CreateTexture(Texture, info, { pixels.Get() }, path);
    }
    else
    {
      asl::list<unsigned char> pixels;
      asl::uint8               components, bytes;
      asl::uint32              width;
      File::ReadBPD(path, pixels, width, components, bytes);

      if(pixels.empty())
        return;
      if(components == 0 || components > 4)
      {
        Log::Error << "Invalid component count for bpd texture." << Log::End;
        return;
      }
      if((components != 4 && bytes > 1) || bytes == 0)
      {
        Log::Error << "Invalid byte count for bpd texture." << Log::End;
        return;
      }

      switch(components)
      {
        case 1: info.Format = GraphicsTypes::ImageFormat::R; break;
        case 2: info.Format = GraphicsTypes::ImageFormat::RG; break;
        case 3: info.Format = GraphicsTypes::ImageFormat::RGB; break;
        case 4:
        {
          switch(bytes)
          {
            case 1: info.Format = GraphicsTypes::ImageFormat::RGBA; break;
            case 2: info.Format = GraphicsTypes::ImageFormat::RGBA16F; break;
            case 4: info.Format = GraphicsTypes::ImageFormat::RGBA32F; break;
            default:
            {
              Log::Error << "Invalid byte count for bpd image." << Log::End;
              return;
            }
          }
          break;
        }
        default: Log::Error << "Invalid component count for bpd image." << Log::End; return;
      }
      info.Width  = width;
      info.Height = ((pixels.size() / bytes) / components) / width;


      Instance->CreateTexture(Texture, info, { pixels.data() }, path);
    }
  }

  TextureHandle::TextureHandle(
      const GraphicsTypes::GraphicsInstance  *instance,
      const std::vector<std::string_view>    &paths,
      const GraphicsTypes::TextureAddressMode address_mode,
      const GraphicsTypes::TextureFilter      filter) :
    Instance(instance)
  {
    std::vector<File::PixelHandle> handles;
    std::vector<const void *>      final_pixels;
    int                            final_width = -1, final_height = -1;
    for(const auto &path : paths)
    {
      int width, height, channels;
      handles.emplace_back(File::ReadImage(path, width, height, channels));
      final_pixels.emplace_back(handles.back().Get());

      if(final_width == -1 && final_height == -1)
      {
        final_width  = width;
        final_height = height;
      }
      else if(final_width != width && final_height != height)
      {
        Log::Error << "Error: trying to load textures of different sizes into array" << Log::End;
        return;
      }
    }

    int min_dim = std::min(final_width, final_height);
    // Calculate mip levels
    int mip_levels = 1;
    while(true)
    {
      if(min_dim < 2)
        break;
      min_dim /= 2;
      mip_levels++;
    }

    GraphicsTypes::TextureInfo info;
    info.Width       = final_width;
    info.Height      = final_height;
    info.Layers      = final_pixels.size();
    info.Format      = GraphicsTypes::ImageFormat::RGBA;
    info.MipLevels   = mip_levels;
    info.Type        = GraphicsTypes::TextureType::TYPE_2D_ARRAY;
    info.AddressMode = address_mode;
    info.Filter      = filter;

    Instance->CreateTexture(Texture, info, final_pixels, paths.front());
  }

  TextureHandle::TextureHandle(
      const GraphicsTypes::GraphicsInstance  *instance,
      const std::vector<std::string>         &paths,
      const GraphicsTypes::TextureAddressMode address_mode,
      const GraphicsTypes::TextureFilter      filter) :
    Instance(instance)
  {
    std::vector<File::PixelHandle> handles;
    std::vector<const void *>      final_pixels;
    int                            final_width = -1, final_height = -1;
    for(const auto &path : paths)
    {
      int width, height, channels;
      handles.emplace_back(File::ReadImage(path, width, height, channels));
      final_pixels.emplace_back(handles.back().Get());

      if(final_width == -1 && final_height == -1)
      {
        final_width  = width;
        final_height = height;
      }
      else if(final_width != width && final_height != height)
      {
        Log::Error << "Error: trying to load textures of different sizes into array" << Log::End;
        return;
      }
    }

    int min_dim = std::min(final_width, final_height);
    // Calculate mip levels
    int mip_levels = 1;
    while(true)
    {
      if(min_dim < 2)
        break;
      min_dim /= 2;
      mip_levels++;
    }

    GraphicsTypes::TextureInfo info;
    info.Width       = final_width;
    info.Height      = final_height;
    info.Layers      = final_pixels.size();
    info.MipLevels   = mip_levels;
    info.Format      = GraphicsTypes::ImageFormat::RGBA;
    info.Type        = GraphicsTypes::TextureType::TYPE_2D_ARRAY;
    info.AddressMode = address_mode;
    info.Filter      = filter;

    Instance->CreateTexture(Texture, info, final_pixels, paths.front());
  }

  TextureHandle::TextureHandle(
      const GraphicsTypes::GraphicsInstance  *instance,
      const std::string_view                  name,
      const std::vector<const void *>        &data,
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
      if(mip_width > 1 && mip_height > 1)
        info.MipLevels += 1;
      else
        break;
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
    info.Type        = type;
    info.Format      = format;
    info.Target      = target;
    info.AddressMode = address_mode;
    info.Filter      = filter;
    Instance->CreateTexture(Texture, info, data, name);
  }


  TextureHandle::TextureHandle(
      const GraphicsTypes::GraphicsInstance  *instance,
      const std::string_view                  name,
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
      if(mip_width > 1 && mip_height > 1)
        info.MipLevels += 1;
      else
        break;
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
    info.Type        = type;
    info.Format      = format;
    info.Target      = target;
    info.AddressMode = address_mode;
    info.Filter      = filter;
    Instance->CreateTexture(Texture, info, {}, name);
  }

  TextureHandle::~TextureHandle() = default;

  void TextureHandle::Reset()
  {
    Texture.reset();
  }

  void TextureHandle::CreateMipMaps() const
  {
    Instance->CreateMipMaps(Texture.get());
  }

  void TextureList::AddTexture(const GraphicsTypes::TextureInstance *tex, const unsigned int id, const bool vertex)
  {
    TexEntry entry{};
    entry.tex    = tex;
    entry.id     = static_cast<int>(id);
    entry.vertex = vertex;
    for(auto &e : Textures)
    {
      if(e.id == entry.id)
      {
        e = entry;
        return;
      }
    }
    Textures.emplace_back(entry);
  }

  void TextureList::Bind(GraphicsTypes::CommandBufferInstance *cb) const
  {
    std::vector<const GraphicsTypes::TextureInstance *> raw_data;
    raw_data.reserve(Textures.size());
    for(const auto &tex : Textures)
      raw_data.emplace_back(tex.tex);
    cb->BindTextures(raw_data.data(), static_cast<int>(raw_data.size()), 2);
  }

  void QuickBindTextures(GraphicsTypes::CommandBufferInstance *cb, const std::vector<const GraphicsTypes::TextureInstance *> &textures)
  {
    TextureList texture_list;
    for(size_t i = 0; i < textures.size(); i++)
      texture_list.AddTexture(textures[i], i, true);
    texture_list.Bind(cb);
  }
} // namespace graphics_layer

#endif
