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
#include "../System.h"
#include "shared/graphics/graphics_layer.h"
#include "shared/graphics/graphics_toplevel_defines.h"
#include "shared/system/File.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

namespace {
	// Scale of the mask image:
	constexpr int GRID = 16;
	// Distance represented by one orthogonal or diagonal step:
	constexpr int ORTH = 5;
	constexpr int DIAG = 7;
	// Limit distances to the size of an unsigned char.
	constexpr int LIMIT = 255;
	// Pad beyond the screen enough to include any system that might "cast light"
	// on the on-screen view.
	constexpr int PAD = LIMIT / ORTH;


	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;

	graphics_layer::ObjectHandle geometry;
	graphics_layer::TextureHandle texture;

	// Keep track of the previous frame's view so that if it is unchanged we can
	// skip regenerating the mask.
	double previousZoom = 0.;
	double previousLeft = 0.;
	double previousTop = 0.;
	int previousColumns = 0;
	int previousRows = 0;
	Point previousCenter;
} // namespace



void FogShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // corner
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // dimensions

	info.AddTexture("tex");

	const auto shader_code = File::ReadShader("../shaders/compilation/fog.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "fog");


	// Corners of a rectangle to draw.
	constexpr float vertexData[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 2 * sizeof(float), vertexData, {}, "fog_geometry");
}



void FogShader::Redraw()
{
	previousZoom = 0.;
}



void FogShader::Draw(const GraphicsTypes::GraphicsInstance *graphics_instance, GraphicsTypes::CommandBufferInstance *cb,
                     const Point &center, const double zoom, const PlayerInfo &player)
{
	// Generate a scaled-down mask image that represents the entire screen plus
	// enough pixels beyond the screen to include any systems that may be off
	// screen but close enough to "illuminate" part of the on-screen map.
	const double left = Screen::Left() - GRID * PAD * zoom + fmod(center.X(), GRID) * zoom;
	const double top = Screen::Top() - GRID * PAD * zoom + fmod(center.Y(), GRID) * zoom;
	int columns = ceil(Screen::Width() / (GRID * zoom)) + 1 + 2 * PAD;
	int rows = ceil(Screen::Height() / (GRID * zoom)) + 1 + 2 * PAD;
	// Round up to a multiple of 4 so the rows will be 32-bit aligned.
	columns = (columns + 3) & ~3;

	// To avoid extra work, don't regenerate the mask buffer if the view has not
	// moved. This might cause an inaccurate mask if you explore more systems,
	// come back to the original, and view the map again without viewing it in
	// between. But, that's an unlikely situation.
	const bool shouldRegenerate =
	    (zoom != previousZoom || center.X() != previousCenter.X() || center.Y() != previousCenter.Y() ||
		 left != previousLeft || top != previousTop || columns != previousColumns || rows != previousRows);
	if(shouldRegenerate)
	{
		// Remember the current viewport attributes.
		previousZoom = zoom;
		previousCenter = center;
		previousLeft = left;
		previousTop = top;
		previousColumns = columns;
		previousRows = rows;

		// This buffer will hold the mask image.
		auto buffer = vector<unsigned char>(static_cast<size_t>(rows) * columns, LIMIT);

		// For each system the player knows about, its "distance" pixel in the
		// buffer should be set to 0.
		for(const auto &system : GameData::Systems() | views::values)
		{
			if(!system.IsValid() || !player.CanView(system))
				continue;
			Point pos = zoom * (system.Position() + center);

			const int x = round((pos.X() - left) / (GRID * zoom));
			const int y = round((pos.Y() - top) / (GRID * zoom));
			if(x >= 0 && y >= 0 && x < columns && y < rows)
				buffer[x + y * columns] = 0;
		}

		// Distance transformation: make two passes through the buffer. In the first
		// pass, propagate down and to the right. In the second, propagate in the
		// opposite direction. Once these two passes are done, each value is equal
		for(int y = 1; y < rows; ++y)
			for(int x = 1; x < columns; ++x)
				buffer[x + y * columns] =
				    min<int>(buffer[x + y * columns],
					         min(ORTH + min(buffer[(x - 1) + y * columns], buffer[x + (y - 1) * columns]),
					             DIAG + min(buffer[(x - 1) + (y - 1) * columns], buffer[(x + 1) + (y - 1) * columns])));
		for(int y = rows - 2; y >= 0; --y)
			for(int x = columns - 2; x >= 0; --x)
				buffer[x + y * columns] =
				    min<int>(buffer[x + y * columns],
					         min(ORTH + min(buffer[(x + 1) + y * columns], buffer[x + (y + 1) * columns]),
					             DIAG + min(buffer[(x - 1) + (y + 1) * columns], buffer[(x + 1) + (y + 1) * columns])));

		// Stretch the distance values so there is no shading up to about 200 pixels
		// away, then it transitions somewhat quickly.
		for(unsigned char &value : buffer)
			value = max(0, min(LIMIT, (value - 60) * 4));
		const void *data = &buffer.front();

		// Set up the OpenGL texture if it doesn't exist yet.
		texture = graphics_layer::TextureHandle(
		    graphics_instance, "fog", {data}, columns, rows, 1, GraphicsTypes::TextureType::TYPE_2D,
		    GraphicsTypes::ImageFormat::R, GraphicsTypes::TextureTarget::READ,
		    GraphicsTypes::TextureAddressMode::CLAMP_TO_EDGE, GraphicsTypes::TextureFilter::LINEAR);
	}

	cb->BindShader(shader.get());

	graphics_layer::QuickBindTextures(cb, {texture.GetTexture()});

	const gm::vec2 corner = {static_cast<float>(left - .5 * GRID * zoom) / (0.5f * Screen::Width()),
	                         static_cast<float>(top - .5 * GRID * zoom) / (-0.5f * Screen::Height())};
	const gm::vec2 dimensions = {GRID * static_cast<float>(zoom) * (columns + 1.0f) / (0.5f * Screen::Width()),
	                             GRID * static_cast<float>(zoom) * (rows + 1.0f) / (-0.5f * Screen::Height())};

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &corner, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &dimensions, ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}


void FogShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	texture.Reset();
	shader.reset();
}
