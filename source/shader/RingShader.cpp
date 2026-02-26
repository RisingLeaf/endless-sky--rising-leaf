/* RingShader.cpp
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

#include "RingShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../GameWindow.h"
#include "../Point.h"
#include "../pi.h"
#include "Shader.h"

#include "graphics/graphics_layer.h"


namespace
{
  Shader                       shader("ring shader");
  graphics_layer::ObjectHandle square;
} // namespace


void RingShader::Init()
{
  auto &info = shader.GetInfo();

  info.SetInputSize(2 * sizeof(float));
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);

  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // u_in vec2  position;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float radius;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float width;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float angle;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float startAngle;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float dash;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // u_in vec4  color;

  shader.Create(*GameData::Shaders().Find("ring"));

  constexpr float vertexData[] = {-1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f};
  square =
      graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {}, "ring_shader_quad");
}

void RingShader::Clear()
{
  shader.Clear();
  square = {};
}


void RingShader::Draw(const Point &pos, float out, float in, const Color &color)
{
  float width = .5f * (1.f + out - in);
  Draw(pos, out - width, width, 1.f, color);
}


void RingShader::Draw(
    const Point &pos,
    float        radius,
    float        width,
    float        fraction,
    const Color &color,
    float        dash,
    float        startAngle)
{
  Bind();

  Add(pos, radius, width, fraction, color, dash, startAngle);

  Unbind();
}


void RingShader::Bind() { shader.Bind(); }


void RingShader::Add(const Point &pos, float out, float in, const Color &color)
{
  float width = .5f * (1.f + out - in);
  Add(pos, out - width, width, 1.f, color);
}


void RingShader::Add(
    const Point &pos,
    const float  radius,
    const float  width,
    const float  fraction,
    const Color &color,
    const float  dash,
    const float  startAngle)
{
  const float position[2] = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};

  const auto angle       = static_cast<float>(fraction * 2.f * PI);
  const auto start_angle = static_cast<float>(startAngle * TO_RAD);
  const auto dash_angle  = static_cast<float>(dash > 0.f ? 2.f * PI / dash : 0.);

  const auto                &info = shader.GetInfo();
  std::vector<unsigned char> data_cp(info.GetUniformSize());
  int                        i = -1;
  info.CopyUniformEntryToBuffer(data_cp.data(), position, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &radius, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &width, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &angle, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &start_angle, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &dash_angle, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);

  GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

  square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}


void RingShader::Unbind() {}
