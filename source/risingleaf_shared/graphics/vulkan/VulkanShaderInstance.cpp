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
//  You should have received a copy of the GNU General Public License along with Astrolative. If not, see
//  <https://www.gnu.org/licenses/>.
//
#include "VulkanShaderInstance.h"

#include "VulkanBootstrap.h"
#include "VulkanDeviceInstance.h"
#include "VulkanHelpers.h"
#include "VulkanPipelineState.h"
#include "VulkanRenderPassInstance.h"
#include "VulkanTranslate.h"
#include "system/File.h"
#include "system/Log.h"


VulkanObjects::VulkanShaderInstance::VulkanShaderInstance(const VulkanDeviceInstance            *device,
                                                          const ShaderInfo                      &info,
                                                          const std::vector<File::ShaderString> &shader_code,
                                                          const std::string_view                 name) :
  GraphicsTypes::ShaderInstance(), Info(info), Device(device), Name(name)
{
  for(const auto &code : shader_code)
  {
    VkShaderModule                  *shader_module     = nullptr;
    VkPipelineShaderStageCreateInfo *stage_create_info = nullptr;
    VulkanTranslate::ShaderStage     stage;
    std::string                      module_name = Name;
    switch(code.stage)
    {
    case GraphicsTypes::ShaderStage::VULKAN_VERTEX:
      shader_module     = &VertexShader;
      stage_create_info = &VertexShaderStage;
      stage             = VulkanTranslate::ShaderStage::VERTEX;
      module_name += "_vert";
      break;
    case GraphicsTypes::ShaderStage::VULKAN_FRAGMENT:
      shader_module     = &FragmentShader;
      stage_create_info = &FragmentShaderStage;
      stage             = VulkanTranslate::ShaderStage::FRAGMENT;
      module_name += "_frag";
      break;
    case GraphicsTypes::ShaderStage::VULKAN_COMPUTE:
      shader_module     = &ComputeShader;
      stage_create_info = &ComputeShaderStage;
      stage             = VulkanTranslate::ShaderStage::COMPUTE;
      module_name += "_comp";
      break;
    case GraphicsTypes::ShaderStage::METAL_COMBINED:
    default:                                         continue;
    }

    auto module_create_info = VulkanBootstrap::GetShaderModuleCreate(code.code);
    VulkanHelpers::VK_CHECK_RESULT(
        vkCreateShaderModule(Device->GetDevice(), &module_create_info, nullptr, shader_module),
        __LINE__,
        __FILE__);
    Device->NameObject(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(*shader_module), module_name);

    *stage_create_info = VulkanBootstrap::GetShaderStageCreate(*shader_module, stage);
  }

  const VkDescriptorSetLayoutBinding buffer =
      VulkanBootstrap::GetDescriptorSetLayoutBinding(0,
                                                     VulkanTranslate::ShaderStage::ALL,
                                                     VulkanTranslate::DescriptorType::UNIFORM_BUFFER);
  std::vector buffer_bindings           = {buffer};
  auto        common_buffer_create_info = VulkanBootstrap::GetDescriptorSetLayoutCreate(buffer_bindings);
  VulkanHelpers::VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Device->GetDevice(),
                                                             &common_buffer_create_info,
                                                             nullptr,
                                                             &DescriptorSetLayoutUBOCommon),
                                 __LINE__,
                                 __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                     reinterpret_cast<uint64_t>(DescriptorSetLayoutUBOCommon),
                     Name + "_cm_ubo_layout");


  auto specific_buffer_create_info = VulkanBootstrap::GetDescriptorSetLayoutCreate(buffer_bindings);
  VulkanHelpers::VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Device->GetDevice(),
                                                             &specific_buffer_create_info,
                                                             nullptr,
                                                             &DescriptorSetLayoutUBOSpecial),
                                 __LINE__,
                                 __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                     reinterpret_cast<uint64_t>(DescriptorSetLayoutUBOSpecial),
                     Name + "_spec_ubo_layout");

  std::vector<VkDescriptorSetLayoutBinding> specific_texture_list_info;
  for(size_t x = 0; x < info.GetSpecificTextureCount(); x++)
  {
    VkDescriptorSetLayoutBinding sampler_layout_binding =
        VulkanBootstrap::GetDescriptorSetLayoutBinding(x,
                                                       VulkanTranslate::ShaderStage::ALL,
                                                       VulkanTranslate::DescriptorType::TEXTURE);
    specific_texture_list_info.emplace_back(sampler_layout_binding);
  }

  auto specific_texture_create_info = VulkanBootstrap::GetDescriptorSetLayoutCreate(specific_texture_list_info);
  VulkanHelpers::VK_CHECK_RESULT(vkCreateDescriptorSetLayout(Device->GetDevice(),
                                                             &specific_texture_create_info,
                                                             nullptr,
                                                             &DescriptorSetLayoutTexturesSpecial),
                                 __LINE__,
                                 __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                     reinterpret_cast<uint64_t>(DescriptorSetLayoutTexturesSpecial),
                     Name + "_tex_layout");

  std::vector descriptor_set_layouts = {
      DescriptorSetLayoutUBOCommon,
      DescriptorSetLayoutUBOSpecial,
      DescriptorSetLayoutTexturesSpecial,
  };

  VkPipelineLayoutCreateInfo pipeline_layout_info =
      VulkanBootstrap::GetPipelineLayoutCreate(descriptor_set_layouts, {});
  VulkanHelpers::VK_CHECK_RESULT(
      vkCreatePipelineLayout(Device->GetDevice(), &pipeline_layout_info, nullptr, &PipelineLayout),
      __LINE__,
      __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                     reinterpret_cast<uint64_t>(PipelineLayout),
                     Name + "_pipeline_layout");
}

VulkanObjects::VulkanShaderInstance::~VulkanShaderInstance()
{
  if(DescriptorSetLayoutUBOCommon)
    vkDestroyDescriptorSetLayout(Device->GetDevice(), DescriptorSetLayoutUBOCommon, nullptr);
  if(DescriptorSetLayoutUBOSpecial)
    vkDestroyDescriptorSetLayout(Device->GetDevice(), DescriptorSetLayoutUBOSpecial, nullptr);
  if(DescriptorSetLayoutTexturesSpecial)
    vkDestroyDescriptorSetLayout(Device->GetDevice(), DescriptorSetLayoutTexturesSpecial, nullptr);

  if(PipelineLayout) vkDestroyPipelineLayout(Device->GetDevice(), PipelineLayout, nullptr);

  if(VertexShader) vkDestroyShaderModule(Device->GetDevice(), VertexShader, nullptr);
  if(FragmentShader) vkDestroyShaderModule(Device->GetDevice(), FragmentShader, nullptr);
  if(ComputeShader) vkDestroyShaderModule(Device->GetDevice(), ComputeShader, nullptr);

  for(const auto &pipeline : PipelinesWithState)
    if(pipeline.second) Device->QueuePipelineForDeletion(pipeline.second);
}

VkPipeline VulkanObjects::VulkanShaderInstance::GetPipelineForState(const VulkanPipelineState &state) const
{
  std::lock_guard guard(PipelinesListMutex);

  for(const auto &[it_state, it_pipeline] : PipelinesWithState)
    if(state == it_state) return it_pipeline;

  // We are using a dynamic viewport and scissor.
  constexpr std::array DYNAMIC_STATES = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_info{};
  dynamic_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(DYNAMIC_STATES.size());
  dynamic_state_info.pDynamicStates    = DYNAMIC_STATES.data();
  VkPipelineViewportStateCreateInfo viewport_state_info{};
  viewport_state_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_info.viewportCount = 1;
  viewport_state_info.scissorCount  = 1;

  // Create Vertex Input Info:
  VkVertexInputBindingDescription vertex_input_binding_info{};
  vertex_input_binding_info.binding   = 0;
  vertex_input_binding_info.stride    = Info.GetVertexSize();
  vertex_input_binding_info.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  std::vector<VkVertexInputAttributeDescription> vertex_attrib_info{};
  for(const auto &v_attrib : Info.GetVertexAttribs())
  {
    VkVertexInputAttributeDescription attribute_description;
    attribute_description.binding  = 0;
    attribute_description.location = v_attrib.Location;
    attribute_description.offset   = v_attrib.Offset;
    switch(v_attrib.Type)
    {
    case GraphicsTypes::ShaderType::INT:    attribute_description.format = VK_FORMAT_R32_SINT; break;
    case GraphicsTypes::ShaderType::FLOAT:  attribute_description.format = VK_FORMAT_R32_SFLOAT; break;
    case GraphicsTypes::ShaderType::INT2:   attribute_description.format = VK_FORMAT_R32G32_SINT; break;
    case GraphicsTypes::ShaderType::INT3:   attribute_description.format = VK_FORMAT_R32G32B32_SINT; break;
    case GraphicsTypes::ShaderType::INT4:   attribute_description.format = VK_FORMAT_R32G32B32A32_SINT; break;
    case GraphicsTypes::ShaderType::FLOAT2: attribute_description.format = VK_FORMAT_R32G32_SFLOAT; break;
    case GraphicsTypes::ShaderType::FLOAT3: attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT; break;
    case GraphicsTypes::ShaderType::FLOAT4: attribute_description.format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
    case GraphicsTypes::ShaderType::MAT2:
    case GraphicsTypes::ShaderType::MAT3:
    case GraphicsTypes::ShaderType::MAT4:
    default:
      {
        Log::Error << "Invalid value type for vertex attribute" << Log::End;
        return nullptr;
      }
    }
    vertex_attrib_info.emplace_back(attribute_description);
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount   = 1;
  vertex_input_info.pVertexBindingDescriptions      = &vertex_input_binding_info;
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attrib_info.size());
  vertex_input_info.pVertexAttributeDescriptions    = vertex_attrib_info.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
  input_assembly_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_info.topology = VulkanTranslate::GetVkPrimitiveTopology(state.RenderState.DrawPrimitiveType);
  input_assembly_info.primitiveRestartEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  if(state.RenderState.WireFrame) rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
  else rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VulkanTranslate::GetVkCullMode(state.RenderState.Culling);
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp          = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VulkanTranslate::GetVkSampleCountFromInt(state.RenderPass->GetState().Samples);

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.blendEnable         = state.RenderState.Blending.BlendingEnabled ? VK_TRUE : VK_FALSE;
  color_blend_attachment.srcColorBlendFactor = VulkanTranslate::GetVkBlendFactor(state.RenderState.Blending.SRCColor);
  color_blend_attachment.dstColorBlendFactor = VulkanTranslate::GetVkBlendFactor(state.RenderState.Blending.DSTColor);
  color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VulkanTranslate::GetVkBlendFactor(state.RenderState.Blending.SRCAlpha);
  color_blend_attachment.dstAlphaBlendFactor = VulkanTranslate::GetVkBlendFactor(state.RenderState.Blending.DSTAlpha);
  color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  color_blend_attachment.colorWriteMask      = (state.RenderState.ColorMask & 1) != 0 ? VK_COLOR_COMPONENT_R_BIT : 0;
  color_blend_attachment.colorWriteMask |= (state.RenderState.ColorMask & 2) != 0 ? VK_COLOR_COMPONENT_G_BIT : 0;
  color_blend_attachment.colorWriteMask |= (state.RenderState.ColorMask & 4) != 0 ? VK_COLOR_COMPONENT_B_BIT : 0;
  color_blend_attachment.colorWriteMask |= (state.RenderState.ColorMask & 8) != 0 ? VK_COLOR_COMPONENT_A_BIT : 0;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY; // Optional
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  VkPipelineDepthStencilStateCreateInfo depth_stencil{};
  depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable       = state.RenderState.DepthTest ? VK_TRUE : VK_FALSE;
  depth_stencil.depthWriteEnable      = state.RenderState.DepthWrite ? VK_TRUE : VK_FALSE;
  depth_stencil.depthCompareOp        = VulkanTranslate::GetVkDepthCompare(state.RenderState.DepthCompare);
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.stencilTestEnable     = VK_FALSE;

  std::vector stages = {VertexShaderStage};
  if(state.Color) stages.emplace_back(FragmentShaderStage);

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount          = stages.size();
  pipeline_info.pStages             = stages.data();
  pipeline_info.pVertexInputState   = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly_info;
  pipeline_info.pViewportState      = &viewport_state_info;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState   = &multisampling;
  pipeline_info.pDepthStencilState  = &depth_stencil;
  pipeline_info.pColorBlendState    = &color_blending;
  pipeline_info.pDynamicState       = &dynamic_state_info;
  pipeline_info.layout              = GetPipelineLayout();
  pipeline_info.renderPass          = state.RenderPass->Get();
  pipeline_info.subpass             = 0;
  pipeline_info.basePipelineHandle  = VK_NULL_HANDLE; // Optional
  pipeline_info.basePipelineIndex   = -1;

  auto &it = PipelinesWithState.emplace_back(state, nullptr);

  VulkanHelpers::VK_CHECK_RESULT(
      vkCreateGraphicsPipelines(Device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &it.second),
      __LINE__,
      __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_PIPELINE,
                     reinterpret_cast<uint64_t>(it.second),
                     Name + "_pipeline");

  return it.second;
}
