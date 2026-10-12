/* BatchShader.cpp
Copyright (c) 2017 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "BatchShader.h"

#include "../GameData.h"
#include "../Screen.h"
#include "../image/Sprite.h"
#include "shared/graphics/graphics_layer.h"
#include "shared/system/File.h"

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;
} // namespace



// Initialize the shaders.
void BatchShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.AddInput(GraphicsTypes::ShaderType::FLOAT3, 2 * sizeof(float), 0);
	info.AddInput(GraphicsTypes::ShaderType::FLOAT, 5 * sizeof(float), 0);
	info.SetInputSize(6 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // frameCount

	info.AddTexture("tex");

	const auto shader_code = File::ReadShader("../shaders/compilation/batch.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "batch");
}



void BatchShader::Bind(GraphicsTypes::CommandBufferInstance *cb)
{
	cb->BindShader(shader.get());
}



void BatchShader::Add(GraphicsTypes::CommandBufferInstance *cb, const Sprite *sprite, const vector<float> &data)
{
	// Do nothing if there are no sprites to draw.
	if(data.empty())
		return;

	const gm::vec2 scale = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const int frame_count = sprite->Frames();


	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &frame_count, ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	graphics_layer::QuickBindTextures(cb, {sprite->Texture().GetTexture()});

	cb->DrawDynamic(data.size() / 6, sizeof(float) * 6, data.data(), GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}



void BatchShader::Unbind()
{
}

void BatchShader::Clear()
{
	shader.reset();
}