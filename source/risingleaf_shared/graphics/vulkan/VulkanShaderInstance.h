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
#ifndef VULKANSHADERINSTANCE_H
#define VULKANSHADERINSTANCE_H

#include <mutex>
#include <vulkan/vulkan_core.h>

#include "graphics/ShaderInfo.h"
#include "graphics/graphics_toplevel_defines.h"

namespace File
{
  struct ShaderString;
}
namespace VulkanObjects {
  class VulkanDeviceInstance;
  struct VulkanPipelineState;

  class VulkanShaderInstance final : public GraphicsTypes::ShaderInstance
  {
    const ShaderInfo     &Info;

    VkDescriptorSetLayout DescriptorSetLayoutUBOCommon       = nullptr;
    VkDescriptorSetLayout DescriptorSetLayoutUBOSpecial      = nullptr;
    VkDescriptorSetLayout DescriptorSetLayoutTexturesSpecial = nullptr;
    VkPipelineLayout      PipelineLayout                     = nullptr;

    VkShaderModule        VertexShader   = nullptr;
    VkShaderModule        FragmentShader = nullptr;
    VkShaderModule        ComputeShader  = nullptr;

    VkPipelineShaderStageCreateInfo VertexShaderStage{};
    VkPipelineShaderStageCreateInfo FragmentShaderStage{};
    VkPipelineShaderStageCreateInfo ComputeShaderStage{};

    mutable std::vector<std::pair<VulkanPipelineState, VkPipeline>> PipelinesWithState;
    mutable std::mutex PipelinesListMutex;

    const VulkanDeviceInstance *Device;

  public:
    VulkanShaderInstance(
      const VulkanDeviceInstance            *device,
      const ShaderInfo                      &info,
      const std::vector<File::ShaderString> &shader_code);

    ~VulkanShaderInstance() override;

    VulkanShaderInstance(const VulkanShaderInstance &other) = delete;
    VulkanShaderInstance(VulkanShaderInstance &&other) noexcept = delete;
    VulkanShaderInstance &operator=(const VulkanShaderInstance &other) = delete;
    VulkanShaderInstance &operator=(VulkanShaderInstance &&other) noexcept = delete;

    VkPipeline GetPipelineForState(const VulkanPipelineState &state) const;
    [[nodiscard]] const VkPipelineShaderStageCreateInfo &GetComputeShaderStage() const { return ComputeShaderStage; }
    [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayoutUBOCommon()        const { return DescriptorSetLayoutUBOCommon; }
    [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayoutUBOSpecial()       const { return DescriptorSetLayoutUBOSpecial; }
    [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayoutTexturesSpecial()  const { return DescriptorSetLayoutTexturesSpecial; }
    [[nodiscard]] VkPipelineLayout      GetPipelineLayout()                      const { return PipelineLayout; }
  };
}


#endif //VULKANSHADERINSTANCE_H
