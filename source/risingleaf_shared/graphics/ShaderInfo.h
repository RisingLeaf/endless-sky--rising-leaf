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
#ifndef SHADERINFO_H
#define SHADERINFO_H

#include <vector>

#include "math/graphics_math/gm.h"

namespace GraphicsTypes
{
  enum class ShaderType;
}

class ShaderInfo {
public:
  struct VertexAttrib
  {
    GraphicsTypes::ShaderType Type;
    size_t     Offset;
    size_t     Location;
  };

  struct UniformBufferEntry
  {
    GraphicsTypes::ShaderType Type;
    size_t     Offset;
    size_t     Alignment;
    size_t     Size;
  };

  struct CommonUniformBufferData
  {
    gm::mat4 view;
    gm::mat4 proj;
    gm::mat4 inverseView;
    gm::mat4 inverseProjection;
    gm::vec3 view_pos;
    gm::vec3 light_dir;
    float     near;
    float     far;
  };

private:
  std::vector<VertexAttrib>       VertexAttributes;
  size_t                          VertexSize = 0;

  std::vector<std::string> Textures;

  static std::vector<UniformBufferEntry> CommonUniformBuffer;
  static size_t                          CommonUniformBufferSize;

  std::vector<UniformBufferEntry> SpecificUniformBuffer;
  size_t                          SpecificUniformBufferSize = 0;
public:

  ShaderInfo();
  void SetInputSize(size_t size);
  void AddInput(GraphicsTypes::ShaderType type, size_t offset, size_t location);
  void AddUniformVariable(GraphicsTypes::ShaderType type);
  void AddTexture(std::string_view name);

  void CopyUniformEntryToBuffer(unsigned char *destination, const void *data, size_t index ) const;
  [[nodiscard]] size_t GetUniformSize() const
  {
    // Metal Uniform buffers are aligned to 16bytes
#if defined(__APPLE__) || defined(ASL_BUILD_WASM)
    return (SpecificUniformBufferSize + 15) & ~(15);
#else
    return SpecificUniformBufferSize;
#endif
  }

  [[nodiscard]] const std::vector<VertexAttrib> &GetVertexAttribs() const { return VertexAttributes; }
  [[nodiscard]] size_t                           GetVertexSize()    const { return VertexSize; }

  [[nodiscard]] size_t GetSpecificTextureCount() const { return Textures.size(); }
  [[nodiscard]] const std::vector<std::string> &GetTextures() const { return Textures; }

  static void Init();
  static void CopyCommonUniformDataToBuffer(unsigned char *destination, const CommonUniformBufferData &data);
  [[nodiscard]] static size_t GetCommonUniformSize() { return CommonUniformBufferSize; }
};



#endif //SHADERINFO_H
