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
#include "../Point.h"
#include "../Screen.h"
#include "../pi.h"
#include "shared/graphics/graphics_layer.h"
#include "shared/graphics/graphics_toplevel_defines.h"
#include "shared/system/File.h"
#include "system/Log.h"

#include <stdexcept>

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo                                     info;

	graphics_layer::ObjectHandle geometry;
} // namespace



void RingShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // position
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // radius
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // width
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // angle
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // startAngle
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // dash
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // color

	const auto shader_code = File::ReadShader("../shaders/compilation/ring.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "ring");

	constexpr float vertexData[] = {-1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f, 1.f};

	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 2 * sizeof(float), vertexData, {}, "ring");
}



void RingShader::Draw(
    GraphicsTypes::CommandBufferInstance *cb, const Point &pos, const float out, const float in, const Color &color)
{
	const float width = .5f * (1.f + out - in);
	Draw(cb, pos, out - width, width, 1.f, color);
}



void RingShader::Draw(GraphicsTypes::CommandBufferInstance *cb,
                      const Point                          &pos,
                      const float                           radius,
                      const float                           width,
                      const float                           fraction,
                      const Color                          &color,
                      const float                           dash,
                      const float                           startAngle)
{
	Bind(cb);
	Add(cb, pos, radius, width, fraction, color, dash, startAngle);
}



void RingShader::Bind(GraphicsTypes::CommandBufferInstance *cb)
{
	if(!shader)
		throw runtime_error("RingShader: Bind() called before Init().");

	cb->BindShader(shader.get());
}



void RingShader::Add(
    GraphicsTypes::CommandBufferInstance *cb, const Point &pos, const float out, const float in, const Color &color)
{
	const float width = .5f * (1.f + out - in);
	Add(cb, pos, out - width, width, 1.f, color);
}



void RingShader::Add(GraphicsTypes::CommandBufferInstance *cb,
                     const Point                          &pos,
                     const float                           radius,
                     const float                           width,
                     const float                           fraction,
                     const Color                          &color,
                     const float                           dash,
                     const float                           startAngle)
{
	const gm::vec2 position    = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};
	const gm::vec2 scale       = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const float    angle       = fraction * 2. * PI;
	const float    startAngleV = startAngle * TO_RAD;
	const float    dashV       = dash ? 2. * PI / dash : 0.;

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int                        i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &position, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &radius, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &width, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &angle, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &startAngleV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &dashV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



void RingShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}
