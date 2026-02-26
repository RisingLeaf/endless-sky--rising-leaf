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
#include "VulkanFrameBufferInstance.h"

#include "VulkanBootstrap.h"
#include "VulkanCommandBuffer.h"
#include "VulkanHelpers.h"
#include "VulkanTexture.h"


VulkanObjects::VulkanFrameBufferInstance::VulkanFrameBufferInstance(
    const VulkanDeviceInstance           *device,
    const GraphicsTypes::FrameBufferInfo &info,
    const GraphicsTypes::StateInfo       &state,
    const size_t                          count,
    const std::string_view                name) :
  RenderBufferInstance(info), RenderPass(device, info.Format, state, std::string(name) + "_render_pass"), Device(device)
{
  if(info.TargetType != GraphicsTypes::RenderBufferType::DEPTH)
  {
    if(info.Samples > 1)
    {
      Images.emplace_back(
          std::make_unique<VulkanImageInstance>(
              Device,
              std::string(name) + "_resolve_image",
              GraphicsTypes::TextureType::TYPE_2D,
              info.Format,
              GraphicsTypes::TextureTarget::DRAW,
              1,
              info.Width,
              info.Height));
      Views.emplace_back(
          std::make_unique<VulkanViewInstance>(
              Device,
              std::string(name) + "_resolve_view",
              Images.back()->Get(),
              info.Format,
              GraphicsTypes::TextureType::TYPE_2D,
              1,
              1));
    }
    Images.emplace_back(
        std::make_unique<VulkanImageInstance>(
            Device,
            std::string(name) + "_color_image",
            GraphicsTypes::TextureType::TYPE_2D,
            info.Format,
            GraphicsTypes::TextureTarget::DRAW,
            info.Samples,
            info.Width,
            info.Height));
    Views.emplace_back(
        std::make_unique<VulkanViewInstance>(
            Device,
            std::string(name) + "_color_view",
            Images.back()->Get(),
            info.Format,
            GraphicsTypes::TextureType::TYPE_2D,
            1,
            1));
  }
  if(info.TargetType != GraphicsTypes::RenderBufferType::COLOR)
  {
    const auto depth_format =
        info.TargetType == GraphicsTypes::RenderBufferType::DEPTH ? info.Format : GraphicsTypes::ImageFormat::DEPTH;
    Images.emplace_back(
        std::make_unique<VulkanImageInstance>(
            Device,
            std::string(name) + "_depth_image",
            GraphicsTypes::TextureType::TYPE_2D,
            depth_format,
            GraphicsTypes::TextureTarget::DRAW,
            info.Samples,
            info.Width,
            info.Height));
    Views.emplace_back(
        std::make_unique<VulkanViewInstance>(
            Device,
            std::string(name) + "_depth_view",
            Images.back()->Get(),
            depth_format,
            GraphicsTypes::TextureType::TYPE_2D,
            1,
            1));
  }

  std::vector<VkImageView> attachments;
  for(const auto &view : Views)
    attachments.emplace_back(view->Get());

  const auto create_info =
      VulkanBootstrap::GetFrameBufferCreate(RenderPass.Get(), attachments, info.Width, info.Height);
  FrameBuffer.resize(count, nullptr);
  for(auto &buffer : FrameBuffer)
  {
    VulkanHelpers::VK_CHECK_RESULT(
        vkCreateFramebuffer(Device->GetDevice(), &create_info, nullptr, &buffer),
        __LINE__,
        __FILE__);
    Device->NameObject(VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(buffer), name);
  }

  GraphicsTypes::TextureInfo texture_info;
  texture_info.Samples   = info.Samples;
  texture_info.Width     = info.Width;
  texture_info.Height    = info.Height;
  texture_info.MipLevels = 1;

  Texture = std::make_unique<VulkanTextureInstance>(
      device,
      std::string(name) + "_texture",
      Images.front().get(),
      Views.front().get(),
      texture_info);
}

VulkanObjects::VulkanFrameBufferInstance::VulkanFrameBufferInstance(
    const VulkanDeviceInstance           *device,
    VkCommandBuffer                       cmd,
    const GraphicsTypes::FrameBufferInfo &info,
    const GraphicsTypes::StateInfo       &state,
    VkImage                               image,
    const std::string_view                name) :
  RenderBufferInstance(info),
  RenderPass(device, info.Format, state, std::string(name) + "_render_pass", true),
  Device(device)
{
  if(info.Samples > 1)
  {
    Images.emplace_back(
        std::make_unique<VulkanImageInstance>(
            Device,
            std::string(name) + "_resolve_image",
            GraphicsTypes::TextureType::TYPE_2D,
            info.Format,
            GraphicsTypes::TextureTarget::DRAW,
            1,
            info.Width,
            info.Height,
            1,
            1,
            1,
            image));
    Images.back()->SetLayout(cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    Views.emplace_back(
        std::make_unique<VulkanViewInstance>(
            Device,
            std::string(name) + "_resolve_view",
            Images.back()->Get(),
            info.Format,
            GraphicsTypes::TextureType::TYPE_2D,
            1,
            1));

    Images.emplace_back(
        std::make_unique<VulkanImageInstance>(
            Device,
            std::string(name) + "_color_image",
            GraphicsTypes::TextureType::TYPE_2D,
            info.Format,
            GraphicsTypes::TextureTarget::DRAW,
            info.Samples,
            info.Width,
            info.Height));
    Views.emplace_back(
        std::make_unique<VulkanViewInstance>(
            Device,
            std::string(name) + "_color_view",
            Images.back()->Get(),
            info.Format,
            GraphicsTypes::TextureType::TYPE_2D,
            1,
            1));
  }
  else {
    Images.emplace_back(
        std::make_unique<VulkanImageInstance>(
            Device,
            std::string(name) + "_color_image",
            GraphicsTypes::TextureType::TYPE_2D,
            info.Format,
            GraphicsTypes::TextureTarget::DRAW,
            1,
            info.Width,
            info.Height,
            1,
            1,
            1,
            image));
    Images.back()->SetLayout(cmd, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    Views.emplace_back(
        std::make_unique<VulkanViewInstance>(
            Device,
            std::string(name) + "_color_view",
            Images.back()->Get(),
            info.Format,
            GraphicsTypes::TextureType::TYPE_2D,
            1,
            1));
  }

  constexpr auto depth_format = GraphicsTypes::ImageFormat::DEPTH;
  Images.emplace_back(
      std::make_unique<VulkanImageInstance>(
          Device,
          std::string(name) + "_depth_image",
          GraphicsTypes::TextureType::TYPE_2D,
          depth_format,
          GraphicsTypes::TextureTarget::DRAW,
          info.Samples,
          info.Width,
          info.Height));
  Views.emplace_back(
      std::make_unique<VulkanViewInstance>(
          Device,
          std::string(name) + "_depth_view",
          Images.back()->Get(),
          depth_format,
          GraphicsTypes::TextureType::TYPE_2D,
          1,
          1));

  std::vector<VkImageView> attachments;
  for(const auto &view : Views)
    attachments.emplace_back(view->Get());

  const auto create_info =
      VulkanBootstrap::GetFrameBufferCreate(RenderPass.Get(), attachments, info.Width, info.Height);
  FrameBuffer.resize(1, nullptr);
  VulkanHelpers::VK_CHECK_RESULT(
      vkCreateFramebuffer(Device->GetDevice(), &create_info, nullptr, &FrameBuffer[0]),
      __LINE__,
      __FILE__);
  Device->NameObject(VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(FrameBuffer[0]), name);

  GraphicsTypes::TextureInfo texture_info;
  texture_info.Samples   = info.Samples;
  texture_info.Width     = info.Width;
  texture_info.Height    = info.Height;
  texture_info.MipLevels = 1;

  Texture = std::make_unique<VulkanTextureInstance>(
      device,
      std::string(name) + "_texture",
      Images.front().get(),
      Views.front().get(),
      texture_info);
}

VulkanObjects::VulkanFrameBufferInstance::~VulkanFrameBufferInstance()
{
  for(const auto buffer : FrameBuffer)
    if(buffer) Device->QueueFrameBufferForDeletion(buffer);
}
