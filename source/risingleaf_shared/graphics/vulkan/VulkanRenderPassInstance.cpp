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
#include "VulkanRenderPassInstance.h"

#include "VulkanBootstrap.h"
#include "VulkanHelpers.h"


VulkanObjects::VulkanRenderPassInstance::VulkanRenderPassInstance(
    const VulkanDeviceInstance      *device,
    const GraphicsTypes::ImageFormat image_format,
    const GraphicsTypes::StateInfo  &state,
    const std::string_view           name,
    const bool                       is_swap_chain) :
  RenderPassInstance(state), Device(device)
{
  VkAttachmentDescription color_attachment{};
  VkAttachmentReference   color_attachment_ref{};
  VkAttachmentDescription depth_attachment{};
  VkAttachmentReference   depth_attachment_ref{};
  VkAttachmentDescription color_attachment_resolve;
  VkAttachmentReference   color_attachment_resolve_ref{};

  int i = 0;

  const bool has_resolve = state.Samples > 1 && state.Color;
  if(has_resolve)
  {
    color_attachment_resolve = VulkanBootstrap::GetAttachmentDescription(
        VulkanTranslate::AttachmentType::COLOR_RESOLVE,
        image_format,
        VulkanTranslate::MIN_SAMPLES,
        is_swap_chain);
    color_attachment_resolve_ref =
        VulkanBootstrap::GetAttachmentReference(i, VulkanTranslate::AttachmentType::COLOR_RESOLVE);
    i++;
  }
  if(state.Color)
  {
    color_attachment = VulkanBootstrap::GetAttachmentDescription(
        VulkanTranslate::AttachmentType::COLOR,
        image_format,
        state.Samples,
        is_swap_chain && !has_resolve);
    color_attachment_ref = VulkanBootstrap::GetAttachmentReference(i, VulkanTranslate::AttachmentType::COLOR);
    i++;
  }
  if(state.Depth)
  {
    depth_attachment = VulkanBootstrap::GetAttachmentDescription(
        VulkanTranslate::AttachmentType::DEPTH,
        GraphicsTypes::ImageFormat::DEPTH,
        state.Samples,
        false);
    depth_attachment_ref = VulkanBootstrap::GetAttachmentReference(i, VulkanTranslate::AttachmentType::DEPTH);
    i++;
  }

  std::vector<VkAttachmentDescription> attachments;
  if(has_resolve) attachments.emplace_back(color_attachment_resolve);
  if(state.Color) attachments.emplace_back(color_attachment);
  if(state.Depth) attachments.emplace_back(depth_attachment);

  std::vector<VkAttachmentReference> color_attachment_refs;
  if(state.Color) color_attachment_refs.emplace_back(color_attachment_ref);

  std::vector<VkSubpassDescription> sub_passes;
  sub_passes.emplace_back(
      VulkanBootstrap::GetSubPassDescription(
          color_attachment_refs,
          depth_attachment_ref,
          color_attachment_resolve_ref));

  std::vector<VkSubpassDependency> dependencies;
  if(has_resolve || is_swap_chain)
  {
    dependencies.emplace_back(VulkanBootstrap::GetSubPassDependencyResolve());
    dependencies.emplace_back(VulkanBootstrap::GetSubPassDependencyResolveExit());
  }

  VkRenderPassCreateInfo render_pass_info = VulkanBootstrap::GetRenderPassCreate(attachments, sub_passes, dependencies);

  VulkanHelpers::VK_CHECK_RESULT(
      vkCreateRenderPass(Device->GetDevice(), &render_pass_info, nullptr, &RenderPass),
      __LINE__,
      __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_RENDER_PASS, reinterpret_cast<uint64_t>(RenderPass), name);
}

VulkanObjects::VulkanRenderPassInstance::~VulkanRenderPassInstance()
{
  if(RenderPass) Device->QueueRenderPassForDeletion(RenderPass);
}
