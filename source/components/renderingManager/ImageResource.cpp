#include "ImageResource.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../../utilities/Images.h"
#include <backends/imgui_impl_vulkan.h>

namespace vke {
  ImageResource::ImageResource(ImageResourceConfig config)
      : m_logicalDevice(std::move(config.logicalDevice))
    {
      createImage(config);

      createImageView(config);

      transitionImageLayout(config);

      if (config.imageResourceType == ImageResourceType::Resolve)
      {
        m_descriptorSet = ImGui_ImplVulkan_AddTexture(
          config.sampler,
          m_imageView,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
      }
    }

    ImageResource::~ImageResource()
    {
      if (m_descriptorSet != VK_NULL_HANDLE)
      {
        ImGui_ImplVulkan_RemoveTexture(m_descriptorSet);
      }

      m_logicalDevice->destroyImageView(m_imageView);
      m_logicalDevice->freeMemory(m_imageMemory);
      m_logicalDevice->destroyImage(m_image);
    }

    VkImage ImageResource::getImage() const
    {
      return m_image;
    }

    VkImageView ImageResource::getImageView() const
    {
      return m_imageView;
    }

    VkDescriptorSet ImageResource::getDescriptorSet() const
    {
      return m_descriptorSet;
    }

    void ImageResource::createImage(const ImageResourceConfig& config)
    {
      VkImageUsageFlags imageUsageFlags = 0;

      if (config.imageResourceType == ImageResourceType::Color)
      {
        imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (config.numSamples > VK_SAMPLE_COUNT_1_BIT)
        {
          imageUsageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }

        if (getFormat(config) == VK_FORMAT_R8G8B8A8_UNORM)
        {
          imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        if (getFormat(config) == VK_FORMAT_D32_SFLOAT)
        {
          imageUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
      }
      else if (config.imageResourceType == ImageResourceType::Resolve)
      {
        imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        if (getFormat(config) == VK_FORMAT_R8G8B8A8_UNORM)
        {
          imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
      }

      Images::createImage(
        m_logicalDevice,
        config.isCubeMap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
        {
          .width = config.extent.width,
          .height = config.extent.height,
          .depth = 1,
        },
        1,
        config.numSamples,
        getFormat(config),
        VK_IMAGE_TILING_OPTIMAL,
        imageUsageFlags,
        m_image,
        m_imageMemory,
        VK_IMAGE_TYPE_2D,
        config.isCubeMap ? 6 : 1
      );
    }

    void ImageResource::createImageView(const ImageResourceConfig& config)
    {
      VkImageAspectFlags imageAspectFlags = 0;

      if (config.imageResourceType == ImageResourceType::Color ||
          config.imageResourceType == ImageResourceType::Resolve)
      {
        imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
      }

      m_imageView = Images::createImageView(
        m_logicalDevice,
        m_image,
        getFormat(config),
        imageAspectFlags,
        1,
        config.isCubeMap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        config.isCubeMap ? 6 : 1
      );
    }

    void ImageResource::transitionImageLayout(const ImageResourceConfig& config) const
    {
      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      if (config.imageResourceType == ImageResourceType::Color)
      {
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      }
      else if (config.imageResourceType == ImageResourceType::Depth)
      {
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      }
      else if (config.imageResourceType == ImageResourceType::Resolve)
      {
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      }

      Images::transitionImageLayout(
        m_logicalDevice,
        config.commandPool,
        m_image,
        getFormat(config),
        VK_IMAGE_LAYOUT_UNDEFINED,
        imageLayout,
        1,
        config.isCubeMap ? 6 : 1
      );
    }

    VkFormat ImageResource::getFormat(const ImageResourceConfig& config)
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

      return VK_FORMAT_UNDEFINED;
    }
} // vke