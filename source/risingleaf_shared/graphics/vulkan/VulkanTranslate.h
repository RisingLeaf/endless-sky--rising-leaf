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
#ifndef VULKANTRANSLATE_H
#define VULKANTRANSLATE_H

#include <cstdint>
#include <stdexcept>

#include <vulkan/vulkan_core.h>

#include "graphics/graphics_toplevel_defines.h"



namespace VulkanTranslate
{
  constexpr uint32_t MIN_SAMPLES = 1;

  enum class CommandBufferType : uint8_t
  {
    ONE_TIME,
    REUSE
  };
  enum class ShaderStage : uint8_t
  {
    VERTEX,
    FRAGMENT,
    COMPUTE,
    ALL
  };
  enum class DescriptorType : uint8_t
  {
    UNIFORM_BUFFER,
    TEXTURE,
    STORAGE_TEXTURE,
  };
  enum class AttachmentType : uint8_t
  {
    COLOR,
    COLOR_RESOLVE,
    DEPTH,
  };

  constexpr VkSampleCountFlagBits GetVkSampleCountFromInt(const uint32_t samples)
  {
    if(samples >= 64) return VK_SAMPLE_COUNT_64_BIT;
    if(samples >= 32) return VK_SAMPLE_COUNT_32_BIT;
    if(samples >= 16) return VK_SAMPLE_COUNT_16_BIT;
    if(samples >=  8) return VK_SAMPLE_COUNT_8_BIT;
    if(samples >=  4) return VK_SAMPLE_COUNT_4_BIT;
    if(samples >=  2) return VK_SAMPLE_COUNT_2_BIT;
    else              return VK_SAMPLE_COUNT_1_BIT;
  }

  constexpr VkImageType GetVkImageType(const GraphicsTypes::TextureType type)
  {
    switch(type)
    {
    case GraphicsTypes::TextureType::TYPE_2D:
    case GraphicsTypes::TextureType::TYPE_2D_ARRAY:   return VK_IMAGE_TYPE_2D;
    case GraphicsTypes::TextureType::TYPE_3D:         return VK_IMAGE_TYPE_3D;
    case GraphicsTypes::TextureType::TYPE_CUBE:
    case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY: return VK_IMAGE_TYPE_2D;
    }
    throw std::runtime_error("Image type not implemented!");
  }

  constexpr VkImageViewType GetVkViewType(const GraphicsTypes::TextureType type)
  {
    switch(type)
    {
    case GraphicsTypes::TextureType::TYPE_2D:         return VK_IMAGE_VIEW_TYPE_2D;
    case GraphicsTypes::TextureType::TYPE_2D_ARRAY:   return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case GraphicsTypes::TextureType::TYPE_3D:         return VK_IMAGE_VIEW_TYPE_3D;
    case GraphicsTypes::TextureType::TYPE_CUBE:       return VK_IMAGE_VIEW_TYPE_CUBE;
    case GraphicsTypes::TextureType::TYPE_CUBE_ARRAY: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
    throw std::runtime_error("ImageView type not implemented!");
  }

  constexpr VkFormat GetVkFormat(const GraphicsTypes::ImageFormat &format)
  {
    switch(format)
    {
    case GraphicsTypes::ImageFormat::R:       return VK_FORMAT_R8_UNORM;
    case GraphicsTypes::ImageFormat::RG:      return VK_FORMAT_R8G8_UNORM;
    case GraphicsTypes::ImageFormat::RGB:     return VK_FORMAT_R8G8B8_UNORM;
    case GraphicsTypes::ImageFormat::RGBA:    return VK_FORMAT_R8G8B8A8_UNORM;
    case GraphicsTypes::ImageFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case GraphicsTypes::ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case GraphicsTypes::ImageFormat::BGRA:    return VK_FORMAT_B8G8R8A8_SRGB;
    case GraphicsTypes::ImageFormat::DEPTH:   return VK_FORMAT_D32_SFLOAT;
    case GraphicsTypes::ImageFormat::INVALID: break;
    }
    throw std::runtime_error("Internal format not implemented/translated in vulkan.");
  }

  constexpr uint32_t GetComponentsOfFormat(const GraphicsTypes::ImageFormat format)
  {
    switch(format)
    {
    case GraphicsTypes::ImageFormat::R:       return 1;
    case GraphicsTypes::ImageFormat::RG:      return 2;
    case GraphicsTypes::ImageFormat::RGB:
    case GraphicsTypes::ImageFormat::RGBA:
    case GraphicsTypes::ImageFormat::RGBA16F:
    case GraphicsTypes::ImageFormat::RGBA32F:
    case GraphicsTypes::ImageFormat::BGRA:    return 4;
    case GraphicsTypes::ImageFormat::DEPTH:   return 1;
    case GraphicsTypes::ImageFormat::INVALID: return 0;
    }
    throw std::runtime_error("Missing Format case in GetComponentsOfFormat");
  }

  constexpr uint32_t GetByteCountOfFormat(const GraphicsTypes::ImageFormat format)
  {
    switch(format)
    {
    case GraphicsTypes::ImageFormat::R:
    case GraphicsTypes::ImageFormat::RG:
    case GraphicsTypes::ImageFormat::RGB:
    case GraphicsTypes::ImageFormat::RGBA:    return 1;
    case GraphicsTypes::ImageFormat::RGBA16F: return 2;
    case GraphicsTypes::ImageFormat::RGBA32F: return 4;
    case GraphicsTypes::ImageFormat::BGRA:    return 1;
    case GraphicsTypes::ImageFormat::DEPTH:   return 4;
    case GraphicsTypes::ImageFormat::INVALID: return 0;
    }
    throw std::runtime_error("Missing Format case in GetComponentsOfFormat");
  }

  constexpr std::pair<VkPipelineStageFlags, VkAccessFlags> GetVkLayoutInfo(const VkImageLayout layout)
  {
    switch(layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:                        return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT     , 0                                                      };
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:             return {VK_PIPELINE_STAGE_TRANSFER_BIT        , VK_ACCESS_TRANSFER_WRITE_BIT                           };
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:             return {VK_PIPELINE_STAGE_TRANSFER_BIT        , 0                                                      };
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:         return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT , VK_ACCESS_SHADER_READ_BIT                              };
    case VK_IMAGE_LAYOUT_GENERAL:                          return {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT  , VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT };
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT    , VK_ACCESS_SHADER_WRITE_BIT                             };
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:         return {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT    , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT                   };
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                  return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT  , VK_ACCESS_NONE_KHR                                     };
    default: break;
    }
    throw std::runtime_error("Layout has no associated information regarding stage and access!");
  }

  constexpr VkPrimitiveTopology GetVkPrimitiveTopology(const GraphicsTypes::PrimitiveType type)
  {
    switch(type)
    {
      case GraphicsTypes::PrimitiveType::TRIANGLES: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      case GraphicsTypes::PrimitiveType::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      case GraphicsTypes::PrimitiveType::LINES:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      case GraphicsTypes::PrimitiveType::POINTS:    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }

    throw std::runtime_error("Missing Primitive Topology in Vulkan");
  }

  constexpr VkCullModeFlagBits GetVkCullMode(const GraphicsTypes::CullMode mode)
  {
    switch(mode)
    {
    case GraphicsTypes::CullMode::CULL_NONE:  return VK_CULL_MODE_NONE;
    case GraphicsTypes::CullMode::CULL_BACK:  return VK_CULL_MODE_BACK_BIT;
    case GraphicsTypes::CullMode::CULL_FRONT: return VK_CULL_MODE_FRONT_BIT;
    }

    throw std::runtime_error("Missing Cull Mode in Vulkan");
  }

  constexpr VkBlendFactor GetVkBlendFactor(const GraphicsTypes::BlendFactor factor)
  {
    switch(factor)
    {
    case GraphicsTypes::BlendFactor::FACTOR_ONE:                 return VK_BLEND_FACTOR_ONE;
    case GraphicsTypes::BlendFactor::FACTOR_ZERO:                return VK_BLEND_FACTOR_ZERO;
    case GraphicsTypes::BlendFactor::FACTOR_SRC_ALPHA:           return VK_BLEND_FACTOR_SRC_ALPHA;
    case GraphicsTypes::BlendFactor::FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }

    throw std::runtime_error("Missing Blend Factor in Vulkan");
  }

  constexpr VkCompareOp GetVkDepthCompare(const GraphicsTypes::DepthCompareMode mode)
  {
    switch(mode)
    {
    case GraphicsTypes::DepthCompareMode::NONE:                   return VK_COMPARE_OP_ALWAYS;
    case GraphicsTypes::DepthCompareMode::COMPARE_GREATER:        return VK_COMPARE_OP_GREATER;
    case GraphicsTypes::DepthCompareMode::COMPARE_GREATER_EQUALS: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case GraphicsTypes::DepthCompareMode::COMPARE_LESS:           return VK_COMPARE_OP_LESS;
    case GraphicsTypes::DepthCompareMode::COMPARE_LESS_EQUALS:    return VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    throw std::runtime_error("Missing Depth Compare in Vulkan");
  }
}



#endif //VULKANTRANSLATE_H
