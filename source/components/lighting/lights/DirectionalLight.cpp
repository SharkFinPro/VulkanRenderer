#include "DirectionalLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/ImageResource.h"

#include <algorithm>
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

  uint32_t DirectionalLight::getCascadeCount() const
  {
    return m_cascadeCount;
  }

  void DirectionalLight::setCascadeCount(const uint32_t count)
  {
    if (count < 1 || count > s_maxShadowCascades)
    {
      throw std::invalid_argument("DirectionalLight::setCascadeCount: the count must be from 1 to the most cascades");
    }

    m_cascadeCount = count;
  }

  float DirectionalLight::getShadowDistance() const
  {
    return m_shadowDistance;
  }

  void DirectionalLight::setShadowDistance(const float distance)
  {
    if (!std::isfinite(distance) || !(distance > 0.0f))
    {
      throw std::invalid_argument("DirectionalLight::setShadowDistance: the distance must be finite and greater than 0");
    }

    m_shadowDistance = distance;
  }

  float DirectionalLight::getCascadeSplitLambda() const
  {
    return m_cascadeSplitLambda;
  }

  void DirectionalLight::setCascadeSplitLambda(const float lambda)
  {
    if (!std::isfinite(lambda) || lambda < 0.0f || lambda > 1.0f)
    {
      throw std::invalid_argument("DirectionalLight::setCascadeSplitLambda: the lambda must be finite and in [0, 1]");
    }

    m_cascadeSplitLambda = lambda;
  }

  void DirectionalLight::updateCascades(const CascadeCamera& camera,
                                        const float farPlane,
                                        const AABB& casterBounds)
  {
    const float distance = std::min(m_shadowDistance, farPlane);

    m_fittedCascadeCount = 0;
    m_fittedShadowDistance = distance;

    if (!(distance > camera.nearPlane))
    {
      return;
    }

    const auto splits = cascadeSplitDistances(camera.nearPlane, distance, m_cascadeCount, m_cascadeSplitLambda);

    for (uint32_t cascade = 0; cascade < m_cascadeCount; ++cascade)
    {
      const ShadowCascade fitted = fitShadowCascade(m_direction, camera, splits[cascade], splits[cascade + 1],
                                                    s_shadowMapSize, casterBounds);

      m_cascadeViewProjections[cascade] = fitted.viewProjection;
      m_cascadeBiases[cascade] = fitted.depthBias;
    }

    m_fittedCascadeCount = m_cascadeCount;
  }

  uint32_t DirectionalLight::getFittedCascadeCount() const
  {
    return m_fittedCascadeCount;
  }

  const glm::mat4& DirectionalLight::getCascadeViewProjection(const uint32_t cascade) const
  {
    return m_cascadeViewProjections.at(cascade);
  }

  LightType DirectionalLight::getLightType() const
  {
    return LightType::directionalLight;
  }

  LightUniform DirectionalLight::getUniform() const
  {
    DirectionalLightUniform uniform {};

    glm::vec4 biases(0.0f);

    for (uint32_t cascade = 0; cascade < m_fittedCascadeCount; ++cascade)
    {
      uniform.cascadeViewProjections[cascade] = m_cascadeViewProjections[cascade];
      biases[static_cast<glm::length_t>(cascade)] = m_cascadeBiases[cascade];
    }

    uniform.cascadeBiases = biases;
    uniform.direction = m_direction;
    uniform.ambient = m_ambient;
    uniform.color = m_color;
    uniform.diffuse = m_diffuse;
    uniform.specular = m_specular;
    uniform.shadowMapIndex = -1;
    uniform.cascadeCount = static_cast<int32_t>(m_fittedCascadeCount);
    uniform.shadowDistance = m_fittedShadowDistance;

    return uniform;
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
      .numSamples = vk::SampleCountFlagBits::e1,
      .layerCount = s_maxShadowCascades
    };

    m_shadowMapDepthImageResource = std::make_shared<ImageResource>(imageResourceConfig);
  }

} // namespace vke
