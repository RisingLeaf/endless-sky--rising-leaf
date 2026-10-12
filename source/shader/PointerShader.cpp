/* PointerShader.cpp
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

#include "PointerShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Screen.h"
#include "shared/graphics/graphics_layer.h"
#include "shared/graphics/graphics_toplevel_defines.h"
#include "shared/system/File.h"

#include <stdexcept>

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;

	graphics_layer::ObjectHandle geometry;
} // namespace



void PointerShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // center
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // angle
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // size
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // offset
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // color

	const auto shader_code = File::ReadShader("../shaders/compilation/pointer.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "pointer");

	constexpr float vertexData[] = {
	    0.f, 0.f, 0.f, 1.f, 1.f, 0.f,
	};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 3, 2 * sizeof(float), vertexData, {}, "pointer");
}



void PointerShader::Draw(GraphicsTypes::CommandBufferInstance *cb, const Point &center, const Point &angle,
                         const float width, const float height, const float offset, const Color &color)
{
	Bind(cb);

	Add(cb, center, angle, width, height, offset, color);
}



void PointerShader::Bind(GraphicsTypes::CommandBufferInstance *cb)
{
	if(!shader)
		throw runtime_error("PointerShader: Bind() called before Init().");

	cb->BindShader(shader.get());
}



void PointerShader::Add(GraphicsTypes::CommandBufferInstance *cb, const Point &center, const Point &angle,
                        const float width, const float height, float offset, const Color &color)
{
	const gm::vec2 scale = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const gm::vec2 c = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	const gm::vec2 a = {static_cast<float>(angle.X()), static_cast<float>(angle.Y())};
	const gm::vec2 size = {width, height};

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &c, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &a, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &size, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &offset, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLES);
}


void PointerShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}