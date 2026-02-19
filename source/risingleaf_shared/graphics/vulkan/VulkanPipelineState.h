//
// This file is part of Astrolative.
//
// Copyright (c) 2025 by Torben Hans
//
// Astrolative is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
//  Astrolative is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see <https://www.gnu.org/licenses/>.
//
#ifndef VULKANPIPELINESTATE_H
#define VULKANPIPELINESTATE_H
#include "graphics/graphics_toplevel_defines.h"


namespace VulkanObjects
{
  struct VulkanPipelineState
  {
    bool                                  Color         = true;
    int                                   Samples       = 1;
    const class VulkanShaderInstance     *Shader        = nullptr;
    const class VulkanRenderPassInstance *RenderPass    = nullptr;

    GraphicsTypes::RenderState            RenderState{};

    bool operator==(const VulkanPipelineState &other) const
    {
      return this->Color         == other.Color
          && this->Samples       == other.Samples
          && this->RenderPass    == other.RenderPass
          && this->Shader        == other.Shader
          && this->RenderState   == other.RenderState;
    }
  };
}

#endif //VULKANPIPELINESTATE_H
