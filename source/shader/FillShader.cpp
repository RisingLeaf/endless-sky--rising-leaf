/* FillShader.cpp
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

#include "FillShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Rectangle.h"
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



void FillShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // center
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // size
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // color

	const auto shader_code = File::ReadShader("../shaders/compilation/fill.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "fill");

	constexpr float vertexData[] = {-.5f, -.5f, .5f, -.5f, -.5f, .5f, .5f, .5f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, sizeof(float) * 2, vertexData, {}, "fill_geometry");
}



void FillShader::Fill(GraphicsTypes::CommandBufferInstance *cb, const Rectangle &area, const Color &color)
{
	Fill(cb, area.Center(), area.Dimensions(), color);
}



void FillShader::Fill(GraphicsTypes::CommandBufferInstance *cb, const Point &center, const Point &size,
                      const Color &color)
{
	if(!shader)
		throw runtime_error("FillShader: Draw() called before Init().");

	cb->BindShader(shader.get());

	const gm::vec2 scale = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const gm::vec2 centerV = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	const gm::vec2 sizeV = {static_cast<float>(size.X()), static_cast<float>(size.Y())};

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &centerV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &sizeV, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}

void FillShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}
