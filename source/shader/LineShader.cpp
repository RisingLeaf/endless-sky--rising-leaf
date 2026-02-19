/* LineShader.cpp
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

#include "LineShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../GameWindow.h"
#include "../Point.h"
#include "Shader.h"

#include <stdexcept>

#include "graphics/graphics_layer.h"

using namespace std;

namespace {
	Shader shader("line shader");
	graphics_layer::ObjectHandle line;
}



void LineShader::Init()
{
	auto &info = shader.GetInfo();

	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);
	info.AddUniformVariable(GraphicsTypes::ShaderType::INT);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4);


	shader.Create(*GameData::Shaders().Find("line"));

	constexpr float vertexData[] = {
		-1.f, -1.f,
		 1.f, -1.f,
		-1.f,  1.f,
		 1.f,  1.f
	};

	line = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {});
}



void LineShader::Draw(const Point &from, const Point &to, float width, const Color &color, bool roundCap)
{
	DrawGradient(from, to, width, color, color, roundCap);
}



void LineShader::DrawDashed(const Point &from, const Point &to, const Point &unit, const float width,
	const Color &color, const double dashLength, double spaceLength, bool roundCap)
{
	const double length = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int segments = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
		Draw(from + unit * (i * length / segments + spaceLength + capOffset),
			from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
			width, color, roundCap);
}



void LineShader::DrawGradient(const Point &from, const Point &to, const float width,
	const Color &fromColor, const Color &toColor, bool roundCap)
{
	if(!shader())
		throw runtime_error("LineShader: Draw() called before Init().");

	shader.Bind();

	const float start[2] = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	const float end[2]   = {static_cast<float>(to.X()), static_cast<float>(to.Y())};

	const auto &info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), start,          ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), end,            ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &width,          ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &roundCap,       ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), fromColor.Get(), ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), toColor.Get(),   ++i);

	GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	line.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



void LineShader::DrawGradientDashed(const Point &from, const Point &to, const Point &unit, const float width,
		const Color &fromColor, const Color &toColor, const double dashLength, double spaceLength, bool roundCap)
{
	const double length = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int segments = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
	{
		float p = static_cast<float>(i) / segments;
		Color mixed = Color::Combine(1. - p, fromColor, p, toColor);
		float pv = static_cast<float>(i + 1) / segments;
		Color mixed2 = Color::Combine(1. - pv, fromColor, pv, toColor);
		DrawGradient(from + unit * (i * length / segments + spaceLength + capOffset),
			from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
			width, mixed, mixed2, roundCap);
	}
}
