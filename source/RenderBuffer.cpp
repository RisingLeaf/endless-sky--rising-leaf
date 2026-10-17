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
#include "graphics/bootstrap/graphics_default_states.h"
#include "graphics/graphics_layer.h"
#include "system/File.h"

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;

	graphics_layer::ObjectHandle geometry;
} // namespace



// Initialize the shaders.
void RenderBuffer::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // size
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // position
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // srcposition
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // srcscale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // fade

	info.AddTexture("tex");

	const auto shader_code = File::ReadShader("../shaders/compilation/renderBuffer.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "renderBuffer");


	constexpr float vertexData[] = {-.5f, -.5f, -.5f, .5f, .5f, -.5f, .5f, .5f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 2 * sizeof(float), vertexData, {}, "renderBuffer");
}


void RenderBuffer::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}


// Create a texture of the given size that can be used as a render target.
RenderBuffer::RenderBuffer(const gm::ivec2 &dimensions)
    : framebuffer(GameWindow::GraphicsInstance(), dimensions[0], dimensions[1], GraphicsTypes::ImageFormat::RGBA_SRGB,
	              GraphicsTypes::ImageFormat::INVALID, 1, "render_buffer"),
      size(dimensions)
{
	GameWindow::GraphicsInstance()->CreateCommandBuffer(RenderCmd);
}



// Destructor. Frees the texture and renderbuffers.
RenderBuffer::~RenderBuffer()
{
}



// Turn this buffer on as a render target. The render target is restored if
// the Activation object goes out of scope.
GraphicsTypes::CommandBufferInstance *RenderBuffer::SetTarget()
{
	RenderCmd->Begin();
	framebuffer.Bind(RenderCmd.get());
	auto render_state = graphics_bootstrap::RenderStateSimpleBlending(size[0], size[1]);
	render_state.ClearColor.a = 0.f;
	RenderCmd->SetState(render_state);
	last_dimensions = {Screen::RawWidth(), Screen::RawHeight()};
	Screen::SetRaw(size[0], size[1], true);
	return RenderCmd.get();
}



// Reset the render target and viewport to the original settings.
void RenderBuffer::Deactivate()
{
	framebuffer.Finish(RenderCmd.get());
	RenderCmd->End();
	RenderCmd->Submit();
	RenderCmd->Wait();
	Screen::SetRaw(last_dimensions[0], last_dimensions[1], true);
}



void RenderBuffer::Draw(GraphicsTypes::CommandBufferInstance *cb, const Point &position)
{
	Draw(cb, position, Point(size[0], size[1]));
}



// Draw the contents of this buffer at the specified position.
void RenderBuffer::Draw(GraphicsTypes::CommandBufferInstance *cb, const Point &position, const Point &clipsize,
                        const Point &srcposition)
{
	cb->BindShader(shader.get());
	graphics_layer::QuickBindTextures(cb, {framebuffer.GetTexture()});

	const auto sizeV = gm::vec2(clipsize.X(), clipsize.Y());
	const auto positionV = gm::vec2(position.X(), position.Y());
	const auto scaleV = gm::vec2(2.f / Screen::Width(), -2.f / Screen::Height());
	const auto srcPositionV = gm::vec2(srcposition.X(), srcposition.Y());
	const auto srcScaleV = gm::vec2(1.f / size[0], 1.f / size[1]);
	const auto fadePaddingV = gm::vec4(fadePadding[0] / clipsize.Y(), fadePadding[1] / clipsize.Y(),
	                                   fadePadding[2] / clipsize.X(), fadePadding[3] / clipsize.X());


	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &sizeV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &positionV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &scaleV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &srcPositionV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &srcScaleV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &fadePaddingV, ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



double RenderBuffer::Top() const
{
	return -size[1] / 2;
}



double RenderBuffer::Bottom() const
{
	return size[1] / 2;
}



double RenderBuffer::Left() const
{
	return -size[0] / 2;
}



double RenderBuffer::Right() const
{
	return size[0] / 2;
}



Point RenderBuffer::Dimensions() const
{
	return Point(size[0], size[1]);
}



double RenderBuffer::Height() const
{
	return size[1];
}



double RenderBuffer::Width() const
{
	return size[0];
}



void RenderBuffer::SetFadePadding(float top, float bottom, float left, float right)
{
	fadePadding[0] = top;
	fadePadding[1] = bottom;
	fadePadding[2] = left;
	fadePadding[3] = right;
}
