#include "SpotLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Images.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vke {
  SpotLight::SpotLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
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

  SpotLight::~SpotLight()
  {
    destroyShadowMap();
  }

  glm::vec3 SpotLight::getDirection() const
  {
    return m_direction;
  }

  float SpotLight::getConeAngle() const
  {
    return m_coneAngle;
  }

  void SpotLight::setDirection(const glm::vec3& direction)
  {
    m_direction = direction;
  }

  void SpotLight::setConeAngle(const float coneAngle)
  {
    m_coneAngle = coneAngle;
  }

  LightType SpotLight::getLightType() const
  {
    return LightType::spotLight;
  }

  LightUniform SpotLight::getUniform() const
  {
    return SpotLightUniform {
      .position = m_position,
      .ambient = m_ambient,
      .color = m_color,
      .diffuse = m_diffuse,
      .direction = m_direction,
      .specular = m_specular,
      .coneAngle = glm::radians(m_coneAngle)
    };
  }

  void SpotLight::createShadowMap(const VkCommandPool& commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    Images::createImage(
       m_logicalDevice,
       0,
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
       1
     );

    m_shadowMapView = Images::createImageView(
      m_logicalDevice,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      1,
      VK_IMAGE_VIEW_TYPE_2D,
      1
    );

    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      1,
      1
    );
  }

  void SpotLight::destroyShadowMap()
  {
    m_logicalDevice->destroyImage(m_shadowMap);
    m_logicalDevice->destroyImageView(m_shadowMapView);
    m_logicalDevice->freeMemory(m_shadowMapMemory);
  }

  VkImage SpotLight::getShadowMap() const
  {
    return m_shadowMap;
  }

  VkImageView SpotLight::getShadowMapView() const
  {
    return m_shadowMapView;
  }

  uint32_t SpotLight::getShadowMapSize() const
  {
    return m_shadowMapSize;
  }

  bool SpotLight::castsShadows() const
  {
    return m_castsShadows;
  }

  glm::mat4 SpotLight::getLightViewProjectionMatrix() const
  {
    const glm::vec3 up = std::abs(m_direction.y) > 0.99f
                         ? glm::vec3(1, 0, 0)
                         : glm::vec3(0, 1, 0);

    const glm::mat4 view = glm::lookAt(
      m_position,
      m_position + m_direction,
      up
    );

    const float fov = m_coneAngle * 2.0f;
    glm::mat4 proj = glm::perspective(
      glm::radians(fov),
      1.0f,
      0.1f,
      100.0f
    );

    proj[1][1] *= -1;

    return proj * view;
  }
} // vke