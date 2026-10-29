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
#include "../Point.h"
#include "../Screen.h"
#include "shared/graphics/graphics_layer.h"
#include "shared/system/File.h"

#include <stdexcept>

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo                                     info;

	graphics_layer::ObjectHandle geometry;
} // namespace



void LineShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // start
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // end
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // width
	info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // cap
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // startColor
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // endColor

	const auto shader_code = File::ReadShader("../shaders/compilation/line.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "line");

	constexpr float vertexData[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 2 * sizeof(float), vertexData, {}, "fog_geometry");
}



void LineShader::Draw(GraphicsTypes::CommandBufferInstance *cb,
                      const Point                          &from,
                      const Point                          &to,
                      float                                 width,
                      const Color                          &color,
                      const bool                            roundCap)
{
	DrawGradient(cb, from, to, width, color, color, roundCap);
}



void LineShader::DrawDashed(GraphicsTypes::CommandBufferInstance *cb,
                            const Point                          &from,
                            const Point                          &to,
                            const Point                          &unit,
                            const float                           width,
                            const Color                          &color,
                            const double                          dashLength,
                            double                                spaceLength,
                            bool                                  roundCap)
{
	const double length        = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int          segments      = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	const float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
		Draw(cb,
		     from + unit * (i * length / segments + spaceLength + capOffset),
		     from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
		     width,
		     color,
		     roundCap);
}



void LineShader::DrawGradient(GraphicsTypes::CommandBufferInstance *cb,
                              const Point                          &from,
                              const Point                          &to,
                              float                                 width,
                              const Color                          &fromColor,
                              const Color                          &toColor,
                              bool                                  roundCap)
{
	if(!shader)
		throw runtime_error("LineShader: Draw() called before Init().");

	cb->BindShader(shader.get());

	const gm::vec2 scale = {static_cast<float>(Screen::Width()), static_cast<float>(Screen::Height())};
	const gm::vec2 start = {static_cast<float>(from.X()), static_cast<float>(from.Y())};
	const gm::vec2 end   = {static_cast<float>(to.X()), static_cast<float>(to.Y())};
	const int      cap   = roundCap;

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int                        i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &start, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &end, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &width, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &cap, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), fromColor.Get(), ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), toColor.Get(), ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



void LineShader::DrawGradientDashed(GraphicsTypes::CommandBufferInstance *cb,
                                    const Point                          &from,
                                    const Point                          &to,
                                    const Point                          &unit,
                                    const float                           width,
                                    const Color                          &fromColor,
                                    const Color                          &toColor,
                                    const double                          dashLength,
                                    double                                spaceLength,
                                    bool                                  roundCap)
{
	const double length        = (to - from).Length();
	const double patternLength = dashLength + spaceLength;
	int          segments      = length / patternLength;
	// If needed, scale pattern down so we can draw at least two of them over length.
	if(segments < 2)
	{
		segments = 2;
		spaceLength *= length / (segments * patternLength);
	}
	spaceLength /= 2.;
	const float capOffset = roundCap ? width : 0.;
	for(int i = 0; i < segments; ++i)
	{
		const float p      = static_cast<float>(i) / segments;
		Color       mixed  = Color::Combine(1.f - p, fromColor, p, toColor);
		const float pv     = static_cast<float>(i + 1) / segments;
		Color       mixed2 = Color::Combine(1.f - pv, fromColor, pv, toColor);
		DrawGradient(cb,
		             from + unit * (i * length / segments + spaceLength + capOffset),
		             from + unit * ((i + 1) * length / segments - spaceLength - capOffset),
		             width,
		             mixed,
		             mixed2,
		             roundCap);
	}
}

void LineShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}
