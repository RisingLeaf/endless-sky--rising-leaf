/* StarField.cpp
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

#include "StarField.h"

#include "../Angle.h"
#include "../Body.h"
#include "../GameData.h"
#include "../Interface.h"
#include "../Preferences.h"
#include "../Random.h"
#include "../Screen.h"
#include "../System.h"
#include "../image/Sprite.h"
#include "../image/SpriteSet.h"
#include "../pi.h"
#include "DrawList.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "GameWindow.h"


namespace
{
  const int TILE_SIZE = 256;
  // The star field tiles in 4000 pixel increments. Have the tiling of the haze
  // field be as different from that as possible. (Note: this may need adjusting
  // in the future if monitors larger than this width ever become commonplace.)
  const double HAZE_WRAP = 6627.;
  // Don't let two haze patches be closer to each other than this distance. This
  // avoids having very bright haze where several patches overlap.
  const double HAZE_DISTANCE = 1200.;
  // This is how many haze fields should be drawn.
  const size_t HAZE_COUNT = 16;
  // This is how fast the crossfading of previous haze and current haze is
  const double FADE_PER_FRAME = 0.01;
  // An additional zoom factor applied to stars/haze on top of the base zoom, to simulate parallax.
  const double STAR_ZOOM = 0.70;
  const double HAZE_ZOOM = 0.90;

  void AddHaze(DrawList                &drawList,
               const std::vector<Body> &haze,
               const Point             &topLeft,
               const Point             &bottomRight,
               double                   transparency)
  {
    for(auto &&it : haze)
    {
      // Figure out the position of the first instance of this haze that is to
      // the right of and below the top left corner of the screen.
      double startX = fmod(it.Position().X() - topLeft.X(), HAZE_WRAP);
      startX += topLeft.X() + HAZE_WRAP * (startX < 0.);
      double startY = fmod(it.Position().Y() - topLeft.Y(), HAZE_WRAP);
      startY += topLeft.Y() + HAZE_WRAP * (startY < 0.);

      const int x_count = std::ceil((bottomRight.X() - startX) / HAZE_WRAP);
      const int y_count = std::ceil((bottomRight.Y() - startY) / HAZE_WRAP);

      // Draw any instances of this haze that are on screen.
      for(int y = 0; y < y_count; y++)
        for(int x = 0; x < x_count; x++)
          drawList.Add(it, Point(startX + x * HAZE_WRAP, startY + y * HAZE_WRAP), transparency);
    }
  }
} // namespace


void StarField::Init(int stars, int width)
{
  SetUpGraphics();
  MakeStars(stars, width);

  lastSprite = SpriteSet::Get("_menu/haze");
  for(size_t i = 0; i < HAZE_COUNT; ++i)
  {
    Point next;
    bool  overlaps = true;
    while(overlaps)
    {
      next     = Point(Random::Real() * HAZE_WRAP, Random::Real() * HAZE_WRAP);
      overlaps = false;
      for(const Body &other : haze[0])
      {
        Point  previous = other.Position();
        double dx       = remainder(previous.X() - next.X(), HAZE_WRAP);
        double dy       = remainder(previous.Y() - next.Y(), HAZE_WRAP);
        if(dx * dx + dy * dy < HAZE_DISTANCE * HAZE_DISTANCE)
        {
          overlaps = true;
          break;
        }
      }
    }
    haze[0].emplace_back(lastSprite, next, Point(), Angle::Random(), 8.);
  }
  haze[1].assign(haze[0].begin(), haze[0].end());
}


void StarField::FinishLoading()
{
  const Interface *constants = GameData::Interfaces().Get("starfield");
  fixedZoom                  = constants->GetValue("fixed zoom");
  velocityReducer            = constants->GetValue("velocity reducer");

  minZoom    = std::max(0., constants->GetValue("minimum zoom"));
  zoomClamp  = constants->GetValue("start clamping zoom");
  clampSlope = std::max(0., (zoomClamp - minZoom) / zoomClamp);
}


const Point &StarField::Position() const { return pos; }


void StarField::SetPosition(const Point &position) { pos = position; }


void StarField::SetHaze(const Sprite *sprite, bool allowAnimation)
{
  // If no sprite is given, set the default one.
  if(!sprite) sprite = SpriteSet::Get("_menu/haze");

  for(Body &body : haze[0])
    body.SetSprite(sprite);

  if(allowAnimation && sprite != lastSprite)
  {
    transparency = 1.;
    for(Body &body : haze[1])
      body.SetSprite(lastSprite);
  }
  lastSprite = sprite;
}


void StarField::Step(Point vel, double zoom)
{
  if(Preferences::Has("Fixed starfield zoom"))
  {
    baseZoom = fixedZoom;
    vel /= velocityReducer;
  }
  else if(zoom < zoomClamp)
  {
    // When the player's view zoom gets too small, the starfield begins to take up
    // an extreme amount of system resources, and the tiling becomes very obvious.
    // If the view zoom gets below the zoom clamp value (default 0.25), start zooming
    // the starfield at a different rate, and don't go below the minimum zoom value
    // (default 0.15) for the starfield's zoom. 0.25 is the vanilla minimum zoom, so
    // this only applies when the "main view" interface has been modified to allow
    // lower zoom values.
    baseZoom = clampSlope * zoom + minZoom;
    // Reduce the movement of the background by the same adjustment as the zoom
    // so that the background doesn't appear like it's moving way quicker than
    // the player is.
    vel /= baseZoom / zoom;
  }
  else baseZoom = zoom;

  pos += vel;
}


void StarField::Draw(const Point &blur, const System *system) const
{
  double density = system ? system->StarfieldDensity() : 1.;

  // Check preferences for the parallax quality.
  const auto parallaxSetting = Preferences::GetBackgroundParallax();
  const int        layers          = (parallaxSetting == Preferences::BackgroundParallax::FANCY) ? 3 : 1;
  const bool       isParallax      = (parallaxSetting == Preferences::BackgroundParallax::FANCY ||
                     parallaxSetting == Preferences::BackgroundParallax::FAST);

  // Draw the starfield unless it is disabled in the preferences.
  double zoom = baseZoom;
  if(Preferences::Has("Draw starfield") && density > 0.)
  {
    shader.Bind();

    for(int pass = 1; pass <= layers; pass++)
    {
      // Modify zoom for the first parallax layer.
      if(isParallax) zoom = baseZoom * STAR_ZOOM * pow(pass, 0.2);

      const auto length = static_cast<float>(blur.Length());
      Point unit   = length > 0. ? blur.Unit() : Point(1., 0.);
      // Don't zoom the stars at the same rate as the field; otherwise, at the
      // farthest out zoom they are too small to draw well.
      unit /= pow(zoom, .75);

      const auto baseZoom   = static_cast<float>(zoom);
      glsl::mat2 rotate;
      rotate.col0[0] = static_cast<float>(unit.Y());
      rotate.col0[1] = static_cast<float>(-unit.X());
      rotate.col1[0] = static_cast<float>(unit.X());
      rotate.col1[1] = static_cast<float>(unit.Y());
      const float elongation = length * static_cast<float>(zoom);
      const float brightness = std::min<float>(1.f, std::pow(static_cast<float>(zoom), .5f));

      const auto                &info = shader.GetInfo();
      std::vector<unsigned char> data_cp(info.GetUniformSize());
      info.CopyUniformEntryToBuffer(data_cp.data(), &baseZoom, 0);
      info.CopyUniformEntryToBuffer(data_cp.data(), &rotate, 1);
      info.CopyUniformEntryToBuffer(data_cp.data(), &elongation, 3);
      info.CopyUniformEntryToBuffer(data_cp.data(), &brightness, 4);

      // Stars this far beyond the border may still overlap the screen.
      const double borderX = fabs(blur.X()) + 1.;
      const double borderY = fabs(blur.Y()) + 1.;
      // Find the absolute bounds of the star field we must draw.
      int       minX = static_cast<int>(pos.X()) + static_cast<int>((Screen::Left() - borderX) / zoom);
      int       minY = static_cast<int>(pos.Y()) + static_cast<int>((Screen::Top() - borderY) / zoom);
      const int maxX = static_cast<int>(pos.X()) + static_cast<int>((Screen::Right() + borderX) / zoom);
      const int maxY = static_cast<int>(pos.Y()) + static_cast<int>((Screen::Bottom() + borderY) / zoom);
      // Round down to the start of the nearest tile.
      minX &= ~(TILE_SIZE - 1l);
      minY &= ~(TILE_SIZE - 1l);

      for(int gy = minY; gy < maxY; gy += TILE_SIZE)
      {
        for(int gx = minX; gx < maxX; gx += TILE_SIZE)
        {
          Point       off          = Point(gx, gy) - pos;
          const float translate[2] = {static_cast<float>(off.X()), static_cast<float>(off.Y())};
          info.CopyUniformEntryToBuffer(data_cp.data(), translate, 2);

          GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

          const int index = (gx & widthMod) / TILE_SIZE + ((gy & widthMod) / TILE_SIZE) * tileCols;
          const int first = tileIndex[index];
          const int count = static_cast<int>((tileIndex[index + 1] - first) * density / layers);
          vertices.Draw(GraphicsTypes::PrimitiveType::TRIANGLES, 6 * (first + (pass - 1) * count), 6 * (count / pass));
        }
      }
    }
  }

  // Draw the background haze unless it is disabled in the preferences.
  if(!Preferences::Has("Draw background haze")) return;

  // Modify zoom for the second parallax layer.
  if(isParallax) zoom = baseZoom * HAZE_ZOOM;

  DrawList drawList;
  drawList.Clear(0, zoom);
  drawList.SetCenter(pos);

  if(transparency > FADE_PER_FRAME) transparency -= FADE_PER_FRAME;
  else transparency = 0.;

  // Any object within this range must be drawn. Some haze sprites may repeat
  // more than once if the view covers a very large area.
  Point size        = Point(1., 1.) * haze[0].front().Radius();
  Point topLeft     = pos + Screen::TopLeft() / zoom - size;
  Point bottomRight = pos + Screen::BottomRight() / zoom + size;
  if(transparency > 0.) AddHaze(drawList, haze[1], topLeft, bottomRight, 1 - transparency);
  AddHaze(drawList, haze[0], topLeft, bottomRight, transparency);

  drawList.Draw();
}


void StarField::SetUpGraphics()
{
  auto &info = shader.GetInfo();

  info.SetInputSize(4 * sizeof(float));
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
  info.AddInput(GraphicsTypes::ShaderType::FLOAT, 2 * sizeof(float), 1);
  info.AddInput(GraphicsTypes::ShaderType::FLOAT, 3 * sizeof(float), 2);

  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::MAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);

  shader.Create(*GameData::Shaders().Find("starfield"));
}


void StarField::MakeStars(int stars, int width)
{
  // We can only work with power-of-two widths above 256.
  if(width < TILE_SIZE || (width & (width - 1))) return;

  widthMod = width - 1;

  tileCols = (width / TILE_SIZE);
  tileIndex.clear();
  tileIndex.resize(static_cast<size_t>(tileCols) * tileCols, 0);

  std::vector<int> off;
  static const int MAX_OFF = 50;
  static const int MAX_D   = MAX_OFF * MAX_OFF;
  static const int MIN_D   = MAX_D / 4;
  off.reserve(MAX_OFF * MAX_OFF * 5);
  for(int x = -MAX_OFF; x <= MAX_OFF; ++x)
    for(int y = -MAX_OFF; y <= MAX_OFF; ++y)
    {
      int d = x * x + y * y;
      if(d < MIN_D || d > MAX_D) continue;

      off.push_back(x);
      off.push_back(y);
    }

  // Generate random points in a temporary std::vector.
  // Keep track of how many fall into each tile, for sorting out later.
  std::vector<asl::uint32> temp;
  temp.reserve(2 * stars);
  {
    asl::uint32 x = Random::Int(width);
    asl::uint32 y = Random::Int(width);
    for(int i = 0; i < stars; ++i)
    {
      for(int j = 0; j < 10; ++j)
      {
        const asl::uint32 index = Random::Int(static_cast<uint32_t>(off.size())) & ~1;
        x += off[index];
        y += off[index + 1];
        x &= widthMod;
        y &= widthMod;
      }
      temp.push_back(x);
      temp.push_back(y);
      const asl::uint32 index = x / TILE_SIZE + y / TILE_SIZE * tileCols;
      ++tileIndex[index];
    }
  }

  // Accumulate item counts so that tileIndex[i] is the index in the array of
  // the first star that falls within tile i, and tileIndex.back() == stars.
  tileIndex.insert(tileIndex.begin(), 0);
  tileIndex.pop_back();
  partial_sum(tileIndex.begin(), tileIndex.end(), tileIndex.begin());

  // Each star consists of six vertices, each with four data elements.
  std::vector data(6 * 4 * stars, 0.f);
  for(auto it = temp.begin(); it != temp.end();)
  {
    // Figure out what tile this star is in.
    const asl::uint32 x     = *it++;
    const asl::uint32 y     = *it++;
    const asl::uint32 index = (x / TILE_SIZE) + (y / TILE_SIZE) * tileCols;

    // Randomize its sub-pixel position and its size / brightness.
    const asl::uint32 random = Random::Int(4096);
    const float fx     = static_cast<float>(x & (TILE_SIZE - 1)) + static_cast<float>(random & 15) * 0.0625f;
    const float fy     = static_cast<float>(y & (TILE_SIZE - 1)) + static_cast<float>(random >> 8) * 0.0625f;
    const float size   = static_cast<float>(((random >> 4) & 15) + 20) * 0.0625f;

    // Fill in the data array.
    auto        dataIt    = data.begin() + 6 * 4 * tileIndex[index]++;
    const float CORNER[6] = {static_cast<float>(0. * PI),
                             static_cast<float>(.5 * PI),
                             static_cast<float>(1.5 * PI),
                             static_cast<float>(.5 * PI),
                             static_cast<float>(1.5 * PI),
                             static_cast<float>(1. * PI)};
    for(float corner : CORNER)
    {
      *dataIt++ = fx;
      *dataIt++ = fy;
      *dataIt++ = size;
      *dataIt++ = corner;
    }
  }
  // Adjust the tile indices so that tileIndex[i] is the start of tile i.
  tileIndex.insert(tileIndex.begin(), 0);

  vertices = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 6 * stars, 4 * sizeof(float), data.data(), {});
}
