/* OutlineShader.cpp
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

#include "OutlineShader.h"

#include "../Color.h"
#include "../GameData.h"
#include "../Point.h"
#include "../Screen.h"
#include "../image/Sprite.h"
#include "shared/system/File.h"

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;

	graphics_layer::ObjectHandle geometry;
} // namespace



void OutlineShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 2 * sizeof(float), 1);
	info.SetInputSize(4 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // position
	info.AddUniformVariable(GraphicsTypes::ShaderType::MAT2);   // transform
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // frame
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // frameCount
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT4); // color
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // off

	info.AddTexture("tex");

	const auto shader_code = File::ReadShader("../shaders/compilation/outline.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "outline");

	constexpr float vertexData[] = {-.5f, -.5f, 0.f, 0.f, .5f, -.5f, 1.f, 0.f, -.5f, .5f, 0.f, 1.f, .5f, .5f, 1.f, 1.f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 4 * sizeof(float), vertexData, {}, "outline");
}



void OutlineShader::Draw(GraphicsTypes::CommandBufferInstance *cb, const Sprite *sprite, const Point &pos,
                         const Point &size, const Color &color, const Point &unit, const float frame)
{
	cb->BindShader(shader.get());

	const gm::vec2 scale = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const gm::vec2 off = {static_cast<float>(.5 / size.X()), static_cast<float>(.5 / size.Y())};
	const float frameCount = sprite->Frames();

	Point uw = unit * size.X();
	Point uh = unit * size.Y();
	const gm::padded_mat2 transform = {static_cast<float>(-uw.Y()), static_cast<float>(uw.X()),
	                                   static_cast<float>(-uh.X()), static_cast<float>(-uh.Y())};

	const gm::vec2 position = {static_cast<float>(pos.X()), static_cast<float>(pos.Y())};

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &position, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &transform, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &frame, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &frameCount, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), color.Get(), ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &off, ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	graphics_layer::QuickBindTextures(cb, {sprite->Texture().GetTexture()});

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}


void OutlineShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	shader.reset();
}