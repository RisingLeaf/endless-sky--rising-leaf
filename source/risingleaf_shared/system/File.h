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
#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>

#include "graphics/graphics_toplevel_defines.h"
#include "risingleaf_shared/base/ASLList.h"


namespace File
{
  struct ShaderString
  {
    GraphicsTypes::ShaderStage stage;
    std::vector<std::byte>     code;
  };

  std::vector<char>         Read(std::string_view path);
  std::string               ReadString(std::string_view path);
  std::vector<ShaderString> ReadShader(std::string_view path);
} // namespace File


#endif // FILE_H
