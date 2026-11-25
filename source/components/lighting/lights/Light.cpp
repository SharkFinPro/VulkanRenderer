#include "Light.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/ImageResource.h"

#include <cmath>
#include <stdexcept>

namespace vke {

  Light::Light(std::shared_ptr<LogicalDevice> logicalDevice,
               const CommonLightData& commonLightData)
    : m_logicalDevice(std::move(logicalDevice)), m_position(commonLightData.position), m_color(commonLightData.color),
      m_ambient(commonLightData.ambient), m_diffuse(commonLightData.diffuse), m_specular(commonLightData.specular)
  {}

  Light::~Light()
  {
    m_logicalDevice->retire(std::move(m_shadowMapDepthImageResource));
  }

  glm::vec3 Light::getPosition() const
  {
    return m_position;
  }

  glm::vec3 Light::getColor() const
  {
    return m_color;
  }

  float Light::getAmbient() const
  {
    return m_ambient;
  }

  float Light::getDiffuse() const
  {
    return m_diffuse;
  }

  float Light::getSpecular() const
  {
    return m_specular;
  }

  void Light::setPosition(const glm::vec3& position)
  {
    m_position = position;
  }

  void Light::setColor(const glm::vec3& color)
  {
    m_color = color;
  }

  void Light::setAmbient(const float ambient)
  {
    m_ambient = ambient;
  }

  void Light::setDiffuse(const float diffuse)
  {
    m_diffuse = diffuse;
  }

  void Light::setSpecular(const float specular)
  {
    m_specular = specular;
  }

  vk::Extent2D Light::getShadowMapExtent() const
  {
    return m_shadowMapExtent;
  }

  float Light::getRange() const
  {
    return m_range;
  }

  void Light::setRange(const float range)
  {
    if (!isValidLightRange(range, s_shadowNearPlane))
    {
      throw std::invalid_argument("Light::setRange: the range must be 0 or finite and greater than the shadow near plane");
    }

    m_range = range;
  }

  float Light::getShadowRange() const
  {
    return m_shadowRange;
  }

  float Light::getEffectiveShadowRange() const
  {
    return effectiveShadowRange(m_shadowRange, m_range);
  }

  void Light::setShadowRange(const float range)
  {
    if (!std::isfinite(range) || !(range > s_shadowNearPlane))
    {
      throw std::invalid_argument("Light::setShadowRange: the range must be finite and greater than the shadow near plane");
    }

    m_shadowRange = range;
  }

  std::shared_ptr<ImageResource> Light::getShadowMapDepthImageResource() const
  {
    return m_shadowMapDepthImageResource;
  }

  bool Light::castsShadows() const
  {
    return m_castsShadows;
  }

} // namespace vke