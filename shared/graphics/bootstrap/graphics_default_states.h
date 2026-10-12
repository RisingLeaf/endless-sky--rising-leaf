//
// This file is part of Astrolative.
//
// Copyright (c) 2026 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see
//  <https://www.gnu.org/licenses/>.
//

#ifndef ASTROLATIVE_GRAPHICS_DEFAULT_STATES_H
#define ASTROLATIVE_GRAPHICS_DEFAULT_STATES_H
#include "graphics/graphics_layer.h"
#include "graphics/graphics_toplevel_defines.h"

namespace graphics_bootstrap
{
  constexpr GraphicsTypes::RenderState RenderStateSimpleBlending(const asl::uint64 width, const asl::uint64 height)
  {
    GraphicsTypes::RenderState render_state;
    render_state.Blending     = { .BlendingEnabled = true,
                                  .SRCColor        = GraphicsTypes::BlendFactor::FACTOR_ONE,
                                  .DSTColor        = GraphicsTypes::BlendFactor::FACTOR_ONE_MINUS_SRC_ALPHA,
                                  .SRCAlpha        = GraphicsTypes::BlendFactor::FACTOR_ONE,
                                  .DSTAlpha        = GraphicsTypes::BlendFactor::FACTOR_ONE_MINUS_SRC_ALPHA };
    render_state.Culling      = GraphicsTypes::CullMode::CULL_NONE;
    render_state.DepthTest    = false;
    render_state.DepthWrite   = false;
    render_state.DepthCompare = GraphicsTypes::DepthCompareMode::NONE;
    render_state.WireFrame    = false;
    render_state.ColorMask    = 1 + 2 + 4 + 8;
    render_state.DrawViewport = GraphicsTypes::Viewport{ 0, 0, width, height };
    render_state.ClearColor   = GraphicsTypes::Color(0.f, 0.f, 0.f, 0.f);
    render_state.ClearDepth   = 0.f;
    return render_state;
  }

} // namespace graphics_bootstrap

#endif // ASTROLATIVE_GRAPHICS_DEFAULT_STATES_H
