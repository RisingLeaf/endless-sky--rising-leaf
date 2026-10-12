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
#include "graphics_wgpu.h"

#include "shared/graphics/ShaderInfo.h"
#include "shared/system/File.h"
#include "shared/system/Log.h"
#include "shared/system/Window.h"

#include <memory>
#include <mutex>

#include <GLES3/gl32.h>


namespace graphics_wgpu
{
  constexpr size_t DYNAMIC_UNIFORM_BUFFER_SIZE = 1024 * 1024 * 4; // 4 MB

  struct WGPUShaderInstance final : GraphicsTypes::ShaderInstance
  {
    const ShaderInfo &shader_info;

    GLuint ShaderId = 0;

    GLuint VAO = 0;

    GLuint CommonUBOLoc   = 0;
    GLuint SpecificUBOLoc = 0;

    std::vector<GLint> TextureLocations;

    explicit WGPUShaderInstance(const ShaderInfo &s) : shader_info(s) {}

    ~WGPUShaderInstance() override
    {
      if(ShaderId)
        glDeleteProgram(ShaderId);
    }
  };

  struct WGPUPipelineState
  {
    bool                           color = true;
    GraphicsTypes::FrameBufferInfo Info;
    GraphicsTypes::RenderState     State;
    const WGPUShaderInstance      *shader = nullptr;

    bool operator==(const WGPUPipelineState &other) const
    {
      return this->color == other.color && this->shader == other.shader && this->Info == other.Info && this->State == other.State;
    }
  };

  struct WGPUBufferInstance final : GraphicsTypes::BufferInstance
  {
    size_t BufferSize = 0;

    GLuint BufferID = 0;

    ~WGPUBufferInstance() override
    {
      if(BufferID)
        glDeleteBuffers(1, &BufferID);
    }
  };

  struct WGPUTextureInstance final : GraphicsTypes::TextureInstance
  {
    GLuint TextureID = 0;

    WGPUTextureInstance() = default;

    explicit WGPUTextureInstance(const GraphicsTypes::TextureInfo &info) : TextureInstance(info) {}

    ~WGPUTextureInstance() override
    {
      if(TextureID)
        glDeleteTextures(1, &TextureID);
    }

    GraphicsTypes::TextureInfo       &GetInfo() { return Info; }
    const GraphicsTypes::TextureInfo &GetInfo() const { return Info; }
  };

  struct WGPURenderBufferInstance final : GraphicsTypes::RenderBufferInstance
  {
    GLuint              FramebufferID = 0;
    WGPUTextureInstance FrameBuffer;
    WGPUTextureInstance OptionalDepth;
    WGPUTextureInstance OptionalColor;

    explicit WGPURenderBufferInstance(const GraphicsTypes::FrameBufferInfo &info) : RenderBufferInstance(info) {}

    ~WGPURenderBufferInstance() override
    {
      if(FramebufferID)
        glDeleteFramebuffers(1, &FramebufferID);
    }
  };

  WGPUGraphicsInstance *ConvertGraphicsInstance(GraphicsTypes::GraphicsInstance *wgpu_instance, const int line, const char *file = __FILE__)
  {
    if(!wgpu_instance)
    {
      Log::Error << "Trying to execute graphics command on invalid instance, in " << file << " : " << line << Log::End;
      return nullptr;
    }
    return reinterpret_cast<WGPUGraphicsInstance *>(wgpu_instance);
  }

  const WGPUGraphicsInstance *ConvertGraphicsInstance(const GraphicsTypes::GraphicsInstance *wgpu_instance,
                                                      const int                              line,
                                                      const char                            *file = __FILE__)
  {
    if(!wgpu_instance)
    {
      Log::Error << "Trying to execute graphics command on invalid instance, in " << file << " : " << line << Log::End;
      return nullptr;
    }
    return reinterpret_cast<const WGPUGraphicsInstance *>(wgpu_instance);
  }

  GLint GetGLFormat(const GraphicsTypes::ImageFormat internal_format)
  {
    GLint format = GL_RGB;
    switch(internal_format)
    {
      case GraphicsTypes::ImageFormat::R:       format = GL_RED; break;
      case GraphicsTypes::ImageFormat::RG:      format = GL_RG; break;
      case GraphicsTypes::ImageFormat::RGB:     format = GL_RGB; break;
      case GraphicsTypes::ImageFormat::RGBA:
      case GraphicsTypes::ImageFormat::RGBA16F:
      case GraphicsTypes::ImageFormat::RGBA32F:
      case GraphicsTypes::ImageFormat::BGRA:    format = GL_RGBA; break;
      case GraphicsTypes::ImageFormat::DEPTH:   format = GL_DEPTH_COMPONENT; break;
      case GraphicsTypes::ImageFormat::INVALID: format = GL_RGB; break;
    }
    return format;
  }

  GLint GetGLInternalFormat(const GraphicsTypes::ImageFormat internal_format)
  {
    switch(internal_format)
    {
      case GraphicsTypes::ImageFormat::R:       return GL_R8;
      case GraphicsTypes::ImageFormat::RG:      return GL_RG8;
      case GraphicsTypes::ImageFormat::RGB:     return GL_RGB8;
      case GraphicsTypes::ImageFormat::RGBA:    return GL_RGBA8;
      case GraphicsTypes::ImageFormat::RGBA16F:
      case GraphicsTypes::ImageFormat::RGBA32F: return GL_RGBA16F;
      case GraphicsTypes::ImageFormat::BGRA:    return GL_RGBA8;
      case GraphicsTypes::ImageFormat::DEPTH:   return GL_DEPTH_COMPONENT24;
      case GraphicsTypes::ImageFormat::INVALID: return GL_RGBA8;
    }
    return GL_RGBA8;
  }

  GLenum GetGLType(const GraphicsTypes::ImageFormat internal_format)
  {
    GLenum type = GL_UNSIGNED_BYTE;
    switch(internal_format)
    {
      case GraphicsTypes::ImageFormat::R:
      case GraphicsTypes::ImageFormat::RG:
      case GraphicsTypes::ImageFormat::RGB:
      case GraphicsTypes::ImageFormat::RGBA:    type = GL_UNSIGNED_BYTE; break;
      case GraphicsTypes::ImageFormat::RGBA16F:
      case GraphicsTypes::ImageFormat::RGBA32F: type = GL_HALF_FLOAT; break;
      case GraphicsTypes::ImageFormat::BGRA:    type = GL_UNSIGNED_BYTE; break;
      case GraphicsTypes::ImageFormat::DEPTH:   type = GL_UNSIGNED_INT; break;
      case GraphicsTypes::ImageFormat::INVALID: type = GL_UNSIGNED_BYTE; break;
    }
    return type;
  }

  GLenum GetGLBlendFactor(const GraphicsTypes::BlendFactor internal)
  {
    switch(internal)
    {
      case GraphicsTypes::BlendFactor::FACTOR_ONE:                 return GL_ONE;
      case GraphicsTypes::BlendFactor::FACTOR_ZERO:                return GL_ZERO;
      case GraphicsTypes::BlendFactor::FACTOR_SRC_ALPHA:           return GL_SRC_ALPHA;
      case GraphicsTypes::BlendFactor::FACTOR_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
    }
    return GL_ZERO;
  }

  WGPUPipelineState current_state;

  ShaderInfo::CommonUniformBufferData common_data;
  bool                                common_data_changed = false;
} // namespace graphics_wgpu

graphics_wgpu::WGPUGraphicsInstance::WGPUGraphicsInstance(Window &window)
{
  ShaderInfo::Init();

  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  attrs.minorVersion = 0;
  attrs.majorVersion = 2;
  attrs.antialias    = true;
  context            = emscripten_webgl_create_context("#canvas", &attrs);
  if(!context)
    throw std::runtime_error("Failed to create WebGL context!");

  emscripten_webgl_make_context_current(context);
  if(!emscripten_webgl_enable_extension(context, "EXT_color_buffer_float"))
    Log::Error << "No EXT_color_buffer_float" << Log::End;
  if(!emscripten_webgl_enable_extension(context, "OES_texture_float_linear"))
    Log::Error << "No OES_texture_float_linear" << Log::End;

  Log::Info << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION)) << Log::End;

  CreateBuffer(DynamicCommon, GraphicsTypes::BufferType::UNIFORM_DYNAMIC, DYNAMIC_UNIFORM_BUFFER_SIZE);
  CreateBuffer(DynamicSpecific, GraphicsTypes::BufferType::UNIFORM_DYNAMIC, DYNAMIC_UNIFORM_BUFFER_SIZE);
}

void graphics_wgpu::WGPUGraphicsInstance::CreateShader(std::unique_ptr<GraphicsTypes::ShaderInstance> &shader_instance,
                                                       const ShaderInfo                               &shader_info,
                                                       const std::vector<File::ShaderString>          &shader_code) const
{
  if(shader_instance)
  {
    Log::Error << "Error at shader creation, trying to assign already used shader instance" << Log::End;
    return;
  }

  auto *wgpu_shader_instance = new WGPUShaderInstance(shader_info);

  const GLuint vertex   = glCreateShader(GL_VERTEX_SHADER);
  const GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);

  auto CompileShader = [](const GLuint &program, const std::string_view type, const std::vector<std::byte> &code) -> void
  {
    const auto *source = reinterpret_cast<const GLchar *>(code.data());
    glShaderSource(program, 1, &source, nullptr);
    glCompileShader(program);
    GLint success;
    glGetShaderiv(program, GL_COMPILE_STATUS, &success);
    if(!success)
    {
      char infoLog[1024];
      glGetShaderInfoLog(program, 1024, nullptr, infoLog);
      Log::Error << "Shader compilation error (" << type << "):\n\t" << infoLog << Log::End;
      Log::Warn << "Code:\n" << std::string(reinterpret_cast<const char *>(code.data())) << Log::End;
    }
  };

  for(const auto &[stage, code] : shader_code)
    if(stage == GraphicsTypes::ShaderStage::GLSL_VERTEX)
      CompileShader(vertex, "vertex", code);
    else if(stage == GraphicsTypes::ShaderStage::GLSL_FRAGMENT)
      CompileShader(fragment, "fragment", code);

  wgpu_shader_instance->ShaderId = glCreateProgram();
  glAttachShader(wgpu_shader_instance->ShaderId, vertex);
  glAttachShader(wgpu_shader_instance->ShaderId, fragment);
  glLinkProgram(wgpu_shader_instance->ShaderId);

  GLint success;
  glGetProgramiv(wgpu_shader_instance->ShaderId, GL_LINK_STATUS, &success);
  if(!success)
  {
    char infoLog[1024];
    glGetProgramInfoLog(wgpu_shader_instance->ShaderId, 1024, nullptr, infoLog);
    Log::Error << "Shader compilation error (link):\n\t" << infoLog << Log::End;
  }

  wgpu_shader_instance->CommonUBOLoc   = glGetUniformBlockIndex(wgpu_shader_instance->ShaderId, "UniformBufferObjectGlob");
  wgpu_shader_instance->SpecificUBOLoc = glGetUniformBlockIndex(wgpu_shader_instance->ShaderId, "SpecUBO");

  for(const auto &tex : shader_info.GetTextures())
    wgpu_shader_instance->TextureLocations.emplace_back(glGetUniformLocation(wgpu_shader_instance->ShaderId, tex.c_str()));

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  shader_instance.reset(wgpu_shader_instance);
}

void graphics_wgpu::WGPUGraphicsInstance::CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                                                       const GraphicsTypes::BufferType                 type,
                                                       const size_t                                    buffer_size) const
{
  if(buffer_instance)
  {
    Log::Error << "Error at buffer creation, trying to assign already used buffer instance" << Log::End;
    return;
  }
  if(buffer_size == 0)
    return;

  auto *wgpu_buffer_instance       = new WGPUBufferInstance;
  wgpu_buffer_instance->BufferSize = buffer_size;

  GLenum usage  = GL_STATIC_DRAW;
  GLenum target = GL_ARRAY_BUFFER;
  switch(type)
  {
    case GraphicsTypes::BufferType::UNIFORM:
      usage  = GL_STATIC_DRAW;
      target = GL_UNIFORM_BUFFER;
      break;
    case GraphicsTypes::BufferType::UNIFORM_DYNAMIC:
      usage  = GL_DYNAMIC_DRAW;
      target = GL_UNIFORM_BUFFER;
      break;
    case GraphicsTypes::BufferType::TEXTURE:
    case GraphicsTypes::BufferType::VERTEX:
      usage  = GL_STATIC_DRAW;
      target = GL_ARRAY_BUFFER;
      break;
    case GraphicsTypes::BufferType::INDEX:
      usage  = GL_STATIC_DRAW;
      target = GL_ELEMENT_ARRAY_BUFFER;
      break;
    case GraphicsTypes::BufferType::STAGING:
      usage  = GL_DYNAMIC_COPY;
      target = GL_ARRAY_BUFFER;
      break;
  }

  glGenBuffers(1, &wgpu_buffer_instance->BufferID);
  glBindBuffer(target, wgpu_buffer_instance->BufferID);
  glBufferData(target, static_cast<GLsizeiptr>(buffer_size), nullptr, usage);
  glBindBuffer(target, 0);

  buffer_instance.reset(wgpu_buffer_instance);
}

void graphics_wgpu::WGPUGraphicsInstance::CreateBuffer(std::unique_ptr<GraphicsTypes::BufferInstance> &buffer_instance,
                                                       const GraphicsTypes::BufferType                 type,
                                                       const size_t                                    buffer_size,
                                                       const void                                     *data) const
{
  if(buffer_instance)
  {
    Log::Error << "Error at buffer creation, trying to assign already used buffer instance" << Log::End;
    return;
  }

  if(buffer_size == 0)
    return;

  auto *wgpu_buffer_instance       = new WGPUBufferInstance;
  wgpu_buffer_instance->BufferSize = buffer_size;

  GLenum usage  = GL_STATIC_DRAW;
  GLenum target = GL_ARRAY_BUFFER;
  switch(type)
  {
    case GraphicsTypes::BufferType::UNIFORM:
      usage  = GL_STATIC_DRAW;
      target = GL_UNIFORM_BUFFER;
      break;
    case GraphicsTypes::BufferType::UNIFORM_DYNAMIC:
      usage  = GL_DYNAMIC_DRAW;
      target = GL_UNIFORM_BUFFER;
      break;
    case GraphicsTypes::BufferType::TEXTURE:
    case GraphicsTypes::BufferType::VERTEX:
      usage  = GL_STATIC_DRAW;
      target = GL_ARRAY_BUFFER;
      break;
    case GraphicsTypes::BufferType::INDEX:
      usage  = GL_STATIC_DRAW;
      target = GL_ELEMENT_ARRAY_BUFFER;
      break;
    case GraphicsTypes::BufferType::STAGING:
      usage  = GL_DYNAMIC_COPY;
      target = GL_ARRAY_BUFFER;
      break;
  }

  glGenBuffers(1, &wgpu_buffer_instance->BufferID);
  glBindBuffer(target, wgpu_buffer_instance->BufferID);
  glBufferData(target, static_cast<GLsizeiptr>(buffer_size), data, usage);
  glBindBuffer(target, 0);

  buffer_instance.reset(wgpu_buffer_instance);
}

void graphics_wgpu::WGPUGraphicsInstance::MapBuffer(GraphicsTypes::BufferInstance *buffer_instance, const void *map_memory) const
{
  if(!buffer_instance)
  {
    Log::Error << "Trying to map to invalid buffer." << Log::End;
    return;
  }
  const auto *wgpu_buffer_instance = reinterpret_cast<WGPUBufferInstance *>(buffer_instance);

  glBindBuffer(GL_ARRAY_BUFFER, wgpu_buffer_instance->BufferID);
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(wgpu_buffer_instance->BufferSize), map_memory);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void graphics_wgpu::WGPUGraphicsInstance::CopyBuffer(GraphicsTypes::BufferInstance *buffer_instance_rhs,
                                                     GraphicsTypes::BufferInstance *buffer_instance_lhs) const
{
  if(!buffer_instance_rhs)
  {
    Log::Error << "Invalid buffer given to copy into" << Log::End;
    return;
  }
  const auto *wgpu_buffer_instance_rhs = reinterpret_cast<WGPUBufferInstance *>(buffer_instance_rhs);
  if(!buffer_instance_lhs)
  {
    Log::Error << "Invalid buffer given to copy from" << Log::End;
    return;
  }
  const auto *wgpu_buffer_instance_lhs = reinterpret_cast<WGPUBufferInstance *>(buffer_instance_lhs);

  glBindBuffer(GL_COPY_READ_BUFFER, wgpu_buffer_instance_rhs->BufferID);
  glBindBuffer(GL_COPY_WRITE_BUFFER, wgpu_buffer_instance_lhs->BufferID);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, static_cast<GLsizeiptr>(wgpu_buffer_instance_lhs->BufferSize));
  glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
  glBindBuffer(GL_COPY_READ_BUFFER, 0);
}
void graphics_wgpu::WGPUGraphicsInstance::CreateTexture(std::unique_ptr<GraphicsTypes::TextureInstance> &texture_instance,
                                                        const GraphicsTypes::TextureInfo                &texture_info,
                                                        const std::vector<const void *>                 &in_data) const
{
  assert(!texture_instance && "Trying to assign already used texture instance");

  auto *wgpu_texture_instance = new WGPUTextureInstance;

  wgpu_texture_instance->GetInfo() = texture_info;
  wgpu_texture_instance->GetInfo().MipLevels =
      static_cast<uint32_t>(std::floor(std::log2(std::max(texture_info.Width, texture_info.Height)))) + 1;

  GLenum target = GL_TEXTURE_2D;
  switch(texture_info.Type)
  {
    case GraphicsTypes::TextureType::TYPE_2D:         target = GL_TEXTURE_2D; break;
    case GraphicsTypes::TextureType::TYPE_2D_ARRAY:   target = GL_TEXTURE_2D_ARRAY; break;
    case GraphicsTypes::TextureType::TYPE_3D:         target = GL_TEXTURE_3D; break;
    case GraphicsTypes::TextureType::TYPE_CUBE:       target = GL_TEXTURE_CUBE_MAP; break;
    case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY: target = GL_TEXTURE_CUBE_MAP_ARRAY; break;
  }

  glGenTextures(1, &wgpu_texture_instance->TextureID);
  glBindTexture(target, wgpu_texture_instance->TextureID);

  glTexParameteri(target,
                  GL_TEXTURE_MIN_FILTER,
                  texture_info.Filter == GraphicsTypes::TextureFilter::NEAREST ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, texture_info.Filter == GraphicsTypes::TextureFilter::NEAREST ? GL_NEAREST : GL_LINEAR);

  const GLenum wrap = texture_info.AddressMode == GraphicsTypes::TextureAddressMode::REPEAT ? GL_REPEAT : GL_CLAMP_TO_EDGE;
  glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
  if(target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_CUBE_MAP_ARRAY)
    glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);

  const GLint  format          = GetGLFormat(texture_info.Format);
  const GLint  internal_format = GetGLInternalFormat(texture_info.Format);
  const GLenum type            = GetGLType(texture_info.Format);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  const uint32_t layers = std::max<uint32_t>(1, texture_info.Layers);
  assert(in_data.empty() || in_data.size() >= layers);

  switch(texture_info.Type)
  {
    case GraphicsTypes::TextureType::TYPE_2D:
    {
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   internal_format,
                   texture_info.Width,
                   texture_info.Height,
                   0,
                   format,
                   type,
                   in_data.empty() ? nullptr : in_data[0]);
      break;
    }

    case GraphicsTypes::TextureType::TYPE_2D_ARRAY:
    {
      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, texture_info.Width, texture_info.Height, layers, 0, format, type, nullptr);

      for(uint32_t layer = 0; layer < layers; ++layer)
        if(!in_data.empty())
          glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, texture_info.Width, texture_info.Height, 1, format, type, in_data[layer]);
      break;
    }

    case GraphicsTypes::TextureType::TYPE_3D:
    {
      glTexImage3D(GL_TEXTURE_3D,
                   0,
                   internal_format,
                   texture_info.Width,
                   texture_info.Height,
                   layers,
                   0,
                   format,
                   type,
                   in_data.empty() ? nullptr : in_data[0]);
      break;
    }

    case GraphicsTypes::TextureType::TYPE_CUBE:
    {
      assert(in_data.size() >= 6 && "Cube map requires 6 faces");

      for(uint32_t face = 0; face < 6; ++face)
      {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                     0,
                     internal_format,
                     texture_info.Width,
                     texture_info.Height,
                     0,
                     format,
                     type,
                     in_data[face]);
      }
      break;
    }

    case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY:
    {
      const uint32_t faceCount = layers * 6;

      glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
                   0,
                   internal_format,
                   texture_info.Width,
                   texture_info.Height,
                   faceCount,
                   0,
                   format,
                   type,
                   nullptr);

      for(uint32_t i = 0; i < faceCount; ++i)
        if(!in_data.empty())
          glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, i, texture_info.Width, texture_info.Height, 1, format, type, in_data[i]);
      break;
    }
  }

  glGenerateMipmap(target);
  glBindTexture(target, 0);

  texture_instance.reset(wgpu_texture_instance);
}

void graphics_wgpu::WGPUGraphicsInstance::CreateRenderBuffer(std::unique_ptr<GraphicsTypes::RenderBufferInstance> &render_buffer_instance,
                                                             const GraphicsTypes::FrameBufferInfo                 &info) const
{
  if(render_buffer_instance)
  {
    Log::Error << "Error at render buffer creation, trying to assign already used render buffer instance." << Log::End;
    return;
  }

  render_buffer_instance            = std::make_unique<WGPURenderBufferInstance>(info);
  auto *wgpu_render_buffer_instance = reinterpret_cast<WGPURenderBufferInstance *>(render_buffer_instance.get());

  const GLint format          = GetGLFormat(info.Format);
  const GLint internal_format = GetGLInternalFormat(info.Format);

  glGenFramebuffers(1, &wgpu_render_buffer_instance->FramebufferID);
  glBindFramebuffer(GL_FRAMEBUFFER, wgpu_render_buffer_instance->FramebufferID);

  if(info.TargetType != GraphicsTypes::RenderBufferType::DEPTH)
  {
    glGenTextures(1, &wgpu_render_buffer_instance->FrameBuffer.TextureID);
    glBindTexture(GL_TEXTURE_2D, wgpu_render_buffer_instance->FrameBuffer.TextureID);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 internal_format,
                 static_cast<GLsizei>(info.Width),
                 static_cast<GLsizei>(info.Height),
                 0,
                 format,
                 GetGLType(info.Format),
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, wgpu_render_buffer_instance->FrameBuffer.TextureID, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if(info.TargetType != GraphicsTypes::RenderBufferType::COLOR)
  {
    const GLint depth_format =
        info.TargetType == GraphicsTypes::RenderBufferType::BOTH ? GetGLFormat(GraphicsTypes::ImageFormat::DEPTH) : format;
    const GLint depth_internal_format =
        info.TargetType == GraphicsTypes::RenderBufferType::BOTH ? GetGLInternalFormat(GraphicsTypes::ImageFormat::DEPTH) : internal_format;
    const GLenum depth_type =
        info.TargetType == GraphicsTypes::RenderBufferType::BOTH ? GetGLType(GraphicsTypes::ImageFormat::DEPTH) : GetGLType(info.Format);
    auto &Target = info.TargetType == GraphicsTypes::RenderBufferType::BOTH ? wgpu_render_buffer_instance->OptionalDepth.TextureID
                                                                            : wgpu_render_buffer_instance->FrameBuffer.TextureID;
    glGenTextures(1, &Target);
    glBindTexture(GL_TEXTURE_2D, Target);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 depth_internal_format,
                 static_cast<GLsizei>(info.Width),
                 static_cast<GLsizei>(info.Height),
                 0,
                 depth_format,
                 depth_type,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Target, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GraphicsTypes::TextureInstance *graphics_wgpu::WGPUGraphicsInstance::GetRenderBufferTexture(
    GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance)
    return nullptr;
  auto *wgpu_render_buffer_instance = reinterpret_cast<WGPURenderBufferInstance *>(render_buffer_instance);
  return &wgpu_render_buffer_instance->FrameBuffer;
}

void graphics_wgpu::WGPUGraphicsInstance::DispatchCompute(const GraphicsTypes::ShaderInstance         *shader,
                                                          const GraphicsTypes::TextureInstance *const *texture_instance,
                                                          const int                                    count,
                                                          const int                                    num_x,
                                                          const int                                    num_y,
                                                          const int                                    num_z) const
{
  const auto wgpu_shader = reinterpret_cast<const WGPUShaderInstance *>(shader);
  if(!wgpu_shader)
    return;

  throw std::runtime_error("compute not implemented yet");
}

void graphics_wgpu::WGPUGraphicsInstance::CreateMipMaps(const GraphicsTypes::TextureInstance *texture_instance) const
{
  if(!texture_instance)
    return;
  const auto wgpu_texture_instance = reinterpret_cast<const WGPUTextureInstance *>(texture_instance);

  glBindTexture(GL_TEXTURE_2D, wgpu_texture_instance->TextureID);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}

bool graphics_wgpu::WGPUGraphicsInstance::StartDraw(Window &window)
{
  CurrentOffsetCommon   = 0;
  CurrentOffsetSpecific = 0;
  glDeleteVertexArrays(VAOs.size(), VAOs.data());
  VAOs.clear();
  DynamicBuffers.clear();
  return true;
}

void graphics_wgpu::WGPUGraphicsInstance::SetState(const GraphicsTypes::RenderState &state) const
{
  glEnable(GL_BLEND);
  glBlendFuncSeparate(GetGLBlendFactor(state.Blending.SRCColor),
                      GetGLBlendFactor(state.Blending.DSTColor),
                      GetGLBlendFactor(state.Blending.SRCAlpha),
                      GetGLBlendFactor(state.Blending.DSTAlpha));

  if(state.Culling != GraphicsTypes::CullMode::CULL_NONE)
  {
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    if(state.Culling == GraphicsTypes::CullMode::CULL_BACK)
      glCullFace(GL_BACK);
    else
      glCullFace(GL_FRONT);
  }
  else
  {
    glDisable(GL_CULL_FACE);
  }

  if(state.DepthTest && state.DepthCompare != GraphicsTypes::DepthCompareMode::NONE)
  {
    glEnable(GL_DEPTH_TEST);
    if(state.DepthCompare == GraphicsTypes::DepthCompareMode::COMPARE_GREATER)
      glDepthFunc(GL_GREATER);
    else if(state.DepthCompare == GraphicsTypes::DepthCompareMode::COMPARE_GREATER_EQUALS)
      glDepthFunc(GL_GEQUAL);
    else if(state.DepthCompare == GraphicsTypes::DepthCompareMode::COMPARE_LESS)
      glDepthFunc(GL_LESS);
    else if(state.DepthCompare == GraphicsTypes::DepthCompareMode::COMPARE_LESS_EQUALS)
      glDepthFunc(GL_LEQUAL);
  }
  else
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
  }

  static bool warn_wireframe = false;
  if(state.WireFrame && !warn_wireframe)
  {
    Log::Warn << "Wireframe not supported in WebGL" << Log::End;
    warn_wireframe = true;
  }

  glColorMask(state.ColorMask & 1, state.ColorMask & 2, state.ColorMask & 4, state.ColorMask & 8);

  glViewport(state.DrawViewport.OffsetX, state.DrawViewport.OffsetY, state.DrawViewport.ExtentX, state.DrawViewport.ExtentY);

  glClearColor(state.ClearColor.r, state.ClearColor.g, state.ClearColor.b, state.ClearColor.a);
  glClearDepthf(state.ClearDepth);

  current_state.State = state;
}

int graphics_wgpu::WGPUGraphicsInstance::AcquireFrameIndex() const
{
  return 0;
}

void graphics_wgpu::WGPUGraphicsInstance::BindShader(const GraphicsTypes::ShaderInstance *shader_instance) const
{
  if(!shader_instance)
  {
    Log::Error << "Trying to bind invalid shader." << Log::End;
    return;
  }
  auto *wgpu_shader_instance = reinterpret_cast<const WGPUShaderInstance *>(shader_instance);

  current_state.shader = wgpu_shader_instance;

  glUseProgram(wgpu_shader_instance->ShaderId);
  glBindVertexArray(wgpu_shader_instance->VAO);
  common_data_changed = true;
}

void graphics_wgpu::WGPUGraphicsInstance::SetCommonUniforms(const ShaderInfo::CommonUniformBufferData &data) const
{
  common_data         = data;
  common_data_changed = true;
}

void graphics_wgpu::WGPUGraphicsInstance::SetColorState(const bool state) const
{
  current_state.color = state;
}

void graphics_wgpu::WGPUGraphicsInstance::BindBufferDynamic(std::vector<unsigned char>       &data,
                                                            const GraphicsTypes::UBOBindPoint bind_point) const
{
  const auto  &buffer = bind_point == GraphicsTypes::UBOBindPoint::Common ? DynamicCommon : DynamicSpecific;
  auto        &offset = bind_point == GraphicsTypes::UBOBindPoint::Common ? CurrentOffsetCommon : CurrentOffsetSpecific;
  const GLuint locale =
      bind_point == GraphicsTypes::UBOBindPoint::Common ? current_state.shader->CommonUBOLoc : current_state.shader->SpecificUBOLoc;
  const GLuint binding = bind_point != GraphicsTypes::UBOBindPoint::Common;

  const auto *wgpu_buffer_instance = reinterpret_cast<WGPUBufferInstance *>(buffer.get());

  glBindBuffer(GL_UNIFORM_BUFFER, wgpu_buffer_instance->BufferID);
  glBufferSubData(GL_UNIFORM_BUFFER, static_cast<GLsizei>(offset), static_cast<GLsizei>(data.size()), data.data());
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindBuffer(GL_UNIFORM_BUFFER, wgpu_buffer_instance->BufferID);
  glUniformBlockBinding(current_state.shader->ShaderId, locale, binding);
  glBindBufferRange(GL_UNIFORM_BUFFER,
                    binding,
                    wgpu_buffer_instance->BufferID,
                    static_cast<GLintptr>(offset),
                    static_cast<GLsizeiptr>(data.size()));

  offset += data.size();
  static GLint uniformBufferAlignment = 0;
  static bool  loaded_alignment       = false;
  if(!loaded_alignment)
  {
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignment);
    loaded_alignment = true;
  }
  offset = (offset + uniformBufferAlignment - 1) & ~(uniformBufferAlignment - 1);
}

void graphics_wgpu::WGPUGraphicsInstance::BindTextures(const GraphicsTypes::TextureInstance *const *texture_instance,
                                                       const int                                    count,
                                                       GraphicsTypes::UBOBindPoint,
                                                       int) const
{
  if(!texture_instance)
  {
    Log::Error << "Trying to bind invalid texture." << Log::End;
    return;
  }
  const auto *const *wgpu_texture_instances = reinterpret_cast<const WGPUTextureInstance *const *>(texture_instance);

  if(static_cast<size_t>(count) > current_state.shader->TextureLocations.size())
  {
    Log::Warn << "Exceeding texture slots for shader: " << count << " vs " << current_state.shader->TextureLocations.size() << Log::End;
    return;
  }
  for(int i = 0; i < count; i++)
  {
    glUniform1i(current_state.shader->TextureLocations[i], i);
    glActiveTexture(GL_TEXTURE0 + i);
    GLenum target = GL_TEXTURE_2D;
    switch(wgpu_texture_instances[i]->GetInfo().Type)
    {
      case GraphicsTypes::TextureType::TYPE_2D:         target = GL_TEXTURE_2D; break;
      case GraphicsTypes::TextureType::TYPE_2D_ARRAY:   target = GL_TEXTURE_2D_ARRAY; break;
      case GraphicsTypes::TextureType::TYPE_3D:         target = GL_TEXTURE_3D; break;
      case GraphicsTypes::TextureType::TYPE_CUBE:       target = GL_TEXTURE_CUBE_MAP; break;
      case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY: target = GL_TEXTURE_CUBE_MAP_ARRAY; break;
    }

    glBindTexture(target, wgpu_texture_instances[i]->TextureID);
  }
}

void graphics_wgpu::WGPUGraphicsInstance::BindVertexBuffer(GraphicsTypes::BufferInstance *buffer_instance) const
{
  if(!buffer_instance)
  {
    Log::Error << "Trying to bind invalid buffer." << Log::End;
    return;
  }
  const auto *wgpu_buffer_instance = reinterpret_cast<WGPUBufferInstance *>(buffer_instance);

  glBindBuffer(GL_ARRAY_BUFFER, wgpu_buffer_instance->BufferID);

  GLuint &VAO = VAOs.emplace_back();
  glGenVertexArrays(1, &VAO);
  for(const auto &[Type, Offset, Location] : current_state.shader->shader_info.GetVertexAttribs())
  {
    GLint  size;
    GLenum type;
    switch(Type)
    {
      case GraphicsTypes::ShaderType::INT:
        size = 1;
        type = GL_INT;
        break;
      case GraphicsTypes::ShaderType::FLOAT:
        size = 1;
        type = GL_FLOAT;
        break;
      case GraphicsTypes::ShaderType::INT2:
        size = 2;
        type = GL_INT;
        break;
      case GraphicsTypes::ShaderType::INT3:
        size = 3;
        type = GL_INT;
        break;
      case GraphicsTypes::ShaderType::INT4:
        size = 4;
        type = GL_INT;
        break;
      case GraphicsTypes::ShaderType::FLOAT2:
        size = 2;
        type = GL_FLOAT;
        break;
      case GraphicsTypes::ShaderType::FLOAT3:
        size = 3;
        type = GL_FLOAT;
        break;
      case GraphicsTypes::ShaderType::FLOAT4:
        size = 4;
        type = GL_FLOAT;
        break;
      default: continue;
    }
    glVertexAttribPointer(Location,
                          size,
                          type,
                          GL_FALSE,
                          current_state.shader->shader_info.GetVertexSize(),
                          reinterpret_cast<void *>(Offset));
    glEnableVertexAttribArray(Location);
  }
}

void graphics_wgpu::WGPUGraphicsInstance::DrawIndexed(const size_t                         count,
                                                      const GraphicsTypes::BufferInstance *buffer_instance,
                                                      const GraphicsTypes::PrimitiveType   prim_type) const
{
  GLenum mode = GL_TRIANGLES;
  switch(prim_type)
  {
    case GraphicsTypes::PrimitiveType::TRIANGLES: mode = GL_TRIANGLES; break;
    case GraphicsTypes::PrimitiveType::LINES:     mode = GL_LINES; break;
    case GraphicsTypes::PrimitiveType::POINTS:    mode = GL_POINTS; break;
    default:                                      Log::Warn << "Unknown primitive type, defaulting to TRIANGLES." << Log::End; break;
  }

  if(common_data_changed)
  {
    std::vector<unsigned char> ubo_data;
    ubo_data.resize(ShaderInfo::GetCommonUniformSize());
    ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data.data(), common_data);

    BindBufferDynamic(ubo_data, GraphicsTypes::UBOBindPoint::Common);
  }

  if(!buffer_instance)
  {
    glDrawArrays(mode, 0, static_cast<GLsizei>(count));
  }
  else
  {
    const auto *wgpu_buffer_instance = reinterpret_cast<const WGPUBufferInstance *>(buffer_instance);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wgpu_buffer_instance->BufferID);

    glDrawElements(mode, static_cast<GLsizei>(count), GL_UNSIGNED_INT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

void graphics_wgpu::WGPUGraphicsInstance::DrawDynamic(const size_t                       count,
                                                      const size_t                       type_size,
                                                      const void                        *data,
                                                      const GraphicsTypes::PrimitiveType prim_type) const
{
  GLenum mode = GL_TRIANGLES;
  switch(prim_type)
  {
    case GraphicsTypes::PrimitiveType::TRIANGLES: mode = GL_TRIANGLES; break;
    case GraphicsTypes::PrimitiveType::LINES:     mode = GL_LINES; break;
    case GraphicsTypes::PrimitiveType::POINTS:    mode = GL_POINTS; break;
    default:                                      Log::Warn << "Unknown primitive type, defaulting to TRIANGLES." << Log::End; break;
  }

  if(common_data_changed)
  {
    std::vector<unsigned char> ubo_data;
    ubo_data.resize(ShaderInfo::GetCommonUniformSize());
    ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data.data(), common_data);

    BindBufferDynamic(ubo_data, GraphicsTypes::UBOBindPoint::Common);
  }

  auto &vertex_buffer = DynamicBuffers.emplace_back();
  CreateBuffer(vertex_buffer, GraphicsTypes::BufferType::VERTEX, type_size * count, data);
  BindVertexBuffer(vertex_buffer.get());

  glDrawArrays(mode, 0, static_cast<GLsizei>(count));
}

void graphics_wgpu::WGPUGraphicsInstance::BindRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance) const
{
  if(!render_buffer_instance)
  {
    Log::Error << "failed to bind invalid render buffer!" << Log::End;
    return;
  }
  const auto wgpu_render_buffer_instance = reinterpret_cast<WGPURenderBufferInstance *>(render_buffer_instance);

  glBindFramebuffer(GL_FRAMEBUFFER, wgpu_render_buffer_instance->FramebufferID);

  const auto &info = wgpu_render_buffer_instance->GetInfo();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, info.Width, info.Height);
}

void graphics_wgpu::WGPUGraphicsInstance::EndRenderBuffer(GraphicsTypes::RenderBufferInstance *render_buffer_instance)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void graphics_wgpu::WGPUGraphicsInstance::StartMainRenderPass()
{
  current_state.Info.Samples  = 4;
  current_state.Info.Format   = GraphicsTypes::ImageFormat::BGRA;
  current_state.Info.HasDepth = true;
  current_state.Info.HasColor = true;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphics_wgpu::WGPUGraphicsInstance::EndRenderPass() {}

void graphics_wgpu::WGPUGraphicsInstance::EndDraw(Window &window) {}

void graphics_wgpu::WGPUGraphicsInstance::Wait() {}

graphics_wgpu::WGPUGraphicsInstance::~WGPUGraphicsInstance() = default;
