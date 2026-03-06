/* Sprite.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Sprite.h"

#include <utility>

#include "../GameWindow.h"
#include "../Preferences.h"
#include "../Screen.h"
#include "ImageBuffer.h"


namespace
{
  void AddBuffer(
      ImageBuffer                   &buffer,
      graphics_layer::TextureHandle &target,
      const bool                     noReduction,
      const std::string_view         name)
  {
    // Check whether this sprite is large enough to require size reduction.
    const Preferences::LargeGraphicsReduction setting = Preferences::GetLargeGraphicsReduction();
    if(!noReduction && (setting == Preferences::LargeGraphicsReduction::ALL ||
                        (setting == Preferences::LargeGraphicsReduction::LARGEST_ONLY &&
                         buffer.Width() * buffer.Height() >= 1'000'000)))
    {
      buffer.ShrinkToHalfSize();
    }

    target = graphics_layer::TextureHandle(
        GameWindow::GetInstance(),
        name,
        buffer.Pixels(),
        buffer.Width(),
        buffer.Height(),
        buffer.Frames(),
        GraphicsTypes::TextureType::TYPE_2D_ARRAY,
        GraphicsTypes::ImageFormat::RGBA,
        GraphicsTypes::TextureTarget::READ);
    target.CreateMipMaps();

    // Free the ImageBuffer memory.
    buffer.Clear();
  }
} // namespace

const std::string &Sprite::Name() const { return name; }

Sprite::Sprite(const std::string &name) : name(std::move(name)) {}


void Sprite::LoadDimensions(const ImageBuffer &buffer)
{
  width  = buffer.Width();
  height = buffer.Height();
  frames = buffer.Frames();
}


bool Sprite::HasDimensions() const { return width != 0 && height != 0 && frames != 0; }


// Add the given frames, optionally uploading them. The given buffers will be cleared afterwards.
void Sprite::AddFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction)
{
  isLoaded = true;
  // The 1x image determines the dimensions of the sprite's size.
  width  = buffer1x.Width();
  height = buffer1x.Height();
  frames = buffer1x.Frames();
  // Do nothing else if the buffer is empty.
  // (The buffer can be empty yet still have a width and height if uploading is disabled.)
  if(!buffer1x.Pixels()) return;

  // Only use the 2x resolution image if it is provided.
  if(buffer2x.Pixels())
  {
    AddBuffer(buffer2x, texture, noReduction, name);
    buffer1x.Clear();
  }
  else {
    AddBuffer(buffer1x, texture, noReduction, name);
  }
}


// Upload the given frames. The given buffers will be cleared afterwards.
void Sprite::AddSwizzleMaskFrames(ImageBuffer &buffer1x, ImageBuffer &buffer2x, bool noReduction)
{
  if(!swizzleMaskFrames)
  {
    swizzleMaskFrames = buffer1x.Frames();
    if(swizzleMaskFrames > 1 && swizzleMaskFrames < frames) swizzleMaskFrames = 1;
  }

  // Do nothing if the buffer is empty.
  if(!buffer1x.Pixels()) return;

  // Only use the 2x resolution image if it is provided.
  if(buffer2x.Pixels())
  {
    AddBuffer(buffer2x, swizzleMask, noReduction, name + "-sw");
    buffer1x.Clear();
  }
  else {
    AddBuffer(buffer1x, swizzleMask, noReduction, name + "-sw");
  }
}


bool Sprite::IsLoaded() const { return isLoaded; }


// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
  texture     = graphics_layer::TextureHandle();
  swizzleMask = graphics_layer::TextureHandle();

  width  = 0.f;
  height = 0.f;
  frames = 0;
}


// Get the width, in pixels, of the 1x image.
float Sprite::Width() const { return width; }


// Get the height, in pixels, of the 1x image.
float Sprite::Height() const { return height; }


// Get the number of frames in the animation.
int Sprite::Frames() const { return frames; }


int Sprite::SwizzleMaskFrames() const { return swizzleMaskFrames; }

// Get the texture index, based on whether the screen is high DPI or not.
const graphics_layer::TextureHandle &Sprite::Texture() const { return texture; }


// Get the offset of the center from the top left corner; this is for easy
// shifting of corner to center coordinates.
Point Sprite::Center() const { return Point(.5 * width, .5 * height); }


// Get the texture index, based on whether the screen is high DPI or not.
const graphics_layer::TextureHandle &Sprite::SwizzleMask() const { return swizzleMask; }

