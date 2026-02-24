/* FogShader.cpp
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "FogShader.h"

#include "../GameData.h"
#include "../PlayerInfo.h"
#include "../Point.h"
#include "../Screen.h"
#include "Shader.h"
#include "../System.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "GameWindow.h"
#include "graphics/graphics_layer.h"



namespace {
	// Scale of the mask image:
	const int GRID = 16;
	// Distance represented by one orthogonal or diagonal step:
	const int ORTH = 5;
	const int DIAG = 7;
	// Limit distances to the size of an unsigned char.
	const int LIMIT = 255;
	// Pad beyond the screen enough to include any system that might "cast light"
	// on the on-screen view.
	const int PAD = LIMIT / ORTH;

	// Graphics objects:
	Shader shader("fog shader");
	graphics_layer::ObjectHandle square;
	graphics_layer::TextureHandle texture;

	// Keep track of the previous frame's view so that if it is unchanged we can
	// skip regenerating the mask.
	double previousZoom = 0.;
	double previousLeft = 0.;
	double previousTop = 0.;
	int previousColumns = 0;
	int previousRows = 0;
	Point previousCenter;
}



void FogShader::Init()
{
	auto &info = shader.GetInfo();

    info.SetInputSize(2 * sizeof(float));
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // corner
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // dimension

	info.AddTexture("tex");

	shader.Create(*GameData::Shaders().Find("fog"));

	constexpr float vertexData[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};

	square = graphics_layer::ObjectHandle(GameWindow::GetInstance(), 4, 2 * sizeof(float), vertexData, {});
}



void FogShader::Redraw()
{
	previousZoom = 0.;
}



void FogShader::Draw(const Point &center, double zoom, const PlayerInfo &player)
{
	// Generate a scaled-down mask image that represents the entire screen plus
	// enough pixels beyond the screen to include any systems that may be off
	// screen but close enough to "illuminate" part of the on-screen map.
	double left = Screen::Left() - GRID * PAD * zoom + fmod(center.X(), GRID) * zoom;
	double top = Screen::Top() - GRID * PAD * zoom + fmod(center.Y(), GRID) * zoom;
	int columns = ceil(Screen::Width() / (GRID * zoom)) + 1 + 2 * PAD;
	int rows = ceil(Screen::Height() / (GRID * zoom)) + 1 + 2 * PAD;
	// Round up to a multiple of 4 so the rows will be 32-bit aligned.
	columns = (columns + 3) & ~3;

	// To avoid extra work, don't regenerate the mask buffer if the view has not
	// moved. This might cause an inaccurate mask if you explore more systems,
	// come back to the original, and view the map again without viewing it in
	// between. But, that's an unlikely situation.
	const bool shouldRegenerate = (
		zoom != previousZoom || center.X() != previousCenter.X() || center.Y() != previousCenter.Y() ||
		left != previousLeft || top != previousTop || columns != previousColumns || rows != previousRows);
	if(shouldRegenerate)
	{
		bool sizeChanged = (!texture.GetTexture() || columns != previousColumns || rows != previousRows);

		// Remember the current viewport attributes.
		previousZoom = zoom;
		previousCenter = center;
		previousLeft = left;
		previousTop = top;
		previousColumns = columns;
		previousRows = rows;

		// This buffer will hold the mask image.
		auto buffer = std::vector<unsigned char>(static_cast<size_t>(rows) * columns, LIMIT);

		// For each system the player knows about, its "distance" pixel in the
		// buffer should be set to 0.
		for(const auto &it : GameData::Systems())
		{
			const System &system = it.second;
			if(!system.IsValid() || !player.CanView(system))
				continue;
			Point pos = zoom * (system.Position() + center);

			int x = round((pos.X() - left) / (GRID * zoom));
			int y = round((pos.Y() - top) / (GRID * zoom));
			if(x >= 0 && y >= 0 && x < columns && y < rows)
				buffer[x + y * columns] = 0;
		}

		// Distance transformation: make two passes through the buffer. In the first
		// pass, propagate down and to the right. In the second, propagate in the
		// opposite direction. Once these two passes are done, each value is equal
		for(int y = 1; y < rows; ++y)
			for(int x = 1; x < columns; ++x)
				buffer[x + y * columns] = std::min<int>(buffer[x + y * columns], std::min(
					ORTH + std::min(buffer[(x - 1) + y * columns], buffer[x + (y - 1) * columns]),
					DIAG + std::min(buffer[(x - 1) + (y - 1) * columns], buffer[(x + 1) + (y - 1) * columns])));
		for(int y = rows - 2; y >= 0; --y)
			for(int x = columns - 2; x >= 0; --x)
				buffer[x + y * columns] = std::min<int>(buffer[x + y * columns], std::min(
					ORTH + std::min(buffer[(x + 1) + y * columns], buffer[x + (y + 1) * columns]),
					DIAG + std::min(buffer[(x - 1) + (y + 1) * columns], buffer[(x + 1) + (y + 1) * columns])));

		// Stretch the distance values so there is no shading up to about 200 pixels
		// away, then it transitions somewhat quickly.
		for(unsigned char &value : buffer)
			value = std::max(0, std::min(LIMIT, (value - 60) * 4));
		const void *data = &buffer.front();

		// Set up the OpenGL texture if it doesn't exist yet.
		if(sizeChanged)
		{
			texture = graphics_layer::TextureHandle(GameWindow::GetInstance(), data, columns, rows, 1, GraphicsTypes::TextureType::TYPE_2D, GraphicsTypes::ImageFormat::R, GraphicsTypes::TextureTarget::READ);
		}
		else
		{
			// TODO: Replace instead
			texture = graphics_layer::TextureHandle(GameWindow::GetInstance(), data, columns, rows, 1, GraphicsTypes::TextureType::TYPE_2D, GraphicsTypes::ImageFormat::R, GraphicsTypes::TextureTarget::READ);
		}
	}


	shader.Bind();

	graphics_layer::TextureList texture_list;
	texture_list.AddTexture(texture.GetTexture(), 0, false);
	texture_list.Bind(GameWindow::GetInstance());

	const float corner[2] = {
		static_cast<float>(left - 0.5 * GRID * zoom) / ( 0.5f * static_cast<float>(Screen::Width())),
		static_cast<float>(top  - 0.5 * GRID * zoom) / (-0.5f * static_cast<float>(Screen::Height()))};
	const float dimensions[2] = {
		GRID * static_cast<float>(zoom) * (static_cast<float>(columns) + 1.f) / ( 0.5f * static_cast<float>(Screen::Width())),
		GRID * static_cast<float>(zoom) * (static_cast<float>(rows)    + 1.f) / (-0.5f * static_cast<float>(Screen::Height()))};

	const auto &info = shader.GetInfo();
	std::vector<unsigned char> data_cp(info.GetUniformSize());

	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), corner,     ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), dimensions, ++i);

	GameWindow::GetInstance()->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	square.Draw(GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}
