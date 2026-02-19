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
#include "File.h"

#include <fstream>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#ifdef __APPLE__
#define STBI_NEON
#endif
#include "external/stb_image.h"

#include "Log.h"


std::vector<char> File::Read(const std::string_view path)
{
  std::ifstream file(path.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    Log::Error<<"Unable to read file: "<<path.data()<<Log::End;
    return {};
  }

  size_t file_size = file.tellg();
  std::vector<char> buffer(file_size);
  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();

  return buffer;
}

std::string File::ReadString(const std::string_view path)
{
  std::ifstream ifs(path.data());
  return {std::istreambuf_iterator(ifs), std::istreambuf_iterator<char>()};
}

File::PixelHandle File::ReadImage(const std::string_view path, int &width, int &height, int &channels, const bool red_only)
{
  stbi_uc *pixels = stbi_load(path.data(), &width, &height, &channels, red_only ? STBI_grey : STBI_rgb_alpha);

  if (!pixels)
  {
    Log::Error<<"Image: "<<path.data()<<" could not be loaded. stbi says:\n\t"<<stbi_failure_reason()<<Log::End;
  }

  if(red_only)
    channels = 1;
  else
    channels = 3;

  return PixelHandle(pixels);
}

File::PixelHandle::~PixelHandle()
{
  if(pixels) stbi_image_free(pixels);
}

void File::ReadBPD(const std::string_view path, asl::list<unsigned char> &data, asl::uint32 &width, asl::uint8 &components, asl::uint8 &bytes)
{
  std::ifstream ifs(path.data(), std::ios_base::binary);

  const auto EOF_CHECK = [&ifs, path]() -> bool
  {
    if(!ifs)
    {
      Log::Error<<"Failed to read file: "<<path.data()<<Log::End;
      return false;
    }
    return true;
  };

  if(!ifs)
  {
    Log::Error<<"Failed to read file: "<<path.data()<<Log::End;
    return;
  }

  ifs.read(reinterpret_cast<char *>(&bytes), 1);
  if(!EOF_CHECK()) return;

  ifs.read(reinterpret_cast<char *>(&components), 1);
  if(!EOF_CHECK()) return;

  ifs.read(reinterpret_cast<char *>(&width), 4);
  if(!EOF_CHECK()) return;

  char val;
  while(true)
  {
    ifs.read(&val, 1);
    if(ifs.eof()) return;
    data.emplace_back(val);
  }
}

std::vector<File::ShaderString> File::ReadShader(const std::string_view path)
{
  std::vector<File::ShaderString> out;
  std::ifstream ifs(path.data(), std::ios_base::binary);
  if(!ifs)
  {
    Log::Error<<"Failed to read file: "<<path.data()<<Log::End;
    return out;
  }

  while(true)
  {
    auto &working = out.emplace_back();
    char shaderType;
    ifs.read(&shaderType, 1);
    if (ifs.eof()) break;

    if(     shaderType == 'v') working.stage = GraphicsTypes::ShaderStage::VULKAN_VERTEX;
    else if(shaderType == 'f') working.stage = GraphicsTypes::ShaderStage::VULKAN_FRAGMENT;
    else if(shaderType == 'c') working.stage = GraphicsTypes::ShaderStage::VULKAN_COMPUTE;
    else if(shaderType == 'm') working.stage = GraphicsTypes::ShaderStage::METAL_COMBINED;
    else if(shaderType == 'x') working.stage = GraphicsTypes::ShaderStage::GLSL_VERTEX;
    else if(shaderType == 'y') working.stage = GraphicsTypes::ShaderStage::GLSL_FRAGMENT;
    else if(shaderType == 'z') working.stage = GraphicsTypes::ShaderStage::GLSL_COMPUTE;
    else
    {
      Log::Warn<<"Invalid shader type encountered: "<<std::string(1, shaderType)<<Log::End;
      return out;
    }

    uint32_t length = 0;
    ifs.read(reinterpret_cast<char*>(&length), sizeof(uint32_t));
    if (!ifs)
    {
      Log::Error<<"Error reading shader length."<<Log::End;
      return out;
    }

    std::vector<std::byte> buffer(length);
    ifs.read(reinterpret_cast<char *>(buffer.data()), length);
    if (!ifs)
    {
      Log::Error<<"Error reading shader code."<<Log::End;
      return out;
    }

    working.code = buffer;

    if(  working.stage == GraphicsTypes::ShaderStage::METAL_COMBINED
      || working.stage == GraphicsTypes::ShaderStage::GLSL_VERTEX
      || working.stage == GraphicsTypes::ShaderStage::GLSL_FRAGMENT
      || working.stage == GraphicsTypes::ShaderStage::GLSL_COMPUTE)
      working.code.emplace_back(std::byte{'\0'});
  }

  return out;
}
