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
#include "../GameWindow.h"
#include "../image/Sprite.h"
#include "Shader.h"
#include "graphics/graphics_layer.h"

using namespace std;

namespace {
	Shader shader("batch shader");
}



// Initialize the shaders.
void BatchShader::Init()
{
	auto &info = shader.GetInfo();
  info.SetInputSize(6 * sizeof(float));
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0,                 0); // vert
	info.AddInput(GraphicsTypes::ShaderType::FLOAT3, 2 * sizeof(float), 1); // texCoord
	info.AddInput(GraphicsTypes::ShaderType::FLOAT,  5 * sizeof(float), 2); // alpha

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT); // frameCount

	info.AddTexture("tex");

	shader.Create(*GameData::Shaders().Find("batch"));
}



void BatchShader::Bind()
{
	shader.Bind();
}



void BatchShader::Add(const Sprite *sprite, const bool isHighDPI, const vector<float> &data)
{
	// Do nothing if there are no sprites to draw.
	if(data.empty())
		return;

	graphics_layer::TextureList texture_list;
	texture_list.AddTexture(sprite->Texture(isHighDPI).GetTexture(), 0, false);
	texture_list.Bind(GameWindow::GetInstance());

	const auto info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	const auto frame_count = static_cast<float>(sprite->Frames());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &frame_count, ++i);

	GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	GameWindow::GetInstance()->DrawDynamic(data.size() / 6, 6 * sizeof(float), data.data(), GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}
