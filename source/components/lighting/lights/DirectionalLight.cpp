#include "DirectionalLight.h"
#include "DirectionalShadow.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/ImageResource.h"

#include <cmath>
#include <glm/geometric.hpp>
#include <stdexcept>

namespace vke {

  namespace {

    [[nodiscard]] bool isFinite(const glm::vec3& v)
    {
      return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    [[nodiscard]] glm::vec3 normalizedDirection(const glm::vec3& direction)
    {
      if (!isFinite(direction))
      {
        throw std::invalid_argument("DirectionalLight: the direction must be finite");
      }

      const float length = glm::length(direction);

      if (!std::isfinite(length) || !(length > 0.0f))
      {
        throw std::invalid_argument("DirectionalLight: the direction must not have zero length");
      }

      return direction / length;
    }

  } // namespace

  DirectionalLight::DirectionalLight(std::shared_ptr<LogicalDevice> logicalDevice,
                                     const CommonLightData& commonLightData,
                                     const glm::vec3 direction,
                                     const vk::CommandPool commandPool)
    : Light(std::move(logicalDevice), commonLightData), m_direction(normalizedDirection(direction))
  {
    m_shadowMapExtent.width = s_shadowMapSize;
    m_shadowMapExtent.height = s_shadowMapSize;

    DirectionalLight::createShadowMap(commandPool);
  }

  glm::vec3 DirectionalLight::getDirection() const
  {
    return m_direction;
  }

  void DirectionalLight::setDirection(const glm::vec3& direction)
  {
    m_direction = normalizedDirection(direction);
  }

  glm::vec3 DirectionalLight::getShadowCenter() const
  {
    return m_shadowCenter;
  }

  float DirectionalLight::getShadowHalfExtent() const
  {
    return m_shadowHalfExtent;
  }

  void DirectionalLight::setShadowCenter(const glm::vec3& center)
  {
    if (!isFinite(center))
    {
      throw std::invalid_argument("DirectionalLight::setShadowCenter: the center must be finite");
    }

    m_shadowCenter = center;
  }

  void DirectionalLight::setShadowHalfExtent(const float halfExtent)
  {
    if (!std::isfinite(halfExtent) || !(halfExtent > 0.0f))
    {
      throw std::invalid_argument("DirectionalLight::setShadowHalfExtent: the half extent must be finite and greater than 0");
    }

    m_shadowHalfExtent = halfExtent;
  }

  LightType DirectionalLight::getLightType() const
  {
    return LightType::directionalLight;
  }

  LightUniform DirectionalLight::getUniform() const
  {
    return DirectionalLightUniform {
      .lightViewProjection = getLightViewProjectionMatrix(),
      .direction = m_direction,
      .ambient = m_ambient,
      .color = m_color,
      .diffuse = m_diffuse,
      .specular = m_specular,
      .shadowMapIndex = -1,
      .shadowBias = directionalShadowBias(s_shadowMapSize),
      .pad0 = 0.0f
    };
  }

  glm::mat4 DirectionalLight::getLightViewProjectionMatrix() const
  {
    return directionalShadowViewProjection(m_direction, m_shadowCenter, m_shadowHalfExtent);
  }

  void DirectionalLight::createShadowMap(const vk::CommandPool commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    ImageResourceConfig imageResourceConfig {
      .imageResourceType = ImageResourceType::Depth,
      .logicalDevice = m_logicalDevice,
      .extent = m_shadowMapExtent,
      .commandPool = commandPool,
      .depthFormat = s_shadowMapFormat,
      .numSamples = vk::SampleCountFlagBits::e1
    };

    m_shadowMapDepthImageResource = std::make_shared<ImageResource>(imageResourceConfig);
  }

} // namespace vke
