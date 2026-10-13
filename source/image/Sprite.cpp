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

#include "../Logger.h"
#include "../Preferences.h"
#include "../text/Format.h"
#include "ImageBuffer.h"

#include <cstring>

using namespace std;

namespace {
	void AddBuffer(const GraphicsTypes::GraphicsInstance *graphics_instance, const string &name, ImageBuffer &buffer,
	               graphics_layer::TextureHandle &target, const bool noReduction)
	{
		// Check whether this sprite is large enough to require size reduction.
		Preferences::LargeGraphicsReduction setting = Preferences::GetLargeGraphicsReduction();
		if(!noReduction && (setting == Preferences::LargeGraphicsReduction::ALL ||
		                    (setting == Preferences::LargeGraphicsReduction::LARGEST_ONLY &&
		                     buffer.Width() * buffer.Height() >= 1000000)))
			buffer.ShrinkToHalfSize();
		const auto width = buffer.Width();
		const auto height = buffer.Height();
		const auto frames = buffer.Frames();
		const auto pixels = buffer.Pixels();

		std::vector<const void *> framePixels;
		framePixels.reserve(frames);

		const size_t pixelsPerFrame = static_cast<size_t>(width) * height;

		for(int i = 0; i < frames; ++i)
			framePixels.emplace_back(pixels + i * pixelsPerFrame);

		target = graphics_layer::TextureHandle(
		    graphics_instance, name, framePixels, width, height, frames, GraphicsTypes::TextureType::TYPE_2D_ARRAY,
		    GraphicsTypes::ImageFormat::RGBA, GraphicsTypes::TextureTarget::READ,
		    GraphicsTypes::TextureAddressMode::CLAMP_TO_EDGE,
		    Preferences::Has("Texture filtering") ? GraphicsTypes::TextureFilter::LINEAR
			                                      : GraphicsTypes::TextureFilter::NEAREST);

		target.CreateMipMaps();

		// Free the ImageBuffer memory.
		buffer.Clear();
	}
} // namespace



Sprite::Sprite(const string &name) : name(name)
{
}



const string &Sprite::Name() const
{
	return name;
}



void Sprite::LoadDimensions(const ImageBuffer &buffer)
{
	width = buffer.Width();
	height = buffer.Height();
	frames = buffer.Frames();
}



bool Sprite::HasDimensions() const
{
	return width != 0 && height != 0 && frames != 0;
}



// Add the given frames, optionally uploading them. The given buffers will be cleared afterwards.
void Sprite::AddFrames(const GraphicsTypes::GraphicsInstance *graphics_instance, ImageBuffer &buffer1x,
                       ImageBuffer &buffer2x, const bool noReduction)
{
	isLoaded = true;
	// The 1x image determines the dimensions of the sprite's size
	// and its number of frames. ImageSet should ensure that either 1x
	// and 2x buffers contain the same number of frames, or the 2x buffer
	// is empty.
	width = buffer1x.Width();
	height = buffer1x.Height();
	frames = buffer1x.Frames();
	// Do nothing else if the buffer is empty.
	// (The buffer can be empty yet still have a width and height if uploading is disabled.)
	if(!buffer1x.Pixels())
		return;

	// Use only the 2x resolution image if it is provided.
	if(buffer2x.Pixels())
	{
		AddBuffer(graphics_instance, name, buffer2x, texture, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(graphics_instance, name, buffer1x, texture, noReduction);
}



// Upload the given frames. The given buffers will be cleared afterwards.
void Sprite::AddSwizzleMaskFrames(const GraphicsTypes::GraphicsInstance *graphics_instance, ImageBuffer &buffer1x,
                                  ImageBuffer &buffer2x, const bool noReduction)
{
	if(!swizzleMaskFrames)
	{
		swizzleMaskFrames = buffer1x.Frames();
		if(swizzleMaskFrames > 1 && swizzleMaskFrames < frames)
			swizzleMaskFrames = 1;
	}

	// Do nothing if the buffer is empty.
	if(!buffer1x.Pixels())
		return;

	// Use only the 2x resolution image if it is provided.
	if(buffer2x.Pixels())
	{
		AddBuffer(graphics_instance, name, buffer2x, swizzleMask, noReduction);
		buffer1x.Clear();
	}
	else
		AddBuffer(graphics_instance, name, buffer1x, swizzleMask, noReduction);
}



bool Sprite::IsLoaded() const
{
	return isLoaded;
}



// Free up all textures loaded for this sprite.
void Sprite::Unload()
{
	texture.Reset();
	swizzleMask.Reset();
	isLoaded = false;
	// Dimension and frame information is retained.
}



// Get the width, in pixels, of the 1x image.
float Sprite::Width() const
{
	return width;
}



// Get the height, in pixels, of the 1x image.
float Sprite::Height() const
{
	return height;
}



// Get the number of frames in the animation.
int Sprite::Frames() const
{
	return frames;
}



int Sprite::SwizzleMaskFrames() const
{
	return swizzleMaskFrames;
}



void Sprite::SetArea(double area)
{
	this->area = area;
}



double Sprite::Area() const
{
	return area;
}



// Get the offset of the center from the top left corner; this is for easy
// shifting of corner to center coordinates.
Point Sprite::Center() const
{
	return Point(.5 * width, .5 * height);
}



// Get the texture index.
const graphics_layer::TextureHandle *Sprite::Texture() const
{
	return &texture;
}



// Get the texture index.
const graphics_layer::TextureHandle *Sprite::SwizzleMask() const
{
	return &swizzleMask;
}
