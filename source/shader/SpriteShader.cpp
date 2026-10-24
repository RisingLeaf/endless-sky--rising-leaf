/* SpriteShader.cpp
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

#include "SpriteShader.h"

#include "../GameData.h"
#include "../Screen.h"
#include "../Swizzle.h"
#include "../image/Sprite.h"
#include "shared/system/File.h"

using namespace std;

namespace {
	std::unique_ptr<GraphicsTypes::ShaderInstance> shader;
	ShaderInfo info;

	graphics_layer::ObjectHandle geometry;
	graphics_layer::TextureHandle dummySwizzleMask;
} // namespace

// Initialize the shaders.
void SpriteShader::Init(const GraphicsTypes::GraphicsInstance *graphics_instance)
{
	info.AddInput(GraphicsTypes::ShaderType::FLOAT2, 0, 0);
	info.SetInputSize(2 * sizeof(float));

	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // scale
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // position
	info.AddUniformVariable(GraphicsTypes::ShaderType::MAT2);   // transform
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT2); // blur
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // clip
	info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // useSwizzleMask
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // frame
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // frameCount
	info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // uniqueSwizzleMaskFrames
	info.AddUniformVariable(GraphicsTypes::ShaderType::MAT4);   // swizzleMatrix
	info.AddUniformVariable(GraphicsTypes::ShaderType::INT);    // useSwizzle
	info.AddUniformVariable(GraphicsTypes::ShaderType::FLOAT);  // alpha

	info.AddTexture("tex");
	info.AddTexture("swizzleMask");

	const auto shader_code = File::ReadShader("../shaders/compilation/sprite.ps");
	graphics_instance->CreateShader(shader, info, shader_code, "sprite");


	constexpr float vertexData[] = {-.5f, -.5f, -.5f, .5f, .5f, -.5f, .5f, .5f};
	geometry = graphics_layer::ObjectHandle(graphics_instance, 4, 2 * sizeof(float), vertexData, {}, "sprite");

	constexpr uint8_t dummyData[] = {255, 0, 255, 255};
	dummySwizzleMask = graphics_layer::TextureHandle(
	    graphics_instance, "dummy", {dummyData}, 1, 1, 1, GraphicsTypes::TextureType::TYPE_2D_ARRAY,
	    GraphicsTypes::ImageFormat::RGBA, GraphicsTypes::TextureTarget::READ);
}



void SpriteShader::Draw(GraphicsTypes::CommandBufferInstance *cb, const Sprite *sprite, const Point &position,
                        float zoom, const Swizzle *swizzle, float frame, const Point &unit)
{
	if(!sprite)
		return;

	Bind(cb);
	Add(cb, Prepare(sprite, position, zoom, swizzle, frame, unit));
}



SpriteShader::Item SpriteShader::Prepare(const Sprite *sprite, const Point &position, float zoom,
                                         const Swizzle *swizzle, float frame, const Point &unit)
{
	Item item{.texture = sprite->Texture(), .swizzleMask = sprite->SwizzleMask()};
	if(!sprite)
		return item;

	item.frame = frame;
	item.frameCount = sprite->Frames();
	item.uniqueSwizzleMaskFrames = sprite->SwizzleMaskFrames() > 1;
	// Position.
	item.position[0] = static_cast<float>(position.X());
	item.position[1] = static_cast<float>(position.Y());
	// Rotation and scale.
	Point scaledUnit = unit * zoom;
	Point uw = scaledUnit * sprite->Width();
	Point uh = scaledUnit * sprite->Height();
	item.transform[0, 0] = static_cast<float>(-uw.Y());
	item.transform[0, 1] = static_cast<float>(uw.X());
	item.transform[1, 0] = static_cast<float>(-uh.X());
	item.transform[1, 1] = static_cast<float>(-uh.Y());
	// Swizzle.
	item.swizzle = swizzle;

	return item;
}



void SpriteShader::Bind(GraphicsTypes::CommandBufferInstance *cb)
{
	cb->BindShader(shader.get());
}



void SpriteShader::Add(GraphicsTypes::CommandBufferInstance *cb, const Item &item, const bool withBlur)
{
	const gm::vec2 scale = {2.f / Screen::Width(), -2.f / Screen::Height()};
	const int useSwizzleMask = item.swizzle && item.swizzle->OverrideMask() ? 0
	                           : item.swizzleMask->GetTexture()             ? 1
	                                                                        : 0;
	const int useSwizzle = item.swizzle && !item.swizzle->IsIdentity();

	if(useSwizzleMask && item.swizzleMask->GetTexture())
		graphics_layer::QuickBindTextures(cb, {item.texture->GetTexture(), item.swizzleMask->GetTexture()});
	else
		graphics_layer::QuickBindTextures(cb, {item.texture->GetTexture(), dummySwizzleMask.GetTexture()});

	static constexpr gm::vec2 UNBLURRED = {0.f, 0.f};
	const gm::vec2 blur = withBlur ? gm::vec2{item.blur[0], item.blur[1]} : UNBLURRED;

	std::vector<unsigned char> data_cp(info.GetUniformSize());
	int i = -1;
	info.CopyUniformEntryToBuffer(data_cp.data(), &scale, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.position, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.transform, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &blur, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.clip, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &useSwizzleMask, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.frame, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.frameCount, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.uniqueSwizzleMaskFrames, ++i);
	if(item.swizzle)
		info.CopyUniformEntryToBuffer(data_cp.data(), item.swizzle->MatrixPtr(), ++i);
	else
	{
		constexpr static auto ID = gm::mat4(1.0f);
		info.CopyUniformEntryToBuffer(data_cp.data(), &ID, ++i);
	}
	info.CopyUniformEntryToBuffer(data_cp.data(), &useSwizzle, ++i);
	info.CopyUniformEntryToBuffer(data_cp.data(), &item.alpha, ++i);
	cb->BindBufferDynamic(data_cp, GraphicsTypes::UBOBindPoint::Specific);

	geometry.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLE_STRIP);
}


void SpriteShader::Clear()
{
	geometry = graphics_layer::ObjectHandle();
	dummySwizzleMask.Reset();
	shader.reset();
}
