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
#include "../GameWindow.h"
#include "../Point.h"
#include "Shader.h"

#include "graphics/graphics_layer.h"



namespace {
	Shader shader("pointer shader");

	graphics_layer::ObjectHandle square;
} // namespace


void PointerShader::Init() {
	auto &info = shader.GetInfo();

  info.SetInputSize(2 * sizeof(float));
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);

	shader.Create(*GameData::Shaders().Find("pointer"));

	constexpr float vertexData[] = {
	        0.f, 0.f, 0.f, 1.f, 1.f, 0.f,
	};

	square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 3, 2 * sizeof(float), vertexData, {});
}


void PointerShader::Draw(const Point &center, const Point &angle, float width, float height, float offset,
                         const Color &color) {
	Bind();

	Add(center, angle, width, height, offset, color);

	Unbind();
}


void PointerShader::Bind() { shader.Bind(); }


void PointerShader::Add(const Point &center, const Point &angle, float width, float height, float offset,
                        const Color &color) {

	const float c[2] = {static_cast<float>(center.X()), static_cast<float>(center.Y())};
	const float a[2] = {static_cast<float>(angle.X()), static_cast<float>(angle.Y())};
	const float size[2] = {width, height};

	const auto &info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), c, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), a, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), size, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &offset, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);

	GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);


	square.Draw(GraphicsTypes::PrimitiveType::TRIANGLES);
}


void PointerShader::Unbind() {
}
