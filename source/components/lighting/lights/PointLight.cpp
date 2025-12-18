#include "PointLight.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../../utilities/Images.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vke {
  PointLight::PointLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
                         const glm::vec3& position,
                         const glm::vec3& color,
                         const float ambient,
                         const float diffuse,
                         const float specular,
                         const VkCommandPool& commandPool)
    : Light(logicalDevice, position, color, ambient, diffuse, specular)
  {
    PointLight::createShadowMap(commandPool);
  }

  PointLight::~PointLight()
  {
    m_logicalDevice->destroyImageView(m_shadowMapRenderView);
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
      .color = m_color,
      .ambient = m_ambient,
      .diffuse = m_diffuse,
      .specular = m_specular
    };
  }

  std::array<glm::mat4, 6> PointLight::getLightViewProjectionMatrices() const
  {
    glm::mat4 projection = glm::perspective(
      glm::radians(90.0f),
      1.0f,
      0.1f,
      100.0f
    );

    projection[1][1] *= -1;

    const std::array<glm::mat4, 6> viewMatrices {
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

  VkImageView PointLight::getShadowMapRenderView() const
  {
    return m_shadowMapRenderView;
  }

  void PointLight::createShadowMap(const VkCommandPool& commandPool)
  {
    if (!m_castsShadows)
    {
      return;
    }

    constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    Images::createImage(
       m_logicalDevice,
       VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
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
       6
     );

    m_shadowMapView = Images::createImageView(
      m_logicalDevice,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      1,
      VK_IMAGE_VIEW_TYPE_CUBE,
      6
    );

    createShadowMapRenderView();

    Images::transitionImageLayout(
      m_logicalDevice,
      commandPool,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      1,
      6
    );
  }

  void PointLight::createShadowMapRenderView()
  {
    constexpr VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    m_shadowMapRenderView = Images::createImageView(
      m_logicalDevice,
      m_shadowMap,
      depthFormat,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      1,
      VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      6
    );
  }
} // vke