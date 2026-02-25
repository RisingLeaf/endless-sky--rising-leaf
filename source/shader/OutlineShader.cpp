/* OutlineShader.cpp
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

#include "OutlineShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../GameWindow.h"
#include "../Point.h"
#include "../Screen.h"
#include "../image/Sprite.h"
#include "Shader.h"
#include "mat2.h"


namespace
{
  Shader                       shader("outline shader");
  graphics_layer::ObjectHandle square;
} // namespace


void OutlineShader::Init()
{
  auto &info = shader.GetInfo();

  info.SetInputSize(4 * sizeof(float));
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 2 * sizeof(float), 1);

  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::MAT2);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);

  info.AddTexture("tex");

  shader.Create(*GameData::Shaders().Find("outline"));

  constexpr float vertexData[] = {-.5f, -.5f, 0.f, 0.f, .5f, -.5f, 1.f, 0.f, -.5f, .5f, 0.f, 1.f, .5f, .5f, 1.f, 1.f};

  square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 4 * sizeof(float), vertexData, {});
}


void OutlineShader::Draw(const Sprite *sprite,
                         const Point  &pos,
                         const Point  &size,
                         const Color  &color,
                         const Point  &unit,
                         const float   frame)
{
  Point uw = unit * size.X();
  Point uh = unit * size.Y();

  shader.Bind();
  const float position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
  glsl::mat2  transform;
  transform.col0[0]        = static_cast<float>(-uw.Y());
  transform.col0[1]        = static_cast<float>(uw.X());
  transform.col1[0]        = static_cast<float>(-uh.X());
  transform.col1[1]        = static_cast<float>(-uh.Y());
  const int32_t frameCount = sprite->Frames();
  const float   off[2]     = {static_cast<float>(.5 / size.X()), static_cast<float>(.5 / size.Y())};

  const auto                &info = shader.GetInfo();
  std::vector<unsigned char> data_cp(info.GetUniformSize());

  int i = -1;
  info.CopyUniformEntryToBuffer(data_cp.data(), position, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &transform, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &frame, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &frameCount, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), off, ++i);

  GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

  graphics_layer::TextureList texture_list;
  texture_list.AddTexture(sprite->Texture(unit.Length() * Screen::Zoom() > 50.).GetTexture(), 0, false);
  texture_list.Bind(GameWindow::GetInstance());

  square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}
