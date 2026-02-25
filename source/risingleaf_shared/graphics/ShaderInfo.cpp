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
#include "ShaderInfo.h"

#include <cstring>

#include "graphics_layer.h"

ShaderInfo::ShaderInfo() = default;

void ShaderInfo::SetInputSize(const size_t size) { VertexSize = size; }

void ShaderInfo::AddInput(GraphicsTypes::ShaderType type, size_t offset, size_t location)
{
  VertexAttributes.emplace_back(type, offset, location);
}

void ShaderInfo::AddUniformVariable(GraphicsTypes::ShaderType type)
{
  UniformBufferEntry &entry = SpecificUniformBuffer.emplace_back(type);
  entry.Alignment           = graphics_layer::GetAlignmentOfType(type);
  SpecificUniformBufferSize = (SpecificUniformBufferSize + entry.Alignment - 1) & ~(entry.Alignment - 1);
  entry.Offset              = SpecificUniformBufferSize;
  entry.Size                = graphics_layer::GetSizeOfType(entry.Type);
  SpecificUniformBufferSize += entry.Size;
}

void ShaderInfo::AddTexture(std::string_view name) { Textures.emplace_back(name); }

void ShaderInfo::CopyUniformEntryToBuffer(unsigned char *destination, const void *data, const size_t index) const
{
  assert(index < SpecificUniformBuffer.size() && "No uniform variable specified for index");
  const auto &it = SpecificUniformBuffer[index];
  memcpy(destination + it.Offset, data, it.Size);
}

std::vector<ShaderInfo::UniformBufferEntry> ShaderInfo::CommonUniformBuffer;
size_t                                      ShaderInfo::CommonUniformBufferSize = 0;

void ShaderInfo::Init()
{
  static bool initialized = false;
  if(!initialized)
  {
    initialized = true;

    CommonUniformBufferSize = 0;

    CommonUniformBuffer.emplace_back(GraphicsTypes::ShaderType::FLOAT2); // scale

    for(auto &entry : CommonUniformBuffer)
    {
      entry.Alignment         = graphics_layer::GetAlignmentOfType(entry.Type);
      CommonUniformBufferSize = (CommonUniformBufferSize + entry.Alignment - 1) & ~(entry.Alignment - 1);
      entry.Offset            = CommonUniformBufferSize;
      entry.Size              = graphics_layer::GetSizeOfType(entry.Type);
      CommonUniformBufferSize += entry.Size;
    }
#ifdef ASL_BUILD_WASM
    CommonUniformBufferSize = (CommonUniformBufferSize + 15) & ~(15);
#endif
#ifdef __APPLE__
    CommonUniformBufferSize = (CommonUniformBufferSize + 15) & ~(15);
#endif
  }
}


void ShaderInfo::CopyCommonUniformDataToBuffer(unsigned char *destination, const CommonUniformBufferData &data)
{
  auto it = CommonUniformBuffer.begin();
  memcpy(destination + it->Offset, &data.scale, it->Size);
  ++it;
}
