#include "Light.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/ImageResource.h"
#include "../../renderingManager/RenderTarget.h"

namespace vke {

  Light::Light(std::shared_ptr<LogicalDevice> logicalDevice,
               const CommonLightData& commonLightData)
    : m_logicalDevice(std::move(logicalDevice)), m_position(commonLightData.position), m_color(commonLightData.color),
      m_ambient(commonLightData.ambient), m_diffuse(commonLightData.diffuse), m_specular(commonLightData.specular)
  {}

  Light::~Light()
  {
    m_logicalDevice->waitIdle();
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

  vk::Image Light::getShadowMap() const
  {
    return m_shadowMapRenderTarget->getDepthImageResource(0).getImage();
  }

  vk::ImageView Light::getShadowMapView() const
  {
    return m_shadowMapRenderTarget->getDepthImageResource(0).getImageView();
  }

  uint32_t Light::getShadowMapSize() const
  {
    return m_shadowMapSize;
  }

  bool Light::castsShadows() const
  {
    return m_castsShadows;
  }

  std::shared_ptr<RenderTarget> Light::getShadowMapRenderTarget() const
  {
    return m_shadowMapRenderTarget;
  }

} // namespace vke