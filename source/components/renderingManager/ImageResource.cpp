#include "ImageResource.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {
  ImageResource::ImageResource(const ImageResourceConfig& config)
    {
      createImage(config);

      createImageView(config);

      transitionImageLayout(config);

      if (config.imageResourceType == ImageResourceType::Resolve)
      {
        m_descriptorSet = ImGui_ImplVulkan_AddTexture(
          config.sampler,
          *m_imageView,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
      }

      if (config.imageResourceType == ImageResourceType::RayTracingOutput)
      {
        m_descriptorImageInfo = vk::DescriptorImageInfo{
          .imageView = *m_imageView,
          .imageLayout = vk::ImageLayout::eGeneral
        };
      }
    }

    ImageResource::~ImageResource()
    {
      if (m_descriptorSet)
      {
        ImGui_ImplVulkan_RemoveTexture(m_descriptorSet);
      }
    }

    vk::Image ImageResource::getImage() const
    {
      return m_image;
    }

    vk::ImageView ImageResource::getImageView() const
    {
      return m_imageView;
    }

    vk::DescriptorSet ImageResource::getDescriptorSet() const
    {
      return m_descriptorSet;
    }

    const vk::DescriptorImageInfo& ImageResource::getDescriptorImageInfo() const
    {
      return m_descriptorImageInfo;
    }

    void ImageResource::createImage(const ImageResourceConfig& config)
    {
      vk::ImageUsageFlags imageUsageFlags{};

      if (config.imageResourceType == ImageResourceType::Color)
      {
        imageUsageFlags = vk::ImageUsageFlagBits::eColorAttachment;

        if (config.numSamples > vk::SampleCountFlagBits::e1)
        {
          imageUsageFlags |= vk::ImageUsageFlagBits::eTransientAttachment;
        }

        if (getFormat(config) == vk::Format::eR8G8B8A8Uint)
        {
          imageUsageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        }
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageUsageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment;

        if (getFormat(config) == vk::Format::eD32Sfloat)
        {
          imageUsageFlags |= vk::ImageUsageFlagBits::eSampled;
        }
      }
      else if (config.imageResourceType == ImageResourceType::Resolve)
      {
        imageUsageFlags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;

        imageUsageFlags |= vk::ImageUsageFlagBits::eTransferDst; // TODO: Don't apply this to everything

        if (getFormat(config) == vk::Format::eR8G8B8A8Unorm)
        {
          imageUsageFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        }
      }
      else if (config.imageResourceType == ImageResourceType::RayTracingOutput)
      {
        imageUsageFlags |= vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc;
      }

      auto [image, imageMemory] = Images::createImage(
        config.logicalDevice,
        {
          .flags = config.isCubeMap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{},
          .extent = {
            .width = config.extent.width,
            .height = config.extent.height,
            .depth = 1,
          },
          .mipLevels = 1,
          .numSamples = config.numSamples,
          .format = getFormat(config),
          .tiling = vk::ImageTiling::eOptimal,
          .usage = imageUsageFlags,
          .imageType = vk::ImageType::e2D,
          .layerCount = static_cast<uint32_t>(config.isCubeMap ? 6 : 1),
          .properties = vk::MemoryPropertyFlagBits::eDeviceLocal
        }
      );

      m_image = std::move(image);
      m_imageMemory = std::move(imageMemory);
    }

    void ImageResource::createImageView(const ImageResourceConfig& config)
    {
      vk::ImageAspectFlags imageAspectFlags{};

      if (config.imageResourceType == ImageResourceType::Color ||
          config.imageResourceType == ImageResourceType::Resolve ||
          config.imageResourceType == ImageResourceType::RayTracingOutput)
      {
        imageAspectFlags = vk::ImageAspectFlagBits::eColor;
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageAspectFlags = vk::ImageAspectFlagBits::eDepth;
      }

      m_imageView = Images::createImageView(
        config.logicalDevice,
        m_image,
        getFormat(config),
        imageAspectFlags,
        1,
        config.isCubeMap ? vk::ImageViewType::eCube : vk::ImageViewType::e2D,
        config.isCubeMap ? 6 : 1
      );
    }

    void ImageResource::transitionImageLayout(const ImageResourceConfig& config) const
    {
      auto imageLayout = vk::ImageLayout::eUndefined;

      if (config.imageResourceType == ImageResourceType::Color)
      {
        imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      }
      else if (config.imageResourceType == ImageResourceType::Resolve)
      {
        imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      }
      else if (config.imageResourceType == ImageResourceType::RayTracingOutput)
      {
        imageLayout = vk::ImageLayout::eGeneral;
      }

      Images::transitionImageLayout(
        config.logicalDevice,
        config.commandPool,
        m_image,
        getFormat(config),
        vk::ImageLayout::eUndefined,
        imageLayout,
        1,
        config.isCubeMap ? 6 : 1
      );
    }

    vk::Format ImageResource::getFormat(const ImageResourceConfig& config)
    {
      if (config.imageResourceType == ImageResourceType::Color)
      {
        return config.colorFormat;
      }

      if (config.imageResourceType == ImageResourceType::Depth)
      {
        return config.depthFormat;
      }

      if (config.imageResourceType == ImageResourceType::Resolve)
      {
        return config.resolveFormat;
      }

      if (config.imageResourceType == ImageResourceType::RayTracingOutput)
      {
        return config.rayTracingFormat;
      }

      return vk::Format::eUndefined;
    }
} // vke