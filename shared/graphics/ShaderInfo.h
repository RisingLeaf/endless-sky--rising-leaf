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
#ifndef SHADERINFO_H
#define SHADERINFO_H

#include <vector>

namespace GraphicsTypes
{
  enum class ShaderType;
}

class ShaderInfo
{
public:
  struct VertexAttrib
  {
    GraphicsTypes::ShaderType Type;
    size_t                    Offset;
    size_t                    Location;
  };

  struct UniformBufferEntry
  {
    GraphicsTypes::ShaderType Type;
    size_t                    Offset;
    size_t                    Alignment;
    size_t                    Size;
  };

private:
  std::vector<VertexAttrib> VertexAttributes;
  size_t                    VertexSize = 0;

  std::vector<VertexAttrib> InstanceAttributes;
  size_t                    InstanceDataSize = 0;

  std::vector<std::string> Textures;

  size_t SSOCount = 0;

  std::vector<UniformBufferEntry> SpecificUniformBuffer;
  size_t                          SpecificUniformBufferSize = 0;

public:
  ShaderInfo();
  void SetInputSize(size_t size);
  void AddInput(GraphicsTypes::ShaderType type, size_t offset, size_t location);
  void SetInstanceInputSize(size_t size);
  void AddInstanceInput(GraphicsTypes::ShaderType type, size_t offset, size_t location);
  void AddUniformVariable(GraphicsTypes::ShaderType type);
  void AddTexture(std::string_view name);

  void AddSSO() { SSOCount += 1; }

  void                 CopyUniformEntryToBuffer(unsigned char *destination, const void *data, size_t index) const;
  [[nodiscard]] size_t GetUniformSize() const
  {
    // Metal Uniform buffers are aligned to 16bytes
#if defined(__APPLE__) || defined(ASL_BUILD_WASM)
    return (SpecificUniformBufferSize + 15) & ~(15);
#else
    return SpecificUniformBufferSize;
#endif
  }

  [[nodiscard]] const std::vector<VertexAttrib> &GetVertexAttributes() const { return VertexAttributes; }
  [[nodiscard]] size_t                           GetVertexSize() const { return VertexSize; }

  [[nodiscard]] const std::vector<VertexAttrib> &GetInstanceAttributes() const { return InstanceAttributes; }
  [[nodiscard]] size_t                           GetInstanceDataSize() const { return InstanceDataSize; }

  [[nodiscard]] size_t                          GetSpecificTextureCount() const { return Textures.size(); }
  [[nodiscard]] const std::vector<std::string> &GetTextures() const { return Textures; }

  [[nodiscard]] size_t                          GetSSOCount() const { return SSOCount; }

  static void                 Init();
};


#endif // SHADERINFO_H
