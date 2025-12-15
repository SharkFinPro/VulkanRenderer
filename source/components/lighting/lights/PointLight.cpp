#include "PointLight.h"

namespace vke {
  PointLight::PointLight(const glm::vec3& position,
                         const glm::vec3& color,
                         const float ambient,
                         const float diffuse,
                         const float specular)
    : Light(position, color, ambient, diffuse, specular)
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