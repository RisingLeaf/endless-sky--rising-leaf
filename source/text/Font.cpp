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
#include "../image/ImageBuffer.h"
#include "../image/ImageFileData.h"
#include "Alignment.h"
#include "DisplayText.h"
#include "Truncate.h"

#include <algorithm>
#include <cmath>

#include "GameWindow.h"
#include "graphics/graphics_layer.h"

using namespace std;

namespace
{
  bool      showUnderlines = false;
  const int KERN           = 2;

  graphics_layer::ObjectHandle square;
} // namespace


Font::Font(const filesystem::path &imagePath) { Load(imagePath); }


void Font::Load(const filesystem::path &imagePath)
{
  // Load the texture.
  ImageBuffer image;
  if(!image.Read(ImageFileData(imagePath))) return;

  LoadTexture(image);
  CalculateAdvances(image);
  SetUpShader(image.Width() / GLYPHS, image.Height());
  widthEllipses = WidthRawString("...");
}


void Font::Draw(const DisplayText &text, const Point &point, const Color &color) const
{
  DrawAliased(text, round(point.X()), round(point.Y()), color);
}


void Font::DrawAliased(const DisplayText &text, double x, double y, const Color &color) const
{
  int          width     = -1;
  const string truncText = TruncateText(text, width);
  const auto  &layout    = text.GetLayout();
  if(width >= 0)
  {
    if(layout.align == Alignment::CENTER) x += (layout.width - width) / 2;
    else if(layout.align == Alignment::RIGHT) x += layout.width - width;
  }
  DrawAliased(truncText, x, y, color);
}


void Font::Draw(const string &str, const Point &point, const Color &color) const
{
  DrawAliased(str, round(point.X()), round(point.Y()), color);
}


void Font::DrawAliased(const string &str, double x, double y, const Color &color) const
{
  shader.Bind();

  graphics_layer::TextureList texture_list;
  texture_list.AddTexture(texture.GetTexture(), 0, false);
  texture_list.Bind(GameWindow::GetInstance());

  float     textPos[2]      = {static_cast<float>(x - 1.), static_cast<float>(y)};
  int       previous        = 0;
  bool      isAfterSpace    = true;
  bool      underlineChar   = false;
  const int underscoreGlyph = max(0, min(GLYPHS - 1, '_' - 32));

  for(char c : str)
  {
    if(c == '_')
    {
      underlineChar = showUnderlines;
      continue;
    }

    int glyph = Glyph(c, isAfterSpace);
    if(c != '"' && c != '\'') isAfterSpace = !glyph;
    if(!glyph)
    {
      textPos[0] += space;
      continue;
    }

    textPos[0] += advance[previous * GLYPHS + glyph] + KERN;

    constexpr float aspect = 1.f;
    float glyph_size[] = {glyphWidth, glyphHeight};

    const auto info = shader.GetInfo();
    std::vector<unsigned char> data_cp(info.GetUniformSize());
    int i = -1;
    info.CopyUniformEntryToBuffer(data_cp.data(), textPos, ++i);
    info.CopyUniformEntryToBuffer(data_cp.data(), &glyph, ++i);
    info.CopyUniformEntryToBuffer(data_cp.data(), &aspect, ++i);
    info.CopyUniformEntryToBuffer(data_cp.data(), &glyph_size, ++i);
    info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
    GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

    square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);

    if(underlineChar)
    {
      float new_aspect = static_cast<float>(advance[glyph * GLYPHS] + KERN) / (advance[underscoreGlyph * GLYPHS] + KERN);
      info.CopyUniformEntryToBuffer(data_cp.data(), &underscoreGlyph, 1);
      info.CopyUniformEntryToBuffer(data_cp.data(), &new_aspect, 2);
      GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

      square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
      underlineChar = false;
    }

    previous = glyph;
  }
}


int Font::Width(const string &str, char after) const { return WidthRawString(str.c_str(), after); }


int Font::FormattedWidth(const DisplayText &text, char after) const
{
  int          width     = -1;
  const string truncText = TruncateText(text, width);
  return width < 0 ? WidthRawString(truncText.c_str(), after) : width;
}


int Font::Height() const noexcept { return height; }


int Font::Space() const noexcept { return space; }


void Font::ShowUnderlines(bool show) noexcept
{
  showUnderlines = show || Preferences::Has("Always underline shortcuts");
}


int Font::Glyph(char c, bool isAfterSpace) noexcept
{
  // Curly quotes.
  if(c == '\'' && isAfterSpace) return 96;
  if(c == '"' && isAfterSpace) return 97;

  return max(0, min(GLYPHS - 3, c - 32));
}


void Font::LoadTexture(ImageBuffer &image)
{
  texture = graphics_layer::TextureHandle(GameWindow::GetInstance(),
                                          image.Pixels(),
                                          image.Width(),
                                          image.Height(),
                                          1,
                                          GraphicsTypes::TextureType::TYPE_2D,
                                          GraphicsTypes::ImageFormat::RGBA,
                                          GraphicsTypes::TextureTarget::READ);
}


void Font::CalculateAdvances(ImageBuffer &image)
{
  // Get the format and size of the surface.
  int width      = image.Width() / GLYPHS;
  height         = image.Height();
  unsigned mask  = 0xFF000000;
  unsigned half  = 0xC0000000;
  int      pitch = image.Width();

  // advance[previous * GLYPHS + next] is the x advance for each glyph pair.
  // There is no advance if the previous value is 0, i.e. we are at the very
  // start of a string.
  memset(advance, 0, GLYPHS * sizeof(advance[0]));
  for(int previous = 1; previous < GLYPHS; ++previous)
    for(int next = 0; next < GLYPHS; ++next)
    {
      int       maxD       = 0;
      int       glyphWidth = 0;
      uint32_t *begin      = image.Begin(0, 0);
      for(int y = 0; y < height; ++y)
      {
        // Find the last non-empty pixel in the previous glyph.
        uint32_t *pend = begin + previous * width;
        uint32_t *pit  = pend + width;
        while(pit != pend && (*--pit & mask) < half)
        {
        }
        int distance = (pit - pend) + 1;
        glyphWidth   = max(distance, glyphWidth);

        // Special case: if "next" is zero (i.e. end of line of text),
        // calculate the full width of this character. Otherwise:
        if(next)
        {
          // Find the first non-empty pixel in this glyph.
          uint32_t *nit  = begin + next * width;
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


void Font::SetUpShader(float glyphW, float glyphH)
{
  auto &info = shader.GetInfo();
  info.SetInputSize(4 * sizeof(float));
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 2 * sizeof(float), 1);

  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::INT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);

  info.AddTexture("tex");

  shader.Create(*GameData::Shaders().Find("font"));

  constexpr float vertices[] = {0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f};
  square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 4 * sizeof(float), vertices, {});

  screenWidth  = 0;
  screenHeight = 0;

  glyphWidth  = glyphW * .5f;
  glyphHeight = glyphH * .5f;
}


int Font::WidthRawString(const char *str, char after) const noexcept
{
  int  width        = 0;
  int  previous     = 0;
  bool isAfterSpace = true;

  for(; *str; ++str)
  {
    if(*str == '_') continue;

    int glyph = Glyph(*str, isAfterSpace);
    if(*str != '"' && *str != '\'') isAfterSpace = !glyph;
    if(!glyph) width += space;
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
  width                = -1;
  const auto   &layout = text.GetLayout();
  const string &str    = text.GetText();
  if(layout.width < 0 || (layout.align == Alignment::LEFT && layout.truncate == Truncate::NONE)) return str;
  width = layout.width;
  switch(layout.truncate)
  {
  case Truncate::NONE:   width = WidthRawString(str.c_str()); return str;
  case Truncate::FRONT:  return TruncateFront(str, width);
  case Truncate::MIDDLE: return TruncateMiddle(str, width);
  case Truncate::BACK:
  default:               return TruncateBack(str, width);
  }
}


string Font::TruncateBack(const string &str, int &width) const
{
  return TruncateEndsOrMiddle(str,
                              width,
                              [](const string &str, int charCount) { return str.substr(0, charCount) + "..."; });
}


string Font::TruncateFront(const string &str, int &width) const
{
  return TruncateEndsOrMiddle(str,
                              width,
                              [](const string &str, int charCount)
                              { return "..." + str.substr(str.size() - charCount); });
}


string Font::TruncateMiddle(const string &str, int &width) const
{
  return TruncateEndsOrMiddle(
      str,
      width,
      [](const string &str, int charCount)
      { return str.substr(0, (charCount + 1) / 2) + "..." + str.substr(str.size() - charCount / 2); });
}


string
Font::TruncateEndsOrMiddle(const string &str, int &width, function<string(const string &, int)> getResultString) const
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
    else high = nextChars - 1;
  }
  width = workingWidth;
  return getResultString(str, workingChars);
}
