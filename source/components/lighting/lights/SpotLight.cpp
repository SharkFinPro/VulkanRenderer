#include "SpotLight.h"
#include <glm/detail/func_trigonometric.inl>

namespace vke {
  SpotLight::SpotLight(const glm::vec3& position,
                       const glm::vec3& color,
                       const float ambient,
                       const float diffuse,
                       const float specular)
    : Light(position, color, ambient, diffuse, specular)
  {}

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
} // vke