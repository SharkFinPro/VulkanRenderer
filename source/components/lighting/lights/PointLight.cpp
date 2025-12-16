#include "PointLight.h"

namespace vke {
  PointLight::PointLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const glm::vec3& position,
                         const glm::vec3& color,
                         const float ambient,
                         const float diffuse,
                         const float specular)
    : Light(logicalDevice, position, color, ambient, diffuse, specular)
  {}

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
} // vke