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
#include "../GameWindow.h"
#include "../Rectangle.h"
#include "Shader.h"

#include <stdexcept>

#include "graphics/graphics_layer.h"

using namespace std;

namespace {
	Shader shader("fill shader");

	graphics_layer::ObjectHandle square;
}



void FillShader::Init()
{
	auto &info = shader.GetInfo();

	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); //u_in vec2 center;
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); //u_in vec2 size;
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); //u_in vec4 color;

	shader.Create(*GameData::Shaders().Find("fill"));

	constexpr float vertexData[] = {
		-.5f, -.5f,
		 .5f, -.5f,
		-.5f,  .5f,
		 .5f,  .5f
	};

	square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {});
}



void FillShader::Fill(const Rectangle &area, const Color &color)
{
	Fill(area.Center(), area.Dimensions(), color);
}



void FillShader::Fill(const Point &center, const Point &size, const Color &color)
{
	if (!shader())
		throw std::runtime_error("Draw called before init!");
	shader.Bind();

	const float centerV[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	const float sizeV[2]   = {static_cast<float>(size.X()), static_cast<float>(size.Y())};

	const auto info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), centerV,     ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), sizeV,       ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);

	GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}
