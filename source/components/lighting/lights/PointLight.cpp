#include "PointLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Images.h"

namespace vke {
  PointLight::PointLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const glm::vec3& position,
                         const glm::vec3& color,
                         const float ambient,
                         const float diffuse,
                         const float specular,
                         const VkCommandPool& commandPool)
    : Light(logicalDevice, position, color, ambient, diffuse, specular)
  {
    createShadowMap(commandPool);
  }

  PointLight::~PointLight()
  {
    destroyShadowMap();
  }

  LightType PointLight::getLightType() const
  {
    return LightType::pointLight;
  }

  LightUniform PointLight::getUniform() const
  {
    return PointLightUniform {
      .position = m_position,
      .color = m_color,
      .ambient = m_ambient,
      .diffuse = m_diffuse,
      .specular = m_specular
    };
  }

  void PointLight::createShadowMap(const VkCommandPool& commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    Images::createImage(
       m_logicalDevice,
       VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
       m_shadowMapSize,
       m_shadowMapSize,
       1,
       1,
       VK_SAMPLE_COUNT_1_BIT,
       depthFormat,
       VK_IMAGE_TILING_OPTIMAL,
       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
       m_shadowMap,
       m_shadowMapMemory,
       VK_IMAGE_TYPE_2D,
       6
     );

    m_shadowMapView = Images::createImageView(
      m_logicalDevice,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      1,
      VK_IMAGE_VIEW_TYPE_CUBE,
      6
    );

    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      1,
      6
    );
  }

  void PointLight::destroyShadowMap()
  {
    m_logicalDevice->destroyImage(m_shadowMap);
    m_logicalDevice->destroyImageView(m_shadowMapView);
    m_logicalDevice->freeMemory(m_shadowMapMemory);
  }
} // vke