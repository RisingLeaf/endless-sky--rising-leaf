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

#ifndef ASTROLATIVE_GRAPHICS_QUICK_ACTIONS_H
#define ASTROLATIVE_GRAPHICS_QUICK_ACTIONS_H
#include "graphics/ShaderInfo.h"
#include "graphics/graphics_layer.h"
#include "loading/WavefrontObj.h"
#include "source/shader_source/ShaderInstance.h"

namespace graphics_bootstrap
{
  inline graphics_layer::ObjectHandle CreateScreenTriangle(const GraphicsTypes::GraphicsInstance *graphics_instance)
  {
    constexpr static std::array<gm::vec2, 3> SCREEN_TRIANGLE_VERTICES{gm::vec2{3., 1.}, gm::vec2{-1., 3.}, gm::vec2{-1., -3.}};
    graphics_layer::ObjectHandle             ScreenTriangle(graphics_instance,
                                                SCREEN_TRIANGLE_VERTICES.size(),
                                                sizeof(gm::vec2),
                                                SCREEN_TRIANGLE_VERTICES.data(),
                                                            {0, 1, 2},
                                                "screen_triangle");
    return ScreenTriangle;
  }

  inline void SetCommonUniforms(GraphicsTypes::CommandBufferInstance *cb, const ShaderInfo::CommonUniformBufferData &data)
  {
    std::vector<unsigned char> ubo_data_cm(ShaderInfo::GetCommonUniformSize());
    ShaderInfo::CopyCommonUniformDataToBuffer(ubo_data_cm.data(), data);
    cb->SetCommonUBO(ubo_data_cm);
  }

  inline void FinalizeRendering(GraphicsTypes::GraphicsInstance           *graphics_instance,
                                   GraphicsTypes::CommandBufferInstance      *cb,
                                   const GraphicsTypes::ShaderInstance       *shader_instance,
                                   const ShaderInfo::CommonUniformBufferData &common_data,
                                   const GraphicsTypes::TextureInstance      *final_picture,
                                   const graphics_layer::ObjectHandle        &screen_triangle)
  {
    graphics_instance->StartMainRenderPass(cb);
    cb->BindShader(shader_instance);
    SetCommonUniforms(cb, common_data);

    graphics_layer::QuickBindTextures(cb, {final_picture});

    screen_triangle.Draw(cb, GraphicsTypes::PrimitiveType::TRIANGLES);

    cb->EndRenderPass();
  }

  inline graphics_layer::ObjectHandle LoadWavefrontIntoObject(const GraphicsTypes::GraphicsInstance *graphics_instance,
                                                                 const std::string_view                 name)
  {
    std::vector<WavefrontObj::Vertex> obj_vertices;
    std::vector<asl::uint32>          indices;
    WavefrontObj::LoadObj(name, obj_vertices, indices);
    std::vector<DefaultShader::Vertex> vertices;
    vertices.reserve(obj_vertices.size());
    for(auto &[Position, Normal, UV] : obj_vertices)
      vertices.emplace_back(Position, Normal, UV);
    return {graphics_instance, vertices.size(), sizeof(DefaultShader::Vertex), vertices.data(), indices, name};
  }

  inline graphics_layer::ObjectHandle LoadWavefrontIntoObjectColor(const GraphicsTypes::GraphicsInstance *graphics_instance,
                                                                 const std::string_view                 name)
  {
    std::vector<WavefrontObj::Vertex> obj_vertices;
    std::vector<asl::uint32>          indices;
    WavefrontObj::LoadObj(name, obj_vertices, indices);
    std::vector<ColorShader::Vertex> vertices;
    vertices.reserve(obj_vertices.size());
    for(auto &[Position, Normal, UV] : obj_vertices)
      vertices.emplace_back(gm::vec4(Position, 0.));
    return {graphics_instance, vertices.size(), sizeof(ColorShader::Vertex), vertices.data(), indices, name};
  }
} // namespace graphics_bootstrap

#endif // ASTROLATIVE_GRAPHICS_QUICK_ACTIONS_H
