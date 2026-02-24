/* SpriteShader.cpp
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

#include "SpriteShader.h"

#include "../GameData.h"
#include "../Screen.h"
#include "../Swizzle.h"
#include "../image/Sprite.h"
#include "GameWindow.h"
#include "Shader.h"



namespace
{
  Shader                        shader("sprite shader");
  graphics_layer::ObjectHandle  square;
  graphics_layer::TextureHandle dummy_tex;
} // namespace

void SpriteShader::Init()
{
  auto &info = shader.GetInfo();
  info.SetInputSize(2 * sizeof(float));
  info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0); // vert

  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // u_in vec2  position;
  info.AddUniformVariable(GraphicsTypes::ShaderType::MAT2);   // u_in mat2  transform;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // u_in vec2  blur;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float clip;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float frame;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float frameCount;
  info.AddUniformVariable(GraphicsTypes::ShaderType::MAT4);   // u_in mat4  swizzleMatrix;
  info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // u_in int   useSwizzle;
  info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // u_in float alpha;
  info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // u_in int   useSwizzleMask;

  info.AddTexture("tex");
  info.AddTexture("swizzleMask");

  shader.Create(*GameData::Shaders().Find("sprite"));

  constexpr float vertexData[] = {-.5f, -.5f, -.5f, .5f, .5f, -.5f, .5f, .5f};

  square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {});

  constexpr asl::uint8 dummy_tex_raw[] = {125, 0, 255, 255};

  dummy_tex = graphics_layer::TextureHandle(GameWindow::GetInstance(),
                                            dummy_tex_raw,
                                            1,
                                            1,
                                            1,
                                            GraphicsTypes::TextureType::TYPE_2D,
                                            GraphicsTypes::ImageFormat::RGBA,
                                            GraphicsTypes::TextureTarget::READ);
}


void SpriteShader::Draw(const Sprite  *sprite,
                        const Point   &position,
                        const float    zoom,
                        const Swizzle *swizzle,
                        const float    frame,
                        const Point   &unit)
{
  if(!sprite) return;

  Bind();
  Add(Prepare(sprite, position, zoom, swizzle, frame, unit));
  Unbind();
}


SpriteShader::Item SpriteShader::Prepare(const Sprite  *sprite,
                                         const Point   &position,
                                         float          zoom,
                                         const Swizzle *swizzle,
                                         float          frame,
                                         const Point   &unit)
{
  if(!sprite) return {};

  Item item;
  item.texture     = sprite->Texture().GetTexture();
  item.swizzleMask = sprite->SwizzleMask().GetTexture();
  item.frame       = frame;
  item.frameCount  = sprite->Frames();
  // Position.
  item.position[0] = static_cast<float>(position.X());
  item.position[1] = static_cast<float>(position.Y());
  // Rotation and scale.
  Point scaledUnit  = unit * zoom;
  Point uw          = scaledUnit * sprite->Width();
  Point uh          = scaledUnit * sprite->Height();
  item.transform[0] = static_cast<float>(-uw.Y());
  item.transform[1] = static_cast<float>(uw.X());
  item.transform[2] = static_cast<float>(-uh.X());
  item.transform[3] = static_cast<float>(-uh.Y());
  // Swizzle.
  item.swizzle = swizzle;

  return item;
}


void SpriteShader::Bind() { shader.Bind(); }


void SpriteShader::Add(const Item &item, bool withBlur)
{
  int use_swizzle_mask = 0;
  int use_swizzle      = 0;
  if(item.swizzle)
  {
    use_swizzle      = !item.swizzle->IsIdentity();
    use_swizzle_mask = !item.swizzle->OverrideMask() && item.swizzleMask;
  }

  graphics_layer::TextureList texture_list;
  texture_list.AddTexture(item.texture, 0, false);
  if(item.swizzleMask) texture_list.AddTexture(item.swizzleMask, 1, false);
  else texture_list.AddTexture(dummy_tex.GetTexture(), 1, false);
  texture_list.Bind(GameWindow::GetInstance());

  const auto                 info = shader.GetInfo();
  std::vector<unsigned char> data_cp(info.GetUniformSize());
  int                        i = -1;

  info.CopyUniformEntryToBuffer(data_cp.data(), item.position, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), item.transform, ++i);
  static constexpr float UNBLURRED[2] = {0.f, 0.f};
  info.CopyUniformEntryToBuffer(data_cp.data(), withBlur ? item.blur : UNBLURRED, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &item.clip, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &item.frame, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &item.frameCount, ++i);
  static constexpr float UNSWIZZLED[16] = {
      1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f,
  };
  info.CopyUniformEntryToBuffer(data_cp.data(), item.swizzle ? item.swizzle->MatrixPtr() : UNSWIZZLED, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &use_swizzle, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &item.alpha, ++i);
  info.CopyUniformEntryToBuffer(data_cp.data(), &use_swizzle_mask, ++i);

  GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

  square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}


void SpriteShader::Unbind() {}
