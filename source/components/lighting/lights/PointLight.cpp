#include "PointLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../renderingManager/ImageResource.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace vke {

  PointLight::PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
                         const CommonLightData& commonLightData,
                         const vk::CommandPool& commandPool)
    : Light(std::move(logicalDevice), commonLightData)
  {
    PointLight::createShadowMap(commandPool);
  }

  LightType PointLight::getLightType() const
  {
    return LightType::pointLight;
  }

  LightUniform PointLight::getUniform() const
  {
    return PointLightUniform {
      .lightViewProjections = getLightViewProjectionMatrices(),
      .position = m_position,
      .shadowNearPlane = s_shadowNearPlane,
      .color = m_color,
      .ambient = m_ambient,
      .diffuse = m_diffuse,
      .specular = m_specular,
      .shadowRange = m_shadowRange
    };
  }

  std::array<glm::mat4, 6> PointLight::getLightViewProjectionMatrices() const
  {
    // The planes swapped give reversed depth
    glm::mat4 projection = glm::perspective(
      glm::radians(90.0f),
      1.0f,
      m_shadowRange,
      s_shadowNearPlane
    );

    projection[1][1] *= -1;

    const std::array viewMatrices {
      glm::lookAt(m_position, m_position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)),
      glm::lookAt(m_position, m_position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)),

      // Y direction order must be swapped for point light shadow maps
      glm::lookAt(m_position, m_position + glm::vec3(0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)),
      glm::lookAt(m_position, m_position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)),

      glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)),
      glm::lookAt(m_position, m_position + glm::vec3(0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0))
    };

    return {
      projection * viewMatrices[0],
      projection * viewMatrices[1],
      projection * viewMatrices[2],
      projection * viewMatrices[3],
      projection * viewMatrices[4],
      projection * viewMatrices[5],
    };
  }

  void PointLight::createShadowMap(const vk::CommandPool commandPool)
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
      .isCubeMap = true
    };

    m_shadowMapDepthImageResource = std::make_shared<ImageResource>(imageResourceConfig);
  }

} // vke