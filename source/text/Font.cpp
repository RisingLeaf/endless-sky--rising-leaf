/* Font.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Font.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Preferences.h"
#include "../Screen.h"
#include "../image/ImageBuffer.h"
#include "../image/ImageFileData.h"
#include "Alignment.h"
#include "DisplayText.h"
#include "GameWindow.h"
#include "Truncate.h"
#include "shared/system/File.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace {
	bool showUnderlines = false;
	const int KERN = 2;
	const float UNDERLINE_OFFSET = 2.f;

	struct FontInstanceData {
		gm::vec2 position;
		int glyph;
		float aspect;
	};
} // namespace



Font::Font(const GraphicsTypes::GraphicsInstance *graphics_instance, const filesystem::path &imagePath)
{
	Load(graphics_instance, imagePath);
}



void Font::Load(const GraphicsTypes::GraphicsInstance *graphics_instance, const filesystem::path &imagePath)
{
	// Load the texture.
	ImageBuffer image;
	if(!image.Read(ImageFileData(imagePath)))
		return;

	LoadTexture(graphics_instance, image);
	CalculateAdvances(image);
	SetUpShader(graphics_instance, image.Width() / GLYPHS, image.Height());
	widthEllipses = WidthRawString("...");
}



void Font::Draw(GraphicsTypes::CommandBufferInstance *cb, const DisplayText &text, const Point &point,
                const Color &color) const
{
	DrawAliased(cb, text, round(point.X()), round(point.Y()), color);
}



void Font::DrawAliased(GraphicsTypes::CommandBufferInstance *cb, const DisplayText &text, double x, double y,
                       const Color &color) const
{
	int width = -1;
	const string truncText = TruncateText(text, width);
	const auto &layout = text.GetLayout();
	if(width >= 0)
	{
		if(layout.align == Alignment::CENTER)
			x += (layout.width - width) / 2;
		else if(layout.align == Alignment::RIGHT)
			x += layout.width - width;
	}
	DrawAliased(cb, truncText, x, y, color);
}



void Font::Draw(GraphicsTypes::CommandBufferInstance *cb, const string &str, const Point &point,
                const Color &color) const
{
	DrawAliased(cb, str, round(point.X()), round(point.Y()), color);
}



void Font::DrawAliased(GraphicsTypes::CommandBufferInstance *cb, const string &str, double x, double y,
                       const Color &color) const
{
	if(str.empty())
		return;

	// Update the scale, only if the screen size has changed.
	if(Screen::Width() != screenWidth || Screen::Height() != screenHeight)
	{
		screenWidth = Screen::Width();
		screenHeight = Screen::Height();
		scale[0] = 2.f / screenWidth;
		scale[1] = -2.f / screenHeight;
	}
	const gm::vec2 glyphSize = {glyphWidth, glyphHeight};

	gm::vec2 textPos = {static_cast<float>(x - 1.), static_cast<float>(y)};
	int previous = 0;
	bool isAfterSpace = true;
	bool underlineChar = false;
	bool doubleUnderlineChar = false;
	constexpr int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));

	std::vector<FontInstanceData> letters;

	for(char c : str)
	{
		if(c == '_')
		{
			if(underlineChar)
				doubleUnderlineChar = true;
			underlineChar = showUnderlines;
			continue;
		}

		int glyph = Glyph(c, isAfterSpace);
		if(c != '"' && c != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
		{
			textPos[0] += space;
			continue;
		}

		constexpr float default_aspect = 1.0f;
		textPos[0] += advance[previous * GLYPHS + glyph] + KERN;

		letters.emplace_back(textPos, glyph, default_aspect);

		if(underlineChar)
		{
			const float aspect =
			    static_cast<float>(advance[glyph * GLYPHS] + KERN) / (advance[underscoreGlyph * GLYPHS] + KERN);
			letters.emplace_back(textPos, underscoreGlyph, aspect);

			if(doubleUnderlineChar)
			{
				const gm::vec2 underlinePos = {textPos[0], textPos[1] + UNDERLINE_OFFSET};
				letters.emplace_back(underlinePos, underscoreGlyph, aspect);
				doubleUnderlineChar = false;
			}

			underlineChar = false;
		}

		previous = glyph;
	}

	if(letters.empty())
		return;

	std::unique_ptr<GraphicsTypes::BufferInstance> instance_buffer;
	GameWindow::GraphicsInstance()->CreateBuffer(instance_buffer, GraphicsTypes::BufferType::VERTEX,
	                                             letters.size() * sizeof(FontInstanceData), letters.data(), str + "_instance_buffer");

	cb->BindShader(shader.get());

	graphics_layer::QuickBindTextures(cb, {texture.GetTexture()});

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &glyphSize, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.DrawInstanced(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP, instance_buffer.get(), letters.size());
}



int Font::Width(const string &str, char after) const
{
	return WidthRawString(str.c_str(), after);
}



int Font::FormattedWidth(const DisplayText &text, char after) const
{
	int width = -1;
	const string truncText = TruncateText(text, width);
	return width < 0 ? WidthRawString(truncText.c_str(), after) : width;
}



int Font::Height() const noexcept
{
	return height;
}



int Font::Space() const noexcept
{
	return space;
}



void Font::ShowUnderlines(bool show) noexcept
{
	showUnderlines = show || Preferences::Has("Always underline shortcuts");
}



int Font::Glyph(char c, bool isAfterSpace) noexcept
{
	// Curly quotes.
	if(c == '\'' && isAfterSpace)
		return 96;
	if(c == '"' && isAfterSpace)
		return 97;

	return max(0, min(GLYPHS - 3, c - 32));
}



void Font::LoadTexture(const GraphicsTypes::GraphicsInstance *graphics_instance, ImageBuffer &image)
{
	texture = graphics_layer::TextureHandle(
	    graphics_instance, "font", {image.Pixels()}, image.Width(), image.Height(), 1,
	    GraphicsTypes::TextureType::TYPE_2D, GraphicsTypes::ImageFormat::RGBA, GraphicsTypes::TextureTarget::READ,
	    GraphicsTypes::TextureAddressMode::CLAMP_TO_EDGE, GraphicsTypes::TextureFilter::LINEAR);
}



void Font::CalculateAdvances(ImageBuffer &image)
{
	// Get the format and size of the surface.
	int width = image.Width() / GLYPHS;
	height = image.Height();
	constexpr unsigned mask = 0xFF000000;
	constexpr unsigned half = 0xC0000000;
	const int pitch = image.Width();

	// advance[previous * GLYPHS + next] is the x advance for each glyph pair.
	// There is no advance if the previous value is 0, i.e. we are at the very
	// start of a string.
	memset(advance, 0, GLYPHS * sizeof(advance[0]));
	for(int previous = 1; previous < GLYPHS; ++previous)
		for(int next = 0; next < GLYPHS; ++next)
		{
			int maxD = 0;
			int glyphWidth = 0;
			uint32_t *begin = image.Pixels();
			for(int y = 0; y < height; ++y)
			{
				// Find the last non-empty pixel in the previous glyph.
				uint32_t *pend = begin + previous * width;
				uint32_t *pit = pend + width;
				while(pit != pend && (*--pit & mask) < half)
				{
				}
				int distance = (pit - pend) + 1;
				glyphWidth = max(distance, glyphWidth);

				// Special case: if "next" is zero (i.e. end of line of text),
				// calculate the full width of this character. Otherwise:
				if(next)
				{
					// Find the first non-empty pixel in this glyph.
					uint32_t *nit = begin + next * width;
					uint32_t *nend = nit + width;
					while(nit != nend && (*nit++ & mask) < half)
					{
					}

					// How far apart do you want these glyphs drawn? If drawn at
					// an advance of "width", there would be:
					// pend + width - pit   <- pixels after the previous glyph.
					// nit - (nend - width) <- pixels before the next glyph.
					// So for zero kerning distance, you would want:
					distance += 1 - (nit - (nend - width));
				}
				maxD = max(maxD, distance);

				// Update the pointer to point to the beginning of the next row.
				begin += pitch;
			}
			// This is a fudge factor to avoid over-kerning, especially for the
			// underscore and for glyph combinations like AV.
			advance[previous * GLYPHS + next] = max(maxD, glyphWidth - 4) / 2;
		}

	// Set the space size based on the character width.
	width /= 2;
	height /= 2;
	space = (width + 3) / 6 + 1;
}



void Font::SetUpShader(const GraphicsTypes::GraphicsInstance *graphics_instance, float glyphW, float glyphH)
{
	glyphWidth = glyphW * .5f;
	glyphHeight = glyphH * .5f;

	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 2 * sizeof(float), 1);
	info.SetInputSize(4 * sizeof(float));

	info.AddInstanceInput(GraphicsTypes::ShaderType::FLOAT2, offsetof(FontInstanceData, position), 2); // position
	info.AddInstanceInput(GraphicsTypes::ShaderType::INT, offsetof(FontInstanceData, glyph), 3);       // glyph
	info.AddInstanceInput(GraphicsTypes::ShaderType::FLOAT, offsetof(FontInstanceData, aspect), 4);    // aspect
	info.SetInstanceInputSize(sizeof(FontInstanceData));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // glyphSize
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // color

	info.AddTexture("tex");

	const auto shader_code = File::ReadShader("../shaders/compilation/font.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "font");

	constexpr float vertices[] = {0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 4 * sizeof(float), vertices, {}, "glyph");

	// We must update the screen size next time we draw.
	screenWidth = 0;
	screenHeight = 0;
}



int Font::WidthRawString(const char *str, char after) const noexcept
{
	int width = 0;
	int previous = 0;
	bool isAfterSpace = true;

	for(; *str; ++str)
	{
		if(*str == '_')
			continue;

		int glyph = Glyph(*str, isAfterSpace);
		if(*str != '"' && *str != '\'')
			isAfterSpace = !glyph;
		if(!glyph)
			width += space;
		else
		{
			width += advance[previous * GLYPHS + glyph] + KERN;
			previous = glyph;
		}
	}
	width += advance[previous * GLYPHS + max(0, min(GLYPHS - 1, after - 32))];

	return width;
}



// Param width will be set to the width of the return value, unless the layout width is negative.
string Font::TruncateText(const DisplayText &text, int &width) const
{
	width = -1;
	const auto &layout = text.GetLayout();
	const string &str = text.GetText();
	if(layout.width < 0 || (layout.align == Alignment::LEFT && layout.truncate == Truncate::NONE))
		return str;
	width = layout.width;
	switch(layout.truncate)
	{
	case Truncate::NONE:
		width = WidthRawString(str.c_str());
		return str;
	case Truncate::FRONT:
		return TruncateFront(str, width);
	case Truncate::MIDDLE:
		return TruncateMiddle(str, width);
	case Truncate::BACK:
	default:
		return TruncateBack(str, width);
	}
}



string Font::TruncateBack(const string &str, int &width) const
{
	return TruncateEndsOrMiddle(str, width, [](const string &str, int charCount) {
		return str.substr(0, charCount) + "...";
	});
}



string Font::TruncateFront(const string &str, int &width) const
{
	return TruncateEndsOrMiddle(str, width, [](const string &str, int charCount) {
		return "..." + str.substr(str.size() - charCount);
	});
}



string Font::TruncateMiddle(const string &str, int &width) const
{
	return TruncateEndsOrMiddle(str, width, [](const string &str, int charCount) {
		return str.substr(0, (charCount + 1) / 2) + "..." + str.substr(str.size() - charCount / 2);
	});
}



string Font::TruncateEndsOrMiddle(const string &str, int &width,
                                  function<string(const string &, int)> getResultString) const
{
	int firstWidth = WidthRawString(str.c_str());
	if(firstWidth <= width)
	{
		width = firstWidth;
		return str;
	}

	int workingChars = 0;
	int workingWidth = 0;

	int low = 0, high = str.size() - 1;
	while(low <= high)
	{
		// Think "how many chars to take from both ends, omitting in the middle".
		int nextChars = (low + high) / 2;
		int nextWidth = WidthRawString(getResultString(str, nextChars).c_str());
		if(nextWidth <= width)
		{
			if(nextChars > workingChars)
			{
				workingChars = nextChars;
				workingWidth = nextWidth;
			}
			low = nextChars + (nextChars == low);
		}
		else
			high = nextChars - 1;
	}
	width = workingWidth;
	return getResultString(str, workingChars);
}
