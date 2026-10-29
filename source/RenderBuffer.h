/* RenderBuffer.h
Copyright (c) 2023 by thewierdnut

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Point.h"
#include "graphics/graphics_layer.h"



namespace GraphicsTypes {
	class GraphicsInstance;
}
// Class that can redirect all drawing commands to an internal texture.
// This buffer uses coordinates from (0, 0) in the top left, to (width, height)
// in the bottom right.
class RenderBuffer {
public:
	// Create a texture of the given size that can be used as a render target.
	RenderBuffer(const gm::ivec2 &dimensions);
	virtual ~RenderBuffer();

	// Initialize the shaders used internally.
	static void Init(const GraphicsTypes::GraphicsInstance *graphics_instance);
	static void Clear();

	// Turn this buffer on as a render target. The render target is restored if
	// the Activation object goes out of scope.
	[[nodiscard]] GraphicsTypes::CommandBufferInstance *SetTarget();
	void                                                Deactivate();

	// Draw the contents of this buffer at the specified position.
	void Draw(GraphicsTypes::CommandBufferInstance *cb, const Point &position);
	// Draw the contents of this buffer at the specified position, clipping the contents.
	void Draw(GraphicsTypes::CommandBufferInstance *cb,
	          const Point                          &position,
	          const Point                          &clipsize,
	          const Point                          &srcposition = Point());

	[[nodiscard]] double Top() const;
	[[nodiscard]] double Bottom() const;
	[[nodiscard]] double Left() const;
	[[nodiscard]] double Right() const;
	[[nodiscard]] Point  Dimensions() const;
	[[nodiscard]] double Height() const;
	[[nodiscard]] double Width() const;

	void SetFadePadding(float top, float bottom, float right = 0, float left = 0);

private:
	std::unique_ptr<GraphicsTypes::CommandBufferInstance> RenderCmd;
	graphics_layer::FrameBufferHandle                     framebuffer;
	gm::ivec2                                             size;
	gm::ivec2                                             last_dimensions;
	float                                                 fadePadding[4] = {};

	Point multiplier;
};
