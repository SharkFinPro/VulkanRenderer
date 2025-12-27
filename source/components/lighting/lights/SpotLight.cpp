#include "SpotLight.h"
#include "../../renderingManager/ImageResource.h"
#include "../../renderingManager/RenderTarget.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vke {

  SpotLight::SpotLight(std::shared_ptr<LogicalDevice> logicalDevice,
                       const glm::vec3& position,
                       const glm::vec3& color,
                       const float ambient,
                       const float diffuse,
                       const float specular,
                       const VkCommandPool& commandPool,
                       const std::shared_ptr<Renderer>& renderer)
    : Light(std::move(logicalDevice), position, color, ambient, diffuse, specular)
  {
    SpotLight::createShadowMap(commandPool);

    registerShadowMapRenderTarget(renderer);
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
      .lightViewProjection = getLightViewProjectionMatrix(),
      .position = m_position,
      .ambient = m_ambient,
      .color = m_color,
      .diffuse = m_diffuse,
      .direction = m_direction,
      .specular = m_specular,
      .coneAngle = glm::radians(m_coneAngle)
    };
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

  void SpotLight::createShadowMap(const VkCommandPool& commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = {
        .width = m_shadowMapSize,
        .height = m_shadowMapSize
      },
      .commandPool = commandPool,
      .depthFormat = VK_FORMAT_D32_SFLOAT,
      .numSamples = VK_SAMPLE_COUNT_1_BIT
    };

    m_shadowMapRenderTarget = std::make_shared<RenderTarget>(imageResourceConfig);
  }

} // vke