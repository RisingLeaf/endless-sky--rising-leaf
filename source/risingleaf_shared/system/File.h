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
#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>

#include "risingleaf_shared/base/ASLList.h"
#include "graphics/graphics_toplevel_defines.h"


namespace File {
  class PixelHandle
  {
    void *pixels;
  public:
    explicit PixelHandle(void *_pixels) : pixels(_pixels) {}

    PixelHandle(const PixelHandle &other) = delete;
    PixelHandle(PixelHandle &&other) noexcept
    :pixels(other.pixels)
    {
      other.pixels = nullptr;
    }
    PixelHandle &operator=(const PixelHandle &other) = delete;
    PixelHandle &operator=(PixelHandle &&other) noexcept
    {
      this->pixels = other.pixels;
      other.pixels = nullptr;
      return *this;
    }

    ~PixelHandle();

    [[nodiscard]] const void * Get() const { return pixels; }
  };

  struct ShaderString
  {
    GraphicsTypes::ShaderStage stage;
    std::vector<std::byte>     code;
  };

  std::vector<char>          Read(std::string_view path);
  std::string                ReadString(std::string_view path);
  PixelHandle                ReadImage(std::string_view path, int &width, int &height, int &channels, bool red_only = false);
  void                       ReadBPD(std::string_view path, asl::list<unsigned char> &data, asl::uint32 &width, asl::uint8 &components, asl::uint8 &bytes);
  std::vector<ShaderString>  ReadShader(std::string_view path);

  constexpr std::string_view GetResourcePath() { return "../resources/"; }
}



#endif //FILE_H
