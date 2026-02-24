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
#ifndef GRAPHICS_LAYER_H
#define GRAPHICS_LAYER_H

#include <memory>
#include <vector>

#include "ShaderInfo.h"
#include "graphics_toplevel_defines.h"

namespace File
{
  struct ShaderString;
}

namespace graphics_layer
{
  size_t GetAlignmentOfType(GraphicsTypes::ShaderType type);
  size_t GetSizeOfType(GraphicsTypes::ShaderType type);

  std::unique_ptr<GraphicsTypes::GraphicsInstance> Init(int width, int height);

  class frame_buffer_handle
  {
    GraphicsTypes::GraphicsInstance *Instance;

    uint32_t                        Width;
    uint32_t                        Height;
    GraphicsTypes::RenderBufferType Type;
    uint32_t                        Samples;

    std::unique_ptr<GraphicsTypes::RenderBufferInstance> frame_buffer;

  public:
    frame_buffer_handle(GraphicsTypes::GraphicsInstance *instance,
                        uint32_t                         width,
                        uint32_t                         height,
                        GraphicsTypes::RenderBufferType  type,
                        uint32_t                         samples);
    ~frame_buffer_handle();

    frame_buffer_handle(const frame_buffer_handle &other)                = delete;
    frame_buffer_handle(frame_buffer_handle &&other) noexcept            = delete;
    frame_buffer_handle &operator=(const frame_buffer_handle &other)     = delete;
    frame_buffer_handle &operator=(frame_buffer_handle &&other) noexcept = delete;

    void Bind() const;
    void Finish() const;

    // Avoid at all cost.
    void Resize(uint32_t width, uint32_t height);

    [[nodiscard]] const GraphicsTypes::TextureInstance *GetTexture() const;
  };

  class ObjectHandle
  {
    const GraphicsTypes::GraphicsInstance *Instance;

    std::unique_ptr<GraphicsTypes::BufferInstance> VertexBuffer = nullptr;
    std::unique_ptr<GraphicsTypes::BufferInstance> IndexBuffer  = nullptr;

    size_t VertexBufferSize = 0;
    size_t Size             = 0;

  public:
    ObjectHandle() = default;
    ObjectHandle(const GraphicsTypes::GraphicsInstance *instance,
                 size_t                                 size,
                 size_t                                 type_size,
                 const void                            *in_data,
                 const std::vector<uint32_t>           &indices);
    ~ObjectHandle();

    ObjectHandle(const ObjectHandle &other) = delete;
    ObjectHandle(ObjectHandle &&other) noexcept :
      Instance(other.Instance), VertexBufferSize(other.VertexBufferSize), Size(other.Size)
    {
      this->VertexBuffer.swap(other.VertexBuffer);
      this->IndexBuffer.swap(other.IndexBuffer);
    }
    ObjectHandle &operator=(const ObjectHandle &other) = delete;
    ObjectHandle &operator=(ObjectHandle &&other) noexcept
    {
      this->Instance = other.Instance;
      this->VertexBuffer.swap(other.VertexBuffer);
      this->IndexBuffer.swap(other.IndexBuffer);
      this->Size             = other.Size;
      this->VertexBufferSize = other.VertexBufferSize;

      return *this;
    }

    void Draw(GraphicsTypes::PrimitiveType prim_type) const;
  };

  class RotatingObjectBuffer
  {
    const GraphicsTypes::GraphicsInstance *Instance;

    std::vector<std::unique_ptr<GraphicsTypes::BufferInstance>> VertexBuffers{};
    std::vector<std::unique_ptr<GraphicsTypes::BufferInstance>> IndexBuffers{};

    size_t CurrentIndex = 0;

    size_t VertexBufferSize = 0;
    size_t Size             = 0;

  public:
    RotatingObjectBuffer() = default;
    RotatingObjectBuffer(const GraphicsTypes::GraphicsInstance *instance,
                         size_t                                 size,
                         size_t                                 type_size,
                         const void                            *in_data,
                         const std::vector<uint32_t>           &indices);
    ~RotatingObjectBuffer();

    RotatingObjectBuffer(const RotatingObjectBuffer &other) = delete;
    RotatingObjectBuffer(RotatingObjectBuffer &&other) noexcept :
      Instance(other.Instance), VertexBufferSize(other.VertexBufferSize), Size(other.Size)
    {
      VertexBuffers.swap(other.VertexBuffers);
      IndexBuffers.swap(other.IndexBuffers);
    }
    RotatingObjectBuffer &operator=(const RotatingObjectBuffer &other) = delete;
    RotatingObjectBuffer &operator=(RotatingObjectBuffer &&other) noexcept
    {
      this->Instance = other.Instance;
      this->VertexBuffers.swap(other.VertexBuffers);
      this->IndexBuffers.swap(other.IndexBuffers);
      this->Size             = other.Size;
      this->VertexBufferSize = other.VertexBufferSize;

      return *this;
    }

    void Draw() const;

    void ReplaceVerticesAndIndices(const void *data, size_t size, const std::vector<uint32_t> &indices) const;
  };

  class TextureHandle
  {
    const GraphicsTypes::GraphicsInstance *Instance;

    std::unique_ptr<GraphicsTypes::TextureInstance> Texture;
    int                                             Width = 0, Height = 0;

  public:
    TextureHandle() = default;
    TextureHandle(const GraphicsTypes::GraphicsInstance *instance,
                  const std::vector<void *>             &data,
                  int                                    width,
                  int                                    height,
                  int                                    depth,
                  GraphicsTypes::TextureType             type,
                  GraphicsTypes::ImageFormat             format,
                  GraphicsTypes::TextureTarget           target,
                  GraphicsTypes::TextureAddressMode      address_mode = GraphicsTypes::TextureAddressMode::REPEAT,
                  GraphicsTypes::TextureFilter           filter       = GraphicsTypes::TextureFilter::LINEAR);
    TextureHandle(const GraphicsTypes::GraphicsInstance *instance,
                  int                                    width,
                  int                                    height,
                  int                                    depth,
                  GraphicsTypes::TextureType             type,
                  GraphicsTypes::ImageFormat             format,
                  GraphicsTypes::TextureTarget           target,
                  GraphicsTypes::TextureAddressMode      address_mode = GraphicsTypes::TextureAddressMode::REPEAT,
                  GraphicsTypes::TextureFilter           filter       = GraphicsTypes::TextureFilter::LINEAR);
    ~TextureHandle();

    TextureHandle(const TextureHandle &other) = delete;
    TextureHandle(TextureHandle &&other) noexcept : Instance(other.Instance)
    {
      this->Texture.swap(other.Texture);
      Width  = other.Width;
      Height = other.Height;
    };
    TextureHandle &operator=(const TextureHandle &other) = delete;
    TextureHandle &operator=(TextureHandle &&other) noexcept
    {
      this->Instance = other.Instance;
      this->Texture.swap(other.Texture);
      this->Width  = other.Width;
      this->Height = other.Height;

      return *this;
    }

    void CreateMipMaps() const;

    GraphicsTypes::TextureInstance                     *GetTexture() { return Texture.get(); }
    [[nodiscard]] const GraphicsTypes::TextureInstance *GetTexture() const { return Texture.get(); }
  };


  class TextureList
  {
    struct TexEntry
    {
      const GraphicsTypes::TextureInstance *tex;
      int                                   id;
      bool                                  vertex;
    };
    std::vector<TexEntry> Textures;


  public:
    TextureList() = default;

    void AddTexture(const GraphicsTypes::TextureInstance *tex, unsigned int id, bool vertex);

    void Bind(const GraphicsTypes::GraphicsInstance *instance) const;
  };
} // namespace graphics_layer
#endif // GRAPHICS_LAYER_H
