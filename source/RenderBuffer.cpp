/* RenderBuffer.cpp
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "RenderBuffer.h"

#include "GameData.h"
#include "GameWindow.h"
#include "Logger.h"
#include "Screen.h"
#include "shader/Shader.h"

#include "graphics/graphics_layer.h"
#include "shader/SpriteShader.h"



namespace {
	Shader shader("renderBuffer shader");

	graphics_layer::ObjectHandle square;
}



// Initialize the shaders.
void RenderBuffer::Init()
{
	auto &info = shader.GetInfo();

  info.SetInputSize(2 * sizeof(float));
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0,                 0);

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);

	shader.Create(*GameData::Shaders().Find("renderBuffer"));

	constexpr float vertexData[] = {
		-0.5f, -0.5f,
		-0.5f,  0.5f,
		 0.5f, -0.5f,
		 0.5f,  0.5f
	};

	square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {});
}


// Create a texture of the given size that can be used as a render target.
RenderBuffer::RenderBuffer(const Point &dimensions)
: size(dimensions),
  FrameBuffer(GameWindow::GetInstance(), 1, 1, GraphicsTypes::RenderBufferType::COLOR, 1)
{
	multiplier = Point(GameWindow::DrawWidth() / Screen::RawWidth(), GameWindow::DrawHeight() / Screen::RawHeight());

	// Attach a blank image to the texture.
	const Point scaledSize = size * multiplier * Screen::Zoom() / 100.0;

	FrameBuffer.Resize(static_cast<uint32_t>(scaledSize.X()), static_cast<uint32_t>(scaledSize.Y()));
}



RenderBuffer::~RenderBuffer() = default;



void RenderBuffer::SetTarget()
{
	FrameBuffer.Bind();

  SpriteShader::Bind();
  ShaderInfo::CommonUniformBufferData cm_data;
  cm_data.scale = {2.f / static_cast<float>(size.X()), -2.f / static_cast<float>(size.Y())};
  GameWindow::GetInstance()->SetCommonUniforms(cm_data);
}



// Reset the render target and viewport to the original settings.
void RenderBuffer::Deactivate()
{
	FrameBuffer.Finish();
}



void RenderBuffer::Draw(const Point &position)
{
	Draw(position, size);
}



// Draw the contents of this buffer at the specified position.
void RenderBuffer::Draw(const Point &position, const Point &clipsize, const Point &srcposition)
{
	shader.Bind();

	const float u_size[2]        = {static_cast<float>(clipsize.X()), static_cast<float>(clipsize.Y())};
	const float u_position[2]    = {static_cast<float>(position.X()), static_cast<float>(position.Y())};
	const float u_srcposition[2] = {static_cast<float>(srcposition.X()), static_cast<float>(srcposition.Y())};
	const float u_srcscale[2]    = {static_cast<float>(1.f / size.X()), static_cast<float>(1.f / size.Y())};
	const float u_fade[4]        = {
		static_cast<float>(fadePadding[0] / clipsize.Y()),
		static_cast<float>(fadePadding[1] / clipsize.Y()),
		static_cast<float>(fadePadding[2] / clipsize.X()),
		static_cast<float>(fadePadding[3] / clipsize.X())
	};

	const auto &info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), u_size,        ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), u_position,    ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), u_srcposition, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), u_srcscale,    ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), u_fade,        ++i);

  GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	graphics_layer::TextureList texture_list;
	texture_list.AddTexture(FrameBuffer.GetTexture(), 0, false);
	texture_list.Bind(GameWindow::GetInstance());

	square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



double RenderBuffer::Top() const
{
	return -size.Y() / 2;
}



double RenderBuffer::Bottom() const
{
	return size.Y() / 2;
}



double RenderBuffer::Left() const
{
	return -size.X() / 2;
}



double RenderBuffer::Right() const
{
	return size.X() / 2;
}



const Point &RenderBuffer::Dimensions() const
{
	return size;
}



double RenderBuffer::Height() const
{
	return size.Y();
}



double RenderBuffer::Width() const
{
	return size.X();
}



void RenderBuffer::SetFadePadding(float top, float bottom, float left, float right)
{
	fadePadding[0] = top;
	fadePadding[1] = bottom;
	fadePadding[2] = left;
	fadePadding[3] = right;
}
